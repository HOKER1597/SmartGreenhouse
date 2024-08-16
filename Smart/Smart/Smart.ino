#include <Arduino.h>
#include <EEPROM.h>
#include "DHT.h"
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <SoftwareSerial.h>
#include <ArduinoJson.h>

#define EEPROM_ADDRESS_TEMPERATURE 0
#define EEPROM_ADDRESS_TOTAL_HUMIDITY 1
#define EEPROM_ADDRESS_SOIL_HUMIDITY 2
#define EEPROM_ADDRESS_PHOTO_RESISTOR 3
#define PeriodbuttonPressTimer 200
#define DHTPINDATA 2
#define BUTTONPINCONTOLNEXT 3
#define BUTTONPINCONTOLBACK 4
#define BUTTONPINCONTOLOK 5
#define RELAYPINHEATING 6
#define RELAYPINLIGHTING 7
#define RELAYPINVENTILATION 8
#define RELAYPINWATERPUMP 9
#define ESPRX 10
#define ESPTX 11
#define ANALOGPHOTORESISTOR 0
#define ANALOGSOILHUMIDITY 1
#define SDALCD 4
#define SCLLCD 5
#define DHTTYPE DHT11

DHT dht(DHTPINDATA, DHTTYPE);

LiquidCrystal_I2C lcd(0x27, 16, 2);

SoftwareSerial esp8266Controller(ESPRX, ESPTX);

float totalHumidity = 0;
float soilHumidity = 0;
float temperature = 0;
float photoresistor = 0;

float totalHumidityUserBound = 50;
float temperatureUserBound = 30;
float soilHumidityUserBound = 50;
float photoresistorUserBound = 700;

int menuItem = 0;

bool buttonPressed = false;
bool editMode = false;
uint32_t buttonPressTimer;

void setup() {
  Serial.begin(57600);
  while (!Serial) {
  }
  esp8266Controller.begin(57600);
  lcd.init();
  lcd.backlight();
  pinMode(DHTPINDATA, INPUT);
  pinMode(BUTTONPINCONTOLNEXT, INPUT_PULLUP);
  pinMode(BUTTONPINCONTOLBACK, INPUT_PULLUP);
  pinMode(BUTTONPINCONTOLOK, INPUT_PULLUP);
  pinMode(RELAYPINHEATING, OUTPUT);
  pinMode(RELAYPINLIGHTING, OUTPUT);
  pinMode(RELAYPINVENTILATION, OUTPUT);
  pinMode(RELAYPINWATERPUMP, OUTPUT);
  if (EEPROM.read(EEPROM_ADDRESS_TEMPERATURE) && EEPROM.read(EEPROM_ADDRESS_TOTAL_HUMIDITY) && EEPROM.read(EEPROM_ADDRESS_SOIL_HUMIDITY) && EEPROM.read(EEPROM_ADDRESS_PHOTO_RESISTOR)) {
    temperatureUserBound = EEPROM.read(EEPROM_ADDRESS_TEMPERATURE);
    totalHumidityUserBound = EEPROM.read(EEPROM_ADDRESS_TOTAL_HUMIDITY);
    soilHumidityUserBound = EEPROM.read(EEPROM_ADDRESS_SOIL_HUMIDITY);
    photoresistorUserBound = EEPROM.read(EEPROM_ADDRESS_PHOTO_RESISTOR);
  }
  dht.begin();
}

