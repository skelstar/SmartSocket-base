#include <Arduino.h>
#include <myWifiHelper.h>
#include <myPushButton.h>
#include <ArduinoJson.h>            // https://github.com/bblanchon/ArduinoJson

char versionText[] = "SmartSocket-base v1.1";

/* ----------------------------------------------------------- */

#define BUTTON          13
#define RELAY_REDLED    5
#define BLUE_LED        4
#define LED_ON          LOW
#define LED_OFF         HIGH

#define ON 				1
#define OFF 			0

#define     WIFI_HOSTNAME 	"/device/smartsocket/SMTSKT01"
#define 	TOPIC_ONLINE	"/device/smartsocket/SMTSKT01/online"
#define     TOPIC_EVENT     "/device/smartsocket/SMTSKT01/event"
#define     TOPIC_COMMAND   "/device/smartsocket/SMTSKT01/command"
#define		TOPIC_TIMESTAMP	"/dev/timestamp"


#define     EVENT_BUTTON_PUSHED     "button pushed"
#define     EVENT_BUTTON_HELD       "button held"

/* ----------------------------------------------------------- */

MyWifiHelper wifiHelper(WIFI_HOSTNAME);

//--------------------------------------------------------------------------------
void button_callback( int eventCode, int eventPin, int eventParam );

#define 	PULLUP	true
#define 	OFF_STATE_HIGH	1
myPushButton button(BUTTON, PULLUP, OFF_STATE_HIGH, button_callback);

void button_callback( int eventCode, int eventPin, int eventParam ) {

	char payload1[20];
	char numSecsBuff[3];

    switch (eventCode) {
        case button.EV_BUTTON_PRESSED:
            Serial.println("EV_BUTTON_PRESSED");
            break;
        case button.EV_RELEASED:
            Serial.println("EV_RELEASED");
            wifiHelper.mqttPublish(TOPIC_EVENT, EVENT_BUTTON_PUSHED);
            break;
		case button.EV_HELD_SECONDS:
		 	strcpy(payload1, "EV_HELD_SECONDS_");
		 	itoa(eventParam, numSecsBuff, 10);
		 	strcat(payload1, numSecsBuff);
		 	puts(payload1);
		 	wifiHelper.mqttPublish(TOPIC_EVENT, payload1);
            break;
        default:    
            break;
    }
}
//--------------------------------------------------------------------------------

void mqttcallback_timestamp(byte* payload, unsigned int length) {
	wifiHelper.mqttPublish(TOPIC_ONLINE, "1");
}

void mqttcallback_Command(byte *payload, unsigned int length) {

	payload[length] = '\0';
	Serial.println((char*) payload);
	
	const char d[2] = "$";
	char* command = strtok((char*)payload, d);

	if (strcmp(command, "LED") == 0) {
		char* value = strtok(NULL, d);
		if (strcmp(value, "ON") == 0) {
			turnBlueLed(ON);
		}
		else if (strcmp(value, "OFF") == 0) {
			turnBlueLed(OFF);
		}
	}

    if (strcmp(command, "RELAY") == 0) {
		char* value = strtok(NULL, d);
        if (strcmp(value, "ON") == 0) {
            turnRelay(ON);
        }
        else if (strcmp(value, "OFF") == 0) {
            turnRelay(OFF);
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
    turnBlueLed(OFF);

    pinMode(RELAY_REDLED, OUTPUT);
    turnRelay(OFF);

    wifiHelper.setupWifi();
    wifiHelper.setupOTA(WIFI_HOSTNAME);
    wifiHelper.setupMqtt();

    wifiHelper.mqttAddSubscription(TOPIC_TIMESTAMP, mqttcallback_timestamp);
	wifiHelper.mqttAddSubscription(TOPIC_COMMAND, mqttcallback_Command);
}

/* ----------------------------------------------------------- */

void loop() {

    ArduinoOTA.handle();

    wifiHelper.loopMqtt();

    button.serviceEvents();

    delay(10);
}

//--------------------------------------------------------------------------------

void turnRelay(int onoff) {
	digitalWrite(RELAY_REDLED, onoff == ON ? HIGH : LOW);
}

void turnBlueLed(int onoff) {
	digitalWrite(BLUE_LED, onoff == ON ? LOW : HIGH);
}
