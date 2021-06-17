#ifndef HeadlightControllerClass_h
#define HeadlightControllerClass_h

#include <PinButton.h>
#include <ESPAsyncWebServer.h>

#include <SimpleKalmanFilter.h>

#include <Preferences.h>


#include "HeadlightHtml.h"

#ifdef ESP32
#define IS_ESP32 true
#include "HeadlightESP32LUT.h"
#else
#define IS_ESP32 false
#endif


class HeadlightControllerClass {

public:

    void setupHeadlight(int activePin, int reversePin, int positionPin) {
        headlightActivePin = activePin;
        headlightReversePin = reversePin;
        headlightPositionPin = positionPin;
        hlPreferences.begin("headlight", false);

        topPosition = hlPreferences.getInt("topPosition", maxAdcPosition);
        botPosition = hlPreferences.getInt("botPosition", 0);

        pinMode(headlightActivePin, OUTPUT);
        pinMode(headlightReversePin, OUTPUT);
        pinMode(headlightPositionPin, INPUT);
    }

    void setupButton(int upPin, int downPin) {
        pinMode(upPin, INPUT_PULLUP);
        pinMode(downPin, INPUT_PULLUP);

        upButton = PinButton(upPin);
        downButton = PinButton(downPin);
        hasButton = true;
    }

    void setupWebControl(AsyncWebServer *server) {
        internalServerSetup(server);
    }

    void loop() {
        internalLoop();
    }

    bool isHeadlightActivating() {
        return digitalRead(headlightActivePin) == HIGH;
    }

    bool isHeadlightReversing() {
        return digitalRead(headlightReversePin) == HIGH;
    }

    int getHeadlightStopTime() {
        return headlightStopTime;
    }

    int currentPositionInPercent() {
        return round(currentPosition * 100 / maxAdcPosition);
    }

    void setHeadlightTimeoutInMillis(int timeout) {
        headlightTimeout = timeout;
    }

    void setMaxAdc(int maxAdc) {
        maxAdcPosition = maxAdc;
    }

    void setDistanceUpFix(int upFix) {
        distanceUpFix = upFix;
    }

    void setDistanceDownFix(int downFix) {
        distanceDownFix = downFix;
    }

#ifdef ESP32
    float calibratedESP32Adc(int inputADC) {
        return calibratedHeadlightADC(inputADC);
    }
#else
    float calibratedESP32Adc(int inputADC) {
        return inputADC;
    }
#endif

private:
#define HEADLIGHT_UP 1
#define HEADLIGHT_DOWN 2
#define HEADLIGHT_NONE 0
//Constant
#ifdef ESP32
    int maxAdcPosition = 4096; //Tùy theo độ phân giải ADC của mcu hoặc theo thực tế. ESP32 là 4096, nhưng thực tế max biến trở của mình chỉ đc 4029
#else
    int maxAdcPosition = 1024;
#endif

    int headlightActivePin; //Chân kích relay bật motor
    int headlightReversePin; //Chân kích relay đảo chiều motor
    int headlightPositionPin; //Chân biến trở vị trí motor
    PinButton upButton = NULL;
    PinButton downButton = NULL;
    bool hasButton = false;


    int topPosition; //Lưu vị trí cao
    int botPosition; //Lưu vị trí thấp
    int acceptDistance = 5; //Xê dịch do nhiễu ADC
    int headlightTimeout = 15000; //Thời gian chạy max, đề phòng kẹt nút

    //Do motor chạy có đà
    int distanceUpFix = 300;
    int distanceDownFix = 300;

    int targetPosition = -1;

    float currentPosition = 0;

    int headlightStartTime = 0;
    int headlightStopTime = 0;

    int headlightDirection = HEADLIGHT_NONE;

    Preferences hlPreferences;
    SimpleKalmanFilter headlightKalman = SimpleKalmanFilter(2, 2, 0.01);

    float totalPosition = 0;
    int totalPositionCount = 0;
    int maxPositionSampleCount = 10;

    void internalLoop() {
        checkStop();

        handlePhysicalButton();
        if (totalPositionCount < maxPositionSampleCount) {
            float rawAdc = 0;

            rawAdc = IS_ESP32 ? calibratedHeadlightADC(analogRead(headlightPositionPin))
                              : analogRead(headlightPositionPin);

            totalPosition += rawAdc;
            totalPositionCount++;
        } else {
            float measured = totalPosition / totalPositionCount;
            currentPosition = headlightKalman.updateEstimate(measured);
            totalPosition = 0;
            totalPositionCount = 0;
        }

    }

    void checkStop() {
        if (targetPosition != -1) {
            if (
                    (headlightDirection == HEADLIGHT_DOWN && currentPosition - distanceDownFix <= targetPosition) ||
                    //lower limit
                    (headlightDirection == HEADLIGHT_UP &&
                     currentPosition + distanceUpFix >= targetPosition) //upper limit
                    )
                headlightStop(); //Target reached
        }
        if (
                (millis() - headlightStartTime) > headlightTimeout || //timeout
                (headlightDirection == HEADLIGHT_DOWN && abs(currentPosition) < acceptDistance) || //lower limit
                (headlightDirection == HEADLIGHT_UP &&
                 abs(currentPosition - maxAdcPosition) < acceptDistance) //upper limit
                )
            headlightStop(); //Prevent over limit

        if (!isHeadlightActivating()) {
            if (millis() - headlightStopTime > 3000) headlightStopTime = 0;
        }

    }

