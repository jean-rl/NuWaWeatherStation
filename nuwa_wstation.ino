#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>

#define SEALEVELPRESSURE_HPA (1013.25)

Adafruit_BME280 bme;

float temperature, humidity, pressure, altitude;

const char* ssid = "IZZI-ABB9";  // SSID of the Wi-Fi network
const char* password = "9CC8FC6FABB9";  //Password of the Wi-Fi network

WebServer server(80);

#include <virtuabotixRTC.h> //RTC library
virtuabotixRTC myRTC(15, 2, 4); //(CLK,DAT,RST)

int contador = 0;

// Libraries for SD card
#include "FS.h"
#include "SD.h"
#include <SPI.h>

//CS pin for the SD card module
#define SD_CS 5

// Save reading number on RTC memory
RTC_DATA_ATTR int readingID = 0;

String dataMessage;

// Data wire is connected to ESP32 GPIO 21
#define ONE_WIRE_BUS 21

// Variables to save date and time
String formattedDate;
String dayStamp;
String timeStamp;

char dayofmonth[3];
char smonth[3];
char syear[3];
char shours[3];
char sminutes[3];
char sseconds[3];
char stemperature[7];
char shumidity[7];
char spressure[7];
char dots[2] = ":";
char finish[4] = " \r\n";
char buf[60];

void setup() {
  Serial.begin(115200);
  delay(100);
  
  bme.begin(0x76);   

  Serial.println("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
  delay(1000);
  Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected..!");
  Serial.print("Got IP: ");  Serial.println(WiFi.localIP());

  server.on("/", handle_OnConnect);
  server.onNotFound(handle_NotFound);

  server.begin();
  Serial.println("HTTP server started");

    // Initialize SD card
  SD.begin(SD_CS);  
  if(!SD.begin(SD_CS)) {
    Serial.println("Card Mount Failed");
    return;
  }
  uint8_t cardType = SD.cardType();
  if(cardType == CARD_NONE) {
    Serial.println("No SD card attached");
    return;
  }
  Serial.println("Initializing SD card...");
  if (!SD.begin(SD_CS)) {
    Serial.println("ERROR - SD card initialization failed!");
    return;    // init failed
  }

  // seconds, minutes, hours, day of the week, day of the month, month, year
 //myRTC.setDS1302Time(00, 13, 15, 1, 6, 12, 2020); //Here you write your actual time/date as shown above 

}
void loop() {
  server.handleClient();
  
  if(contador < 10000)
  {
    Serial.print("Contador: ");
    Serial.println(contador);
    contador++;
  }
  if(contador >= 10000)
  {
    myRTC.updateTime();
    Serial.println("Escribiendo datos ficticios");
    File file = SD.open("/data.txt");
    if(!file) {
      Serial.println("File doens't exist");
      Serial.println("Creating file...");
      writeFile(SD, "/data.txt", "day, month, year, hours, minutes, seconds: temperature, humidity, pressure\r\n");
    }
    else {
      sprintf(dayofmonth, "%d", myRTC.dayofmonth);
      sprintf(smonth, "%d", myRTC.month);
      sprintf(syear, "%d", myRTC.year);
      sprintf(shours, "%d", myRTC.hours);
      sprintf(sminutes, "%d", myRTC.minutes);
      sprintf(sseconds, "%d", myRTC.seconds);
      sprintf(stemperature, "%f", bme.readTemperature());
      sprintf(shumidity, "%f", bme.readHumidity());
      sprintf(spressure, "%f", bme.readPressure()/ 100.0F);
      strcpy(buf,dayofmonth);
      strcat(buf,dots);
      strcat(buf,smonth);
      strcat(buf,dots);
      strcat(buf,syear);
      strcat(buf,dots);
      strcat(buf,shours);
      strcat(buf,dots);
      strcat(buf,sminutes);
      strcat(buf,dots);
      strcat(buf,sseconds);
      strcat(buf,dots);
      strcat(buf,stemperature);
      strcat(buf,dots);
      strcat(buf,shumidity);
      strcat(buf,dots);
      strcat(buf,spressure);
      strcat(buf,finish);
      Serial.println("File already exists");  
      appendFile(SD, "/data.txt", buf);
    }
    file.close();
    contador = 0;
  }
}

void handle_OnConnect() {
  temperature = bme.readTemperature();
  humidity = bme.readHumidity();
  pressure = bme.readPressure() / 100.0F;
  altitude = bme.readAltitude(SEALEVELPRESSURE_HPA);
  server.send(200, "text/html", SendHTML(temperature,humidity,pressure,altitude));   
}

void handle_NotFound(){
  server.send(404, "text/plain", "Not found");
}

String SendHTML(float temperature,float humidity,float pressure,float altitude){
  String ptr = "<!DOCTYPE html> <html>\n";
  ptr +="<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0, user-scalable=no\">\n";
  ptr +="<title>ESP32 Weather Station</title>\n";
  ptr +="<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}\n";
  ptr +="body{margin-top: 50px;} h1 {color: #444444;margin: 50px auto 30px;}\n";
  ptr +="p {font-size: 24px;color: #444444;margin-bottom: 10px;}\n";
  ptr +="</style>\n";
  ptr +="</head>\n";
  ptr +="<body>\n";
  ptr +="<div id=\"webpage\">\n";
  ptr +="<h1>ESP32 Weather Station</h1>\n";
  ptr +="<p>Temperature: ";
  ptr +=temperature;
  ptr +="&deg;C</p>";
  ptr +="<p>Humidity: ";
  ptr +=humidity;
  ptr +="%</p>";
  ptr +="<p>Pressure: ";
  ptr +=pressure;
  ptr +="hPa</p>";
  ptr +="<p>Altitude: ";
  ptr +=altitude;
  ptr +="m</p>";
  ptr +="</div>\n";
  ptr +="</body>\n";
  ptr +="</html>\n";
  return ptr;
}

// Write to the SD card 
void writeFile(fs::FS &fs, const char * path, const char * message) {
  Serial.printf("Writing file: %s\n", path);

  File file = fs.open(path, FILE_WRITE);
  if(!file) {
    Serial.println("Failed to open file for writing");
    return;
  }
  if(file.print(message)) {
    Serial.println("File written");
  } else {
    Serial.println("Write failed");
  }
  file.close();
}

// Append data to the SD card 
void appendFile(fs::FS &fs, const char * path, const char * message) {
  Serial.printf("Appending to file: %s\n", path);

  File file = fs.open(path, FILE_APPEND);
  if(!file) {
    Serial.println("Failed to open file for appending");
    return;
  }
  if(file.print(message)) {
    Serial.println("Message appended");
  } else {
    Serial.println("Append failed");
  }
  file.close();
}
