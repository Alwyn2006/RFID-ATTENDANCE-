
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <SPI.h>
#include <MFRC522.h>
#include <HTTPSRedirect.h>

#include<Wire.h>
#include<LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(0x27, 16, 2);
//---------------------------------------------------------------------------------------------------------
// Enter Google Script Deployment ID:
const char *GScriptId = "A";
String gate_number = "UB 1011";
//---------------------------------------------------------------------------------------------------------
// Enter network credentials:
const char* ssid     = "vivo X100";
const char* password = "";
//---------------------------------------------------------------------------------------------------------
// Enter command (insert_row or append_row) and your Google Sheets sheet name (default is Sheet1):
String payload_base =  "{\"command\": \"insert_row\", \"sheet_name\": \"Sheet1\", \"values\": ";
String payload = "";
//---------------------------------------------------------------------------------------------------------
// Google Sheets setup (do not edit)
const char* host        = "script.google.com";
const int   httpsPort   = 443;
const char* fingerprint = "";
String url = String("/macros/s/") + GScriptId + "/exec";
HTTPSRedirect* client = nullptr;
//------------------------------------------------------------
// Declare variables that will be published to Google Sheets
String student_id;
//------------------------------------------------------------
int blocks[] = {4,5,6,8,9};
#define total_blocks  (sizeof(blocks) / sizeof(blocks[0]))
//------------------------------------------------------------
#define RST_PIN  0  //D3
#define SS_PIN   2  //D4
#define BUZZER   4  //D2
//------------------------------------------------------------
MFRC522 mfrc522(SS_PIN, RST_PIN);
MFRC522::MIFARE_Key key;  
MFRC522::StatusCode status;
//------------------------------------------------------------
/* Be aware of Sector Trailer Blocks */
int blockNum = 2;  
/* Create another array to read data from Block */
/* Length of buffer should be 2 Bytes more than the size of Block (16 Bytes) */
byte bufferLen = 18;
byte readBlockData[18];
//------------------------------------------------------------

/****************************************************************************************************
 * setup Function
****************************************************************************************************/
void setup() {
  //----------------------------------------------------------
  Serial.begin(9600);        
  delay(1500);  // Changed delay time to 1500 ms (1.5 seconds)
  Serial.println('\n');
  //----------------------------------------------------------
  SPI.begin();
  //----------------------------------------------------------
  //initialize lcd screen
  lcd.init();
  // turn on the backlight
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0,0); //col=0 row=0
  lcd.print("Connecting to");
  lcd.setCursor(0,1); //col=0 row=0
  lcd.print("WiFi...");
  //----------------------------------------------------------
  // Connect to WiFi
  WiFi.begin(ssid, password);             
  Serial.print("Connecting to ");
  Serial.print(ssid); Serial.println(" ...");
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(1500);  // Changed delay time to 1500 ms (1.5 seconds)
    Serial.print(".");
  }
  Serial.println('\n');
  Serial.println("WiFi Connected!");
  Serial.println(WiFi.localIP());
  //----------------------------------------------------------
  // Use HTTPSRedirect class to create a new TLS connection
  client = new HTTPSRedirect(httpsPort);
  client->setInsecure();
  client->setPrintResponseBody(true);
  client->setContentTypeHeader("application/json");
  //----------------------------------------------------------
  lcd.clear();
  lcd.setCursor(0,0); //col=0 row=0
  lcd.print("Connecting to");
  lcd.setCursor(0,1); //col=0 row=0
  lcd.print("Google ");
  delay(1500);  // Changed delay time to 1500 ms (1.5 seconds)
  //----------------------------------------------------------
  Serial.print("Connecting to ");
  Serial.println(host);
  //----------------------------------------------------------
  // Try to connect for a maximum of 5 times
  bool flag = false;
  for(int i=0; i<5; i++){ 
    int retval = client->connect(host, httpsPort);
    if (retval == 1){
      flag = true;
      String msg = "Connected. OK";
      Serial.println(msg);
      lcd.clear();
      lcd.setCursor(0,0); //col=0 row=0
      lcd.print(msg);
      delay(1500);  // Changed delay time to 1500 ms (1.5 seconds)
      break;
    }
    else
      Serial.println("Connection failed. Retrying...");
  }
  //----------------------------------------------------------
  if (!flag){
    lcd.clear();
    lcd.setCursor(0,0); //col=0 row=0
    lcd.print("Connection fail");
    Serial.print("Could not connect to server: ");
    Serial.println(host);
    delay(1500);  // Changed delay time to 1500 ms (1.5 seconds)
    return;
  }
  //----------------------------------------------------------
  delete client;    // delete HTTPSRedirect object
  client = nullptr; // delete HTTPSRedirect object
  //----------------------------------------------------------
}

