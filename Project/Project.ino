//Libraries
#include "WiFiEsp.h"
#include "ThingSpeak.h"
#include "SoftwareSerial.h"

#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include <GxEPD2_BW.h>

#include <Fonts/FreeMonoBold9pt7b.h>

#include <MFRC522.h>
//

//Global variables
#define ESP_BAUDRATE 115200
char ssid[] = "SSID"; // lab's network SSID (name)
char pass[] = "PASS"; // lab's network password
int status = WL_IDLE_STATUS; // the Wifi radio's status
WiFiEspClient client;

PROGMEM const unsigned long channelNumber = 1295386;
PROGMEM const unsigned int priceFieldNumber = 1;
PROGMEM const unsigned int P1StockFieldNumber = 3;
PROGMEM const unsigned int P2StockFieldNumber = 4;
const char * writeAPI = "writeAPI";
const char * readAPI = "readAPI";
//SoftwareSerial Serial1(19, 18);

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3D ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#define ARRAYSIZE 2

// defines pins numbers
PROGMEM const int trigPin = 8;
PROGMEM const int echoPin = 9;
#define SS_PIN 53
#define RST_PIN 5
int red_light_pin = 3;
int green_light_pin = 4;
int blue_light_pin = 6;

// defines variables
long duration;
int distance;

double prev_time;
double updateData_time;

float price;
float oldPrice;

boolean expire;
boolean outOfStock;
String expire_product;
String oos_product;

String today = "30-03-2021";

MFRC522 rfid(SS_PIN, RST_PIN); // Create object Instance
MFRC522::MIFARE_Key key; //create variable “key” with MIFARE_Key structure

// Init array that will store new NUID
byte nuidPICC[4];
int count_product[ARRAYSIZE] = {0, 0};
String expDate[ARRAYSIZE] = {"30-03-2021", "04-11-2021"};
String product_id;

//
void setup() {

  // Initialize Ultrasonic
  pinMode(trigPin, OUTPUT); // Sets the trigPin as an Output
  pinMode(echoPin, INPUT); // Sets the echoPin as an Input

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;);
  }

  // initialize serial
  Serial.begin(115200);

  // Init SPI bus
  SPI.begin();

  // Init the MFRC522 chip
  rfid.PCD_Init();

  for (byte i = 0; i < 6; i++) {
    key.keyByte[i] = 0xFF;
  }

  Serial.println(F("This code scan the MIFARE Classsic NUID."));
  Serial.print(F("Using the following key:"));
  printHex(key.keyByte, MFRC522::MF_KEY_SIZE);

  // initialize serial for ESP module
  Serial1.begin(ESP_BAUDRATE);
  // initialize ESP module
  WiFi.init(&Serial1);

  // Connect or reconnect to WiFi
  if (WiFi.status() != WL_CONNECTED) {
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(ssid);
    while (WiFi.status() != WL_CONNECTED) {
      WiFi.begin(ssid, pass); // Connect to WPA/WPA2 network
      Serial.print(".");
      delay(5000);
    }
  }
  Serial.println("\nConnected");

  // Initialize ZigBee
  Serial3.begin(115200);

  // Initialize ThingSpeak client
  ThingSpeak.begin(client);

  prev_time = 0;
  updateData_time = millis();

  oldPrice = 0;
  price = readCloudData(priceFieldNumber);
    
  Serial.println(price);
  writeToZigbee(price);

  expire = false;
  outOfStock = false;
  expire_product = "";
  oos_product = "";

  display_first();

  // Initialize LED
  pinMode(red_light_pin, OUTPUT);
  pinMode(green_light_pin, OUTPUT);
  pinMode(blue_light_pin, OUTPUT);

}

