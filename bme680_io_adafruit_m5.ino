#include "config.h"

#include <Wire.h>
#include <HTTPClient.h>
#include <M5Stack.h>
#include <AdafruitIO_WiFi.h>
#include <Adafruit_BME680.h>

/* Create config.h file and paste them:
#define IO_USERNAME  "PLEASE_FILL_HERE"
#define IO_KEY       "PLEASE_FILL_HERE"
#define WIFI_SSID "PLEASE FILL_HERE"
#define WIFI_PASS "PLEASE_FILL_HERE"
*/

#define IIC_ADDR  uint8_t(0x76)

AdafruitIO_WiFi io(IO_USERNAME, IO_KEY, WIFI_SSID, WIFI_PASS);
AdafruitIO_Feed *gas = io.feed("my-office-gas");
AdafruitIO_Feed *temp = io.feed("my-office-temp");
AdafruitIO_Feed *pressure = io.feed("my-office-pressure");
AdafruitIO_Feed *humidity = io.feed("my-office-humidity");

Adafruit_BME680 bme;
TFT_eSprite lcd = TFT_eSprite(&M5.Lcd);

void setup() {
	setupM5();
	setupBME();
  	connectToAdafruitIO();
}

void setupM5() {
	M5.begin();
	M5.Power.setPowerBoostKeepOn(true);
	dacWrite(25, 0);
	lcd.setTextSize(2);
    lcd.setColorDepth(8);
    lcd.createSprite(320, 240);
}

void setupBME() {
	bme.begin(IIC_ADDR, true);
  	bme.setTemperatureOversampling(BME680_OS_8X);
  	bme.setHumidityOversampling(BME680_OS_2X);
  	bme.setPressureOversampling(BME680_OS_4X);
  	bme.setIIRFilterSize(BME680_FILTER_SIZE_3);
  	bme.setGasHeater(320, 150); // 320*C for 150 ms
}

void connectToAdafruitIO() {
	Serial.print("Connecting to Adafruit IO");

	  // connect to io.adafruit.com
	  io.connect();
	
	  // wait for a connection
	  while(io.status() < AIO_CONNECTED) {
	    Serial.print(".");
	    delay(500);
	  }
	
	  // we are connected
	  Serial.println();
	  Serial.println(io.statusText());
}

/*
 *  Loop
 */

unsigned long endTime = 0;
void loop() {
	if (readCompleted()) {
		// EV: begin reading
		endTime = max(bme.beginReading(), millis() + (long unsigned int)3000);
		if (endTime == 0) {
    		Serial.println(F("Failed to begin reading :("));
    		return;
  		}
	}

	
	if(millis() > endTime && bme.endReading()) {
		// EV: done reading
		drawInfo();
		publishIfNeeded();
    	endTime = 0;
	}
}

bool readCompleted() {
	return endTime == 0;
}

unsigned long lastPublished=0;
void publishIfNeeded() {
	if(millis() - lastPublished > 10000) {
		// it seems to be wrong value
		if(1 >= bme.gas_resistance / 1000.0) {
			return;
		}
		
		gas->save(bme.gas_resistance / 1000.0);
		pressure->save(bme.pressure / 100.0);
		temp->save(bme.temperature);
		humidity->save(bme.humidity);
		lastPublished = millis();
	}
}

void drawInfo() {
	lcd.setCursor(10, 160);
	lcd.print("Refreshed: " + String(millis()) + " " + String(endTime) + " " + String(lastPublished));

	lcd.setCursor(0, 10);
	lcd.print(String(bme.temperature) + "*C " + String(bme.pressure / 100.0) + "hPa");
	lcd.setCursor(0, 50);
	lcd.print(String(bme.humidity) + "% " + String(bme.gas_resistance / 1000.0) + "KOhms ");

	lcd.setCursor(10, 120);
	lcd.print(io.statusText());
	
	lcd.pushSprite(0, 0);
}

void resetIfNeeded() {
	if(millis() > 1000 * 60 * 60 * 12) { // 12 hours
		M5.Power.reset();
	} 
}
