/*****
  MQTT IOT Turnout Controller
  Copyright 2020 David J Bristow
  Version 1.0.0 - May 11, 2020

  - connects to an MQTT broker via wifi
  - subscribes to the topic acts/to/trnCtlrxx where xx is the this controller
  - {"to":"1|2|3|4","command":"throw|close|status"}
  - checks current state of turnout and either switches the turnout using an L293 and changes state or ignores
  - responds to status command for a turnout
  - determines the direction of the turnout under command
  - gets Epoch time from an NTP server
  - formats the results as a JSON string
  - {"et":"1588827073","cntrlr":"trnCtlr01","to":"1","dir":"thrown"}
  - and publishes the JSON String to the topic "sensors/toc"
  
  Note: items marked as "configurable" need to be set for the specifics of
  the model railroad under control

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

#include <Wire.h>
#include "Adafruit_MCP23017.h"
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <PubSubClient.h>
#include <NTPClient.h>

Adafruit_MCP23017 MCP;
void ICACHE_RAM_ATTR handleInterrupt();
uint8_t gpioB = 0xFF;
bool suscribeFlag = false;
int turnout = 0;
byte turnoutDir = 0x63;
byte cmd = 0x73;
const char* ssid = "CenturyLink3021";  // <===== Configurable
const char* password = "rda34348b7e4ed";  // <===== Configurable
WiFiClient espWiFiClient;
int mqttPort = 1883;
IPAddress mqtt_server(192, 168, 0, 7);  // <===== Configurable
String clientId = "trnCtlr00";  // <===== Configurable
PubSubClient client(mqtt_server, mqttPort, espWiFiClient);
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "192.168.0.7", 3600, 60000);  // <===== Configurable
unsigned long epochTime = 0;
char pubTopic[] = "sensors/toc";  // <===== Configurable
char subTopic[] = "acts/to/trnCtlr00";  // <===== Configurable

// This function sets the A port pins as outputs controlling 4 Tortoise stall motors
// and sets up teh B port pins as inputs with pull up internal resistors
// After the pins are setup the direction of each turnout is determined and published
void setup_MCP23017() {
  timeClient.update();
  epochTime = timeClient.getEpochTime();
  MCP.begin();
  // GPA0 and GPA1 (pins 21 & 22) output TO1
  MCP.pinMode(0, OUTPUT);
  MCP.pinMode(1, OUTPUT);
  // GPA0 and GPA1 (pins 23 & 24) output TO2
  MCP.pinMode(2, OUTPUT);
  MCP.pinMode(3, OUTPUT);
  // GPA0 and GPA1 (pins 25 & 26) output TO3
  MCP.pinMode(4, OUTPUT);
  MCP.pinMode(5, OUTPUT);
  // GPA0 and GPA1 (pins 27 & 28) output TO4
  MCP.pinMode(6, OUTPUT);
  MCP.pinMode(7, OUTPUT);
  MCP.setupInterrupts(true, false, LOW);
  // GPB0 and GPB1 (pins 1 & 2) input TO1 position
  MCP.pinMode(8, INPUT);
  MCP.pullUp(8, HIGH);
  MCP.setupInterruptPin(8, CHANGE);
  MCP.pinMode(9, INPUT);
  MCP.pullUp(9, HIGH);
  MCP.setupInterruptPin(9, CHANGE);
  // GPB2 and GPB3 (pins 3 & 4) input TO2 position
  MCP.pinMode(10, INPUT);
  MCP.pullUp(10, HIGH);
  MCP.pinMode(11, INPUT);
  MCP.pullUp(11, HIGH);
  // GPB2 and GPB3 (pins 5 & 6) input TO3 position
  MCP.pinMode(12, INPUT);
  MCP.pullUp(12, HIGH);
  MCP.pinMode(13, INPUT);
  MCP.pullUp(13, HIGH);
  // GPB2 and GPB3 (pins 7 & 8) input TO4 position
  MCP.pinMode(14, INPUT);
  MCP.pullUp(14, HIGH);
  MCP.pinMode(15, INPUT);
  MCP.pullUp(15, HIGH);
  gpioB = (uint8_t)((MCP.readGPIOAB() & 0xFF00) >> 8);
  epochTime = timeClient.getEpochTime();
  for (int i = 1; i <= 4; i++) {
    String payload = buildJson(clientId, i, epochTime, (determine_dir(i)));
    publishMqtt(payload);
  }
}

// This function determines the direction of the Tortoise motor as
// either thrown or closed from the vale of the B port and the pin that
// caused the interrupt
byte determine_dir(int turnout) {
  byte turnoutDir = 0x63;  // default to closed
  uint8_t mask = 0x03 << (turnout - 1) * 2;
  uint8_t dir = (gpioB & mask) >> (turnout - 1) * 2;
  if (dir == 1) {
    turnoutDir = 0x74;
  }
  return turnoutDir;
}

// This function builds a Json string from turnout info
String buildJson(String id, int turnout, unsigned long et, byte turnout_direction) {
  String dir = "closed";
  if (turnout_direction == 0x74) {
    dir = "thrown";
  }
  String mqttMsg = "{\"et\":\"";
  mqttMsg = mqttMsg +  String(et);
  mqttMsg = mqttMsg +  "\",\"cntrlr\":\"";
  mqttMsg = mqttMsg +  id;
  mqttMsg = mqttMsg +  "\",\"to\":\"";
  mqttMsg = mqttMsg +  turnout;
  mqttMsg = mqttMsg +  "\",\"dir\":\"";
  mqttMsg = mqttMsg +  dir;
  mqttMsg = mqttMsg +  "\"}";
  return mqttMsg;
}

// This function connects the ESP8266 to the LAN
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

// This function reconnects the ESP8266 to the MQTT broker
void reconnect() {
  while (!client.connected()) {
    if (client.connect(clientId.c_str())) {
      // do nothing
    } else {
      Serial.print("failed, rc=");
      Serial.println(client.state());
      delay(5000); // Wait 5 seconds before retrying
    }
  }
}

// This function extracts the data from a subscribed message
// and writes to the turnout pins if command is either close or throw
void callback(char* topic, byte* payload, unsigned int length) {
  suscribeFlag = true;
  turnout = payload[7] - 48;
  cmd = payload[21];
  if (cmd == 0x63) {  // close cmd rxd
    MCP.digitalWrite((turnout - 1) * 2, LOW);
    MCP.digitalWrite(((turnout - 1) * 2) + 1, HIGH);
  }
  if (cmd == 0x74) { // throw cmd rxd
    MCP.digitalWrite((turnout - 1) * 2, HIGH);
    MCP.digitalWrite(((turnout - 1) * 2) + 1, LOW);
  }
}

void publishMqtt(String payload) {
  char buf[100];
  int strLength = payload.length() + 1;
  payload.toCharArray(buf, strLength);
  if (!client.connected()) {
    reconnect();
  }
  if (client.connected()) {
    client.publish(pubTopic, buf);
  }
}

void subscribeMqtt() {
  if (!client.connected()) {
    reconnect();
  }
  if (client.connected()) {
    client.subscribe(subTopic);
  }
}
void setup() {
  Serial.begin(115200);
  Serial.println("WiFi Turnout Cntlr");
  Serial.println("Start Setup");
  setup_wifi();
  setup_MCP23017();
  timeClient.begin();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
    Serial.println("Finished Setup");
}

// The main loop function runs over and over again forever
// first subscribing to a message
// if a message was rxd and it wasn't a status cmd a loop is
// chceks to see that the closed or thrown position has been
// achieved. Then a message is published with the status of
// the turnout under command
// 
void loop() {
  subscribeMqtt();
  if (suscribeFlag) {
    gpioB = (uint8_t)((MCP.readGPIOAB() & 0xFF00) >> 8);
    turnoutDir = determine_dir(turnout);
    if (cmd != 0x73) { // status cmd rxd
      for (int i = 0; i <= 9; i++) {
        if (turnoutDir != cmd) {
          delay(500);
        } else {
          break;
        }
        gpioB = (uint8_t)((MCP.readGPIOAB() & 0xFF00) >> 8);
        turnoutDir = determine_dir(turnout);
      }
    }
    gpioB = (uint8_t)((MCP.readGPIOAB() & 0xFF00) >> 8);
    epochTime = timeClient.getEpochTime();
    String msg = buildJson(clientId, turnout, epochTime, turnoutDir);
    publishMqtt(msg);
    suscribeFlag = false;
  }
  client.loop();
  delay(50);
}
