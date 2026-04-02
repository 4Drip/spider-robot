import socket
import threading
import time
import numpy as np
import tflite_runtime.interpreter as tflite

HOST = "0.0.0.0"
PORT = 5000

is_manual = True
lock = threading.Lock()

# ===== AI SETUP =====
interpreter = tflite.Interpreter(model_path="tiny_robot_model.tflite")
interpreter.allocate_tensors()

input_details = interpreter.get_input_details()
output_details = interpreter.get_output_details()

action_map = {
    0: "F",
    1: "L",
    2: "R",
    3: "S"
}

def run_ai(front, left, right):
    input_data = np.array([[front, left, right]], dtype=np.float32) / 50.0

    with lock:
        interpreter.set_tensor(input_details[0]['index'], input_data)
        interpreter.invoke()
        pred = interpreter.get_tensor(output_details[0]['index'])

    action = np.argmax(pred)
    return action_map[action]

def ai_loop():
    global is_manual

    while True:
        if not is_manual:
            # TODO: replace with real sensors
            front = np.random.randint(0, 50)
            left = np.random.randint(0, 50)
            right = np.random.randint(0, 50)

            cmd = run_ai(front, left, right)

            print("AI:", cmd)
            # send_command(cmd)

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