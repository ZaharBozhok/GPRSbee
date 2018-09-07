#include <HX711.h>
#include <sim800.h>
#include <gprs.h>
#include<OneWire.h>

#include"defines.h"
#include"RGBLed.h"

#define GPRS_BAUD   9600
#define SERIAL_BAUD 9600

OneWire ds(TEMPERATURE_SENSOR_PIN);
GPRS gprs(GPRS_BAUD);

RGBLed rgb(RGB_PIN_R, RGB_PIN_G, RGB_PIN_B);
HX711 weights(WEIGHTS_DT_PIN,WEIGHTS_SCK_PIN);
 
//it's number : +380962118922

void collectData(double &weight, double &temp, double &voltage);
float getTemperature();
void getScaler(void);

float getVoltage(const size_t &count = 20);
void setup()
{
	rgb.On(Blue);
	Serial.begin(SERIAL_BAUD);
	Serial.println("Strt");

	analogReference(EXTERNAL);
	pinMode(GPRS_RST, OUTPUT);
	//pinMode(GPRS_PWR, OUTPUT);
	pinMode(SLR_PWR,  OUTPUT);
	pinMode(INT_PIN_0, INPUT_PULLUP);
	pinMode(INT_PIN_1, INPUT_PULLUP);
	pinMode(SLR_PWR, OUTPUT);

	while (0 != gprs.sendATTest())
	{
		Serial.println("sim800 init error!");
	}
	Serial.println("sim800 init O.K!");
	Serial.println("Getting scaler...");
  getScaler();
}
void loop()
{
	double weight = 0, temp=0, voltage = 0;
	rgb.On(Green);
	collectData(weight, temp, voltage);
	if (weight < 0)
		weight *= -1;
	sendDataToServer(weight, temp, voltage);
	rgb.Off();
	for (int i = 0; i < 60; i++)
		for (int j = 0; j < 30; j++)
			delay(1000);
	
}


void collectData(double &weight, double &temp, double &voltage)
{
	 temp = getTemperature();
	 weight = weights.get_units(20);
	 voltage = getVoltage();
#ifdef DEBUG
	Serial.print("sensor value : ");
	Serial.println(temp);
	Serial.print("weights value : ");
	Serial.println(weight);
#endif
}

float getTemperature()
{
	byte i;
	byte present = 0;
	byte type_s;
	byte data[12];
	byte addr[8];
	float celsius, fahrenheit;

	if (!ds.search(addr)) {
#ifdef DEBUG
		Serial.println("No more addresses.");
		Serial.println();
#endif
		ds.reset_search();
		delay(250);
		return;
	}
#ifdef DEBUG
	Serial.print("ROM =");
	for (i = 0; i < 8; i++) {
		Serial.write(' ');
		Serial.print(addr[i], HEX);
	}
#endif

	if (OneWire::crc8(addr, 7) != addr[7]) {
#ifdef DEBUG
		Serial.println("CRC is not valid!");
#endif
		return;
	}
#ifdef DEBUG
	Serial.println();
#endif

	// the first ROM byte indicates which chip
	switch (addr[0]) {
	case 0x10:
#ifdef DEBUG
		Serial.println("  Chip = DS18S20");  // or old DS1820
#endif
		type_s = 1;
		break;
	case 0x28:
#ifdef DEBUG
		Serial.println("  Chip = DS18B20");
#endif
		type_s = 0;
		break;
	case 0x22:
#ifdef DEBUG
		Serial.println("  Chip = DS1822");
#endif
		type_s = 0;
		break;
	default:
#ifdef DEBUG
		Serial.println("Device is not a DS18x20 family device.");
#endif
		return;
	}

	ds.reset();
	ds.select(addr);
	ds.write(0x44, 1);        // start conversion, with parasite power on at the end

	delay(1000);     // maybe 750ms is enough, maybe not
					 // we might do a ds.depower() here, but the reset will take care of it.

	present = ds.reset();
	ds.select(addr);
	ds.write(0xBE);         // Read Scratchpad

#ifdef DEBUG
	Serial.print("  Data = ");
	Serial.print(present, HEX);
	Serial.print(" ");
#endif
	for (i = 0; i < 9; i++) {           // we need 9 bytes
		data[i] = ds.read();
#ifdef DEBUG
		Serial.print(data[i], HEX);
		Serial.print(" ");
#endif
	}
#ifdef DEBUG
	Serial.print(" CRC=");
	Serial.print(OneWire::crc8(data, 8), HEX);
	Serial.println();
#endif

	// Convert the data to actual temperature
	// because the result is a 16 bit signed integer, it should
	// be stored to an "int16_t" type, which is always 16 bits
	// even when compiled on a 32 bit processor.
	int16_t raw = (data[1] << 8) | data[0];
	if (type_s) {
		raw = raw << 3; // 9 bit resolution default
		if (data[7] == 0x10) {
			// "count remain" gives full 12 bit resolution
			raw = (raw & 0xFFF0) + 12 - data[6];
		}
	}
	else {
		byte cfg = (data[4] & 0x60);
		// at lower res, the low bits are undefined, so let's zero them
		if (cfg == 0x00) raw = raw & ~7;  // 9 bit resolution, 93.75 ms
		else if (cfg == 0x20) raw = raw & ~3; // 10 bit res, 187.5 ms
		else if (cfg == 0x40) raw = raw & ~1; // 11 bit res, 375 ms
											  //// default is 12 bit resolution, 750 ms conversion time
	}
	celsius = (float)raw  / 16.0;
#ifdef DEBUG
	Serial.print("  Temperature = ");
	Serial.print(celsius);
	Serial.print(" Celsius, ");
	Serial.print(fahrenheit);
	Serial.println(" Fahrenheit");
#endif
	return celsius;
}

