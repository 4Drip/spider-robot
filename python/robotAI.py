import numpy as np
import random
import time
import tflite_runtime.interpreter as tflite

# =========================
# CONFIG
# =========================

USE_ARDUINO = False

if USE_ARDUINO:
    import serial
    arduino = serial.Serial('/dev/ttyACM0', 9600, timeout=1)
    time.sleep(2)
    print("Connected to Arduino")

# =========================
# SEND COMMAND (SAFE)
# =========================

def send_command(cmd):
    if USE_ARDUINO:
        arduino.write((cmd + "\n").encode())
    else:
        print("SEND:", cmd)

# =========================
# LOAD TFLITE MODEL
# =========================

interpreter = tflite.Interpreter(model_path="tiny_robot_model.tflite")
interpreter.allocate_tensors()

input_details = interpreter.get_input_details()
output_details = interpreter.get_output_details()

# =========================
# ACTION MAP
# =========================

action_map = {
    0: "MOVE_FORWARD",
    1: "TURN_LEFT",
    2: "TURN_RIGHT",
    3: "STOP"
}

# =========================
# LOGIC HELPERS
# =========================

def decide_face(front, left, right):
    danger = sum([front < 8, left < 8, right < 8])
    warning = sum([front < 18, left < 18, right < 18])

    if danger >= 2:
        return "angry"
    elif danger == 1:
        return "sad"
    elif warning >= 2:
        return "annoyed"
    else:
        return random.choice(["happy", "neutral"])


def maybe_random_move(pred_action, front):
    if front > 30 and random.random() < 0.02:
        return random.choice([1, 2])
    return pred_action

# =========================
# AI LOOP
# =========================

print("\nRobot AI running...\n")

while True:

    # TODO: replace with real Arduino sensors
    front = random.randint(0, 50)
    left = random.randint(0, 50)
    right = random.randint(0, 50)

    # normalize input
    input_data = np.array([[front, left, right]], dtype=np.float32) / 50.0

    # inference
    interpreter.set_tensor(input_details[0]['index'], input_data)
    interpreter.invoke()

    pred = interpreter.get_tensor(output_details[0]['index'])
    action = np.argmax(pred)

    # small exploration tweak
    action = maybe_random_move(action, front)

    # command output
    command = action_map[action]
    send_command(command)

    # emotional state
    face = decide_face(front, left, right)

    print(
        f"Front={front:2d}, "
        f"Left={left:2d}, "
        f"Right={right:2d} -> "
        f"{command:13s} | Face={face}"
    )

    time.sleep(0.2)