/****************************************************************************************************
 * loop Function
****************************************************************************************************/
void loop() {
  //----------------------------------------------------------------
  static bool flag = false;
  if (!flag){
    client = new HTTPSRedirect(httpsPort);
    client->setInsecure();
    flag = true;
    client->setPrintResponseBody(true);
    client->setContentTypeHeader("application/json");
  }
  if (client != nullptr){
    if (!client->connected()){
      int retval = client->connect(host, httpsPort);
      if (retval != 1){
        Serial.println("Disconnected. Retrying...");
        lcd.clear();
        lcd.setCursor(0,0); //col=0 row=0
        lcd.print("Disconnected.");
        lcd.setCursor(0,1); //col=0 row=0
        lcd.print("Retrying...");
        return; 
      }
    }
  }
  else{
    Serial.println("Error creating client object!"); 
  }
  //----------------------------------------------------------------
  lcd.clear();
  lcd.setCursor(0,0); //col=0 row=0
  lcd.print("Scan your Tag");
  /* Initialize MFRC522 Module */
  mfrc522.PCD_Init();
  if ( ! mfrc522.PICC_IsNewCardPresent()) {return;}
  if ( ! mfrc522.PICC_ReadCardSerial()) {return;}
  //----------------------------------------------------------------
  String values = "", data;
  for (byte i = 0; i < total_blocks; i++) {
    ReadDataFromBlock(blocks[i], readBlockData);
    if(i == 0){
      data = String((char*)readBlockData);
      data.trim();
      student_id = data;
      values = "\"" + data + ",";
    }
    else{
      data = String((char*)readBlockData);
      data.trim();
      values += data + ",";
    }
  }
  values += gate_number + "\"}";
  payload = payload_base + values;
  //----------------------------------------------------------------
  lcd.clear();
  lcd.setCursor(0,0); 
  lcd.print("Publishing Data");
  lcd.setCursor(0,1); 
  lcd.print("Please Wait...");
  //----------------------------------------------------------------
  Serial.println("Publishing data...");
  Serial.println(payload);
  if(client->POST(url, host, payload)){ 
    Serial.println("[OK] Data published.");
    lcd.clear();
    lcd.setCursor(0,0); 
    lcd.print("Student ID: "+student_id);
    lcd.setCursor(0,1); 
    lcd.print("Thanks");
    delay(1500);  // Changed delay time to 1500 ms (1.5 seconds) after publishing
  }
  else{
    Serial.println("Error while connecting");
    lcd.clear();
    lcd.setCursor(0,0); 
    lcd.print("Failed.");
    lcd.setCursor(0,1); 
    lcd.print("Try Again");
    delay(1500);  // Changed delay time to 1500 ms (1.5 seconds) if publishing fails
  }
  //----------------------------------------------------------------
  delay(1500);  // Changed delay time to 1500 ms (1.5 seconds) after each loop
}

/****************************************************************************************************
 * ReadDataFromBlock() function
 ****************************************************************************************************/
void ReadDataFromBlock(int blockNum, byte readBlockData[]) 
{ 
  //---------------------------------------------------------------------------- 
  for (byte i = 0; i < 6; i++) {
    key.keyByte[i] = 0xFF;
  }
  //---------------------------------------------------------------------------- 
  status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, blockNum, &key, &(mfrc522.uid));
  if (status != MFRC522::STATUS_OK){
     Serial.print("Authentication failed for Read: ");
     Serial.println(mfrc522.GetStatusCodeName(status));
     return;
  }
  else {
    Serial.println("Authentication success");
  }
  //----------------------------------------------------------------------------  
  status = mfrc522.MIFARE_Read(blockNum, readBlockData, &bufferLen);
  if (status != MFRC522::STATUS_OK) {
    Serial.print("Reading failed: ");
    Serial.println(mfrc522.GetStatusCodeName(status));
    return;
  }
  else {
    readBlockData[16] = ' ';
    readBlockData[17] = ' ';
    Serial.println("Block was read successfully");  
  }
  //----------------------------------------------------------------------------  
}
