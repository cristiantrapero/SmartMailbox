#include <Arduino.h>
#include <WiFi.h>
#include <SPI.h>
#include <U8g2lib.h>
#include <PubSubClient.h>
#include <EEPROM.h>
#include "Keypad.h"
#include "NimBLEDevice.h"
#include "time.h"

// OLED Display
U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);

// WIFI credentials
const char *WIFI_SSID = "test";
const char *WIFI_PASSWORD = "test";
const String HOSTNAME = "SmartMailbox";

// Bluetooth serial
#define SERVICE_UUID "2af412d8-3e7e-11ec-9bbc-0242ac130002"
#define PIN_CHARACTERISTIC_UUID "12fa1c0a-25cc-4281-9c22-8e7951b1ac03"
#define OPEN_CHARACTERISTIC_UUID "e7a6ec42-2713-4484-81d5-39e5d3e9060b"
BLEServer *pServer = NULL;
BLECharacteristic *openCharacteristic = NULL;
BLECharacteristic *pinCharacteristic = NULL;

// NTP configuration
const char* NTPSERVER = "pool.ntp.org";
const long  GMTOFFSET = 3600;
const int   DAYLIGHTOFFSET = 3600;

// MQTT configuration
const char* MQTTSERVER = "mqtt.cristiantrapero.es";
const int MQTTPORT = 1883;
const char* MQTTUSER = "test";
const char* MQTTPASSWORD = "test";
WiFiClient espClient;
PubSubClient client(espClient);

// Reciever topics
const char* PINTOPIC = "smartmailbox/pin";
const char* RELAYTOPIC = "smartmailbox/relay";

// Sender topics
const char* LETTERTOPIC = "smartmailbox/letter";
const char* OPENTOPIC = "smartmailbox/openrequest";
const char* OPENEDTOPIC = "smartmailbox/opened";
const char* PINCHANGEDTOPIC = "smartmailbox/pinchanged";

// Keypad size
const byte ROWS = 4;
const byte COLS = 4;

// Keypad keys distribution
char keys[ROWS][COLS] = {
    {'1', '2', '3', 'A'},
    {'4', '5', '6', 'B'},
    {'7', '8', '9', 'C'},
    {'*', '0', '#', 'D'}};

// Keypad pinout and variables
byte rowPins[ROWS] = {13, 12, 14, 27};
byte colPins[COLS] = {23, 25, 33, 32};
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);
String mailboxPIN = "1234"; //TODO: Get from eeprom
String inputPIN;
String screenMessage;

// Constants declarations
const int CALL_BUTTON = 4;
const int DOOR_RELAY = 26;
const int ECHO = 19;
const int TRIGGER = 18;

// Auxiliar variables
int pressedButton = 0;
long usDistance;
long usTime;
unsigned long lastLetterCall;
unsigned long lastOpenCall;
unsigned long lastMessageShow;

// Functions prototype declaration
void connectToWiFi();
void enableBluetooth();
void changePasswordPin(String pin);
void openMailbox();
void reconnectMQTT();
void sendTimeMessageToTopic(const char* topic);
void sendOpenMailboxRequest();
void sendLetterNotification(int usDistance);
void WiFiStationDisconnected(WiFiEvent_t event, WiFiEventInfo_t info);
void mqttCallback(char* topic, byte* message, unsigned int length);
void showMessageOnScreen();
void write_word(int addr, String word);
String read_word(int addr);

void setup()
{
  Serial.begin(9600);

  // WiFi configuration
  // WiFi event on disconnect, retry the conexion
  WiFi.onEvent(WiFiStationDisconnected, SYSTEM_EVENT_STA_DISCONNECTED);
  connectToWiFi();

  // NTP
  configTime(GMTOFFSET, DAYLIGHTOFFSET, NTPSERVER);

  // Bluetooth configuration
  enableBluetooth();

  // Ultrasonic sensor
  pinMode(TRIGGER, OUTPUT);
  pinMode(ECHO, INPUT);
  digitalWrite(TRIGGER, LOW);

  // Call button
  pinMode(CALL_BUTTON, INPUT);

  // Relay
  pinMode(DOOR_RELAY, OUTPUT);
  digitalWrite(DOOR_RELAY, LOW);

  // MQTT
  client.setServer(MQTTSERVER, MQTTPORT);
  client.setCallback(mqttCallback);

  // OLED
  u8g2.begin();
  u8g2.enableUTF8Print();


}

