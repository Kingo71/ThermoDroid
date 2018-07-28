/*
 Name:		ThermoDroid.ino
 Created:	7/18/2018 11:13:16 PM
 Author:	Kingo
*/

#include <SerialCommand.h>

#include <OneWire.h>

#include <DallasTemperature.h>

#include <LiquidCrystal_I2C.h>

#include <Button.h>

#include <Streaming.h>

#include <DS3232RTC.h>

#include <Time.h>

#include <TimeLib.h>

#include <SoftwareSerial.h>

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

float tempril;
int alarmhightemp = 25;
int alarmlowtemp = 10;
int ledstat1 = LOW;
int ledstat2 = LOW;
int pos = 0;
int long interval = 1000;
int long bttimeout = 15000;
int xore;
int xminuti;
bool client = false;
bool LED1 = false;
bool LED2 = false;
bool hightemp = false;
bool lowtemp = false;
bool interface;
bool clockmenu = true;
bool blink = false;
unsigned long clientlastmillis;
unsigned long lastmillis = 0;

time_t t;

// set the LCD address to 0x3f for a 16 chars 2 line display
// Set the pins on the I2C chip used for LCD connections:
//                    addr, en,rw,rs,d4,d5,d6,d7,bl,blpol

LiquidCrystal_I2C lcd(0x3f, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);

SoftwareSerial BtSerial(bluetoothRX, bluetoothTX); // RX, TX

SerialCommand cmdTherm(BtSerial);


// make a cute degree symbol

uint8_t degree[8] = { 140,146,146,140,128,128,128,128 };

Button button = Button(Button1, BUTTON_PULLUP);

void onPress(Button& b) {
	clockmenu = !clockmenu;
	lcd.clear();
}

void setup()
{

	// Start up the library
	Serial.begin(115200);
	BtSerial.begin(9600);
	sensor_inhouse.begin();
	button.pressHandler(onPress);
	if (year(RTC.get()) < 2015) Serial.println(F(__TIME__));
	//setSyncProvider() causes the Time library to synchronize with the
	//external RTC by calling RTC.get() every five minutes by default.
	setSyncProvider(RTC.get);
	Serial << F("RTC Sync");
	if (timeStatus() != timeSet) Serial << F(" FAIL!");
	Serial << endl;

	// Imposta il valore di righe e colonne del display LCD
	lcd.begin(16, 2);
	lcd.createChar(0, degree);
	pinMode(ledPin1, OUTPUT);
	pinMode(ledPin2, OUTPUT);

	// ------- Quick 3 blinks of backlight  -------------

	for (int i = 0; i< 3; i++)
	{
		lcd.backlight();
		delay(250);
		lcd.noBacklight();
		delay(250);
	}
	lcd.backlight(); // finish with backlight on 

					 // Setup callbacks for SerialCommand commands
	cmdTherm.addCommand("<T", syncpctime);          // Syc time
	cmdTherm.addCommand("gtemp", gettemp);         // Get current Temperature
	cmdTherm.addDefaultHandler(unrecognized);      // Handler for command that isn't matched  (says "What?")
	Serial.println("Ready");

}



void loop()
{
	static time_t tLast;
	t = now();

	lcd.setCursor(0, 1); // bottom left
	lcd.print(hour(t));
	lcd.print(":");
	lcd.print(minute(t));
	lcd.print(":");
	lcd.print(second(t));
	lcd.setCursor(9, 1);
	lcd.print(monthShortStr(month(t)));
	lcd.print(" ");
	String anno = String(year(t));
	lcd.print(anno.substring(4, 2));


	if ((millis() - clientlastmillis) >= bttimeout && client == true)
	{
		client = false;
		Serial.println("Client connection timeout...");
	}
	if ((millis() - lastmillis) >= interval)
	{
		sensor_inhouse.requestTemperatures();
		tempril = sensor_inhouse.getTempCByIndex(0);
		lcd.clear();
		lcd.setCursor(0, 0); // upper left
		lcd.print("Temp:");
		lcd.print(int(tempril));
		lcd.write((byte)0);
		lcd.setCursor(9, 0);
		lcd.print(dayShortStr(weekday(t)));
		lcd.print(" ");
		lcd.print(day(t));



		if (t != tLast) {
			tLast = t;

			BtSerial << "<T " << tempril;
			BtSerial << "\n";
		}


		if (tempril < alarmlowtemp) lowtemp = true;
		else lowtemp = false;
		if (tempril > alarmhightemp) hightemp = true;
		else hightemp = false;
		if (LED1 &&lowtemp && ledstat1 == LOW)
		{
			ledstat1 = HIGH;
			if (client)
			{
				BtSerial.print("ld01 1");
				BtSerial.print("\n");
			}
		}
		else
		{
			ledstat1 = LOW;
		}
		if (LED2 && hightemp && ledstat2 == LOW)
		{
			ledstat2 = HIGH;
			if (client)
			{
				BtSerial.print("ld02 1");
				BtSerial.print("\n");
			}
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


	button.process();
	cmdTherm.readSerial();

}


// Functions --------------------------------

// This gets set as the default handler, and gets called when no other command matches.
void unrecognized() {
	Serial.println("What?");
}

void syncpctime()
{

	double aNumber;
	char *arg;
	const unsigned long DEFAULT_TIME = 1357041600; // Jan 1 2013 

	Serial.println("We're in processCommand");
	arg = cmdTherm.next();

	if (arg != NULL) {
		aNumber = atof(arg);    // Converts a char string to an integer
		if (aNumber < DEFAULT_TIME) { // check the value is a valid time (greater than Jan 1 2013)
			return; // time is not valid
		}
		Serial.println(arg);
		time_t t = aNumber;
		if (t != 0) {
			t = t + 3600;
			RTC.set(t);   // set the RTC and the system time to the received value
			setTime(t);
			Serial.println("Sync succed...");
			return;
		}

	}
	else {
		Serial.println("No arguments");
	}

}


void gettemp()
{

	Serial.print("Temperatura Rilevata:");
	Serial.print(tempril);
	Serial.println(" C");

}





