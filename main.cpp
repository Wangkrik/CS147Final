#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <WiFi.h>
#include <inttypes.h>
#include <stdio.h>
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs.h"
#include "nvs_flash.h"
#include <ESP_Mail_Client.h>
#define SERVICE_UUID "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

char WIFI_ssid[50] = "APT329";
char WIFI_pass[50] = "329qwerty";
#define SMTP_HOST "smtp.gmail.com"
#define SMTP_PORT 465
char Master_Email[50] = "wxs18ss@gmail.com";
char Slave_Email[50] = "sumkrikk@gmail.com";
char Slave_Pass[50] = "gafwknsqpkshzwgm";
void nvs_access();
void sendAlert();
void setup_wifi();
void calibrate();
void setupSMTP();
void setupBLE();

SMTPSession smtp;
void smtpCallback(SMTP_Status status);


#define ledPin 27
#define lightPin 33
#define HC_pin 37
#define red_led 12
unsigned long overallStartTime;
unsigned long currentTime;
int Max_light = 4000;
int Min_light = 300;
bool test_light = false;
bool atHome = true;
bool door_open = false;
bool HC_detected = false;
//using ESP32 ble to send data to phone 
class MyCallbacks : public BLECharacteristicCallbacks {
  
  void onWrite(BLECharacteristic *pCharacteristic) {
    std::string value = pCharacteristic->getValue();
    if (value.length() > 0) {
      Serial.println("*********");
      Serial.print("New value: ");
      for (int i = 0; i < value.length(); i++)
        Serial.print(value[i]);

        
      if(value =="light"){
        test_light = true;
        Serial.println("light");
        return;
      }
        
      if(value =="close"){
        test_light = false;
        Serial.println("close");
        digitalWrite(ledPin, LOW);
        return;
      }
      if(value =="home"){
        Serial.println("at home");
        atHome = true;
        return;
      }
      if(value =="away"){
        Serial.println("away");
        atHome = false;
        return;
      }
      
      
    }
  }
};





void setup() {
  // put your setup code here, to run once:
  pinMode(ledPin, OUTPUT);
  pinMode(HC_pin, INPUT);
  pinMode(red_led, OUTPUT);
  Serial.begin(9600);
  //nvs_access();
  delay(1000);
  setupBLE();
  delay(1000);
  setup_wifi();
  delay(1000);
  setupSMTP();
  // calibrate light sensor first
  delay(1000);
  overallStartTime = millis();
  //calibrate();
  delay(1000);
  overallStartTime = millis();
  digitalWrite(red_led, LOW);

  // imu settings
  
}

void loop() {
  // put your main code here, to run repeatedly:
  // read the data from the sensor
  
  currentTime = millis();
  // take a break
  if (currentTime - overallStartTime < 10000)
  {
    delay(1000);
    return;
  }
  if(atHome){
    Serial.println("at home");
    delay(1000);
    return;
  }
  if (test_light){
    
    // read the input on analog pin 13:
    int sensorValue = analogRead(lightPin);
    // print out the value you read:
    Serial.println(sensorValue);
    delay(100);
    if (sensorValue <= Min_light+500)
    {
      if(!door_open){
        Serial.println("Door is open");
        digitalWrite(ledPin, HIGH);
      }
      door_open = true;
    }
  }
  delay(1000);
  // read the data from the HR sensor
  int HC = digitalRead(HC_pin);
  if (HC == HIGH&& !HC_detected)
  {
    Serial.println("HC detected");
    if(!HC_detected){
      digitalWrite(red_led, HIGH);
    }
    HC_detected = true;

  }else{
    digitalWrite(red_led, LOW);
  }
  delay(1000);
  // send email if door is open and HC is detected
  if (door_open && HC_detected)
  {
    sendAlert();
    Serial.println("Alert sent");
    delay(1000);
    overallStartTime = millis();
    // update time
    HC_detected = false;
    door_open = false;
    digitalWrite(ledPin, LOW);
    digitalWrite(red_led, LOW);
    // reset the door open and HC detected
  }
  
}

