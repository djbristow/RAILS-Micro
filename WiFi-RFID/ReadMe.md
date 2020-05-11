# MQTT IOT RFID Reader
&copy; David J Bristow, 2020

# Version
* 1.0.0 - 5/11/2020

This software impmlemnts a IoT turnout controller for Tortoise motors.
For further information see http://kjcrr.org/

The following actions are accomplished:
  - connects to an MQTT broker via wifi
  - reads values from a single ID-12LA or 7491E RFID reader
  - formats the results as a JSON string
  - gets Epoch time from an NTP server
  - and publishes the JSON String to the topic "sensors/rfid"
  
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