void loop()
{
  // check the network connection once every 10 seconds
  //Serial.println();
  //printWifiData();

  ultra_sonic();

  display_first();

  double t = millis();

  if (t - updateData_time > 100000) //Update Data every 3 mins
  {
    Serial.println(F("Update data"));
    float cloudPrice = readCloudData(priceFieldNumber);
    Serial.println("cloudprice ");
    Serial.println(cloudPrice);

     if (cloudPrice != price)
    {
        price = cloudPrice;
        writeToZigbee(price);  
    }

    for (int i = 0; i <= 1; i++) {
      String date = readCloudExpDate(i);
      Serial.println(F("Read exp date:"));
      Serial.println(date);
      determineExp(date, "Product " + String(i));
    }

    updateData_time = millis();
  }


  // Reset the loop if no new card present on the sensor/reader. This saves the entire process when idle.
  if ( ! rfid.PICC_IsNewCardPresent()) // Check whether there is PICC card
    return;

  // Verify if the NUID has been read
  if ( ! rfid.PICC_ReadCardSerial()) //Read the card, if true, then continue
    return;

  MFRC522::PICC_Type piccType = rfid.PICC_GetType(rfid.uid.sak);

  RFID_Update(key.keyByte, MFRC522::MF_KEY_SIZE);

  // Check whether the card type is Classic MIFARE type or not
  if (piccType != MFRC522::PICC_TYPE_MIFARE_MINI &&
      piccType != MFRC522::PICC_TYPE_MIFARE_1K &&
      piccType != MFRC522::PICC_TYPE_MIFARE_4K) {
    Serial.println(F("Your tag is not of type MIFARE Classic."));
    return;
  }

  // check whether the UID of the card is the same as the stored one
  if (rfid.uid.uidByte[0] != nuidPICC[0] ||
      rfid.uid.uidByte[1] != nuidPICC[1] ||
      rfid.uid.uidByte[2] != nuidPICC[2] ||
      rfid.uid.uidByte[3] != nuidPICC[3] ) {
    Serial.println(F("A new product has been detected."));

    // Store NUID into nuidPICC array
    for (byte i = 0; i < 4; i++) {
      nuidPICC[i] = rfid.uid.uidByte[i];
    }

  }
  else Serial.println(F("Product read previously."));

  // Halt PICC
  rfid.PICC_HaltA();

  // Stop encryption on PCD
  rfid.PCD_StopCrypto1();


}
void writeToZigbee(float p){
    String t = (String)p;

  for(int i = 0; i<t.length();i++){
    Serial3.write(t[i]);
  }
}

// RFID
/**
  Helper routine to dump a byte array as hex values to Serial.
*/
void printHex(byte * buffer, byte bufferSize) {
  for (byte i = 0; i < bufferSize; i++) {
    Serial.print(buffer[i] < 0x10 ? " 0" : " ");
    Serial.print(buffer[i], HEX);
  }
  Serial.println(" ");
}


void RFID_Update(byte * buffer, byte bufferSize) {
  String content = "";
  byte letter;

  for (byte i = 0; i < rfid.uid.size; i++)
  {
    content.concat(String(rfid.uid.uidByte[i] < 0x10 ? " 0" : " "));
    content.concat(String(rfid.uid.uidByte[i], HEX));
  }

  Serial.println();
  content.toUpperCase();
  product_id = content.substring(1);
  Serial.print(F("The product id: "));
  Serial.println(product_id);
  String product[ARRAYSIZE] PROGMEM = {"FC 06 8C 17", "DC 5A 12 30"};




  for (int i = 0; i < ARRAYSIZE; i++)
  {
    if (content.substring(1) == product[i])
    {
      count_product[i] += 1;

      Serial.print(F("Stock keeping of product "));
      Serial.print(i);
      Serial.print(F(": "));
      Serial.println(count_product[i]);
      Serial.println(F("Expiry date of product :"));
      Serial.println(expDate[i]);


      switchColor(i);

      updateCloud(i);
     // clearAddStock(i);

      if (i == 0)
        setOutofStock(false, "Product 0");

      break;
    }
  }
}

//RGB LED setting
void RGB_color(int red_light_value, int green_light_value, int blue_light_value)
{
  analogWrite(red_light_pin, red_light_value);
  analogWrite(green_light_pin, green_light_value);
  analogWrite(blue_light_pin, blue_light_value);
}

//change the LED to corresponding color
void switchColor(int x)
{
  RGB_color(0, 0, 0);
  switch (x)
  {
    //Red
    case 0:
      RGB_color(255, 0, 0);
      delay(2000);
      RGB_color(0, 0, 0);
      break;
    //blue
    case 1:
      RGB_color(0, 0, 255);
      delay(2000);
      RGB_color(0, 0, 0);
      break;
    //green
    case 2:
      RGB_color(0, 255, 0);
      delay(2000);
      RGB_color(0, 0, 0);
      break;
    default:
      RGB_color(0, 0, 0);
  }
}


// OLED & Ultrasonic

