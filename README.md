# AI-Powered Exercise Recognition Glove

This project involves designing and creating a smart, wearable glove that recognizes different exercise movements using AI. The glove uses an **Arduino Nano BLE 33 Sense** board, an **accelerometer** and **gyroscope** to detect hand movements, and an **AI model** to classify and validate the movements. The system provides real-time feedback and progress tracking.

## Features:
- **Movement Recognition**: Differentiates between at least five different exercise movements.
- **Interactive Button Interface**: Allows users to select exercises.
- **Real-Time Feedback**: Buzzer announces the exercise and counts repetitions.
- **Statechart Management**: Visualizes and manages the system's states and transitions.
- **Hardware**: Arduino Nano BLE 33 Sense, accelerometer, gyroscope, Buzzer, and button.

## Hardware Requirements:
- Arduino Nano BLE 33 Sense
- Accelerometer and gyroscope
- Buzzer
- Push button
- LED for feedback

## How to Use:
1. Clone the repository or download the ZIP file.
2. Open the `glove.ino` file in the **Arduino IDE**.
3. Connect the **Arduino Nano BLE 33 Sense** to your computer.
4. Upload the code to the Arduino board.
5. Interact with the glove by pressing the button to cycle through different exercises.

## AI Model:
The AI model is trained on the sensor data to recognize movements. It is stored in the `model.h` file, which is integrated into the Arduino code for real-time classification.

## Data Collection:
The `Data/` folder contains the dataset used for training the AI model. You can collect more data if needed to improve classification accuracy.

## Statechart:
The system is visualized using a statechart, which helps in managing various states like **Idle**, **Counting down**, **Detecting movement**, and **Classifying gesture**. You can find the statechart diagram in the `statechart/` folder.
