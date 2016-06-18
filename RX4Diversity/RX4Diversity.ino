#include "U8glib.h"			//display driver and graphics
#include "FastLED.h"		//WS2812/SK6812 driver
#include <avr/pgmspace.h>	//pull values out of flash (LUTs etc)
#include <EEPROM.h>			//settings storage

// Settings and such
#define VERSION_NUMBER 1.0
#define RX_TYPE 1			//0 = RX5808, 1 = RX158
#define MODULE_COUNT 4		//number of RX modules

// Pinout mapping
#define VOLTAGE_SENSE A6	//ADC6, 19
#define BUZZER 3			//PD3, 01
#define BTN_1 2				//PD2, 32
#define BTN_2 4				//PD4, 02

#define WS2812_DATA  5		//PB5, 17
#define WS2812_COUNT 4		//led count might be different from rx count

#define SPI_DIO 10			//PB2, 14
#define SPI_CS  11			//PB3, 15
#define SPI_CLK 12			//PB4, 1
// #define UART_RX 0 //PD0, 12
// #define UART_TX 1 //PD1, 31
// #define I2C_SDA A4  //PC4, 27
// #define I2C_SCL A5  //PC5, 28

#define BATTERY_MAX 4.2
#define BATTERY_MIN 3.4
#define BATTERY_WARN 3.7

const int RX_RSSI_PIN[MODULE_COUNT] = {A2, A0, A3, A1};
const int RX_SEL_PIN[MODULE_COUNT] = {6, 7, 8, 9};
// #define RSSI1 A2  //PC2, 25
// #define RSSI2 A0  //PC0, 23
// #define RSSI3 A3  //PC3, 26
// #define RSSI4 A1  //PC1, 24
//#define RXSEL1  6 //PD6, 10
//#define RXSEL2  7 //PD7, 11
//#define RXSEL3  8 //PB0, 12
//#define RXSEL4  9 //PB1, 13


// Driver Object Definitions
CRGB leds[WS2812_COUNT];
U8GLIB_SSD1306_128X32 u8g(U8G_I2C_OPT_NONE);	// I2C / TWI 

// Defines
int button1, button2 = 0;
float input_voltage = 0;
int cell_count, batt_percentage = 0;

volatile int rx_rssi[MODULE_COUNT] = {0, 0, 0, 0};
int active_module = 0;
int active_freq = 0;
int active_band = 0;

int menu_State = 0;
int state_timeout = 0;

// Channels to sent to the SPI registers
const uint16_t channelTable[] PROGMEM = {
  // Channel 1 - 8
  0x2A05,    0x299B,    0x2991,    0x2987,    0x291D,    0x2913,    0x2909,    0x289F,    // Band A
  0x2903,    0x290C,    0x2916,    0x291F,    0x2989,    0x2992,    0x299C,    0x2A05,    // Band B
  0x2895,    0x288B,    0x2881,    0x2817,    0x2A0F,    0x2A19,    0x2A83,    0x2A8D,    // Band E
  0x2906,    0x2910,    0x291A,    0x2984,    0x298E,    0x2998,    0x2A02,    0x2A0C,    // Band F / Airwave
  0x281D,    0x288F,    0x2902,    0x2914,    0x2978,    0x2999,    0x2A0C,    0x2A1E     // Band C / Immersion Raceband
};

// Channels with their Mhz Values
const uint16_t channelFreqTable[] PROGMEM = {
  // Channel 1 - 8
  5865, 5845, 5825, 5805, 5785, 5765, 5745, 5725, // Band A
  5733, 5752, 5771, 5790, 5809, 5828, 5847, 5866, // Band B
  5705, 5685, 5665, 5645, 5885, 5905, 5925, 5945, // Band E
  5740, 5760, 5780, 5800, 5820, 5840, 5860, 5880, // Band F / Airwave
  5658, 5695, 5732, 5769, 5806, 5843, 5880, 5917  // Band C / Immersion Raceband
};

