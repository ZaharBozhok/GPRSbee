#include <HX711.h>
#include <sim800.h>
#include <gprs.h>
#include<OneWire.h>

#include"defines.h"
#include"RGBLed.h"

#define GPRS_BAUD   9600
#define SERIAL_BAUD 9600

//extern uint32_t END_OF_LINK_ROM;

OneWire ds(TEMPERATURE_SENSOR_PIN);
GPRS gprs(GPRS_BAUD);

RGBLed rgb(RGB_PIN_R, RGB_PIN_G, RGB_PIN_B);
HX711 weights(WEIGHTS_DT_PIN,WEIGHTS_SCK_PIN);
 
//it's number : +380962118922
#undef DEBUG

const size_t nums_size = MAX_NUMS_SIZE; 
String ownerNumber; 
String Numbers[nums_size];
size_t timer;
char timer_ch;
bool timerActivator;
size_t alrm0;
size_t alrm1;
bool alarmActivator;
bool configuringMode;
long long seconds_from_start;
size_t confCounter;

void gprsOn(void);
void gprsOff(void);
void gprsReset(void);
void collectData(double &weight, double &temp, double &voltage);
float getTemperature();
size_t callArray(const String *arr, const size_t &size);
void sendArray(const String *numbers, const String message, const size_t &size);
void Debug();
bool parseSMS(const size_t &smsNumber, char *sms, char *num);
void checkSMS(const size_t &count);
void getScaler(void);
size_t findNumber(const String &num);
size_t addNumber(const String &num);
size_t rmNumber(const String &num);
void Alarm(void);
void alarmInitializer(void);
bool startsWith(const char *pre, const char *str);
void sendDataToServer(const size_t &w, const size_t & t, const double &v);
void sendDataToOwner(const double & w, const double & t, const double &v);
bool getBalance(String &message);
float getVoltage(const size_t &count = 20);

#ifdef DEBUG
void freeRam() {
	extern int __heap_start, *__brkval;
	int v;
	Serial.print("free ram:");
	Serial.println( (int)&v - (__brkval == 0 ? (int)&__heap_start : (int)__brkval));
	//Serial.print(',');
	//Serial.println(END_OF_LINK_ROM);
}
void logging()
{
    Serial.print("voltage=");
    Serial.print(getVoltage());
    Serial.println(";");
    Serial.print("time=");
    Serial.print((long)seconds_from_start);
    Serial.println(";");
   // freeRam();
  }