void Debug()
{
#ifdef DEBUG
	Serial.println("Start debugging SIM800.");
#endif
	while (1)
	{
		if (Serial.available())
		{
			gprs.serialSIM800.write(Serial.readString().c_str());
			gprs.serialSIM800.write("\r\n");
			rgb.On(Red);
			delay(50);
			rgb.On(Green);
		}
		if (gprs.serialSIM800.available())
		{
			Serial.write(gprs.serialSIM800.read());
		}
	}
}


void getScaler(void)
{
	rgb.Blink(Red,30);
#ifdef DEBUG
	Serial.println("Weights taring...");
#endif // DEBUG
	weights.tare(50);
	double null_val = weights.get_value(50);
	rgb.Blink(Blue, 30);
#ifdef DEBUG
	Serial.print("null_val = ");
	Serial.println(null_val);
#endif // DEBUG
	double kg_val = weights.get_value(50);
#ifdef DEBUG
	Serial.print("kg_val = ");
	Serial.println(kg_val);
#endif // DEBUG
	weights.set_scale(kg_val - null_val);
#ifdef DEBUG
	Serial.println("Autotuning done.");
#endif // DEBUG
	rgb.On(Green);
}

void sendDataToServer(const size_t & w, const size_t & t,const double &v)
{
	unsigned long del = 100;
	gprs.sendCmdAndWaitForResp("AT+SAPBR=3,1,Contype,GPRS\r\n","OK",del);
	gprs.sendCmdAndWaitForResp("AT+SAPBR=3,1,APN," APN "\r\n","OK",del);
	gprs.sendCmdAndWaitForResp("AT+SAPBR=1,1\r\n", "OK", del);
	gprs.sendCmdAndWaitForResp("AT+HTTPINIT\r\n", "OK", del);
	gprs.sendCmdAndWaitForResp("AT+HTTPPARA=CID,1\r\n", "OK", del);
	gprs.sendCmdAndWaitForResp("AT+HTTPPARA=URL," URL "\r\n", "OK", del);
	gprs.sendCmdAndWaitForResp("AT+HTTPPARA=CONTENT,application/x-www-form-urlencoded\r\n", "OK", del);

	gprs.sendCmdAndWaitForResp("AT+HTTPDATA=320,10000\r\n","DOWNLOAD",del);
	{
		gprs.sendCmdAndWaitForResp(("key=123&weight="+String(w)+
			"&temperature="+String(t)+
			"&voltage=" + String(v)+
			"\r\n").c_str(), "OK", 10000);
	}
	//key=123&weight=100&temperature=20&voltage=5.0
	gprs.sendCmdAndWaitForResp("AT+HTTPACTION=1\r\n","+HTTPACTION:",5000);
	gprs.serialSIM800.write("AT+HTTPTERM\r\n");

#ifdef DEBUG
	Serial.print("Sending data to ");
	Serial.print(URL);
	Serial.println(" done.");
#endif // DEBUG

}

bool getBalance(String &message)
{
#ifdef DEBUG
	Serial.println("Get balance");
#endif // DEBUG
	if (gprs.sendCmdAndWaitForResp("AT+CUSD=1,\"*111#\"\r\n", "OK", DEFAULT_TIMEOUT*3) == 0)
	{
#ifdef DEBUG
		Serial.println("AT+CUSD=1,\"*111#\"");
#endif // DEBUG

		if (gprs.waitForResp("+CUSD:", DEFAULT_TIMEOUT) == 0)
		{
			while (gprs.serialSIM800.read() != '\"');
			message = gprs.serialSIM800.readStringUntil('\n');
#ifdef DEBUG
			Serial.println(message);
#endif // DEBUG
			return 1;
		}
		else return 0;
	}
	else return 0;
}

float getVoltage(const size_t &count = 20)
{
	float ret = 0;
	for (size_t i = 0; i<count; ++i)
  {
    int clear_analog = analogRead(PWR_PIN);
		float tmp_voltage = ((clear_analog*1.0)*AREF)/1023;
    ret+=tmp_voltage*2;
  }
	return ret / count;
}

//+380503213610 - Zahar
//+380962135865 - Artem kiyv
//+380935341486 - Artem life
