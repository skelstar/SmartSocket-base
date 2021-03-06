#include <Arduino.h>
#include <myWifiHelper.h>
#include <myPushButton.h>
#include <ArduinoJson.h> // https://github.com/bblanchon/ArduinoJson

/* ----------------------------------------------------------- */

#define BUTTON 13
#define RELAY_REDLED 5
#define BLUE_LED 4
#define LED_ON LOW
#define LED_OFF HIGH

#define ON 1
#define OFF 0

#define WIFI_HOSTNAME "/device/smartsocket/SMTSKT02"
#define TOPIC_EVENT "/device/smartsocket/SMTSKT02/event"
#define TOPIC_ONLINE "/device/smartsocket/SMTSKT02/online"
#define TOPIC_COMMAND "/device/smartsocket/SMTSKT02/command"
#define TOPIC_TIMESTAMP "/dev/timestamp"

/* ----------------------------------------------------------- */

MyWifiHelper wifiHelper(WIFI_HOSTNAME);

void setRelay(int onoff);
void setBlueLed(int onoff);

//--------------------------------------------------------------------------------
void button_callback(int eventCode, int eventPin, int eventParam);

#define PULLUP true
#define OFF_STATE_HIGH 1
myPushButton button(BUTTON, PULLUP, OFF_STATE_HIGH, button_callback, 500);

void button_callback(int eventCode, int eventPin, int eventParam)
{

	char payload1[20];
	char numSecsBuff[3];

	switch (eventCode)
	{
	case button.EV_SPECFIC_TIME_REACHED:
		Serial.println("EV_SPECFIC_TIME_REACHED");
		wifiHelper.mqttPublish(TOPIC_EVENT, "EV_SPECFIC_TIME_REACHED");
		break;
	case button.EV_RELEASED:
		Serial.println("EV_RELEASED");
		wifiHelper.mqttPublish(TOPIC_EVENT, "EV_RELEASED");
		break;
	default:
		break;
	}
}
//--------------------------------------------------------------------------------
int onlineCounter = 0;
char buff[4];

void mqttcallback_timestamp(byte *payload, unsigned int length)
{

#define ONLINE_MESSAGE "ONLINE-%02ds"
	char onlineMsg[11];
	sprintf(onlineMsg, ONLINE_MESSAGE, onlineCounter);
	wifiHelper.mqttPublish(TOPIC_ONLINE, onlineMsg);
	if (onlineCounter < 500)
	{
		onlineCounter++;
	}
}

void mqttcallback_Command(byte *payload, unsigned int length)
{

	payload[length] = '\0';
	//Serial.println((char*) payload);

	const char d[2] = "$";
	char *command; // = strtok((char*)payload, d);
	char *p = (char *)payload;

	while ((command = strtok_r(p, "$", &p)) != NULL)
	{
		char *value = strtok_r(p, "$", &p);

		if (inStr(command, "LED") && inStr(value, "ON"))
		{
			setBlueLed(ON);
		}
		else if (inStr(command, "LED") && inStr(value, "OFF"))
		{
			setBlueLed(OFF);
		}
		else if (inStr(command, "RELAY") && inStr(value, "ON"))
		{
			setRelay(ON);
		}
		else if (inStr(command, "RELAY") && inStr(value, "OFF"))
		{
			setRelay(OFF);
		}
	}
}

bool inStr(char *str, const char *value)
{
	return strcmp(str, value) == 0;
}

//--------------------------------------------------------------------------------

void setup()
{

	Serial.begin(9600);
	delay(100);
	Serial.println("Booting");

	pinMode(BLUE_LED, OUTPUT);
	setBlueLed(ON);

	pinMode(RELAY_REDLED, OUTPUT);
	setRelay(OFF);

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

void loop()
{

	ArduinoOTA.handle();

	wifiHelper.loopMqtt();

	button.serviceEvents();

	delay(10);
}

//--------------------------------------------------------------------------------

void setRelay(int onoff)
{
	digitalWrite(RELAY_REDLED, onoff == ON ? HIGH : LOW);
	Serial.printf("setRelay(%d)\n", onoff);
}

void setBlueLed(int onoff)
{
	digitalWrite(BLUE_LED, onoff == ON ? LOW : HIGH);
	Serial.printf("setBlueLed(%d)\n", onoff);
}