#endif
void setup()
{
	rgb.On(Blue);
#ifdef DEBUG
	Serial.begin(SERIAL_BAUD);
	Serial.println("Strt");
#endif
 
 ownerNumber =            OWNER_NUMBER;
 Numbers[nums_size] =   { OWNER_NUMBER };
 timer =                  START_TIMER_VALUE;
 timer_ch =                 START_TIMER_CHAR;
 timerActivator =           FALSE;
 alrm0 =                  DEF_ALRM_STATE;
 alrm1 =                  DEF_ALRM_STATE;
 alarmActivator =           FALSE;
 configuringMode =          START_CONFIGURING_MODE;
  seconds_from_start =    0;
 confCounter  =           0;

  analogReference(EXTERNAL);
	pinMode(GPRS_RST, OUTPUT);
	//pinMode(GPRS_PWR, OUTPUT);
	pinMode(SLR_PWR,  OUTPUT);
	pinMode(INT_PIN_0, INPUT_PULLUP);
	pinMode(INT_PIN_1, INPUT_PULLUP);
	pinMode(SLR_PWR, OUTPUT);
  
	digitalWrite(SLR_PWR, LOW);

 digitalWrite(GPRS_RST, HIGH);
 delay(1000);
 digitalWrite(GPRS_RST, LOW);
 delay(4000);
 while(0 != gprs.sendCmdAndWaitForResp("AT\r\n", "AT", DEFAULT_TIMEOUT))
 {
  Serial.println("my Aterror");
  }
  //gprs.sendCmdAndWaitForResp("AT+CSCLK=2\r\n", "AT+CSCLK=2", DEFAULT_TIMEOUT);



#ifdef DEBUG
	Serial.println( "sim strtd");
#endif
	if (DEF_ALRM_STATE == 1) {
		attachInterrupt(digitalPinToInterrupt(INT_PIN_0), alarmInitializer, FRONT_0);
		attachInterrupt(digitalPinToInterrupt(INT_PIN_1), alarmInitializer, FRONT_1);
#ifdef DEBUG
		Serial.println("Intrrpts attchd");
#endif
	}
	TCCR1A = 0;
	TCCR1B = 0;
	TCNT1 = 3036;
	TCCR1B |= (1 << CS12);
	TIMSK1 |= (1 << TOIE1);

	interrupts();
	digitalWrite(SLR_PWR, HIGH);
	rgb.Off();
	delay(100);

  getScaler();
  seconds_from_start =    0;
  #ifdef DEBUG
    logging();
  #endif
}
void loop()
{
  if(confCounter>CONF_AWAIT_TIME_TICK)
  {
configuringMode = FALSE;
confCounter = 0;
    }
	if (configuringMode)
	{
		checkSMS(1);
    ++confCounter;
#ifdef DEBUG
    Serial.println("CONF");
		rgb.On(Magenta);
    logging();
#endif // DEBUG
	}
	else
		if (alarmActivator == 1)
		{
#ifdef DEBUG
			Serial.println("ALARM");
			rgb.On(Red);
      logging();
#endif // DEBUG
			gprsReset();
			Alarm();
			alarmActivator = 0;
			configuringMode = 1;
		}
	else
	if (timerActivator)
	{
#ifdef DEBUG
    Serial.println("TIMER");
		rgb.On(White);
    logging();
#endif // DEBUG
		gprsReset();

		double t = 0, w = 0, v = 0;
		collectData(w, t, v);
		//sendDataToOwner(w, t,v); // sending SMS
		//sendArray(Numbers, ("temperature : " + String(t) + "\nweigth : " + String(w)).c_str(), nums_size);
		sendDataToServer(w,t,v); // sending to server
		checkSMS(MAX_SMS_COMMANDS);
		timerActivator = 0;
		#ifdef DEBUG
      Serial.println("TIMER_END");
    #endif
	}
	else if (configuringMode == 0)
	{
    rgb.Off();
    confCounter = 0;
		gprsOff();
	}
}


void gprsOn(void)
{
  digitalWrite(GPRS_RST,LOW);
  delay(750);
}
  void gprsOff(void)
{
  digitalWrite(GPRS_RST,HIGH);
}
void gprsReset(void)
{
  gprsOff();
  delay(1000);
  gprsOn();
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

size_t callArray(const String *numbers, const size_t &size)
{
	size_t reacted = 0;
#ifdef DEBUG
	Serial.println("Array calling started.");
#endif
	for (size_t i = 0; i < size; ++i)
	{
		if (numbers[i].length() < 1)
			continue;
		gprs.callUp(const_cast<char*>(numbers[i].c_str()));
#ifdef DEBUG
   Serial.print("Calling to ");
    Serial.println(numbers[i].c_str());
#endif
		if (gprs.waitForResp("+COLP:", 30000) == 0)
		{
			++reacted;
#ifdef DEBUG
			Serial.println("ANSWERED");
#endif
		}
		gprs.sendCmd("ATH\r\n");
#ifdef DEBUG
		Serial.println("SIM HANGED");
#endif
	}
	return reacted;
}


void sendArray(const String *numbers, const String message, const size_t &size)
{
#ifdef DEBUG
	Serial.println("Send array started.");
#endif
	for (size_t i = 0; i < size; ++i)
	{
		if (numbers[i].length() < 1)
			continue;
#ifdef DEBUG
		Serial.print("Sending sms ");
		Serial.print("\"");
		Serial.print(message);
		Serial.print("\" to ");
		Serial.println(numbers[i]);
#endif // DEBUG
		gprs.sendSMS(const_cast<char*>(numbers[i].c_str()), const_cast<char*>((message).c_str()));
		delay(1000);
#ifdef DEBUG
		Serial.println("Sent (maybe...)");
#endif // DEBUG
	}
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
			//rgb.On(Red);
			//delay(50);
			//rgb.On(Green);
		}
		if (gprs.serialSIM800.available())
		{
			Serial.write(gprs.serialSIM800.read());
		}
	}
}