const char* bandNameTable[] = {
  "Boscam A", // Band A
  "Boscam B", // Band B
  "DJI / E", // Band E
  "Immersion", // Band F / Airwave
  "RaceBand", // Band C / Immersion Raceband
};

// All Channels of the above List ordered by Mhz
const uint8_t channelList[] PROGMEM = {
  19, 18, 32, 17, 33, 16, 7, 34, 8, 24, 6, 9, 25, 5, 35, 10, 26, 4, 11, 27, 3, 36, 12, 28, 2, 13, 29, 37, 1, 14, 30, 0, 15, 31, 38, 20, 21, 39, 22, 23
};


void setup_pins()
{
	sm_setup_pinMode();
	rx_setup_pinMode();

	pinMode(SPI_CLK, OUTPUT);
	pinMode(SPI_CS, OUTPUT);
	pinMode(SPI_DIO, OUTPUT);

	pinMode(BTN_1, INPUT);
	pinMode(BTN_2, INPUT);
	pinMode(VOLTAGE_SENSE, INPUT);
	pinMode(BUZZER, OUTPUT);
	pinMode(WS2812_DATA, OUTPUT);
}

void setup_peripherals()
{
	//i2c display init
	// u8g.setRot180();	//flip if needed

	// assign default color value
	if ( u8g.getMode() == U8G_MODE_R3G3B2 ) {
		u8g.setColorIndex(255);	// white
	}
	else if ( u8g.getMode() == U8G_MODE_GRAY2BIT ) {
		u8g.setColorIndex(3);	// max intensity
	}
	else if ( u8g.getMode() == U8G_MODE_BW ) {
		u8g.setColorIndex(1);	// pixel on
	}
	else if ( u8g.getMode() == U8G_MODE_HICOLOR ) {
		u8g.setHiColorByRGB(255,255,255);
	}

	//WS2812/SK6812 LED Setup
	// FastLED.addLeds<WS2812B, WS2812_DATA, GRB>(leds, WS2812_COUNT);
	// FastLED.setBrightness(0);
	// FastLED.show();
	//rx module init


}

void setup() 
{
	Serial.begin(115200);
	Serial.println("setup: entry");

	setup_pins();
	setup_peripherals();
	alert_startup();
	sm_set_active(1);

}

void loop()
{
	button1 = digitalRead(BTN_1);
	button2 = digitalRead(BTN_2);

	rx_sample_rssi();

	if(button1 == 0)
	{
		if(active_module >= MODULE_COUNT)
		{
			sm_set_active(1);
		} 
		else
		{
			sm_set_active(active_module+1);
		}
		delay(100);
	}

	if(button2 == 0)
	{
		Serial.print("Bt1:"); Serial.print(button1); Serial.print(" Bt2:");Serial.println(button2);
		rx_set_freq(active_freq+1);

		if(active_freq > 7)
		{
			rx_set_band(active_band+1);

			if(active_band > 4)
			{
				active_band = 0;
			}

			active_freq = 0;
		}

		delay(100);
		Serial.print("Band:"); Serial.print(active_band); Serial.print(" chSet:");Serial.println(active_freq);
		rx_push_data(rx_calculate_channel());
	}



	measure_battery();


	// picture loop
	u8g.firstPage();  
	do {
		draw();
	} while( u8g.nextPage() );
	
	// FastLED.setBrightness(0);
	// FastLED.show();
}

void test_cycle()
{
	if(active_module < 4)
	{
		sm_set_active(active_module+1);
	}
	else
	{
		sm_set_active(1);
	}

	for(int i = 0; i < 40; i++)
	{
		rx_push_data(i);
		delay(500);
		rx_sample_rssi();
		Serial.print("Ch"); Serial.print(i);  

		for(int j = 0; j < MODULE_COUNT; j++)
		{
			Serial.print(", "); Serial.print(rx_rssi[j]);	
		}
		Serial.println("");
	}

	Serial.println("\n\n");
}
//------- Display Handling --------

