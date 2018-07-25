/*
 Name:		ThermoDroid.ino
 Created:	7/18/2018 11:13:16 PM
 Author:	Kingo
*/

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Streaming.h>
#include <DS3232RTC.h>
#include <Time.h>
#include <TimeLib.h>

#define ONE_WIRE_BUS_1 2

/*  code to process time sync messages from the serial port   */
#define TIME_HEADER  "T"   // Header tag for serial time sync message

OneWire oneWire_in(ONE_WIRE_BUS_1);
DallasTemperature sensor_inhouse(&oneWire_in);

// PINS
#define bluetoothRX 11 // RX Pin
#define bluetoothTX 12 // TX Pin
#define ledPin1 9
#define ledPin2 10
#define Button1 8 
// Variables declaration


String days[7] = { "Sun","Mon","Tue","Wed","Thu","Fri","Sat" };
float tempril;
int alarmhightemp = 25;
int alarmlowtemp = 10;
int ledstat1 = LOW;
int ledstat2 = LOW;
int pos = 0;
int interval = 1000;
int xore;
int xminuti;
bool LED1 = false;
bool LED2 = false;
bool hightemp = false;
bool lowtemp = false;
bool blink = false;
unsigned long lastmillis = 0;

time_t t;

// OLED display TWI address
#define OLED_ADDR   0x3C

// reset pin not used on 4-pin OLED module
Adafruit_SSD1306 display(-1);  // -1 = no reset pin

// 128 x 64 pixel display
#if (SSD1306_LCDHEIGHT != 64)
#error("Height incorrect, please fix Adafruit_SSD1306.h!");
#endif

unsigned static char temperature_icon16x16[] =
{
	0b00000001, 0b11000000, //        ###      
	0b00000011, 0b11100000, //       #####     
	0b00000111, 0b00100000, //      ###  #     
	0b00000111, 0b11100000, //      ######     
	0b00000111, 0b00100000, //      ###  #     
	0b00000111, 0b11100000, //      ######     
	0b00000111, 0b00100000, //      ###  #     
	0b00000111, 0b11100000, //      ######     
	0b00000111, 0b00100000, //      ###  #     
	0b00001111, 0b11110000, //     ########    
	0b00011111, 0b11111000, //    ##########   
	0b00011111, 0b11111000, //    ##########   
	0b00011111, 0b11111000, //    ##########   
	0b00011111, 0b11111000, //    ##########   
	0b00001111, 0b11110000, //     ########    
	0b00000111, 0b11100000, //      ######     
};


void setup()
{

	// Start up the library
	Serial.begin(115200);
	sensor_inhouse.begin();
	if (year(RTC.get()) < 2015) Serial.println(F(__TIME__));
	//setSyncProvider() causes the Time library to synchronize with the
	//external RTC by calling RTC.get() every five minutes by default.
	setSyncProvider(RTC.get);
	Serial << F("RTC Sync");
	if (timeStatus() != timeSet) Serial << F(" FAIL!");
	Serial << endl;

	// initialize and clear display
	display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR);
	display.clearDisplay();
	display.display();
	display.setTextSize(1);
	display.setTextColor(WHITE);

	pinMode(ledPin1, OUTPUT);
	pinMode(ledPin2, OUTPUT);

	Serial.println("Ready");

}



void loop()
{
	static time_t tLast;
	t = now();
	display.setTextSize(1);
	display.setCursor(15, 0); // upper left
	display.print(" ");
	display.print(days[weekday(t) - 1]);
	display.print(" ");
	if (day(t)<10) display.print("0");
	display.print(day(t));
	display.print("/");
	if (month(t)<10) display.print("0");
	display.print(month(t));
	display.print("/");
	display.print(year(t));
	display.display();

	display.clearDisplay();
	display.setTextSize(2);
	display.setTextColor(WHITE);
	display.setCursor(15, 20); // bottom left
	if (hour(t)<10) display.print("0");
	display.print(hour(t));
	display.print(":");
	if (minute(t)<10) display.print("0");
	display.print(minute(t));
	display.print(":");
	if (second(t)<10) display.print("0");
	display.print(second(t));
	display.drawBitmap(35, 45, temperature_icon16x16, 16, 16, 1);
	display.setCursor(52, 45);
	display.print(int(tempril));
	display.print((char)247);


	if ((millis() - lastmillis) >= interval)
	{
		sensor_inhouse.requestTemperatures();
		tempril = sensor_inhouse.getTempCByIndex(0);
		
		if (t != tLast) {
			tLast = t;
						
		}


		if (tempril < alarmlowtemp) lowtemp = true;
		else lowtemp = false;
		if (tempril > alarmhightemp) hightemp = true;
		else hightemp = false;
		if (LED1 &&lowtemp && ledstat1 == LOW)
		{
			ledstat1 = HIGH;
		}
		else
		{
			ledstat1 = LOW;
		}
		if (LED2 && hightemp && ledstat2 == LOW)
		{
			ledstat2 = HIGH;
		}
		else
		{
			ledstat2 = LOW;
		}

		lastmillis = millis();
		blink = !blink;

	}

	digitalWrite(ledPin1, ledstat1);
	digitalWrite(ledPin2, ledstat2);

	

}






