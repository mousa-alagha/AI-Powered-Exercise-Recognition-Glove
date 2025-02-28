#include "model.h"
#include <Arduino_LSM9DS1.h>
#include <TensorFlowLite.h>
#include <tensorflow/lite/micro/all_ops_resolver.h>
#include <tensorflow/lite/micro/micro_interpreter.h>
#include <tensorflow/lite/schema/schema_generated.h>
#include <tensorflow/lite/micro/micro_error_reporter.h>

#define PB 2
#define RED LED_BUILTIN
#define BUZZER A7
#define NUM_SAMPLES 100
#define NUM_AXES 6

constexpr int tensorArenaSize = 8 * 1024;
byte tensorArena[tensorArenaSize] __attribute__((aligned(16)));

const char* GESTURES[] = {"punch", "flex", "stab", "slash", "upper"};

tflite::AllOpsResolver tflOpsResolver;
const tflite::Model* tflModel = nullptr;
tflite::MicroInterpreter* tflInterpreter = nullptr;
TfLiteTensor* tflInputTensor = nullptr;
TfLiteTensor* tflOutputTensor = nullptr;

unsigned long prevTime = 0;
unsigned long lastButtonPressTime = 0;
unsigned long lastIdleMsgTime = 0;
int numOfPresses = 0;
int curPB = 1, prevPB = 1;
bool recording = false;
float aX, aY, aZ, gX, gY, gZ;
float scores[5];
int selectedGesture = -1;
float sensorData[NUM_SAMPLES][NUM_AXES];

enum State {
    Idle,
    Countdown,
    DetectingGesture,
    CollectData,
    Classify,
    Success,
    Failure
};

State currentState = Idle;

void setup() {
    Serial.begin(9600);
    pinMode(PB, INPUT_PULLUP);
    pinMode(BUZZER, OUTPUT);
    pinMode(RED, OUTPUT);
    digitalWrite(RED, LOW);

    tflModel = tflite::GetModel(model);
    if (tflModel->version() != 3) {
        Serial.println("Model version mismatch!");
        while (1);
    }

    static tflite::MicroInterpreter staticInterpreter(
        tflModel, tflOpsResolver, tensorArena, tensorArenaSize, nullptr);
    tflInterpreter = &staticInterpreter;
    tflInterpreter->AllocateTensors();
    tflInputTensor = tflInterpreter->input(0);
    tflOutputTensor = tflInterpreter->output(0);

    if (!IMU.begin()) {
        Serial.println("Failed to Load Sensors");
        while (1);
    }

    lastButtonPressTime = millis();
    lastIdleMsgTime = millis();
}

void collectSensorData() {
    for (int i = 0; i < NUM_SAMPLES; i++) {
        if (IMU.accelerationAvailable() && IMU.gyroscopeAvailable()) {
            IMU.readAcceleration(aX, aY, aZ);
            IMU.readGyroscope(gX, gY, gZ);

            sensorData[i][0] = aX;
            sensorData[i][1] = aY;
            sensorData[i][2] = aZ;
            sensorData[i][3] = gX;
            sensorData[i][4] = gY;
            sensorData[i][5] = gZ;
        }
        delay(10);
    }
}

void populateTensorInput() {
    int tensor_idx = 0;
    for (int i = 0; i < NUM_SAMPLES; i++) {
        for (int j = 0; j < NUM_AXES; j++) {
            tflInputTensor->data.f[tensor_idx] = sensorData[i][j];
            tensor_idx++;
        }
    }
}

void resetSystem() {
    currentState = Idle;
    numOfPresses = 0;
    selectedGesture = -1;
    lastButtonPressTime = millis();
    digitalWrite(RED, LOW);
}

void loop() {
    curPB = digitalRead(PB);
    unsigned long currentTime = millis();

    switch (currentState) {
        case Idle: {
            if (currentTime - lastIdleMsgTime >= 5000) {
                Serial.println("Please select a movement by pressing the button!");
                lastIdleMsgTime = currentTime;
            }

            if (curPB == LOW && prevPB == HIGH) {
                numOfPresses++;
                lastButtonPressTime = currentTime;
                Serial.println("Button press detected: " + String(numOfPresses));
            }

            if (numOfPresses == 0 && (currentTime - lastButtonPressTime >= 10000)) {
                selectedGesture = 0;
                Serial.println("Timeout reached - Defaulting to Punch detection");
                currentState = Countdown;
            } else if (numOfPresses > 0 && (currentTime - lastButtonPressTime >= 2000)) {
                selectedGesture = numOfPresses - 1;
                if (selectedGesture >= 0 && selectedGesture < 5) {
                    Serial.println("Selected gesture: " + String(GESTURES[selectedGesture]));
                    currentState = Countdown;
                } else {
                    Serial.println("Invalid number of presses, resetting...");
                    numOfPresses = 0;
                }
            }
            break;
        }

        case Countdown: {
            Serial.println("Get ready! Movement detection starting in:");
            for (int count = 3; count > 0; count--) {
                Serial.println(count);
                delay(1000);
            }
            Serial.println("Go!");
            currentState = DetectingGesture;
            break;
        }

        case DetectingGesture: {
            if (IMU.accelerationAvailable()) {
                IMU.readAcceleration(aX, aY, aZ);
                float aSum = fabs(aX) + fabs(aY) + fabs(aZ);
                if (aSum >= 1.5) {
                    currentState = CollectData;
                    recording = true;
                    Serial.println("Movement detected, starting recording...");
                }
            }
            break;
        }

        case CollectData: {
            if (!recording) return;
            collectSensorData();
            populateTensorInput();
            recording = false;
            currentState = Classify;
            Serial.println("Data collection complete, classifying...");
            break;
        }

        case Classify: {
            tflInterpreter->Invoke();
            for (int i = 0; i < 5; i++) {
                scores[i] = tflOutputTensor->data.f[i];
                Serial.print(GESTURES[i]);
                Serial.print(": ");
                Serial.println(scores[i]);
            }

            float selectedScore = scores[selectedGesture];
            Serial.print("Selected gesture score: ");
            Serial.println(selectedScore);

            if (selectedScore > 0.7) {
                currentState = Success;
            } else {
                currentState = Failure;
            }
            break;
        }

        case Success: {
            Serial.println("Correct movement detected!");
            tone(BUZZER, 1000);
            delay(1000);
            noTone(BUZZER);
            resetSystem();
            break;
        }

        case Failure: {
            Serial.println("Movement not recognized!");
            digitalWrite(RED, HIGH);
            delay(1000);
            digitalWrite(RED, LOW);
            resetSystem();
            break;
        }
    }

    prevPB = curPB;
}