void loop() {
  if (Serial.available()) {
    String data = Serial.readStringUntil('\n');
    parseSerialData(data);
  }
  soilHumidity = map(analogRead(ANALOGSOILHUMIDITY), 0, 595, 0, 100);
  totalHumidity = dht.readHumidity();
  temperature = dht.readTemperature();
  photoresistor = analogRead(ANALOGPHOTORESISTOR);
  // Создаем JSON-объект
  StaticJsonDocument<200> jsonDoc;
  // Заполняем JSON-объект данными
  jsonDoc["soilHumidity"] = soilHumidity;
  jsonDoc["totalHumidity"] = totalHumidity;
  jsonDoc["temperature"] = temperature;
  jsonDoc["photoresistor"] = photoresistor;
  jsonDoc["heatingRelay"] = digitalRead(RELAYPINHEATING);
  jsonDoc["lightingRelay"] = digitalRead(RELAYPINLIGHTING);
  jsonDoc["ventilationRelay"] = digitalRead(RELAYPINVENTILATION);
  jsonDoc["waterPumpRelay"] = digitalRead(RELAYPINWATERPUMP);

  // Сериализуем JSON-объект в строку JSON
  String jsonString;
  serializeJson(jsonDoc, jsonString);

  // Отправляем строку JSON через Serial
  Serial.println(jsonString);
  if (totalHumidity > totalHumidityUserBound) {
    digitalWrite(RELAYPINVENTILATION, HIGH);
  } else {
    digitalWrite(RELAYPINVENTILATION, LOW);
  }
  if (temperature < temperatureUserBound) {
    digitalWrite(RELAYPINHEATING, HIGH);
  } else {
    digitalWrite(RELAYPINHEATING, LOW);
  }
  if (photoresistor < photoresistorUserBound) {
    digitalWrite(RELAYPINLIGHTING, HIGH);
  } else {
    digitalWrite(RELAYPINLIGHTING, LOW);
  }
  if (soilHumidity < soilHumidityUserBound) {
    digitalWrite(RELAYPINWATERPUMP, HIGH);
  } else {
    digitalWrite(RELAYPINWATERPUMP, LOW);
  }
  MenuControl();
}
void MenuControl() {
  bool btnStateNext = !digitalRead(BUTTONPINCONTOLNEXT);
  bool btnStateBack = !digitalRead(BUTTONPINCONTOLBACK);
  bool btnStateOK = !digitalRead(BUTTONPINCONTOLOK);

  if (!buttonPressed && btnStateNext && millis() - buttonPressTimer > 100) {
    buttonPressTimer = millis();
    buttonPressed = true;
    if (!editMode) {
      if (menuItem != 3) {
        menuItem++;
      } else {
        menuItem = 0;
      }
    }
  }

  if (!btnStateNext && buttonPressed && millis() - buttonPressTimer > 100) {
    buttonPressTimer = millis();
    buttonPressed = false;
  }

  if (!buttonPressed && btnStateBack && millis() - buttonPressTimer > 100) {
    buttonPressTimer = millis();
    buttonPressed = true;
    if (!editMode) {
      if (menuItem != 0) {
        menuItem--;
      } else {
        menuItem = 3;
      }
    }
  }

  if (!btnStateBack && buttonPressed && millis() - buttonPressTimer > 100) {
    lcd.clear();
    buttonPressTimer = millis();
    buttonPressed = false;
  }

  if (!buttonPressed && btnStateOK && millis() - buttonPressTimer > 100) {
    lcd.clear();
    buttonPressTimer = millis();
    buttonPressed = true;
    editMode = !editMode;
    if (editMode == false) {
      EEPROM.write(EEPROM_ADDRESS_TEMPERATURE, temperatureUserBound);
      EEPROM.write(EEPROM_ADDRESS_TOTAL_HUMIDITY, totalHumidityUserBound);
      EEPROM.write(EEPROM_ADDRESS_SOIL_HUMIDITY, soilHumidityUserBound);
      EEPROM.write(EEPROM_ADDRESS_PHOTO_RESISTOR, photoresistorUserBound);
    }
  }

  if (!btnStateOK && buttonPressed && millis() - buttonPressTimer > 100) {
    lcd.clear();
    buttonPressTimer = millis();
    buttonPressed = false;
  }

  switch (menuItem) {
    case 0:
      lcd.setCursor(0, 0);
      lcd.print("T: " + String(temperature, 2));
      lcd.setCursor(0, 1);
      lcd.print("set T: " + String(temperatureUserBound, 2));
      if (editMode && btnStateNext && !buttonPressed && millis() - buttonPressTimer > 100) {
        buttonPressTimer = millis();
        temperatureUserBound += 1.0;
      }
      if (editMode && btnStateBack && !buttonPressed && millis() - buttonPressTimer > 100) {
        buttonPressTimer = millis();
        temperatureUserBound--;
      }
      break;
    case 1:
      lcd.setCursor(0, 0);
      lcd.print("TH: " + String(totalHumidity, 2));
      lcd.setCursor(0, 1);
      lcd.print("set TH: " + String(totalHumidityUserBound, 2));
      if (editMode && btnStateNext && !buttonPressed && millis() - buttonPressTimer > 100) {
        buttonPressTimer = millis();
        totalHumidityUserBound++;
      }
      if (editMode && btnStateBack && !buttonPressed && millis() - buttonPressTimer > 100) {
        buttonPressTimer = millis();
        totalHumidityUserBound--;
      }
      break;
    case 2:
      lcd.setCursor(0, 0);
      lcd.print("SH: " + String(soilHumidity));
      lcd.setCursor(0, 1);
      lcd.print("set SH: " + String(soilHumidityUserBound, 2));
      if (editMode && btnStateNext && !buttonPressed && millis() - buttonPressTimer > 100) {
        buttonPressTimer = millis();
        soilHumidityUserBound++;
      }
      if (editMode && btnStateBack && !buttonPressed && millis() - buttonPressTimer > 100) {
        buttonPressTimer = millis();
        soilHumidityUserBound--;
      }
      break;
    case 3:
      lcd.setCursor(0, 0);
      lcd.print("P: " + String(photoresistor, 2));
      lcd.setCursor(0, 1);
      lcd.print("set  P: " + String(photoresistorUserBound, 2));
      if (editMode && btnStateNext && !buttonPressed && millis() - buttonPressTimer > 100) {
        buttonPressTimer = millis();
        photoresistorUserBound += 10;
      }
      if (editMode && btnStateBack && !buttonPressed && millis() - buttonPressTimer > 100) {
        buttonPressTimer = millis();
        photoresistorUserBound -= 10;
      }
      break;
  }
}

void parseSerialData(String data) {
  StaticJsonDocument<200> jsonDoc;
  DeserializationError error = deserializeJson(jsonDoc, data);

  if (error) {
    Serial.print("Deserialization error: ");
    Serial.println(error.c_str());
    return;
  }

  if (jsonDoc.containsKey("temperatureUserBound")) {
    temperatureUserBound = jsonDoc["temperatureUserBound"].as<float>();
    EEPROM.write(EEPROM_ADDRESS_TEMPERATURE, temperatureUserBound);
  }
  if (jsonDoc.containsKey("totalHumidityUserBound")) {
    totalHumidityUserBound = jsonDoc["totalHumidityUserBound"].as<float>();
    EEPROM.write(EEPROM_ADDRESS_TOTAL_HUMIDITY, totalHumidityUserBound);
  }
  if (jsonDoc.containsKey("soilHumidityUserBound")) {
    soilHumidityUserBound = jsonDoc["soilHumidityUserBound"].as<float>();
    EEPROM.write(EEPROM_ADDRESS_SOIL_HUMIDITY, soilHumidityUserBound);
  }
  if (jsonDoc.containsKey("photoresistorUserBound")) {
    photoresistorUserBound = jsonDoc["photoresistorUserBound"].as<float>();
    EEPROM.write(EEPROM_ADDRESS_PHOTO_RESISTOR, photoresistorUserBound);
  }
}

void SaveUserData() {
}