void draw(void) {
  // graphic commands to redraw the complete screen should be placed here  
  u8g.setFontPosTop();
  u8g.setFont(u8g_font_unifont); 

  ui_manager();
}

void ui_manager()
{
	switch(menu_State)
	{
		case 0:
			draw_splash();

			if(state_timeout > 100)
			{
				menu_State++;
				state_timeout = 0;
			}
			else
			{
				state_timeout++;
			}
			
		break;

		case 1:
			draw_mainpage();
		break;

		case 2:
			draw_settingslist();
		break;

		default:
			draw_error();
		break;
	}
} 

void draw_splash()
{
	u8g.setFont(u8g_font_helvR10r);
	u8g.drawStr( 12, 15, "ROTORMAGIC");

	u8g.setFont(u8g_font_helvR08r);
	u8g.drawStr( 8, 28, "4-Way Diversity RX 0.1");
	//TODO - Take version number from define
}

void draw_mainpage()
{
	char buf[10];

	//selected frequency
	sprintf(buf, "%d", pgm_read_word_near(channelFreqTable + (active_band*8 + active_freq)) );
	u8g.setFont(u8g_font_helvR14r);
	u8g.drawStr( 10, 15, buf);

	//Human readable band/ch selected
	u8g.setFont(u8g_font_helvR08r);
	sprintf(buf, "%s %d", bandNameTable[active_band], active_freq+1);
	u8g.drawStr( 0, 28, buf);

	
	//print battery stats
	sprintf (buf, "%dS %02d%%", cell_count, batt_percentage);
	u8g.drawStr( 85, 10, buf);


	sprintf (buf, "%d %d", rx_rssi[2-1], rx_rssi[1-1]);
	u8g.drawStr( 85, 20, buf);

	sprintf (buf, "%d %d", rx_rssi[4-1], rx_rssi[3-1]);
	u8g.drawStr( 85, 30, buf);

	//sprintf (buf, "%d %d", button1, button2);
	//u8g.drawStr( 98, 30, buf);
}

void draw_settingslist()
{
	u8g.setFont(u8g_font_helvR10r);
	u8g.drawStr( 12, 15, "Settings Menu");

	u8g.setFont(u8g_font_helvR08r);
	u8g.drawStr( 6, 28, "To be completed");
}

void draw_error()
{
	u8g.setFont(u8g_font_helvR10r);
	u8g.drawStr( 12, 15, "MENU ERROR");

	u8g.setFont(u8g_font_helvR08r);
	u8g.drawStr( 6, 28, "Something went wrong :(");
}
}
//------- Battery Handling ---------
void measure_battery()
{
	float adc_Voltage = (analogRead(VOLTAGE_SENSE) / 1024) * 5.0;
	//adc reads 10bit, 0-5v range
	//10k and 2k divider gives 4.67v max adc for 28V input = 0.167x
	input_voltage = adc_Voltage / 0.167;
}

void determine_cell_count()
{
	//how the fuck do we do this reliably?
	cell_count = (input_voltage / BATTERY_MAX) + 1;

	if(cell_count > 6)
	{
		//TODO - handle over cell count detection or stupid user

	}

	if(cell_count < 2)
	{
		//TODO - handle 1S detected, or no valid reading

	}
}

void calculate_battery_percentage()
{
	batt_percentage = ((input_voltage - (BATTERY_MIN * cell_count)) * 100) / ((BATTERY_MAX - BATTERY_MIN) * cell_count);
}

void check_battery_health()
{
	if(input_voltage < (BATTERY_WARN * cell_count))
	{
		//TODO - handle low battery alarm

	}
	else if(input_voltage < (BATTERY_MIN * cell_count))
	{
		//TODO - handle flat/urgent battery alarm

	}
	else 	
	{
		//battery is sufficiently full, should probably schedule the next check or something

	}
}

