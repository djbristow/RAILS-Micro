# MQTT IOT RFID Reader
&copy; David J Bristow, 2020

# Version
* 1.1.0 - 5/19/2020

This software impmlemnts a IoT RFID reader.
For further information see https://kjcrr.org/rails/hardware-development/mqtt-wifi-rfid-reader

The following actions are accomplished:
  - connects to an MQTT broker via wifi
  - publishes info about this reader to the topic "micros"
  - published format: {"et":"1590556915","micro":"rfidRdr01","ip":"192.168.0.19"}
  - reads values from a single ID-12LA or 7491E RFID reader
  - formats the results as a JSON string
  - gets Epoch time from an NTP server
  - and publishes the JSON String to the topic "sensors/rfid"
  - published format: {"et":"1590557122","sensor":"rfidRdr01","rfid":"AAADCDDAC7"}
  
**NOTE**: Items marked as "configurable" need to be set for the specifics of the model railroad under control

## License

   This code  is licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

     http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
