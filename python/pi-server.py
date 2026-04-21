import socket
import threading
import time
import random
import serial
import serial.tools.list_ports
import sys

HOST = "0.0.0.0"
PORT = 5000

# ── Auto-detect Arduino ────────────────────────────────────────
def find_arduino():
    candidates = []
    for p in serial.tools.list_ports.comports():
        desc = (p.description or "").lower()
        name = (p.device   or "").lower()
        if any(x in desc for x in ["arduino","ch340","ch341","cp210","ftdi","atmega"]):
            candidates.append(p.device)
        elif any(x in name for x in ["ttyacm","ttyusb"]):
            candidates.append(p.device)
    if candidates:
        print(f"Arduino found: {candidates[0]}")
        return candidates[0]
    for fb in ["/dev/ttyACM0","/dev/ttyACM1","/dev/ttyUSB0","/dev/ttyUSB1"]:
        try:
            s = serial.Serial(fb, 9600, timeout=0.5); s.close()
            print(f"Arduino found (fallback): {fb}")
            return fb
        except:
            pass
    return None

port = find_arduino()
if not port:
    print("ERROR: Arduino not found. Check USB cable and run:")
    print("  ls /dev/ttyACM* /dev/ttyUSB*")
    print("  sudo usermod -a -G dialout $USER  (then log out/in)")
    sys.exit(1)

try:
    arduino = serial.Serial(port, 9600, timeout=1)
    time.sleep(2)
    print(f"Arduino connected on {port}")
except Exception as e:
    print(f"Serial open failed: {e}")
    sys.exit(1)

# ── Shared state ───────────────────────────────────────────────
is_manual  = True
dist_front = 99
dist_left  = 99
dist_right = 99
last_cmd   = None
mood       = "neutral"
mood_timer = 0

serial_lock  = threading.Lock()
clients_lock = threading.Lock()
connected_clients = []          # active Java connections

# ── Serial helpers ─────────────────────────────────────────────
def send_to_arduino(cmd: str):
    with serial_lock:
        try:
            arduino.write((cmd + "\n").encode())
            print(f"  -> Arduino: {cmd}")
        except Exception as e:
            print(f"Serial write error: {e}")

def send_face(name: str):
    send_to_arduino(f"FACE:{name}")

def send_move(cmd: str):
    global last_cmd
    if cmd != last_cmd:
        send_to_arduino(cmd)
        last_cmd = cmd

# ── Thread 1: read Arduino serial ─────────────────────────────
def serial_reader():
    """Reads SONAR:F,L,R lines from Arduino. Restarts on error."""
    global dist_front, dist_left, dist_right
    last_log = 0
    while True:
        try:
            buf = ""
            while True:
                if arduino.in_waiting:
                    c = arduino.read().decode("utf-8", errors="ignore")
                    if c == "\n":
                        line = buf.strip()
                        buf  = ""
                        if line.startswith("SONAR:"):
                            parts = line[6:].split(",")
                            if len(parts) == 3:
                                try:
                                    dist_front = int(parts[0])
                                    dist_left  = int(parts[1])
                                    dist_right = int(parts[2])
                                    now = time.time()
                                    if now - last_log > 2.0:
                                        print(f"  [sonar] F={dist_front} L={dist_left} R={dist_right}")
                                        last_log = now
                                except ValueError:
                                    pass
                        elif line:
                            print(f"  <- Arduino: {line}")
                    else:
                        buf += c
                else:
                    time.sleep(0.005)
        except Exception as e:
            print(f"serial_reader crashed: {e} — restarting in 1s")
            time.sleep(1)

# ── Thread 2: push sonar to all Java clients every 300ms ───────
def broadcast_sonar():
    """Continuously pushes sonar data to every connected Java client.
    This is the main fix: data flows even when no button is pressed."""
    while True:
        try:
            msg = f"SONAR:{dist_front},{dist_left},{dist_right}\n".encode()
            dead = []
            with clients_lock:
                for conn in connected_clients:
                    try:
                        conn.sendall(msg)
                    except Exception:
                        dead.append(conn)
                for conn in dead:
                    connected_clients.remove(conn)
        except Exception as e:
            print(f"broadcast_sonar error: {e}")
        time.sleep(0.3)

# ── Thread 3: AI movement logic ────────────────────────────────
def decide_movement() -> str:
    if dist_front < 8:   return "S"
    if dist_front < 20:  return "L" if dist_left > dist_right else "R"
    if random.random() < 0.04: return random.choice(["L","R"])
    return "F"

def decide_mood() -> str:
    global mood, mood_timer
    danger  = sum([dist_front < 8,  dist_left < 8,  dist_right < 8])
    warning = sum([dist_front < 18, dist_left < 18, dist_right < 18])
    if   danger  >= 2: mood = "scared";  mood_timer = 12
    elif danger  == 1: mood = "angry";   mood_timer = 8
    elif warning >= 2: mood = "annoyed"; mood_timer = 5
    elif warning == 1: mood = "sad";     mood_timer = 4
    else:
        if mood_timer <= 0:
            mood = random.choice(["happy","cute","neutral","happy"])
            mood_timer = random.randint(15, 30)
        else:
            mood_timer -= 1
    return mood

def ai_loop():
    while True:
        try:
            if not is_manual:
                cmd  = decide_movement()
                face = decide_mood()
                print(f"[AI] {cmd} | F={dist_front} L={dist_left} R={dist_right} | {face}")
                send_move(cmd)
                send_face(face)
        except Exception as e:
            print(f"ai_loop error: {e}")
        time.sleep(0.3)

# ── Thread per client: receives commands from Java ─────────────
def handle_client(conn, addr):
    global is_manual
    print(f"Java client connected: {addr}")
    with clients_lock:
        connected_clients.append(conn)
    try:
        while True:
            data = conn.recv(1024)
            if not data:
                break
            for raw in data.decode(errors="ignore").splitlines():
                cmd = raw.strip()
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
                elif is_manual and cmd in ("F","B","L","R","S"):
                    send_move(cmd)
                    faces = {"F":"happy","B":"annoyed","L":"cute","R":"cute","S":"neutral"}
                    send_face(faces.get(cmd,"neutral"))
    except ConnectionResetError:
        pass
    except Exception as e:
        print(f"handle_client error: {e}")
    finally:
        with clients_lock:
            if conn in connected_clients:
                connected_clients.remove(conn)
        try:
            conn.close()
        except:
            pass
        print(f"Java client disconnected: {addr}")

# ── Main ───────────────────────────────────────────────────────
if __name__ == "__main__":
    threading.Thread(target=serial_reader,   daemon=True, name="serial-rx").start()
    threading.Thread(target=broadcast_sonar, daemon=True, name="sonar-tx").start()
    threading.Thread(target=ai_loop,         daemon=True, name="ai").start()

    srv = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    srv.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    srv.bind((HOST, PORT))
    srv.listen(5)
    print(f"Listening on {HOST}:{PORT}")
    print("Find your Pi IP with:  hostname -I")

    while True:
        conn, addr = srv.accept()
        threading.Thread(target=handle_client, args=(conn, addr),
                         daemon=True, name=f"client-{addr}").start()