//---------- RX Functions ----------
void rx_setup_pinMode()
{
	for(int i = 0; i < MODULE_COUNT; i++)
	{
		pinMode(RX_RSSI_PIN[i], INPUT);
	}
}

void rx_set_freq(int freqNum)
{
	active_freq = constrain(freqNum, 0, 8);
}

void rx_set_band(int bandNum)
{
	active_band = constrain(bandNum, 0, 5);
}

int rx_calculate_channel() {
	Serial.print("Calculated ch"); 	Serial.println(active_band*8 + active_freq);
	return active_band*8 + active_freq;
}

void rx_sample_rssi()
{
	for(int i = 0; i < MODULE_COUNT; i++)
	{
		rx_rssi[i] = analogRead(RX_RSSI_PIN[i]);
	}
}

void rx_decide_active()
{

}

void rx_calibration()
{

}

void rx_autoscan()
{

}

void rx_push_data(uint8_t channel)
{
  uint8_t i;
  uint16_t channelData;

  channelData = pgm_read_word_near(channelTable + channel);

  // bit bang  25 bits of data
  // Order: A0-3, !R/W, D0-D19
  // A0=0, A1=0, A2=0, A3=1, RW=0, D0-19=0
  rx_enable_high();
  delayMicroseconds(1);
  //delay(2);
  rx_enable_low();

  rx_send_bit0();
  rx_send_bit0();
  rx_send_bit0();
  rx_send_bit1();

  rx_send_bit0();

  // remaining zeros
  for (i = 20; i > 0; i--)
  {
    rx_send_bit0();
  }

  // Clock the data in
  rx_enable_high();
  //delay(2);
  delayMicroseconds(1);
  rx_enable_low();

  // Second is the channel data from the lookup table
  // 20 bytes of register data are sent, but the MSB 4 bits are zeros
  // register address = 0x1, write, data0-15=channelData data15-19=0x0
  rx_enable_high();
  rx_enable_low();

  // Register 0x1
  rx_send_bit1();
  rx_send_bit0();
  rx_send_bit0();
  rx_send_bit0();

  // Write to register
  rx_send_bit1();

  // D0-D15
  //   note: loop runs backwards as more efficent on AVR
  for (i = 16; i > 0; i--)
  {
    // Is bit high or low?
    if (channelData & 0x1)
    {
      rx_send_bit1();
    }
    else
    {
      rx_send_bit0();
    }

    // Shift bits along to check the next one
    channelData >>= 1;
  }

  // Remaining D16-D19
  for (i = 4; i > 0; i--)
  {
    rx_send_bit0();
  }
  // Finished clocking data in
  rx_enable_high();
  delayMicroseconds(1);
  //delay(2);

  digitalWrite(SPI_CS, LOW);
  digitalWrite(SPI_CLK, LOW);
  digitalWrite(SPI_DIO, LOW);
}

void rx_send_bit1()
{
  digitalWrite(SPI_CLK, LOW);
  delayMicroseconds(1);

  digitalWrite(SPI_DIO, HIGH);
  delayMicroseconds(1);
  digitalWrite(SPI_CLK, HIGH);
  delayMicroseconds(1);

  digitalWrite(SPI_CLK, LOW);
  delayMicroseconds(1);
}

void rx_send_bit0()
{
  digitalWrite(SPI_CLK, LOW);
  delayMicroseconds(1);

  digitalWrite(SPI_DIO, LOW);
  delayMicroseconds(1);
  digitalWrite(SPI_CLK, HIGH);
  delayMicroseconds(1);

  digitalWrite(SPI_CLK, LOW);
  delayMicroseconds(1);
}

void rx_enable_high()
{
  delayMicroseconds(1);
  digitalWrite(SPI_CS, HIGH);
  delayMicroseconds(1);
}

void rx_enable_low()
{
  delayMicroseconds(1);
  digitalWrite(SPI_CS, LOW);
  delayMicroseconds(1);
}

