import socket
import threading
import time
import numpy as np
import random

HOST = "0.0.0.0"
PORT = 5000

is_manual = True
lock = threading.Lock()

# ===== "AI" STATE =====
mood = "neutral"
mood_timer = 0

action_map = {
    0: "F",
    1: "L",
    2: "R",
    3: "S"
}

# ===== CORE DECISION LOGIC =====
def run_ai(front, left, right):
    """
    Rule-based AI (replaces TensorFlow model)
    """

    # emergency stop
    if front < 8:
        return "S"

    # obstacle avoidance
    if front < 20:
        if random.random() < 0.1:
            return random.choice(["L", "R"])
        return "L" if left > right else "R"

    # forward with personality randomness
    if random.random() < 0.03:
        return random.choice(["L", "R"])

    return "F"


# ===== EMOTION SYSTEM =====
def update_mood(front, left, right):
    global mood, mood_timer

    danger = sum([front < 8, left < 8, right < 8])
    warning = sum([front < 18, left < 18, right < 18])

    if danger >= 2:
        mood = "angry"
        mood_timer = 10
    elif danger == 1:
        mood = "scared"
        mood_timer = 8
    elif warning >= 2:
        mood = "annoyed"
        mood_timer = 5
    else:
        if mood_timer <= 0:
            mood = random.choice(["happy", "curious", "neutral"])
            mood_timer = 20
        else:
            mood_timer -= 1

    return mood


# ===== AI LOOP =====
def ai_loop():
    global is_manual

    while True:
        if not is_manual:

            # TODO: replace with real sensors later
            front = random.randint(0, 50)
            left = random.randint(0, 50)
            right = random.randint(0, 50)

            cmd = run_ai(front, left, right)
            mood_state = update_mood(front, left, right)

            # "alive feeling" randomness
            if random.random() < 0.02:
                cmd = random.choice(["L", "R"])

            print(f"AI: {cmd} | Mood: {mood_state}")

            # send_command(cmd)
            # send_face(mood_state)

        time.sleep(0.2)


# ===== SOCKET SERVER =====
server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)

server.bind((HOST, PORT))
server.listen(1)

print("Server in ascolto...")

conn, addr = server.accept()
print("Connesso da:", addr)

threading.Thread(target=ai_loop, daemon=True).start()


# ===== MAIN LOOP =====
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
            print("Modalità AI attiva")

        elif cmd == "MODE_MANUAL":
            is_manual = True
            print("Modalità manuale")

        elif is_manual:
            print("Manual:", cmd)
            # send_command(cmd)

    except ConnectionResetError:
        print("Connessione persa")
        break

    except Exception as e:
        print("Error:", e)