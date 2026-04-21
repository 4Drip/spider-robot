import socket
import threading
import time
import random
import serial
import serial.tools.list_ports
import sys

HOST = "0.0.0.0"
PORT = 5000

# ── Auto-detect Arduino serial port ───────────────────────────
def find_arduino():
    """Try to find the Arduino automatically."""
    # Common Arduino port patterns
    candidates = []
    for port in serial.tools.list_ports.comports():
        desc = (port.description or "").lower()
        name = (port.device or "").lower()
        if any(x in desc for x in ["arduino", "ch340", "ch341", "cp210", "ftdi", "atmega"]):
            candidates.append(port.device)
        elif any(x in name for x in ["ttyacm", "ttyusb"]):
            candidates.append(port.device)

    if candidates:
        print(f"Arduino trovato su: {candidates[0]}")
        if len(candidates) > 1:
            print(f"  (altri candidati: {candidates[1:]})")
        return candidates[0]

    # Fallback: try common ports in order
    for fallback in ["/dev/ttyACM0", "/dev/ttyACM1", "/dev/ttyUSB0", "/dev/ttyUSB1"]:
        try:
            s = serial.Serial(fallback, 9600, timeout=0.5)
            s.close()
            print(f"Arduino trovato su fallback: {fallback}")
            return fallback
        except:
            pass

    return None

port = find_arduino()
if port is None:
    print("ERRORE: Arduino non trovato!")
    print("Controlla: ls /dev/ttyACM* /dev/ttyUSB*")
    print("Aggiungi utente al gruppo dialout: sudo usermod -a -G dialout $USER")
    sys.exit(1)

try:
    arduino = serial.Serial(port, 9600, timeout=1)
    time.sleep(2)  # wait for Arduino reset
    print(f"Arduino connesso su {port}")
except Exception as e:
    print(f"Errore apertura seriale: {e}")
    sys.exit(1)

# ── Stato globale ──────────────────────────────────────────────
is_manual  = True
dist_front = 99
dist_left  = 99
dist_right = 99
last_cmd   = None
mood       = "neutral"
mood_timer = 0

# ── Lock per scrittura seriale (thread-safe) ───────────────────
serial_lock = threading.Lock()

def send_to_arduino(cmd: str):
    global last_cmd
    with serial_lock:
        try:
            arduino.write((cmd + "\n").encode())
            print(f"  -> Arduino: {cmd}")
        except Exception as e:
            print(f"Errore seriale: {e}")

def send_face(face_key: str):
    send_to_arduino(f"FACE:{face_key}")

def send_move(cmd: str):
    global last_cmd
    if cmd != last_cmd:
        send_to_arduino(cmd)
        last_cmd = cmd

# ── Thread lettura seriale Arduino ────────────────────────────
def serial_reader():
    global dist_front, dist_left, dist_right
    buf = ""
    while True:
        try:
            if arduino.in_waiting:
                c = arduino.read().decode('utf-8', errors='ignore')
                if c == '\n':
                    line = buf.strip()
                    buf = ""
                    if line.startswith("SONAR:"):
                        parts = line[6:].split(",")
                        if len(parts) == 3:
                            try:
                                f = int(parts[0])
                                l = int(parts[1])
                                r = int(parts[2])
                                dist_front = f
                                dist_left  = l
                                dist_right = r
                                # Print sonar data every ~2s to avoid log spam
                            except ValueError:
                                pass
                    elif line:
                        print(f"  <- Arduino: {line}")
                else:
                    buf += c
        except Exception as e:
            print(f"Errore lettura seriale: {e}")
            time.sleep(0.5)
        time.sleep(0.005)

# ── Logica AI ─────────────────────────────────────────────────
def decide_movement() -> str:
    if dist_front < 8:
        return "S"
    if dist_front < 20:
        return "L" if dist_left > dist_right else "R"
    if random.random() < 0.04:
        return random.choice(["L", "R"])
    return "F"

def decide_mood() -> str:
    global mood, mood_timer
    danger  = sum([dist_front < 8,  dist_left < 8,  dist_right < 8])
    warning = sum([dist_front < 18, dist_left < 18, dist_right < 18])
    if danger >= 2:
        mood = "scared";  mood_timer = 12
    elif danger == 1:
        mood = "angry";   mood_timer = 8
    elif warning >= 2:
        mood = "annoyed"; mood_timer = 5
    elif warning == 1:
        mood = "sad";     mood_timer = 4
    else:
        if mood_timer <= 0:
            mood = random.choice(["happy", "cute", "neutral", "happy"])
            mood_timer = random.randint(15, 30)
        else:
            mood_timer -= 1
    return mood

def ai_loop():
    while True:
        if not is_manual:
            cmd      = decide_movement()
            mood_now = decide_mood()
            print(f"[AI] {cmd} | F={dist_front} L={dist_left} R={dist_right} | {mood_now}")
            send_move(cmd)
            send_face(mood_now)
        time.sleep(0.3)

# ── Handler client Java ────────────────────────────────────────
def handle_client(conn, addr):
    global is_manual
    print(f"Client connesso: {addr}")
    try:
        while True:
            data = conn.recv(1024)
            if not data:
                break
            for line in data.decode(errors='ignore').splitlines():
                cmd = line.strip()
                if not cmd:
                    continue
                print(f"[Java] {cmd}")

                if cmd == "MODE_AI":
                    is_manual = False
                    send_to_arduino("MODE_AI")
                    conn.sendall(b"ACK:MODE_AI\n")

                elif cmd == "MODE_MANUAL":
                    is_manual = True
                    send_to_arduino("MODE_MANUAL")
                    conn.sendall(b"ACK:MODE_MANUAL\n")

                elif cmd in ("F", "B", "L", "R", "S") and is_manual:
                    send_move(cmd)
                    face_map = {"F":"happy","B":"annoyed","L":"cute","R":"cute","S":"neutral"}
                    send_face(face_map.get(cmd, "neutral"))

                # Always reply with latest sonar data
                try:
                    conn.sendall(f"SONAR:{dist_front},{dist_left},{dist_right}\n".encode())
                except:
                    break

    except ConnectionResetError:
        print("Client disconnesso")
    except Exception as e:
        print(f"Errore client: {e}")
    finally:
        conn.close()
        print(f"Connessione chiusa: {addr}")

# ── Main ───────────────────────────────────────────────────────
if __name__ == "__main__":
    threading.Thread(target=serial_reader, daemon=True).start()
    threading.Thread(target=ai_loop, daemon=True).start()

    server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    server.bind((HOST, PORT))
    server.listen(5)
    print(f"Server in ascolto su {HOST}:{PORT}...")
    print(f"IP del Raspberry Pi: usa 'hostname -I' per trovarlo")

    while True:
        conn, addr = server.accept()
        threading.Thread(target=handle_client, args=(conn, addr), daemon=True).start()