    void handlePhysicalButton() {
        if (!hasButton) return;
        upButton.update();
        downButton.update();

        if (upButton.isSingleClick()) {
            targetPosition = topPosition;
            adjustToTargetPosition(currentPosition);
            Serial.println("up single");
        } else if (upButton.isDoubleClick()) {
            saveTopPosition(currentPosition);
            Serial.println("up double");
        } else if (upButton.isLongClick()) {
            targetPosition = -1;
            headlightUp();
            Serial.println("up long");
        } else if (upButton.isReleased()) {
            Serial.println("up release");
            headlightStop();
        } else {
            if (downButton.isSingleClick()) {
                targetPosition = botPosition;
                adjustToTargetPosition(currentPosition);
                Serial.println("dn single");
            } else if (downButton.isDoubleClick()) {
                saveBotPosition(currentPosition);
                Serial.println("dn double");
            } else if (downButton.isLongClick()) {
                targetPosition = -1;
                headlightDown();
                Serial.println("dn long");
            } else if (downButton.isReleased()) {
                Serial.println("dn release");
                headlightStop();
            }
        }
    }

    void adjustToTargetPosition(float currentPosition) {
        if (currentPosition > targetPosition) headlightDown();
        else if (currentPosition < targetPosition) headlightUp();
        else headlightStop();
    }

    void headlightUp() {
        headlightStartTime = millis();
        digitalWrite(headlightActivePin, HIGH);
        digitalWrite(headlightReversePin, LOW);
        headlightDirection = HEADLIGHT_UP;
    }

    void headlightDown() {
        headlightStartTime = millis();
        digitalWrite(headlightActivePin, HIGH);
        digitalWrite(headlightReversePin, HIGH);
        headlightDirection = HEADLIGHT_DOWN;
    }

    void headlightStop() {
        if (headlightDirection == HEADLIGHT_NONE) return;
        digitalWrite(headlightActivePin, LOW);
        digitalWrite(headlightReversePin, LOW);

        headlightStartTime = 0;
        headlightStopTime = millis();
        targetPosition = -1;

        headlightDirection = HEADLIGHT_NONE;
    }

    void saveBotPosition(float currentPosition) {
        botPosition = currentPosition;
        hlPreferences.putInt("botPosition", (int) currentPosition);
    }

    void saveTopPosition(float currentPosition) {
        topPosition = currentPosition;
        hlPreferences.putInt("topPosition", (int) currentPosition);
    }

    String paramProcessor(String &var) {
        if (var == "CURRENT_POSITION") {
            return String(currentPosition);
        } else if (var == "MAX_POSITION") {
            return String(maxAdcPosition);
        } else if (var == "TOP_POSITION") {
            return String(topPosition);
        } else if (var == "BOT_POSITION") {
            return String(botPosition);
        }
        return String();
    }

    void internalServerSetup(AsyncWebServer *server) {
        server->on("/headlight-controller", HTTP_GET, [&](AsyncWebServerRequest *request) {
            request->send_P(200, "text/html", webControlHtml, [&](String s) {
                return paramProcessor(s);
            });

        });

        server->on("/adjust-headlight", HTTP_GET, [&](AsyncWebServerRequest *request) {
            String inputMessage;
            if (request->hasParam("value")) {
                inputMessage = request->getParam("value")->value();
                int sliderValue = inputMessage.toInt();
                targetPosition = sliderValue;
                adjustToTargetPosition(currentPosition);
            }
            request->send(200, "text/plain", "OK");
        });

        server->on("/save-top", HTTP_GET, [&](AsyncWebServerRequest *request) {
            String inputMessage;
            if (request->hasParam("value")) {
                inputMessage = request->getParam("value")->value();
                int sliderValue = inputMessage.toInt();
                saveTopPosition(sliderValue);
            }
            request->send(200, "text/plain", "OK");
        });

        server->on("/save-bottom", HTTP_GET, [&](AsyncWebServerRequest *request) {
            String inputMessage;
            if (request->hasParam("value")) {
                inputMessage = request->getParam("value")->value();
                int sliderValue = inputMessage.toInt();
                saveBotPosition(sliderValue);
            }
            request->send(200, "text/plain", "OK");
        });
        server->on("/fast-top", HTTP_GET, [&](AsyncWebServerRequest *request) {
            targetPosition = topPosition;
            adjustToTargetPosition(currentPosition);
            request->send(200, "text/plain", "OK");
        });
        server->on("/fast-bottom", HTTP_GET, [&](AsyncWebServerRequest *request) {
            targetPosition = botPosition;
            adjustToTargetPosition(currentPosition);
            request->send(200, "text/plain", "OK");
        });

        server->on("/currentPosition", HTTP_GET, [&](AsyncWebServerRequest *request) {
            request->send(200, "text/plane", String(currentPosition));
        });
    }
};

HeadlightControllerClass HeadlightController;

#endif