bool parseSMS(const size_t &smsNumber, char *sms, char *num)
{
	if (gprs.sendCmdAndWaitForResp((("AT+CMGR=" + String(smsNumber) + "\r\n").c_str()), "+CMGR:", 5) == 0)
	{
		while (gprs.serialSIM800.read() != '\"');
		while (gprs.serialSIM800.read() != '\"');
		while (gprs.serialSIM800.read() != '\"');
		gprs.serialSIM800.readBytesUntil('\"', &num[0], NUMBER_SIZE);
		num[NUMBER_SIZE] = 0;

		//parsing sms
		while (gprs.serialSIM800.read() != '\n');
		gprs.serialSIM800.readBytesUntil('\n', sms, SMS_SIZE);
		sms[SMS_SIZE] = 0;
		return 1;
	}
	return 0; 
}

void checkSMS(const size_t &count = 1)
{
#ifdef DEBUG
	Serial.println("Sms checking started.");
#endif // DEBUG
	char * buff = nullptr;
	char * sms = new char[SMS_SIZE + 1];
	bool is_sms = 0;
	bool is_owner_number = 0;
	for (size_t i = 1; i <= count; ++i)
	{
		buff = new char[NUMBER_SIZE + 1];
#ifdef DEBUG
		Serial.print(" Reading sms #");
		Serial.println(i);
		Serial.println(ownerNumber);
#endif // DEBUG
		is_sms = parseSMS(i, sms, buff);
#ifdef DEBUG
		Serial.print("sms : ");
		Serial.println(sms);
		Serial.print("num : ");
		Serial.println(buff);
		Serial.print("Is sms : ");
		Serial.println(is_sms);
#endif // DEBUG
		is_owner_number = (strcmp(ownerNumber.c_str(), buff) == 0);
		delete[]buff;
		if (is_owner_number)
		{
			//Serial.print("num : ");/////////////////////////
			//Serial.println(buff);////////////////////
			//Serial.print("sms : ");/////////////////////////
			//Serial.println(sms);////////////////////
#ifdef DEBUG
			Serial.println("Owner!");
#endif // DEBUG
			String message = "";
			if (startsWith("?",sms))
			{
				message = "/conf\n/alarm[0-1]=[0-1]\n/timer=[1-99][h,m,d]\n/addnum=+(12)[0-9]\n/rmnum=+(12)[0-9]\n/balance\n/tare";
			}
			else if (startsWith("!",sms))
			{
				configuringMode = 0;
#ifdef DEBUG
				Serial.println("Configuring mode disabled");
#endif
				message = "Configuring mode disabled.";
			}
			else if (startsWith("/conf",sms ))///////////////////
			{
				configuringMode = 1;
#ifdef DEBUG
				Serial.println("Configuring mode enabled");
#endif
			}
			else if (startsWith("/alarm", sms))
			{
				if (sms[7] == '=' && (sms[8] == '1' || sms[8] == '0'))
				{
					size_t val = sms[8] - 48;
					if (sms[6] == '0')
					{
						alrm0 = val;
						if (val == 0)
							detachInterrupt(digitalPinToInterrupt(INT_PIN_0));
						else if (val == 1)
							attachInterrupt(digitalPinToInterrupt(INT_PIN_0), alarmInitializer, FRONT_0);
					}
					else if (sms[6] == '1')
					{
						alrm1 = val;
						if (val == 0)
							detachInterrupt(digitalPinToInterrupt(INT_PIN_1));
						else if (val == 1)
							attachInterrupt(digitalPinToInterrupt(INT_PIN_1), alarmInitializer, FRONT_1);
					}
					else message = "Index error : " + sms[6];
#ifdef DEBUG
					Serial.print("Alarm0 value = ");
					Serial.println(alrm0);
					Serial.print("Alarm1 value = ");
					Serial.println(alrm1);
#endif 
				}
				else { message = "Syntax error"; }
			}
			else if (startsWith("/timer=", sms))
			{
#ifdef DEBUG 
				Serial.println("Entered to timer settings");
#endif 
				char tim = 0;
				size_t size = 0;
				size_t i = 7;
#ifdef DEBUG 
				Serial.println("Loop started");
#endif 
				while (tim != 'm' && tim != 'h' && tim != 'd' && tim != 's' && size < 8)
				{
					tim = sms[i++];
					size++;
#ifdef DEBUG 
					Serial.print("tim = ");
					Serial.println(tim);
					Serial.print("size = ");
					Serial.println(size);
#endif 
				}
#ifdef DEBUG 
				Serial.println("Loop ended");
				Serial.print("Timer delimetr : ");
				Serial.println(tim);
#endif 
				buff = new char[size + 1];
				for (i = 0; i < size; i++)
					buff[i] = sms[i + 7];
				buff[size] = 0;
#ifdef DEBUG 
				Serial.println(buff);
#endif				
				size_t &val = size;
				val = atoi(buff);
				delete[] buff;
#ifdef DEBUG 
				Serial.print("Timer value : ");
				Serial.println(val);
#endif 
				if (val > 99)
					timer = 99;
				else timer = val;
				if (tim == 'm' || tim == 'h' || tim == 'd' || tim == 's')
					timer_ch = tim;
				else
					message = "Wrong format";
#ifdef DEBUG 
				Serial.print("New timer value : ");
				Serial.println(timer);
#endif 
			}
			/*New code*/
			else if (startsWith("/addnum=", sms))
			{
				//sms[21] = 0;
				String number(&sms[8]);
#ifdef DEBUG
				Serial.print("Adding number : ");
				Serial.println(number);
#endif // DEBUG
				size_t resp = addNumber(number);
				if (resp == -1)
					message = "All numbers filled out";
			}
			else if (startsWith("/rmnum=", sms))
			{
				String number(&sms[7]);
#ifdef DEBUG
				Serial.print("Removing number : ");
				Serial.println(number);
#endif // DEBUG
				size_t resp = rmNumber(number);
				if (resp == -1)
					message = "Number removing error";
			}
			else if (startsWith("/balance", sms))
			{
				if(!getBalance(message))
					message = "Can't get balance";
			}
			else if (startsWith("/tare", sms))
			{
				getScaler();
				message = "Tared!";
			}
			/*New code*/
			else { message = "Unknonw command"; }

#ifdef DEBUG 
			if (message.length() > 1)
			{
				Serial.print("Message : ");
				Serial.println(message);
			}
#endif 

			if (message.length() > 0)
     {
				gprs.sendSMS(const_cast<char*>(ownerNumber.c_str()), const_cast<char*>(message.c_str()));
     }
			else
			{
				if (0 == gprs.sendCmdAndWaitForResp("AT+CMGF=1\r\n", "OK", DEFAULT_TIMEOUT)) { // Set message mode to ASCII
					delay(500);
					if (0 == gprs.sendCmdAndWaitForResp(("AT+CMGS=\"" + ownerNumber + "\"\r\n").c_str(), ">", DEFAULT_TIMEOUT)) {
						delay(1000);
						gprs.serialSIM800.write("/alarm0=");
						gprs.serialSIM800.write((char)(alrm0+'0'));
						gprs.serialSIM800.write("\n/alarm1=");
						gprs.serialSIM800.write((char)(alrm1 + '0'));
						gprs.serialSIM800.write("\n/timer=");
						gprs.serialSIM800.print(timer);
						gprs.serialSIM800.print(timer_ch);
						gprs.serialSIM800.write("\nnumbers:\n");
						for (size_t i = 0; i < nums_size; ++i)
						{
							if (Numbers[i].length() == 0)
								continue;
							gprs.serialSIM800.write(Numbers[i].c_str());
							gprs.serialSIM800.write('\n');
						}
						delay(500);
						gprs.sendEndMark();
					}
					else { ERROR("myERROR:CMGS"); }
				}
				else { ERROR("myERROR:CMGF"); };
			}
			gprs.waitForResp("+CMGS:", DEFAULT_TIMEOUT*3);
		}
		else
		{
#ifdef DEBUG
			Serial.println("Not owner's number.");
#endif // DEBUG
		}

		gprs.cleanBuffer(sms, SMS_SIZE);

		if (is_sms)
		{
			gprs.sendCmdAndWaitForResp("AT+CMGD=1,4\r\n", "OK", DEFAULT_TIMEOUT);
#ifdef DEBUG 
			Serial.println("Deleted all sms");
#endif 
		}
		
	}
	delete[]sms;
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

size_t findNumber(const String &num)
{
#ifdef DEBUG
	Serial.print("Searching for ");
	Serial.println(num);
#endif // DEBUG

	for (size_t i = 0; i < nums_size; i++)
		if (Numbers[i] == num)
			return i;
#ifdef DEBUG
	Serial.print(num);
	Serial.println(" not found");
#endif // DEBUG
	return -1;
}

size_t addNumber(const String &num)
{
#ifdef DEBUG
	Serial.println("Searching for empty place");
#endif // DEBUG
	size_t pos = findNumber("");
	if (pos != -1)
	{
		Numbers[pos] = num;
#ifdef DEBUG
		Serial.print(num);
		Serial.print(" added at place ");
		Serial.println(pos);
#endif // DEBUG
	}
	return pos;
}

size_t rmNumber(const String &num)
{
#ifdef DEBUG
	Serial.print("Searching for (rm) ");
	Serial.println(num);
#endif // DEBUG
	size_t pos = findNumber(num);
	if (pos != -1)
	{
		Numbers[pos] = "";
#ifdef DEBUG
		Serial.print(num);
		Serial.print(" removed at place ");
		Serial.println(pos);
#endif // DEBUG
	}
	return pos;
}

void Alarm(void)
{
#ifdef DEBUG
	Serial.println("ALARM!");
#endif
	 sendArray(Numbers, "ALARM!", nums_size);
   size_t answer = 0;
   for(size_t i=0; i<6; i++)
   {
    if(callArray(Numbers, nums_size))
    {
      Serial.println("Answered!");
      answer=1;
      break;
    }
   }
   if(answer == 0)
   {
    Serial.println("Nobody answered");
    gprsReset();
    }
}


void alarmInitializer(void)
{
#ifdef DEBUG
	Serial.println("alarm interrupt");
#endif // DEBUG
		
	alarmActivator = 1;
}

bool startsWith(const char *pre, const char *str)
{
	size_t lenpre = strlen(pre),
		lenstr = strlen(str);
	return lenstr < lenpre ? false : strncmp(pre, str, lenpre) == 0;
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

void sendDataToOwner(const double & w, const double & t, const double & v)
{
	gprs.sendSMS(const_cast<char*>(ownerNumber.c_str()), const_cast<char*>(
		("temperature : " + String(t) +
			"\nweigth : " +String(w) +
		"\nvoltage : " + String(v)).c_str()
		));
#ifdef DEBUG
	Serial.println("Sending data to owner done.");
#endif // DEBUG
}

ISR(TIMER1_OVF_vect)
{
	TCNT1 = 3036;
	seconds_from_start++;
	long long val = timer;
	switch (timer_ch)
	{
	case 's':
		break;
	case 'm':
		val *=  60;
		break;
	case 'h':
		val *=  60 * 60;
		break;
	case 'd':
		val *=  60 * 60 * 24;
		break;
	}
	if (seconds_from_start % val == 0)
	{		
		timerActivator = 1;
	}
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
