#include <HeadlightController.h>
#include <ArduinoOTA.h>
#include <AsyncElegantOTA.h>

int headlightActivePin = 32; //Chân kích relay bật motor
int headlightReversePin = 33; //Chân kích relay đảo chiều motor
int headlightPositionPin = 35; //Chân biến trở vị trí motor
int headlightUpButtonPin = 26; //Chân nút chỉnh lên
int headlightDownButtonPin = 27; //Chân nút chỉnh xuống



/* Soft AP SSID and Password */
const char *ssidAP = "TranAnh-Vehicle";
const char *passwordAP = "12345687";

/* Soft AP IP Address details */
IPAddress local_ip(192, 168, 1, 1);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);


const char *ssid = "****";
const char *password = "****";

AsyncWebServer server(80);
bool isOTASet = false;

void setup() {
    Serial.begin(115200);
    WiFi.disconnect(); //added to start with the wifi off, avoid crashing
    WiFi.mode(WIFI_AP_STA); //added to start with the wifi off, avoid crashing
    WiFi.softAPConfig(local_ip, gateway, subnet);

    WiFi.softAP(ssidAP, passwordAP, 1, true, 5);

    WiFi.begin(ssid, password);
    //Basic setup
    HeadlightController.setupHeadlight(headlightActivePin, headlightReversePin, headlightPositionPin);
    //Enable physical button control
    HeadlightController.setupButton(headlightUpButtonPin, headlightDownButtonPin);
    //Enable web control
    HeadlightController.setupWebControl(&server);

    AsyncElegantOTA.begin(&server);

    // Start server
    server.begin();

}

void loop() {
    ArduinoOTA.handle();
    AsyncElegantOTA.loop();
    if (!isOTASet) {
        if (WiFi.status() == WL_CONNECTED) {
            setupOTA();
            isOTASet = true;
        }
    }

    HeadlightController.loop();

    //DO NOT block loop while headlight is running
    if (millis() / 1000 % 2 == 0) {
        Serial.print("Current position: ");
        Serial.println(HeadlightController.currentPositionInPercent());
    }

}


void setupOTA() {
    ArduinoOTA
            .onStart([]() {
                String type;
                if (ArduinoOTA.getCommand() == U_FLASH)
                    type = "sketch";
                else // U_SPIFFS
                    type = "filesystem";

                // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
                Serial.println("Start updating " + type);
            })
            .onEnd([]() {
                Serial.println("\nEnd");
            })
            .onProgress([](unsigned int progress, unsigned int total) {
                Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
            })
            .onError([](ota_error_t error) {
                Serial.printf("Error[%u]: ", error);
                if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
                else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
                else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
                else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
                else if (error == OTA_END_ERROR) Serial.println("End Failed");
            });

    ArduinoOTA.begin();

    Serial.println("Ready");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());

}