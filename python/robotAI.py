import numpy as np
import random
import tensorflow as tf
import serial
import time

# Connect to Arduino
arduino = serial.Serial('/dev/ttyACM0', 9600, timeout=1)
time.sleep(2)

print("Connected to Arduino")

# Load TFLite model
interpreter = tf.lite.Interpreter(model_path="tiny_robot_model.tflite")
interpreter.allocate_tensors()

input_details = interpreter.get_input_details()
output_details = interpreter.get_output_details()

action_map = {
    0: "MOVE_FORWARD",
    1: "TURN_LEFT",
    2: "TURN_RIGHT",
    3: "STOP"
}

def send_command(cmd):
    arduino.write((cmd + "\n").encode())

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


print("\nRobot AI running...\n")

while True:

    # TODO: replace with real sensor values later
    front = random.randint(0, 50)
    left = random.randint(0, 50)
    right = random.randint(0, 50)

    input_data = np.array([[front, left, right]], dtype=np.float32) / 50.0

    interpreter.set_tensor(input_details[0]['index'], input_data)
    interpreter.invoke()

    pred_probs = interpreter.get_tensor(output_details[0]['index'])
    action = np.argmax(pred_probs)

    action = maybe_random_move(action, front)

    command = action_map[action]
    send_command(command)

    face = decide_face(front, left, right)

    print(
        f"Front={front:2d}, "
        f"Left={left:2d}, "
        f"Right={right:2d} -> "
        f"{command:13s} | Face={face}"
    )

    time.sleep(0.2)