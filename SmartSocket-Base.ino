#include <Arduino.h>
#include <myWifiHelper.h>
#include <myPushButton.h>
#include <ArduinoJson.h>            // https://github.com/bblanchon/ArduinoJson

char versionText[] = "SmartSocket-base v0.9";

/* ----------------------------------------------------------- */

#define BUTTON          13
#define RELAY_REDLED    5
#define BLUE_LED        4
#define LED_ON          LOW
#define LED_OFF         HIGH

#define     TOPIC_EVENT     "/smartsocket-base/event"
#define     TOPIC_COMMAND   "/smartsocket-base/command"


#define     EVENT_BUTTON_PUSHED     "button pushed"
#define     EVENT_BUTTON_HELD       "button held"

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
            wifiHelper.mqttPublish(TOPIC_EVENT, EVENT_BUTTON_HELD);
            break;
        case button.EV_RELEASED:
            Serial.println("EV_RELEASED");
            wifiHelper.mqttPublish(TOPIC_EVENT, EVENT_BUTTON_PUSHED);
            break;
        case button.EV_RELEASED_FROM_HELD_TIME:
            Serial.println("EV_RELEASED_FROM_HELD_TIME");
            break;
        default:    
            break;
    }
}
//--------------------------------------------------------------------------------
void mqttcallback_Command(byte *payload, unsigned int length) {

    const char* command = wifiHelper.mqttGetJsonCommand(payload);
    const char* value = wifiHelper.mqttGetJsonCommandValue();

    // Serial.print("command: "); Serial.println(command);
    // Serial.print("value: "); Serial.println(value);

    if (strcmp(command, "LED") == 0) {
        if (strcmp(value, "ON") == 0) {
            digitalWrite(BLUE_LED, LOW);
        }
        else if (strcmp(value, "OFF") == 0) {
            digitalWrite(BLUE_LED, HIGH);
        }
    }
}

//--------------------------------------------------------------------------------

void setup() {

    Serial.begin(9600);
    delay(100);
    Serial.println("Booting");
    Serial.println(versionText);

    pinMode(BLUE_LED, OUTPUT);
    digitalWrite(BLUE_LED, HIGH);


    wifiHelper.setupWifi();
    wifiHelper.setupOTA(WIFI_HOSTNAME);
    wifiHelper.setupMqtt();

    wifiHelper.mqttAddSubscription(TOPIC_COMMAND, mqttcallback_Command);
}

/* ----------------------------------------------------------- */

void loop() {

    ArduinoOTA.handle();

    wifiHelper.loopMqtt();

    button.serviceEvents();

    delay(10);
}
