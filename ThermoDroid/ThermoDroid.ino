/*
 Name:		ThermoDroid.ino
 Created:	7/18/2018 11:13:16 PM
 Author:	Kingo
*/

#include <Button.h>
#include <SoftwareSerial.h>
#include <SerialCommand.h>
#include <SSD1306AsciiWire.h>
#include <SSD1306Ascii.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Streaming.h>
#include <DS3232RTC.h>
#include <Time.h>
#include <TimeLib.h>


#define ONE_WIRE_BUS_1 2

/*  code to process time sync messages from the serial port   */
#define TIME_HEADER  "T"   // Header tag for serial time sync message

const int timeZone = 2;     // Central European Time
							//const int timeZone = -5;  // Eastern Standard Time (USA)
							//const int timeZone = -4;  // Eastern Daylight Time (USA)
							//const int timeZone = -8;  // Pacific Standard Time (USA)
							//const int timeZone = -7;  // Pacific Daylight Time (USA)



// PINS
#define bluetoothRX 11 // RX Pin
#define bluetoothTX 12 // TX Pin
#define ledPin1 9
#define ledPin2 10
#define Button1 8 


OneWire oneWire_in(ONE_WIRE_BUS_1);
DallasTemperature sensor_inhouse(&oneWire_in);
Button button = Button(Button1, BUTTON_PULLUP);


// Variables declaration

float tempril;
int alarmhightemp = 25;
int alarmlowtemp = 10;
int ledstat1 = LOW;
int ledstat2 = LOW;
int interval = 10000;
int blinkTime = 1000;
bool LED1 = false;
bool LED2 = false;
bool hightemp = false;
bool lowtemp = false;
bool blink = false;
String ipAddress = "0.0.0.0";
unsigned long lastmillis = 0;
unsigned long lastBlink = 0;
static time_t tLast;

time_t t;

// 0X3C+SA0 - 0x3C or 0x3D
#define I2C_ADDRESS 0x3C

// Define proper RST_PIN if required.
#define RST_PIN -1

SoftwareSerial BtSerial(bluetoothRX, bluetoothTX); // RX, TX

SerialCommand cmdTherm(BtSerial);

SSD1306AsciiWire display;


void setup()
{

	// Start up the library
	Serial.begin(115200);
	BtSerial.begin(9600);
	sensor_inhouse.begin();
		
	//Serial.println(year(RTC.get()));
	//if (year(RTC.get()) < 2015) Serial.println(F(__TIME__));
	//setSyncProvider() causes the Time library to synchronize with the
	//external RTC by calling RTC.get() every five minutes by default.
	setSyncProvider(RTC.get);
	//Serial << F("RTC Sync");
	//if (timeStatus() != timeSet) Serial << F(" FAIL!");
	//Serial << endl;
	adjustTime(2 * SECS_PER_HOUR);

#if RST_PIN >= 0
	display.begin(&Adafruit128x64, I2C_ADDRESS, RST_PIN);
#else // RST_PIN >= 0
	display.begin(&Adafruit128x64, I2C_ADDRESS);
#endif // RST_PIN >= 0
		
	Serial.println("AT+CIPMUX=1");
	while (!Serial.available());
	Serial.println("AT+CIPSERVER=1,80");
	while (!Serial.available());
	
	

	display.setFont(System5x7);
	display.clear();

	pinMode(ledPin1, OUTPUT);
	pinMode(ledPin2, OUTPUT);


	// Setup callbacks for SerialCommand commands
	cmdTherm.addCommand("<T", syncpctime);          // Syc time
	cmdTherm.addCommand("gtemp", gettemp);         // Get current Temperature
	cmdTherm.addDefaultHandler(unrecognized);      // Handler for command that isn't matched  (says "What?")
	Serial.println("Ready");
			

	readTemp();
	printTime();
	printDate();
	
}



void loop()
{
	
	printTime();
	
	if (button.uniquePress()) {
		
		LED1 = !LED1;
		LED2 = !LED2;
	}

	if ((millis() - lastmillis) >= interval)
	{

		readTemp();
		printDate();
		lastmillis = millis();
		int startIP;
		int endIP;
		Serial.println("AT+CIFSR");
		while (!Serial.available());
		ipAddress = Serial.readString();
		BtSerial.println(ipAddress);
		startIP = ipAddress.indexOf("STAIP");
		endIP = ipAddress.indexOf('"', startIP + 7);
		ipAddress = ipAddress.substring(startIP + 7, endIP);
		Serial.println("AT+CIPSEND=0,8");
		while (!Serial.available());
		Serial.println(tempril);
		while (!Serial.available());

		if (t != tLast) {
			tLast = t;
			BtSerial << "<T " << tempril;
			BtSerial << "\n";
		}

	}

	if ((millis() - lastBlink) >= blinkTime)
	{
		
		


		if (tempril < alarmlowtemp) lowtemp = true;
		else lowtemp = false;
		if (tempril > alarmhightemp) hightemp = true;
		else hightemp = false;
		if (LED1 && lowtemp && ledstat1 == LOW)
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

		lastBlink = millis();
		blink = !blink;

	}

	digitalWrite(ledPin1, ledstat1);
	digitalWrite(ledPin2, ledstat2);

	cmdTherm.readSerial();
	
}


// Functions --------------------------------
void printTime(){
	
	t = now();
	display.set2X();
	display.setCursor(15, 2); // bottom left
	if (hour(t)<10) display.print("0");
	display.print(hour(t));
	display.print(":");
	if (minute(t)<10) display.print("0");
	display.print(minute(t));
	display.print(":");
	if (second(t)<10) display.print("0");
	display.println(second(t));
	display.setCursor(0, 5);
	display.print("Temp : ");
	display.print(int(tempril));
	display.println((char)128);
	
	display.set1X();
	display.setCursor(0, 7);
	display.println("IP: " + ipAddress);

}

void printDate() {

	display.set1X();
	display.setCursor(10, 0); // upper left
	display.print(" ");
	display.print(dayShortStr(weekday(t)));
	display.print(" ");
	if (day(t)<10) display.print("0");
	display.print(day(t));
	display.print("/");
	display.print(monthShortStr(month(t)));
	display.print("/");
	display.print(year(t));
	
}

void readTemp() {

	sensor_inhouse.requestTemperatures();
	tempril = sensor_inhouse.getTempCByIndex(0);
	//tempril = RTC.temperature() / 4.0;
}




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
			t = t + ( timeZone * SECS_PER_HOUR);
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

