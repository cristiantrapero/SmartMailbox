# Smart Mailbox

This is the final project for the subject _Sistemas Distribuidos y Empotrados_.

This project implements a mailbox where packages can be received, without the receiver being at home.

To do this, the postman can request the user to open the mailbox by pressing a button, and the user can open it remotely with their mobile phone. In addition, if the receiver is not available, he can provide an access code to the postman, to enter it and open the mailbox door.

## Hardware
The hardware used in the project will be the following:
1. [ESP32 Nodemcu](https://www.amazon.es/dp/B071P98VTG/ref=cm_sw_r_tw_dp_CPSVVDB972EZTASYY59S)
2. [Display OLED 1'3 pulgadas I2C](https://www.amazon.es/dp/B078J78R45/ref=cm_sw_r_tw_dp_ZB90DXNJR4ACQF2T5VBF?_encoding=UTF8&psc=1)
3. [Cerradura eléctrica DC 5V](https://www.amazon.es/dp/B0782QNSQ1/ref=cm_sw_r_tw_dp_QN883FGKAA1P83ZPV05A?_encoding=UTF8&psc=1)
4. [Arduino Keypad de 4x4](https://www.amazon.es/dp/B018CGKAYY/ref=cm_sw_r_tw_dp_NX5TY4J0240DQH7HGF7F)
5. [Rele 5V](https://www.amazon.es/dp/B07BVXT1ZK/ref=cm_sw_r_tw_dp_871TCR1Q2JNB6FPTSF2Z)

## Software
Libraries:
1. Bluetooth Low Energy (BLE) Library: https://github.com/h2zero/NimBLE-Arduino
2. Keypad Library: https://playground.arduino.cc/Code/Keypad/
3. LCD Screen U8g2: https://github.com/olikraus/u8g2/
4. MQTT Arduino: https://github.com/knolleary/pubsubclient

## How to run
1. Open the project in PlatformIO
2. Connect ESP32 to PC
3. Change WIFI_SSID and WIFI_PASSWORD
4. Change MQTT configuration parameters
5. Build and upload the code to the ESP32
6. Just enjoy!

## Deployment
1. Deploy a MQTT server on a public server
2. Modify the MQTT configuration parameters in the project
3. Load the project in a ESP32


## TODO tasks
1. Add LCD screen messages
2. User EEPROM in pin code and WiFi parameters
3. Disable BLE advertising when is connected to a smarphone
4. Encrypt comunications
