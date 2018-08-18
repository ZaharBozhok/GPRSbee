#define RGB_INVERSE_LOGIC
#ifndef __RGBLed__
#define __RGBLed__

#define DefGreenPin    3
#define DefBluePin    2
#define DefRedPin   4

#define OFF 0
#define ON	1

#ifdef RGB_INVERSE_LOGIC
#define OFF 1
#define ON	0

#define rgb_delay 66
#endif

enum RGBColor {
	White = 0b111,
	Red = 0b100,
	Green = 0b010,
	Blue = 0b001,
	Cyan = 0b011,
	Magenta = 0b101,
	Yellow = 0b110
};

class RGBLed {
public:
	RGBLed(const short &rp = DefRedPin, const short &gp = DefGreenPin, const short &bp = DefBluePin) :
		red_pin(rp), green_pin(gp), blue_pin(bp)
	{
		pinMode(this->red_pin, OUTPUT);
		pinMode(this->green_pin, OUTPUT);
		pinMode(this->blue_pin, OUTPUT);
	}
	void On(const RGBColor & color)
	{
		this->mask_to_color(color);
	}
	void Off()
	{
		this->mask_to_color(0);
	}
	void Blink(const RGBColor & color, const size_t & count)
	{
		for (int i = 0; i < count; i++)
		{
			this->Off();
			delay(rgb_delay);
			this->On(color);
			delay(rgb_delay);
		}
	}
private:
	short red_pin;
	short green_pin;
	short blue_pin;

	bool red_value;
	bool green_value;
	bool blue_value;
	void on(const bool &r, const bool &g, const bool &b)
	{
		this->red_value = r;
		this->green_value = g;
		this->blue_value = b;
		digitalWrite(this->red_pin, this->red_value);
		digitalWrite(this->green_pin, this->green_value);
		digitalWrite(this->blue_pin, this->blue_value);
	}
	void mask_to_color(const int & mask)
	{
		on(((mask >> 2) & 1) ^ OFF,
			((mask >> 1) & 1) ^ OFF,
			((mask >> 0) & 1) ^ OFF);
	}
};

#endif