//------- Switching Matrix --------
void sm_setup_pinMode()
{
	for(int i = 0; i < MODULE_COUNT; i++)
	{
		pinMode(RX_SEL_PIN[i], OUTPUT);
	}
}

// Sets the 4 pins for the digital switches to route selected module to the output.
void sm_set_active(int moduleRequested)
{
	if(moduleRequested == 0)
	{
		alert_debug("sm mR0");
		sm_set_active(1);	//always default to something
	}

	for(int i = 0; i < MODULE_COUNT; i++)
	{
		if(i == moduleRequested-1)
		{
			digitalWrite(RX_SEL_PIN[i],HIGH);
			active_module = moduleRequested;	//only write values here, everywhere else should RO
		}
		else
		{
			digitalWrite(RX_SEL_PIN[i],LOW);
		}
	}
}

//------------- Alerts -------------
void alert_startup()
{
	status_setup();
	buzz_startup();
}

void alert_low_battery() 
{
	buzz_battery_low();
}

void alert_critical_battery() 
{
	buzz_battery_critical();
}

void alert_low_signal()
{

}

void alert_rx_error()
{

}

void alert_channel_change()
{
	buzz_freq_change();
}

void alert_band_change()
{
	buzz_band_change();
}

void alert_debug(String debugText)
{
	Serial.println(debugText);
	buzz_alert();
}

//---------- LED Outputs ----------

void status_setup()
{
	// int col = 0;

	// for(int i = 0; i < 254; i++)
	// {
	// 	for(int j = 0; j < strip.numPixels(); j++) {
	//     	strip.setPixelColor(j, strip.Color(col,col,col));
	//     	col++;
 //  		}
 //  		strip.show();
 //  		delay(10);
	// }

	// strip.clear();
	// strip.show();
}

void status_autoscan()
{

}

void status_dominant_rx()
{
	for(int i = 0; i < WS2812_COUNT; i++) {

		if(active_module-1 == i)
		{
			leds[i] = CRGB::Red;
		}
		else
		{
			leds[i] = CRGB::Black; //.fadeToBlackBy(128);
		}

	}

}

void status_low_battery()
{

}

void status_setting_save() 
{

}

//led solid for module led
void status_channel_change()
{

}

//flash all leds with colour code for band
void status_band_change()
{

}


//--------- Buzzer -----------

void buzz_tone(int note, int duration)
{
	tone(BUZZER,note,duration); 
}

void buzz_alert()
{
	buzz_tone(280, 40);
	delay(40);
	buzz_tone(280, 40);
}

void buzz_band_change()
{
	buzz_tone(650, 40);

}

void buzz_freq_change()
{
	buzz_tone(440, 80);

}

void buzz_module_change()
{
	buzz_tone(1250, 15);
}

void buzz_battery_low()
{
	buzz_tone(200, 50);
	delay(40);
	buzz_tone(350, 50);
}

void buzz_battery_critical()
{
	buzz_tone(350, 50);
	delay(40);
	buzz_tone(350, 50);
	delay(40);
	buzz_tone(350, 50);
}

void buzz_startup()
{
	buzz_tone(440, 100);
	delay(150);
	buzz_tone(860, 100);
	delay(150);
	buzz_tone(1280, 140);
}

//-------- Setting Save --------

void memory_check_first_run()
{

}

void memory_read_settings()
{

}

void memory_pull_settings()
{

}

void memory_push_settings()
{

}






/*

TODO

split codebase into files per topic
move pinout arrays into progmem

LED handling code
	remap leds to correct order on board
	startup animation
	channel indication led (in band colour)
	band indication flash (in band colour)
	fading between states/hues
	error animation
	low battery animation
	rssi visualisation mode (rainbow hues for each led)

Buzzer code
	complex tone generation

Menu System
	Settings Menu
	Overlay alert screens

General functions
	Calibration function
	module selection logic
	button handling
	non-blocking timers
	timer based callbacks

EEPROM
	Save/recall functions
	Channel and band settings
	Store calibration data
	store user settings

Create startup tutorial
*/