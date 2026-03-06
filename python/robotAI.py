import numpy as np
import random
import tensorflow as tf

num_samples = 4000
data_X = []
data_y = []

for _ in range(num_samples):
    if random.random() < 0.65:
        front = random.randint(25, 50)
    else:
        front = random.randint(0, 25)

    if front < 20:
        left = random.randint(10, 50)
        right = random.randint(10, 50)
    else:
        left = random.randint(0, 50)
        right = random.randint(0, 50)

    if front < 8:
        action = 3
    elif front < 20:
        action = 1 if left > right else 2
    else:
        action = 0

    data_X.append([front, left, right])
    data_y.append(action)

X = np.array(data_X, dtype=float)
y = np.array(data_y)

X = X / 50.0



model = tf.keras.Sequential([
    tf.keras.layers.Input(shape=(3,)),
    tf.keras.layers.Dense(12, activation='relu'),
    tf.keras.layers.Dense(8, activation='relu'),
    tf.keras.layers.Dense(4, activation='softmax')
])

model.compile(
    optimizer=tf.keras.optimizers.Adam(learning_rate=0.001),
    loss='sparse_categorical_crossentropy',
    metrics=['accuracy']
)

print("Training tiny robot AI...")
model.fit(X, y, epochs=40, batch_size=32, verbose=1)



converter = tf.lite.TFLiteConverter.from_keras_model(model)
tflite_model = converter.convert()

with open("tiny_robot_model.tflite", "wb") as f:
    f.write(tflite_model)

print("Model converted to tiny_robot_model.tflite!")



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
    # Only explore when safe
    if front > 30 and random.random() < 0.02:
        return random.choice([1, 2])  # random turn
    return pred_action



print("\nSimulating robot behavior:\n")

action_map = {
    0: "MOVE_FORWARD",
    1: "TURN_LEFT",
    2: "TURN_RIGHT",
    3: "STOP"
}

for _ in range(200000):

    test_front = random.randint(0, 50)
    test_left = random.randint(0, 50)
    test_right = random.randint(0, 50)

    input_data = np.array([[test_front, test_left, test_right]], dtype=float) / 50.0

    pred_probs = model.predict(input_data, verbose=0)
    action = np.argmax(pred_probs)

    action = maybe_random_move(action, test_front)
    face = decide_face(test_front, test_left, test_right)

    print(
        f"Front={test_front:2d}, "
        f"Left={test_left:2d}, "
        f"Right={test_right:2d} -> "
        f"{action_map[action]:13s} | Face={face}"
    )
