# MQTT IOT Turnout Controller
&copy; David J Bristow, 2020

# Version
* 1.0.2 - 5/17/2020

This software impmlemnts a IoT turnout controller for Tortoise motors.
For further information see http://kjcrr.org/

The following actions are accomplished:
  - connects to an MQTT broker via wifi
  - subscribes to the topic acts/to/trnCtlrxx where xx is the this controller
  - subscribed format: {"to":"1|2|3|4","command":"throw|close|status"}
  - either sets the turnout using an L293 to closed or thrown based on the command rxd
  - responds to status command for a turnout
  - determines the direction of the turnout under command
  - gets Epoch time from an NTP server
  - formats the results as a JSON string
  - published format: {"et":"1588827073","cntrlr":"trnCtlr00","to":"1","dir":"thrown"}
  - and publishes the JSON String to the topic "sensors/toc"
  
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
