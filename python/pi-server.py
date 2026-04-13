"""
pi_server.py  –  Raspberry Pi bridge
Java App  →  WiFi TCP  →  RPi  →  Serial  →  Arduino

Legge i dati SONAR reali dall'Arduino (formato "SONAR:FF,LL,RR")
e gestisce la logica AI sul lato RPi.
"""

import socket
import threading
import time
import random
import serial

HOST = "0.0.0.0"
PORT = 5000

# ── Connessione Arduino ────────────────────────────────────────
arduino = serial.Serial('/dev/ttyACM0', 9600, timeout=1)
time.sleep(2)
print("Arduino connesso")

# ── Stato globale ──────────────────────────────────────────────
is_manual   = True
dist_front  = 99
dist_left   = 99
dist_right  = 99
last_cmd    = None
mood        = "neutral"
mood_timer  = 0

# ── Faccine disponibili ────────────────────────────────────────
FACES = {
    "happy":   "happy",
    "sad":     "sad",
    "angry":   "angry",
    "annoyed": "annoyed",
    "cute":    "cute",
    "neutral": "neutral",
    "scared":  "scared",
}

# ── Lock per scrittura seriale (thread-safe) ───────────────────
serial_lock = threading.Lock()

def send_to_arduino(cmd: str):
    """Invia un comando all'Arduino (thread-safe)."""
    global last_cmd
    with serial_lock:
        try:
            arduino.write((cmd + "\n").encode())
            print(f"  → Arduino: {cmd}")
        except Exception as e:
            print(f"Errore seriale: {e}")

def send_face(face_key: str):
    """Invia comando faccina all'Arduino."""
    send_to_arduino(f"FACE:{face_key}")

def send_move(cmd: str):
    """Invia comando movimento (F/B/L/R/S)."""
    global last_cmd
    if cmd != last_cmd:
        send_to_arduino(cmd)
        last_cmd = cmd

# ── Thread lettura seriale Arduino ────────────────────────────
def serial_reader():
    """Legge continuamente dalla seriale Arduino.
    Gestisce il formato: SONAR:front,left,right
    """
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
                                dist_front = int(parts[0])
                                dist_left  = int(parts[1])
                                dist_right = int(parts[2])
                            except ValueError:
                                pass
                    elif line:
                        print(f"  ← Arduino: {line}")
                else:
                    buf += c
        except Exception as e:
            print(f"Errore lettura seriale: {e}")
        time.sleep(0.005)

# ── Logica AI ─────────────────────────────────────────────────
def decide_movement() -> str:
    """Decide il prossimo comando in base ai sensori."""
    if dist_front < 8:
        return "S"  # fermati subito
    if dist_front < 20:
        # Ostacolo avanti: gira verso il lato più libero
        return "L" if dist_left > dist_right else "R"
    # Via libera — occasionalmente aggiungi variazione
    if random.random() < 0.04:
        return random.choice(["L", "R"])
    return "F"

def decide_mood() -> str:
    """Calcola lo stato d'animo in base ai sensori."""
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
    """Loop AI: gira ogni 300ms se in modalità AI."""
    while True:
        if not is_manual:
            cmd       = decide_movement()
            mood_now  = decide_mood()

            print(f"[AI] {cmd} | sonar: F={dist_front} L={dist_left} R={dist_right} | mood={mood_now}")

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
            for line in data.decode().splitlines():
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

                    # Faccina in base al comando manuale
                    face_map = {
                        "F": "happy",
                        "B": "annoyed",
                        "L": "cute",
                        "R": "cute",
                        "S": "neutral",
                    }
                    send_face(face_map.get(cmd, "neutral"))

                # Invia i dati sonar attuali al client Java
                conn.sendall(f"SONAR:{dist_front},{dist_left},{dist_right}\n".encode())

    except ConnectionResetError:
        print("Client disconnesso")
    except Exception as e:
        print(f"Errore client: {e}")
    finally:
        conn.close()
        print(f"Connessione chiusa: {addr}")

# ── Main ───────────────────────────────────────────────────────
if __name__ == "__main__":
    # Avvia thread lettura seriale
    threading.Thread(target=serial_reader, daemon=True).start()
    # Avvia loop AI
    threading.Thread(target=ai_loop, daemon=True).start()

    server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    server.bind((HOST, PORT))
    server.listen(5)
    print(f"Server in ascolto su {HOST}:{PORT}...")

    while True:
        conn, addr = server.accept()
        threading.Thread(target=handle_client, args=(conn, addr), daemon=True).start()