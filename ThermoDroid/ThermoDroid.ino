/*
 Name:		ThermoDroid.ino
 Created:	7/18/2018 11:13:16 PM
 Author:	Kingo
*/

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <SerialCommand.h>
#include <OneWire.h>
#include <DallasTemperature.h>
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

String incomingstring;
String par1;
String par2;
String cmd;
String days[7] = { "Sun","Mon","Tue","Wen","Thu","Fri","Sat" };
String months[12] = { "Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec" };
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

SoftwareSerial BtSerial(bluetoothRX, bluetoothTX); // RX, TX

SerialCommand cmdTherm(BtSerial);

// OLED display TWI address
#define OLED_ADDR   0x3C

// reset pin not used on 4-pin OLED module
Adafruit_SSD1306 display(-1);  // -1 = no reset pin

// 128 x 64 pixel display
#if (SSD1306_LCDHEIGHT != 64)
#error("Height incorrect, please fix Adafruit_SSD1306.h!");
#endif

Button button = Button(Button1, BUTTON_PULLUP);

void onPress(Button& b) {
	clockmenu = !clockmenu;
	// display.clearDisplay();
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

	// initialize and clear display
	display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR);
	display.clearDisplay();
	display.display();
	display.setTextSize(1);
	display.setTextColor(WHITE);

	pinMode(ledPin1, OUTPUT);
	pinMode(ledPin2, OUTPUT);

	
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

	display.setCursor(0, 1); // bottom left
	display.print(hour(t));
	display.print(":");
	display.print(minute(t));
	display.print(":");
	display.print(second(t));
	display.setCursor(9, 1);
	display.print(months[month(t) - 1]);
	display.print(" ");
	String anno = String(year(t));
	display.print(anno.substring(4, 2));
	display.display();


	if ((millis() - clientlastmillis) >= bttimeout && client == true)
	{
		client = false;
		Serial.println("Client connection timeout...");
	}
	if ((millis() - lastmillis) >= interval)
	{
		sensor_inhouse.requestTemperatures();
		tempril = sensor_inhouse.getTempCByIndex(0);
		display.clearDisplay();
		display.setCursor(0, 0); // upper left
		display.print("Temp:");
		display.print(int(tempril));
		display.write((byte)0);
		display.setCursor(9, 0);
		display.print(days[weekday(t) - 1]);
		display.print(" ");
		display.print(day(t));
		display.display();



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

void decodecommand(String command, String param1, String param2)

// Decode and execute command

{

	int cmdindex;
	int totalcmd = 3;
	String cmdarray[] = { "gtemp:","tmplw:","tmphg:","cmdhl:" }; // elenco comandi
	String cmdhelp[] = { " Return temperature"," Show or set low temperature alarm"," Show or set high temperature alarm", " Show this help" };  // Help comandi
	cmd = "none";
	for (cmdindex = 0; cmdindex <= totalcmd; cmdindex++)
	{
		if (command == cmdarray[cmdindex]) break;
		else if (cmdindex == totalcmd) return;
	}
	switch (cmdindex)
	{
	case 0:
	{
		switch (interface)
		{
		case 0:
			Serial.print("Temperatura Rilevata:");
			Serial.print(tempril);
			Serial.println(" C");
			break;
		case 1:
			BtSerial.print("Temperatura Rilevata:");
			BtSerial.print(tempril);
			BtSerial.println(" C");
			break;
		}
	}
	break;
	case 1:

		if (param1.toInt() > 0)
		{
			LED1 = true;
			alarmlowtemp = param1.toInt();
			BtSerial.print("Allarme temperatura minima:");
			BtSerial.print(alarmlowtemp);
			BtSerial.println(" C");
		}
		else if (param1 != "")
		{
			LED1 = false;
			BtSerial.println("Allarme temperatura minima disattivato");
		}
		else
		{
			BtSerial.print("Allarme temperatura minima:");
			BtSerial.print(alarmlowtemp);
			BtSerial.print(" C , Stato: ");
			if (LED1) BtSerial.println("ATTIVATO");
			else BtSerial.println("Disattivato");
		}
		break;
	case 2:
		if (param1.toInt() > 0)
		{
			LED2 = true;
			alarmhightemp = param1.toInt();
			BtSerial.print("Allarme temperatura massima:");
			BtSerial.print(alarmhightemp);
			BtSerial.println(" C");
		}
		else if (param1 != "")
		{
			LED2 = false;
			BtSerial.println("Allarme temperatura massima disattivato");
		}
		else
		{
			BtSerial.print("Allarme temperatura massima:");
			BtSerial.print(alarmhightemp);
			BtSerial.print(" C , Stato: ");
			if (LED2) BtSerial.println("ATTIVATO");
			else BtSerial.println("Disattivato");
		}
		break;
	case 3:
		BtSerial.println("Elenco comandi disponibili:");
		for (cmdindex = 0; cmdindex <= totalcmd; cmdindex++)
		{
			BtSerial.print(cmdarray[cmdindex]);
			BtSerial.println(cmdhelp[cmdindex]);
		}
		break;

	}
}




