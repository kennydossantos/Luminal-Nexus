# Luminal Nexus
 
![NTI Banner](NTIB.png)

## Project Description
This project aims to create internet-connected lamps that serve as a tool to enhance and facilitate communication between individuals at a distance. By pressing the lamps, users can signal presence and show attention, providing a meaningful and technically accessible solution to foster contact and a sense of community over distances.

## Components
### Hardware:
- ESP8266 unit
- NeoPixel Ring for illumination
- Touch sensor for interaction (TTP223B)
- Wires and soldering equipment
- 3D-printed chassis to hold the components

### Software:
- Arduino IDE for coding
- NeoPixelBrightnessBus library for NeoPixel animations
- WiFiManager library for easy WiFi connection management
- AdafruitIO library for connecting and communicating with Adafruit IO

## Configuration Mode
1. Power on the lamp.
2. Wait for the lamp to create a Wi-Fi hotspot named "Lamp" (password: "password").
3. Connect to the hotspot and open a web browser. (if you have an iPhone the website will automatically open.
4. Go to "see the IP in the serial monitor" to access the configuration portal.
5. Enter your Wi-Fi credentials and save.

## Setup with Libraries
1. Install the necessary libraries:
   - NeoPixelBrightnessBus
   - WiFiManager
   - AdafruitIO
   
2. Open the Arduino IDE and load the project.

3. Adjust the configuration parameters, such as IO_USERNAME and IO_KEY, to match your Adafruit IO credentials.

4. Compile and upload the code to the ESP8266 unit.

## Controls
- Press and hold the touch sensor for two seconds to enter color selection mode.
- Click to cycle through color options.
- After selecting a color, the signal is sent automatically.
- Click again to respond, and multiple clicks generate "pulses."

## Arduino IO
Arduino IO is utilized to connect the lamps to cloud-based communication. It facilitates the transfer of data between devices and is used to send and receive messages, enabling remote communication.

## WiFi Solution
The WiFi solution is implemented using the ESP8266 unit and the WiFiManager library. WiFiManager simplifies the process of connecting the device to a WiFi network and provides a convenient configuration mode access point if the connection is lost.

## Credits
© Kenny Dos Santos

For more information about NTI Johanneberg, check [NTI Johanneberg 2023](https://ntigymnasiet.se/johanneberg/).