void nvs_access()
{
  // Initialize NVS
  esp_err_t err = nvs_flash_init();
  if (err == ESP_ERR_NVS_NO_FREE_PAGES ||
      err == ESP_ERR_NVS_NEW_VERSION_FOUND)
  {
    // NVS partition was truncated and needs to be erased
    // Retry nvs_flash_init
    ESP_ERROR_CHECK(nvs_flash_erase());
    err = nvs_flash_init();
  }
  ESP_ERROR_CHECK(err);

  // Open
  Serial.println("\n");
  Serial.println("Opening Non-Volatile Storage (NVS) handle... ");
  nvs_handle my_handle;
  err = nvs_open("storage", NVS_READWRITE, &my_handle);
  if (err != ESP_OK)
  {
    Serial.printf("Error (%s) opening NVS handle!\n", esp_err_to_name(err));
  }
  else
  {
    Serial.printf("Done\n");
    Serial.printf("Retrieving WIFI_ssid/PASSWD\n");
    size_t ssid_len;
    size_t pass_len;
    size_t slave_email_len;
    size_t slave_pass_len;
    err = nvs_get_str(my_handle, "WIFI_ssid", WIFI_ssid, &ssid_len);
    err |= nvs_get_str(my_handle, "WIFI_pass", WIFI_pass, &pass_len);
    err |= nvs_get_str(my_handle, "Slave_Email", Slave_Email, &slave_email_len);
    err |= nvs_get_str(my_handle, "Slave_Pass", Slave_Pass, &slave_pass_len);
    switch (err)
    {
    case ESP_OK:
      Serial.printf("Done\n");
      Serial.printf("WIFI_ssid: %s\n", WIFI_ssid);
      Serial.printf("WIFI_pass: %s\n", WIFI_pass);
      break;
    case ESP_ERR_NVS_NOT_FOUND:
      Serial.printf("The value is not initialized yet!\n");
      break;
    default:
      Serial.printf("Error (%s) reading!\n", esp_err_to_name(err));
    }
  }
  nvs_close(my_handle);
}
void calibrate()
{
  currentTime = millis();
  while (currentTime - overallStartTime < 10000)
  {
    currentTime = millis();
    digitalWrite(ledPin, HIGH);
    // read the input on analog pin 13:
    int sensorValue = analogRead(lightPin);
    // print out the value you read:
    // Serial.println(sensorValue);
    if (sensorValue > Max_light)
    {
      Max_light = sensorValue;
    }
    if (sensorValue < Min_light)
    {
      Min_light = sensorValue;
    }
    Serial.println(Max_light);
    Serial.println(Min_light);
    delay(100);
  }
  Serial.println("Calibration done");
  Serial.print("Max_light: ");
  Serial.println(Max_light);
  Serial.print("Min_light: ");
  Serial.println(Min_light);
  delay(500);
  digitalWrite(ledPin, LOW);
}

void sendAlert()
{
  ESP_Mail_Session session;
  session.server.host_name = SMTP_HOST;
  session.server.port = SMTP_PORT;
  session.login.email = Slave_Email;
  session.login.password = Slave_Pass;
  session.login.user_domain = "";
  if (!smtp.connect(&session))
  {
    Serial.println("Connection Failed!");
  }
  else
  {
    Serial.println("SMTP Connected!");
  }
  SMTP_Message message;
  message.sender.name = "ESP32-Alert";
  message.sender.email = Slave_Email;
  message.subject = "Security Alert - Someone is in your room!";
  message.addRecipient("zack", Master_Email);
  String htmlMsg = "<div style=\"color:#4D4D4D;\"> <h1>Security Alert</h1> <p>Someone is in your room!</p> </div>";
  message.html.content = htmlMsg.c_str();
  message.text.charSet = "us-ascii";
  message.html.transfer_encoding = Content_Transfer_Encoding::enc_base64;

  if (!MailClient.sendMail(&smtp, &message))
  {
    Serial.println("Error sending Email, ");
    Serial.println(smtp.errorReason());
  }
  else
  {
    Serial.println("Email Sent!");
  }
}

void setup_wifi(){
  Serial.print("Connecting to ");
  Serial.println(WIFI_ssid);
  WiFi.begin(WIFI_ssid, WIFI_pass);
  Serial.print("Waiting for WiFi connection");
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.println("MAC address: ");
  Serial.println(WiFi.macAddress());
}

void setupSMTP(){
  Serial.println("-----------------------");
  Serial.println("Connecting to SMTP Server");
  smtp.debug(1);
  smtp.callback(smtpCallback);
  
}

void setupBLE(){
  Serial.println("Starting BLE work!");
  BLEDevice::init("CS147SecuritySystem");
  BLEServer *pServer = BLEDevice::createServer();
  BLEService *pService = pServer->createService(SERVICE_UUID);
  BLECharacteristic *pCharacteristic = pService->createCharacteristic(
      CHARACTERISTIC_UUID,
      BLECharacteristic::PROPERTY_READ |
          BLECharacteristic::PROPERTY_WRITE);
  pCharacteristic->setCallbacks(new MyCallbacks());
  pService->start();
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x0);
  pAdvertising->setMinPreferred(0x12);
  BLEDevice::startAdvertising();
  Serial.println("Characteristic defined! Now you can read it in your phone!");
}

void smtpCallback(SMTP_Status status)
{
  /* Print the current status */
  Serial.println(status.info());

  /* Print the sending result */
  if (status.success())
  {
    Serial.println("----------------");
    ESP_MAIL_PRINTF("Message sent success: %d\n", status.completedCount());
    ESP_MAIL_PRINTF("Message sent failled: %d\n", status.failedCount());
    Serial.println("----------------\n");
    struct tm dt;

    for (size_t i = 0; i < smtp.sendingResult.size(); i++)
    {
      /* Get the result item */
      SMTP_Result result = smtp.sendingResult.getItem(i);
      time_t ts = (time_t)result.timestamp;
      localtime_r(&ts, &dt);

      ESP_MAIL_PRINTF("Message No: %d\n", i + 1);
      ESP_MAIL_PRINTF("Status: %s\n", result.completed ? "success" : "failed");
      ESP_MAIL_PRINTF("Date/Time: %d/%d/%d %d:%d:%d\n", dt.tm_year + 1900, dt.tm_mon + 1, dt.tm_mday, dt.tm_hour, dt.tm_min, dt.tm_sec);
      ESP_MAIL_PRINTF("Recipient: %s\n", result.recipients.c_str());
      ESP_MAIL_PRINTF("Subject: %s\n", result.subject.c_str());
    }
    Serial.println("----------------\n");
  }
}