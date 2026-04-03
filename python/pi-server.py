import socket
import threading
import time
import random
import serial

HOST = "0.0.0.0"
PORT = 5000

is_manual = True

arduino = serial.Serial('/dev/ttyACM0', 9600, timeout=1)
time.sleep(2)
print("Arduino connected")

faces = {
    "happy": "^_^",
    "sad": "T_T",
    "angry": ">_<",
    "annoyed": "-_-",
    "cute": "OwO",
    "neutral": ":|"
}

last_face = None
last_cmd = None

mood = "neutral"
mood_timer = 0

def send_command(cmd):
    global last_cmd
    if cmd != last_cmd:
        arduino.write((cmd + "\n").encode())
        last_cmd = cmd

def send_face(face_key):
    global last_face
    if face_key != last_face:
        message = f"FACE:{faces[face_key]}\n"
        arduino.write(message.encode())
        last_face = face_key

def movement(front, left, right):
    if front < 8:
        return "S"

    if front < 20:
        if random.random() < 0.1:
            return random.choice(["L", "R"])
        return "L" if left > right else "R"

    if random.random() < 0.03:
        return random.choice(["L", "R"])

    return "F"

def update_mood(front, left, right):
    global mood, mood_timer

    danger = sum([front < 8, left < 8, right < 8])
    warning = sum([front < 18, left < 18, right < 18])

    if danger >= 2:
        mood = "angry"
        mood_timer = 10
    elif danger == 1:
        mood = "sad"
        mood_timer = 8
    elif warning >= 2:
        mood = "annoyed"
        mood_timer = 5
    else:
        if mood_timer <= 0:
            mood = random.choice(["happy", "cute", "neutral"])
            mood_timer = 20
        else:
            mood_timer -= 1

    return mood

def ai_loop():
    global is_manual

    while True:
        if not is_manual:
            front = random.randint(0, 50)
            left = random.randint(0, 50)
            right = random.randint(0, 50)

            cmd = movement(front, left, right)
            mood_state = update_mood(front, left, right)

            if random.random() < 0.02:
                cmd = random.choice(["L", "R"])

            print(f"{cmd} | {faces[mood_state]}")

            send_command(cmd)
            send_face(mood_state)

        time.sleep(0.2)

server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)

server.bind((HOST, PORT))
server.listen(1)

print("Server in ascolto...")

conn, addr = server.accept()
print("Connesso da:", addr)

threading.Thread(target=ai_loop, daemon=True).start()

while True:
    try:
        data = conn.recv(1024)

        if not data:
            print("Client disconnected")
            break

        cmd = data.decode().strip()
        print("Ricevuto:", cmd)

        if cmd == "MODE_AI":
            is_manual = False

        elif cmd == "MODE_MANUAL":
            is_manual = True

        elif is_manual:
            send_command(cmd)

    except ConnectionResetError:
        print("Connessione persa")
        break

    except Exception as e:
        print("Error:", e)