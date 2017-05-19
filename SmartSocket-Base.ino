#include <Arduino.h>
#include <myWifiHelper.h>
#include <myPushButton.h>

char versionText[] = "SmartSocket-base v0.9";

/* ----------------------------------------------------------- */

#define BUTTON          13
#define RELAY_REDLED    5
#define BLUE_LED        4
#define LED_ON          LOW
#define LED_OFF         HIGH

/* ----------------------------------------------------------- */
#define     WIFI_HOSTNAME "SmartSocket-base"

MyWifiHelper wifiHelper(WIFI_HOSTNAME);

//--------------------------------------------------------------------------------
void button_callback(int eventCode, int eventParam);
myPushButton button(BUTTON, true, 3000, HIGH, button_callback);

void button_callback(int eventCode, int eventParam) {
    switch (eventParam) {
        case button.EV_BUTTON_PRESSED:
            Serial.println("EV_BUTTON_PRESSED");
            break;
        case button.EV_HELD_FOR_LONG_ENOUGH:
            Serial.println("EV_HELD_FOR_LONG_ENOUGH");
            break;
        case button.EV_RELEASED:
            Serial.println("EV_RELEASED");
            break;
        case button.EV_RELEASED_FROM_HELD_TIME:
            Serial.println("EV_RELEASED_FROM_HELD_TIME");
            break;
        default:    
            break;
    }
}
//--------------------------------------------------------------------------------

void setup() {

    Serial.begin(9600);
    delay(100);
    Serial.println("Booting");
    Serial.println(versionText);


    wifiHelper.setupWifi();
    wifiHelper.setupOTA(WIFI_HOSTNAME);
    wifiHelper.setupMqtt();
}

/* ----------------------------------------------------------- */

void loop() {

    ArduinoOTA.handle();

    wifiHelper.loopMqtt();

    button.serviceEvents();

    delay(10);
}
