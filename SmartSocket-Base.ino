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

#define     WIFI_HOSTNAME 	"/device/smartsocket/SMTSKT04"
#define     TOPIC_EVENT     "/device/smartsocket/SMTSKT04/event"
#define     TOPIC_ONLINE    "/device/smartsocket/SMTSKT04/online"
#define     TOPIC_COMMAND   "/device/smartsocket/SMTSKT04/command"
#define		TOPIC_TIMESTAMP	"/dev/timestamp"

/* ----------------------------------------------------------- */

MyWifiHelper wifiHelper(WIFI_HOSTNAME);


void turnRelay(int onoff);
void turnBlueLed(int onoff);

//--------------------------------------------------------------------------------
void button_callback( int eventCode, int eventPin, int eventParam );

#define 	PULLUP	true
#define 	OFF_STATE_HIGH	1
myPushButton button(BUTTON, PULLUP, OFF_STATE_HIGH, button_callback, 500);

void button_callback( int eventCode, int eventPin, int eventParam ) {

	char payload1[20];
	char numSecsBuff[3];

    switch (eventCode) {
        case button.EV_BUTTON_PRESSED:
            Serial.println("EV_BUTTON_PRESSED");
            break;
        case button.EV_SPECFIC_TIME_REACHED:
        	Serial.println("EV_SPECFIC_TIME_REACHED");
		 	wifiHelper.mqttPublish(TOPIC_EVENT, "EV_SPECFIC_TIME_REACHED");
        	break;
        case button.EV_RELEASED:
            Serial.println("EV_RELEASED");
            wifiHelper.mqttPublish(TOPIC_EVENT, "EV_RELEASED");
            break;
		case button.EV_HELD_SECONDS:
		 	strcpy(payload1, "EV_HELD_SECONDS_");
		 	itoa(eventParam, numSecsBuff, 10);
		 	strcat(payload1, numSecsBuff);
		 	puts(payload1);
            Serial.println(payload1);
		 	wifiHelper.mqttPublish(TOPIC_EVENT, payload1);
            break;
        default:    
            break;
    }
}
//--------------------------------------------------------------------------------
int onlineCounter = 0;
char buff[4];

void mqttcallback_timestamp(byte* payload, unsigned int length) {
	
	#define ONLINE_MESSAGE	"ONLINE-%02ds"
	char onlineMsg[11];
	sprintf(onlineMsg, ONLINE_MESSAGE, onlineCounter);
	wifiHelper.mqttPublish(TOPIC_ONLINE, onlineMsg);
	if (onlineCounter < 500)
		onlineCounter++;
}

void mqttcallback_Command(byte *payload, unsigned int length) {

	payload[length] = '\0';
	//Serial.println((char*) payload);
	
	const char d[2] = "$";
	char* command;	// = strtok((char*)payload, d);
	char* p = (char*)payload;

	while ((command = strtok_r(p, "$", &p)) != NULL) {
		if (strcmp(command, "LED") == 0) {
			char* value = strtok_r(p, "$", &p);
			if (strcmp(value, "ON") == 0) {
				turnBlueLed(ON);
			}
			else if (strcmp(value, "OFF") == 0) {
				turnBlueLed(OFF);
			}			
		}
		else if (strcmp(command, "RELAY") == 0) {
			char* value = strtok_r(p, "$", &p);
			if (strcmp(value, "ON") == 0) {
				turnRelay(ON);
			}
			else if (strcmp(value, "OFF") == 0) {
				turnRelay(OFF);
			}
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
    turnBlueLed(ON);

    pinMode(RELAY_REDLED, OUTPUT);
    turnRelay(OFF);

    wifiHelper.setupWifi();
    wifiHelper.setupOTA(WIFI_HOSTNAME);
    wifiHelper.setupMqtt();

    wifiHelper.mqttAddSubscription(TOPIC_TIMESTAMP, mqttcallback_timestamp);
	wifiHelper.mqttAddSubscription(TOPIC_COMMAND, mqttcallback_Command);

	wifiHelper.loopMqtt();
	wifiHelper.mqttPublish(TOPIC_EVENT, "BOOT");
	wifiHelper.loopMqtt();
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
	Serial.printf("turnRelay(%d)\n", onoff);
}

void turnBlueLed(int onoff) {
	digitalWrite(BLUE_LED, onoff == ON ? LOW : HIGH);
	Serial.printf("turnBlueLed(%d)\n", onoff);
}