void loop()
{

  // Keypad input
  char pressedKey = keypad.getKey();

  if (pressedKey)
  {
    Serial.println(pressedKey);

    if (pressedKey == '*')
    {
      inputPIN = "";
    }
    else if (pressedKey == '#')
    {
      if (mailboxPIN == inputPIN)
      {
        Serial.println("Password is correct. Opening mailbox.");
        openMailbox();
      }
      else
      {
        screenMessage = "PIN incorrecto";
        lastMessageShow = millis();
        Serial.println("Password is incorrect, try again.");
      }

      inputPIN = "";
    }
    else
    {
      inputPIN += pressedKey;
    }
  }

  // Call open mailbox button
  pressedButton = digitalRead(CALL_BUTTON);

  if (pressedButton == HIGH)
  {
    sendOpenMailboxRequest();
  }

  // Ultrasonic calculations
  digitalWrite(TRIGGER, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIGGER, LOW);

  usTime = pulseIn(ECHO, HIGH);
  usDistance = usTime / 59;
  sendLetterNotification(usDistance);

  showMessageOnScreen();

  if (!client.connected()) {
    reconnectMQTT();
  }
  client.loop();
}

void showMessageOnScreen(){
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_6x10_tf);
  u8g2.setCursor(0, 15);
  u8g2.drawStr(25,10,"Pulsa boton o");
  u8g2.drawStr(25,25,"introduce PIN");
  u8g2.drawStr(27,40,"seguido de #");

  if (screenMessage != ""){
    if (millis() - lastMessageShow <= 3*1000UL){
      char message[50];
      screenMessage.toCharArray(message, 50);
      u8g2.drawStr(0, 60, message);
    }else {
      screenMessage = "";
    }
  } else{
      u8g2.drawStr(0,60, "PIN:");
      char pin[50];
      inputPIN.toCharArray(pin, 50);
      u8g2.drawStr(30, 60, pin);
  }

  u8g2.sendBuffer();

}

void mqttCallback(char* topic, byte* message, unsigned int length) {
  Serial.print("Message arrived on topic: ");
  Serial.print(topic);
  Serial.print(". Message: ");
  String command;
  
  for (int i = 0; i < length; i++) {
    Serial.print((char)message[i]);
    command += (char)message[i];
  }
  Serial.println();
  if (String(topic) == RELAYTOPIC) {
    if(command == "on"){
      openMailbox();
    }
  } else if (String(topic) == PINTOPIC){
      changePasswordPin(command);
  }
}

void reconnectMQTT() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect("SmartMailbox", MQTTUSER, MQTTPASSWORD)) {
      Serial.println("\nMQTT connected");
      client.subscribe(PINTOPIC);
      client.subscribe(RELAYTOPIC);
    } else {
      Serial.print("Mqtt connection failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void sendOpenMailboxRequest(){
  if (millis() - lastOpenCall >= 5*1000UL){
    lastOpenCall = millis();
    Serial.println("Send mailbox open request");
    screenMessage = "Apertura solicitada";
    lastMessageShow = millis();
    sendTimeMessageToTopic(OPENTOPIC);
  }
}

void changePasswordPin(String pin){
  mailboxPIN = pin;
  screenMessage = "Codigo PIN cambiado";
  lastMessageShow = millis();
  sendTimeMessageToTopic(PINCHANGEDTOPIC);
}

void openMailbox()
{
  Serial.println("Open smartmailbox");
  Serial.println("Send mailbox opened notification");
  sendTimeMessageToTopic(OPENEDTOPIC);
  digitalWrite(DOOR_RELAY, HIGH);
  delay(5000);
  digitalWrite(DOOR_RELAY, LOW);
  screenMessage = "Buzon abierto";
  lastMessageShow = millis();
}

void sendLetterNotification(int usDistance)
{
    if (usDistance < 7)
    {
      if (millis() - lastLetterCall >= 10*1000UL){
        lastLetterCall = millis();
        Serial.println("Send mailbox letter notification");
        screenMessage = "Cartas introducidas";
        lastMessageShow = millis();  
        sendTimeMessageToTopic(LETTERTOPIC);
    }
  }
}

void sendTimeMessageToTopic(const char* topic) {
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time");
    return;
  }
  char timestamp[50];
  strftime(timestamp, sizeof(timestamp), "%A, %B %d %Y %H:%M:%S", &timeinfo);
  client.publish(topic, timestamp);
}