void display_first() {


  if (expire == false && outOfStock == false)
  {
    display.clearDisplay();
    display.setCursor(10, 10);
    display.setTextColor(SSD1306_WHITE);
    display.setTextSize(2);
    display.print(F("No alert."));

    display.display();
    delay(2000);

  } else {
    if (expire == true)
    {
      display.clearDisplay();
      display.setTextSize(2);
      display.setCursor(10, 10);
      display.print(expire_product);
      display.setCursor(10, 40);
      display.setTextSize(1);
      display.print(F("is Expire"));


      display.display();
      delay(2000);

    }

    if (outOfStock == true)
    {
      display.clearDisplay();
      display.setTextSize(2);
      display.setCursor(10, 10);
      display.print(oos_product);
      display.setCursor(10, 40);
      display.setTextSize(1);
      display.print(F("is Out of Stock."));

      display.display();
      delay(2000);
    }


  }
}


void ultra_sonic() {

  // Clears the trigPin
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  // Sets the trigPin on HIGH state for 10 micro seconds
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  // Reads the echoPin, returns the sound wave travel time in microseconds
  duration = pulseIn(echoPin, HIGH);
  // Calculating the distance
  distance = duration * 0.034 / 2;
  // Prints the distance on the Serial Monitor
  //Serial.print("Distance: ");
  //Serial.println(distance);

  if (distance > 10 && outOfStock != true) {

    if (prev_time == 0)
      prev_time = millis();

    double t = millis();

    if (prev_time != 0 && (t - prev_time > 60000)) //if detected nothing for 3 mins
    {
      setOutofStock(true, "Product 0");
      notifyAddStock(0);
      prev_time = 0;
    }

  } else {
    prev_time = 0;
  }

  delay(2000);
}


void determineExp(String date, String product) {
  if (date == today && expire == false)
    setExpire(true, product);
}

void setExpire(boolean b, String prod) {
  expire = b;
  expire_product = prod;
}

void setOutofStock(boolean b, String prod) {
  outOfStock = b;
  oos_product = prod;
}


// WiFi & ThingSpeak
void printWifiData()
{
  // print your WiFi shield's IP address
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);
}

float readCloudData(unsigned int field)
{
  // read data from ThingSpeak
  delay(15000);
  float result;
  do {
    result = ThingSpeak.readFloatField(channelNumber, field, readAPI);
    delay(5000);
  }
  while (ThingSpeak.getLastReadStatus() != 200);

  return result;
}

bool checkReadStatus(int read_status)
{
  if (read_status == 200) {
    Serial.println("Read successful.");
    return true;
  }
  else {
    Serial.println("Problem reading. HTTP error code " + String(read_status));
    return false;
  }  
}

int readCloudStock(int productNumber)
{
  delay(15000);
  int result;
  do {
    result = ThingSpeak.readIntField(channelNumber, productNumber + 3, readAPI);
    delay(15000);
  }
  while (ThingSpeak.getLastReadStatus() != 200);

  return result;
}

String readCloudExpDate(int productNumber)
{
  delay(15000);
  String result;
  do {
    result = ThingSpeak.readStringField(channelNumber, productNumber + 5, readAPI);
    delay(15000);
  }
  while (!checkReadStatus(ThingSpeak.getLastReadStatus()));

  return result;
}

bool checkWriteStatus(int writeStatus)
{
  if (writeStatus == 200) {
    Serial.println("Update successful.");
    return true;
  }
  else {
    Serial.println("Problem updating. HTTP error code " + String(writeStatus));
    return false;
  }
}



void updateCloud(int productNumber)
{
  int writeStatus = 0;
  do{
    ThingSpeak.setField(productNumber + 3, count_product[productNumber]);
    ThingSpeak.setField(productNumber + 5, expDate[productNumber]);
    ThingSpeak.setField(productNumber + 7, 0);
    
    writeStatus = ThingSpeak.writeFields(channelNumber, writeAPI);
  
  delay(15000);
  }
  while(!checkWriteStatus(writeStatus));
}


void notifyAddStock(int productNumber)
{
  int writeStatus = 0;
  do {
    writeStatus = ThingSpeak.writeField(channelNumber, productNumber + 7, 1, writeAPI);

    delay(15000);
  }
  while (!checkWriteStatus(writeStatus));
  Serial.println("Adding stock notified.");
}
/*
void clearAddStock(int productNumber)
{
  int writeStatus = 0;
  do {
    writeStatus = ThingSpeak.writeField(channelNumber, productNumber + 7, 0, writeAPI);

    delay(15000);
  }
  while (!checkWriteStatus(writeStatus));
  Serial.println("Notification cleared");
}
*/
