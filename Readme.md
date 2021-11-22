# Smart Mailbox

This is the final project for the subject _Sistemas Distribuidos y Empotrados_.

This project implements a mailbox where packages can be received, without the receiver being at home.

To do this, the postman can request the user to open the mailbox by pressing a button, and the user can open it remotely with their mobile phone. In addition, if the receiver is not available, he can provide an access code to the postman, to enter it and open the mailbox door. Thus, it detects when you have ordinary mail and it notifies you to the mobile phone.

### Video demo: 
[![Watch the video](https://img.youtube.com/vi/oHOBRobXwFI/hqdefault.jpg)](https://youtu.be/oHOBRobXwFI)

## Hardware
The hardware used in the project will be the following:
1. [ESP32 Nodemcu](https://amzn.to/3oO3XcI)
2. [Display OLED 1'3 pulgadas I2C](https://amzn.to/30S4hiy)
3. [Electric lock DC 5V](https://amzn.to/30PEneJ)
4. [Arduino Keypad 4x4](https://amzn.to/3FU6HMD)
5. [Rele 5V](https://amzn.to/3HIzxB6)
6. [Ultrasonic sensor](https://amzn.to/3oUviKf)
7. [Arduino switch](https://amzn.to/3CCfIrj)

### Connection diagram
![Connection diagram](https://i.ibb.co/Kjqt165/Esquema-conexiones.png)

## Software
Android APP available in: https://github.com/cristiantrapero/SmartMailbox-Android 

ESP32 Libraries:
1. Bluetooth Low Energy (BLE) Library: https://github.com/h2zero/NimBLE-Arduino
2. Keypad Library: https://playground.arduino.cc/Code/Keypad/
3. LCD Screen U8g2: https://github.com/olikraus/u8g2/
4. MQTT Arduino: https://github.com/knolleary/pubsubclient

## How to run
1. Open the project in VSCode with PlatformIO
2. Connect ESP32 to PC
3. Change WIFI_SSID and WIFI_PASSWORD
4. Change MQTT configuration parameters
5. Build and upload the code to the ESP32
6. Just enjoy!

## Deployment
1. Deploy a MQTT server on a public server
2. Modify the MQTT configuration parameters in the project
3. Load the project in a ESP32
4. Install Android App
5. Control the mailbox with the Android app

## TODO tasks
1. Add LCD screen messages
2. User EEPROM in pin code and WiFi parameters
3. Disable BLE advertising when is connected to a smarphone
4. Encrypt comunications