void connectToWiFi()
{
  WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE, INADDR_NONE);
  WiFi.setHostname(HOSTNAME.c_str());
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to the router " + String(WIFI_SSID) + " with password " + String(WIFI_PASSWORD) + "\n");
  Serial.print("Trying");

  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print('.');
    delay(1000);
  }

  Serial.print("\nSmartMailbox connected to " + String(WIFI_SSID) + " with IP: ");
  Serial.print(WiFi.localIP());
}

void WiFiStationDisconnected(WiFiEvent_t event, WiFiEventInfo_t info)
{
  Serial.println("Disconnected from WiFi access point");
  Serial.print("WiFi lost connection. Reason: ");
  Serial.println(info.disconnected.reason);
  connectToWiFi();
}

class MyServerCallbacks : public BLEServerCallbacks
{
  void onConnect(BLEServer *pServer)
  {
    Serial.println("BLE device connected");
  };

  void onDisconnect(BLEServer *pServer)
  {
    Serial.println("BLE device disconnected");
  }
};

class openCharacteristicCallbackBLE : public BLECharacteristicCallbacks
{
  void onWrite(BLECharacteristic *openCharacteristic)
  {
    std::string rxValue = openCharacteristic->getValue();
    String command = "";

    if (rxValue.length() > 0)
    {
      Serial.println("*********");
      Serial.print("Received open value: ");
      for (int i = 0; i < rxValue.length(); i++){
        Serial.print(rxValue[i]);
        command += rxValue[i];
      }

      Serial.println();
      Serial.println("*********");

      if(command == "on"){
        openMailbox();
      }
    }
  }
};

class pinCharacteristicCallbackBLE : public BLECharacteristicCallbacks
{
  void onWrite(BLECharacteristic *openCharacteristic)
  {
    std::string rxValue = openCharacteristic->getValue();

    String newPin = "";
    if (rxValue.length() > 0)
    {
      Serial.println("*********");
      Serial.print("Received new pin value: ");
      for (int i = 0; i < rxValue.length(); i++){
        newPin += rxValue[i];
        Serial.print(rxValue[i]);
      }
      Serial.println();
      Serial.println("*********");
      changePasswordPin(newPin);
    }
  }
};

void enableBluetooth()
{
  // Init the BLE device with the name
  BLEDevice::init("SmartMailbox");

  // Create the server
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // Create the service
  BLEService *pService = pServer->createService(SERVICE_UUID);

  // Create BLE characteristics
  openCharacteristic = pService->createCharacteristic(
      OPEN_CHARACTERISTIC_UUID,
      NIMBLE_PROPERTY::WRITE);
  openCharacteristic->setCallbacks(new openCharacteristicCallbackBLE());

  pinCharacteristic = pService->createCharacteristic(
    PIN_CHARACTERISTIC_UUID,
    NIMBLE_PROPERTY::WRITE);
  pinCharacteristic->setCallbacks(new pinCharacteristicCallbackBLE());

  pService->start();

  // Start advertising
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06); // functions that help with iPhone connections issue
  pAdvertising->setMinPreferred(0x12);
  BLEDevice::startAdvertising();
  Serial.println("\nThe " + HOSTNAME + " bluetooth started");
}
