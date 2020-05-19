/*****
  MQTT IOT RFID Reader
  Copyright 2020 David J Bristow
  Version 1.1.0 - May 19, 2020
  - connects to an MQTT broker via wifi
  - publishes info about this reader to the topic micros
  - reads values from a single ID-12LA or 7491E RFID reader
  - formats the results as a JSON string
  - gets Epoch time from an NTP server
  - and publishes the JSON String to the topic "sensors/rfid"

  Note: items marked as "configurable" need to be set for the specifics of
  the railroad under control

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at
     http://www.apache.org/licenses/LICENSE-2.0
   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*****/

/*****************  CONSTANTS ********************************************/
#define useID12LA_Reader
//#define use7491E_Reader
#define D7 (13)
#define noTxPin -1
#define BAUD_RATE 9600

/******************  LIBRARY SECTION *************************************/
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <SoftwareSerial.h>

/***********************  WIFI, MQTT AND SERIAL SETUP *****************************/
WiFiClient espWiFi01; // <===== Configurable
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "192.168.0.7", 3600, 60000);  // <===== Configurable
SoftwareSerial RFID(D7, noTxPin); // RX with no TX
const char* ssid = "CenturyLink";  // <===== Configurable
const char* password = "************";  // <===== Configurable
IPAddress mqtt_server(192, 168, 0, 7);  // <===== Configurable
int mqttPort = 1883;
char pubTopic[] = "sensors/rfid";  // <===== Configurable
String clientId = "rfidRdr01";  // <===== Configurable
PubSubClient client(mqtt_server, mqttPort, espWiFi01);

/*****************  GLOBAL VARIABLES  ************************************/
char mqttMsg[800]; //buffer used to publish messages via mqtt
unsigned long epochTime = 0;
char buf[100];
int strLength = 0;
String rfTagId = "";
String rfidJsonPayload = "";

/*****************  SYSTEM FUNCTIONS  *************************************/
void setup_wifi() {
  delay(10);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.print("WiFi connected - ESP IP address: ");
  Serial.println(WiFi.localIP());
}

void reconnect() {
  while (!client.connected()) {
    //Serial.print("Attempting MQTT connection...");
    if (client.connect(clientId.c_str())) {
      // if (client.connect(clientId)) {
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println();
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

/*****************  RFID READER FUNCTIONS  *********************************/
/*****************  RFID ID-12LA READER FUNCTION  **************************/
#ifdef useID12LA_Reader
String getTagId() {
  String tagId = "";
  char tagChr;
  int i = 0;
  while (!RFID.available()) {
    delay(200);
  }
  while (RFID.available()) {
    tagChr = RFID.read();
    if (tagChr > 0x0f && i < 11) {
      tagId.concat(tagChr);
    }
    i++;
  }
  return tagId;
}
#endif

/*****************  RFID 7491E READER FUNCTION  *****************************/
#ifdef use7491E_Reader
char valToHex(uint8_t val) {
  if ((val & 0x0f) < 10)
    return ('0' + val);
  else
    return ('a' + (val - 10));
}

String byteToHexString(uint8_t b) {
  String buffer = "";
  buffer += valToHex(b & 0x0f);
  b >>= 4;
  buffer = valToHex(b & 0x0f) + buffer;
  return buffer;
}

String getTagId() {
  String tagId = "";
  String rfidStrg = "";
  char tagChr;
  int i = 0;
  boolean tryingToReadTag = true;

  while (tryingToReadTag) {
    if (RFID.available()) {
      tagChr = RFID.read();
      if (tagChr == 0x02) { // check for header STX
        while (i < 9) {
          delay(2);
          tagChr = RFID.read();
          i++;
          rfidStrg = byteToHexString(tagChr);
          if (i > 2 && i < 8) {
            tagId.concat(rfidStrg);
          }
        }
      }
      tryingToReadTag = false;
    } else {
      //Serial.print("*");
      delay(500);
    }
  }
  return tagId;
}
#endif

/***************** JSON MESSAGE BUILDER FUNCTION *********************/
String buildJson(String id, String sensor, String et) {
  String mqttMsg = "{\"epochTime\":\"";
  mqttMsg = mqttMsg + et;
  mqttMsg = mqttMsg + "\",\"sensor\":\"";
  mqttMsg = mqttMsg + sensor;
  mqttMsg = mqttMsg + "\",\"rfid\":\"";
  mqttMsg = mqttMsg + id;
  mqttMsg = mqttMsg + "\"}";
  return mqttMsg;
}

/*****************  MQTT PUBLISH FUNCTION  ****************************/
void publishMqtt(String payload, char topic[]) {
  char buf[100];
  int strLength = payload.length() + 1;
  payload.toCharArray(buf, strLength);
  if (!client.connected()) {
    reconnect();
  }
  if (client.connected()) {
    client.publish(topic, buf);
  }
}

/*****************  SETUP FUNCTION  ***********************************/
void setup() {
  Serial.begin(9600);
  Serial.println("WiFi RFID Reader");
  Serial.println("Start Setup");
  RFID.begin(BAUD_RATE, SWSERIAL_8N1);
  setup_wifi();
  timeClient.begin();
  timeClient.update();
  epochTime = timeClient.getEpochTime();
  String mqttMsg = "{\"epochTime\":\"";
  mqttMsg = mqttMsg + timeClient.getEpochTime();
  mqttMsg = mqttMsg + "\",\"sensor\":\"";
  mqttMsg = mqttMsg + clientId;
  mqttMsg = mqttMsg + "\",\"ip\":\"";
  mqttMsg = mqttMsg + WiFi.localIP().toString();
  mqttMsg = mqttMsg + "\"}";
  publishMqtt(mqttMsg, "micros");
  Serial.println(" Finished Setup");
}

/*****************  MAIN LOOP  ****************************************/
void loop() {
  rfTagId = getTagId();
  if (rfTagId.length() == 10) {
    timeClient.update();
    epochTime = timeClient.getEpochTime();
    rfidJsonPayload = buildJson(rfTagId, "rfidRdr01", String(epochTime));
    strLength = rfidJsonPayload.length() + 1;
    publishMqtt(rfidJsonPayload, pubTopic);
  }
  delay(25);
}
