
// #if IS_ESP32
// #include <WiFi.h>
// #include <ESPmDNS.h>
// #else
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
// #endif
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <PubSubClient.h>

#include <wifi_creds.h>
#include <elapsedMillis.h>
#include <Button2.h>

#define ON 1
#define OFF 0

WiFiClient client;
IPAddress mqtt_server(192, 168, 1, 105);
bool statusLedOn, relayOn = false;

#define SUCCESSFUL_CONNECT 1
#define FAILED_CONNECT 2
#define WIFI_RECONNECT_TIME 5000
#define MQTT_RECONNECT_TIME 1000

PubSubClient mqttclient(client);
#include <string.h>

//----------------------------------------------

bool inStr(char *str, const char *value)
{
  return strcmp(str, value) == 0;
}

bool blueLedState = OFF;
void turnBlueLed(bool on)
{
  digitalWrite(BLUE_LED, on == false);
  blueLedState = on;
  Serial.printf("turning BLUE LED %s\n", on ? "ON" : "OFF");
}

void turnRelay(bool on)
{
  relayOn = on;
  digitalWrite(RELAY_AND_LED, on);
  Serial.printf("turning relay %s\n", on ? "ON" : "OFF");

  char buff[40];
  sprintf(buff, "/device/smartsocket/SMTSKT%02d/event", SMTSKT_NUMBER);
  mqttclient.publish(
      buff,
      relayOn
          ? "ST_RELAY_ON"
          : "ST_RELAY_OFF");
}

void command_topic_cb(char *topic, byte *payload, unsigned int length)
{
  payload[length] = '\0';

  char buff[length];
  sprintf(buff, "%s", payload);

  char *command;
  char *p = (char *)payload;

  while ((command = strtok_r(p, "$", &p)) != NULL)
  {
    if (inStr(command, "LED"))
    {
      char *value = strtok_r(p, "$", &p);
      turnBlueLed(inStr(value, "ON"));
    }
    else if (inStr(command, "RELAY"))
    {
      char *value = strtok_r(p, "$", &p);
      turnRelay(inStr(value, "ON"));
    }
    else if (inStr(command, "TOGGLE"))
    {
      // no value
      relayOn = !relayOn;
      turnRelay(relayOn);
    }
  }
}

void subscribeToTopics()
{
  char topicCommand[40];
  sprintf(topicCommand, TOPIC_COMMAND_FORMAT, SMTSKT_NUMBER);
  mqttclient.subscribe(topicCommand);
}

Button2 button(BUTTON__PIN);

char hostName[40];
int otaProgress = 0;

void setup()
{
  Serial.begin(115200);
  Serial.printf("Booting...\n");

  pinMode(RELAY_AND_LED, OUTPUT);
  turnRelay(OFF);

  pinMode(BLUE_LED, OUTPUT);
  turnBlueLed(ON);
  delay(500);
  turnBlueLed(OFF);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  button.setLongClickDetectedHandler([](Button2 &btn) {
    char buff[40];
    sprintf(buff, "/device/smartsocket/SMTSKT%02d/event", SMTSKT_NUMBER);
    digitalWrite(RELAY_AND_LED, LOW);
    mqttclient.publish(buff, "EV_LONG_PRESS");
  });

  button.setClickHandler([](Button2 &btn) {
    char buff[40];
    sprintf(buff, "/device/smartsocket/SMTSKT%02d/event", SMTSKT_NUMBER);
    mqttclient.publish(buff, "EV_CLICKED");
  });

  button.setDoubleClickHandler([](Button2 &btn) {
    char buff[40];
    sprintf(buff, "/device/smartsocket/SMTSKT%02d/event", SMTSKT_NUMBER);
    mqttclient.publish(buff, "EV_DOUBLE_CLICK");
  });

  while (WiFi.waitForConnectResult() != WL_CONNECTED)
  {
    Serial.println("Connection Failed! Rebooting...");
    delay(5000);
    ESP.restart();
  }

  mqttclient.setServer(mqtt_server, 1883); // ie "192.168.1.105"
  mqttclient.setCallback(command_topic_cb);

  ArduinoOTA.setPort(3232);

  sprintf(hostName, "SMTSKT%02d", SMTSKT_NUMBER);
  ArduinoOTA.setHostname(hostName);

  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH)
      type = "sketch";
    else // U_SPIFFS
      type = "filesystem";

    // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
    Serial.println("Start updating " + type);
    turnBlueLed(OFF);
    turnRelay(OFF);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
    turnBlueLed(OFF);
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    if (total < otaProgress + 10)
    {
      blueLedState = !blueLedState;
      turnBlueLed(blueLedState);
      otaProgress += 10;
    }
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR)
      Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR)
      Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR)
      Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR)
      Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR)
      Serial.println("End Failed");
  });

  ArduinoOTA.begin();

  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  Serial.printf("OTA ready\n");
}

elapsedMillis sincePublishedOnline;
int onlineCounter = 0;

void publishDetailsPacket()
{
  char topic[40];
  sprintf(topic, TOPIC_EVENT_FORMAT, SMTSKT_NUMBER);
  char details[100];
  sprintf(details, "DETAILS: %s|%s", WiFi.localIP().toString().c_str(), SOFTWARE_VERSION);

  mqttclient.publish(topic, details);
}

void loop()
{
  if (!mqttclient.connected())
  {
    turnBlueLed(OFF);
    Serial.printf("mqttclient not connected?\n");
    if (mqttclient.connect(hostName, "skelstar", "ec11225f87"))
    {
      turnBlueLed(ON);
      Serial.println("mqtt connected");
      subscribeToTopics();
      publishDetailsPacket();
    }
  }

  if (mqttclient.connected() && sincePublishedOnline > 2000)
  {
    sincePublishedOnline = 0;
#define ONLINE_MESSAGE "ONLINE-%02ds"
    char onlineMsg[11];
    sprintf(onlineMsg, ONLINE_MESSAGE, onlineCounter);

    char topicOnline[40];
    sprintf(topicOnline, TOPIC_ONLINE_FORMAT, SMTSKT_NUMBER);

    mqttclient.publish(topicOnline, onlineMsg);
    if (onlineCounter < 500)
      onlineCounter++;
  }

  mqttclient.loop();

  button.loop();

  ArduinoOTA.handle();

  delay(10);
}
