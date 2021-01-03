
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

bool inStr(char *str, const char *value)
{
  return strcmp(str, value) == 0;
}

void turnLed(bool on)
{
  // digitalWrite(RELAY_AND_LED, on == false);
  // Serial.printf("turning led %d\n", on);
}

void turnRelay(bool on)
{
  digitalWrite(RELAY_AND_LED, on == false);
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
    char *value = strtok_r(p, "$", &p);
    if (inStr(command, "LED"))
    {
      turnLed(inStr(value, "ON"));
    }
    else if (inStr(command, "RELAY"))
    {
      turnRelay(inStr(value, "ON"));
    }
  }
}
#define MAX_SUBSCRIPTIONS 6

// struct subscriptionType
// {
//   char *topic;
//   SubscriptionCallbackType callback;
// };

// subscriptionType subscription[MAX_SUBSCRIPTIONS];

#define SUCCESSFUL_CONNECT 1
#define FAILED_CONNECT 2
#define WIFI_RECONNECT_TIME 5000
#define MQTT_RECONNECT_TIME 1000

PubSubClient mqttclient(client);
#include <string.h>

void subscribeToTopics()
{
  //   mqttclient.subscribe(TOPIC_COMMAND);
  //   // for (int i = 0; i < mqttSubHead; i++)
  //   // {
  //   //   mqttclient.subscribe(subscription[i].topic);
  //   // }
}

Button2 button(BUTTON_PIN);

void setup()
{
  Serial.begin(115200);
  Serial.printf("Booting...\n");

  pinMode(RELAY_AND_LED, OUTPUT);
  digitalWrite(STATUS_LED, LOW); // default: ON

  pinMode(BLUE_LED, OUTPUT);
  digitalWrite(BLUE_LED, LOW); // OFF

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  button.setClickHandler([](Button2 &btn) {
    char buff[40];
    sprintf(buff, "/device/smartsocket/SMTSKT%02d/event", SMTSKT_NUMBER);
    mqttclient.publish(buff, "EV_RELEASED");
    relayOn = !relayOn;
    digitalWrite(RELAY_AND_LED, relayOn);
    Serial.printf("relay: %d\n", relayOn);
  });

  while (WiFi.waitForConnectResult() != WL_CONNECTED)
  {
    Serial.println("Connection Failed! Rebooting...");
    delay(5000);
    ESP.restart();
  }

  digitalWrite(STATUS_LED, LOW);

  mqttclient.setServer(mqtt_server, 1883); // ie "192.168.1.105"
  mqttclient.setCallback(command_topic_cb);

  ArduinoOTA.setPort(3232);
  ArduinoOTA.setHostname(HOST_NAME);

  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH)
      type = "sketch";
    else // U_SPIFFS
      type = "filesystem";

    // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
    Serial.println("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
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

void loop()
{
  if (!mqttclient.connected())
  {
    Serial.printf("mqttclient not connected?\n");
    if (mqttclient.connect(HOST_NAME, "skelstar", "ec11225f87"))
    {
      Serial.println("mqtt connected");
      subscribeToTopics();
    }
  }

  if (mqttclient.connected() && sincePublishedOnline > 2000)
  {
    sincePublishedOnline = 0;
#define ONLINE_MESSAGE "ONLINE-%02ds"
    char onlineMsg[11];
    sprintf(onlineMsg, ONLINE_MESSAGE, onlineCounter);

    mqttclient.publish(TOPIC_ONLINE, onlineMsg);
    if (onlineCounter < 500)
      onlineCounter++;
  }

  mqttclient.loop();

  button.loop();

  ArduinoOTA.handle();

  delay(200);
}
