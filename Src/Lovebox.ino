#include <LiquidCrystal.h>
#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <EEPROM.h>
#include <Servo.h>
#include <vector>
#include <iostream> 
#include "credentials.h"
const int rs = 16; // GPIO16
const int en = 5;  // GPIO5
const int d4 = 4;  // GPIO4
const int d5 = 0;  // GPIO0
const int d6 = 2;  // GPIO2
const int d7 = 14; // GPIO14
const int backlightPin = 13; // Pin connected to LCD backlight

const char* ssid = _ssid;
const char* password = _password;
const String url = _url;

String line1;
String line2;

LiquidCrystal lcd(rs, en, d4, d5, d6, d7);
Servo myservo; 
int pos = 90;
int increment = -5;
int lightValue;
String line;
String messageMode;
char idSaved; 
bool wasRead;  

std::vector<String> split(String s, String delimiter) {
    size_t pos_start = 0, pos_end;
    int delim_len = delimiter.length();
    String token;
    std::vector<String> res;

    while ((pos_end = s.indexOf(delimiter, pos_start)) != -1) {
        token = s.substring(pos_start, pos_end);
        pos_start = pos_end + delim_len;
        res.push_back(token);
    }

    res.push_back(s.substring(pos_start));
    return res;
}

void drawMessage(String& message) {
  lcd.clear();

  String firstLine;
  String secondLine;

  if(messageMode[0] == 't'){
    String delimiter = " ";
    std::vector<String> s = split(message, delimiter);

    int tot = 0;
    int line = 0;

    if (s.size() > 1)
    {
      for (String word : s)
      {
        if(tot + word.length() > 15 && !line)
        {
          lcd.print(firstLine);
          line++;
          tot = 0;
          lcd.setCursor(0,1);
        }
        if(!line)
          firstLine += " " + word;
        else
          secondLine += " " + word;
        tot += word.length();
      }
      if(secondLine.length())
        lcd.print(secondLine);
    }
    else
    {
      if(s.size())
      {
        lcd.print(s[0]);
      }    
    }    
  } 

  line1 = firstLine;
  line2 = secondLine;
      
  lcd.display();
}

void wifiConnect() {
  if (WiFi.status() != WL_CONNECTED) {
    WiFi.begin(ssid, password);

    // connecting to WiFi
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
    }
  }
}

void getGistMessage() {
  const int httpsPort = 443;
  const char* host = "gist.githubusercontent.com";
  
  WiFiClientSecure client;
  client.setInsecure();

  if (!client.connect(host, httpsPort)) {
    return; // failed to connect
  }
  
  client.print(String("GET ") + url + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" +
               "User-Agent: ESP8266\r\n" +
               "Connection: close\r\n\r\n");

  while (client.connected()) {
    String temp = client.readStringUntil('\n');
    if (temp == "\r") {
      break;
    }
  }
  String id = " ";
  while(!isdigit(id[0]))
  {
    id = client.readStringUntil('\n');
  }

 if(id[0] != idSaved){ // new message
    messageMode = client.readStringUntil('\n');
    if (messageMode[0] == 't')
    {
      line = client.readStringUntil(0);
    } else {
      // binary image is corrupted if readStringUntil() takes too long
      // fix: read string line by line
      line = "";
      for (int i = 0; i < 64; i++)     
      {
        line += client.readStringUntil('\n');
        line += "\n";
      }
      if (line.length() != 8256)
      {
        getGistMessage();
      }
    }
    wasRead = 0;
    idSaved = id[0];
    EEPROM.write(142, idSaved);
    EEPROM.write(144, wasRead);
    EEPROM.commit(); 
    drawMessage(line);
  }
}

void spinServo(){
    myservo.write(pos);      
    delay(50);    // wait 50ms to turn servo

    if(pos == 55 || pos == 135)
        increment *= -1;

    pos += increment;
}

void makeTextSpin()
{
  lcd.clear(); // Clear the display
  lcd.setCursor(0, 0); // Reset cursor to the first line
  lcd.print(line1); // Reprint the original text
  lcd.setCursor(0, 1); // Move to the second line
  lcd.print(line2); // Reprint the second line text

  for (int positionCounter = 0; positionCounter < 50; positionCounter++) {
    delay(450);
    lcd.setCursor(0,0);
    lcd.scrollDisplayLeft();
    lcd.setCursor(0,1);
    lcd.scrollDisplayLeft();

    delay(450);
  }
  lcd.clear(); // Clear the display
  lcd.setCursor(0, 0); // Reset cursor to the first line
  lcd.print(line1); // Reprint the original text
  lcd.setCursor(0, 1); // Move to the second line
  lcd.print(line2); // Reprint the second line text
}

void turnOffLCD() {
  digitalWrite(backlightPin, LOW); // Turn backlight OFF
  lcd.noDisplay(); // Turn off characters on the LCD
}

void turnOnLCD() {
  digitalWrite(backlightPin, HIGH); // Turn backlight ON
  lcd.display(); // Turn on characters on the LCD
}

void setup() {
  myservo.attach(12);       // Servo on D6
  
  lcd.begin(16, 2);
  lcd.print("<3 LOVEBOX <3");
  lcd.display();

  pinMode(backlightPin, OUTPUT); // Set backlight pin as output
  digitalWrite(backlightPin, HIGH); // Turn backlight ON
  
  wifiConnect();

  EEPROM.begin(512);
  idSaved = EEPROM.get(142, idSaved);
  wasRead = EEPROM.get(144, wasRead);

  Serial.begin(9600);
  
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    wifiConnect();
  }

  if(wasRead){
    getGistMessage();   
  }

  turnOffLCD();

  while(!wasRead){   
    yield();
    spinServo();    // turn heart
    lightValue = analogRead(A0);      // read light value

    if(lightValue > 750) { 
      wasRead = 1;
      EEPROM.write(144, wasRead);
      EEPROM.commit();

      turnOnLCD();
    }
  }
  
  makeTextSpin();

  delay(60000); // wait a minute before request gist again
}
