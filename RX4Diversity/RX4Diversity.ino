#include "U8glib.h"
#include "FastLED.h"
#include <EEPROM.h>

// Settings and such
#define VERSION_NUMBER 1.0
#define RX_TYPE 1 //0 = RX5808, 1 = RX158
#define MODULE_COUNT 4	//number of RX modules

// Pinout mapping
#define VOLTAGE_SENSE A6  //ADC6, 19
#define BUZZER  3 //PD3, 01
#define BTN_1 2 //PD2, 32
#define BTN_2 4 //PD4, 02

#define WS2812_DATA  5  //PB5, 17
#define WS2812_COUNT 4	//led count might be different from rx count

#define SPI_DIO 10  //PB2, 14
#define SPI_CS  11  //PB3, 15
#define SPI_CLK 12  //PB4, 1
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
int cell_count = 0;
volatile int rx_rssi[MODULE_COUNT] = {0, 0, 0, 0};

int menu_State = 0;
int active_module = 0;

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
	FastLED.addLeds<WS2812B, WS2812_DATA, GRB>(leds, WS2812_COUNT);

	//rx module init


}

void setup() 
{
	Serial.begin(115200);
	Serial.println("setup: entry");

	setup_pins();
	setup_peripherals();

}

void loop()
{
	if(active_module < 4)
	{
		sm_set_active(active_module+1);
	}
	else
	{
		sm_set_active(1);
		menu_State = 1;
	}

	status_dominant_rx();
	button1 =  digitalRead(BTN_1);
	button2 = digitalRead(BTN_1);

  // if(button1 == 0)
  // {
  // 	Serial.println("Button 1");
  // }

  // if(button2 == 0)
  // {
  // 	Serial.println("Button 2");
  // }

 // leds[0] = CRGB::Red;
 //  FastLED.show();
 //  delay(500);
 //  // Now turn the LED off, then pause
 //  leds[0] = CRGB::Black;
 //  FastLED.show();


	// picture loop
	u8g.firstPage();  
	do {
		draw();
	} while( u8g.nextPage() );


	Serial.println("loop: exit");
	delay(100);

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

		break;

		case 1:
			draw_mainpage();

		break;

		case 2:

		break;

		default:

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
	u8g.setFont(u8g_font_helvR14r);
	u8g.drawStr( 10, 15, "5470");

	u8g.setFont(u8g_font_helvR08r);
	u8g.drawStr( 0, 28, "FatShark 01");



	u8g.drawStr( 80, 10, "VD");
	char buf1[6];
	sprintf (buf1, "%d", input_voltage);
	u8g.drawStr( 98, 10, buf1);

	u8g.drawStr( 80, 20, "B1");
	char buf[2];
	sprintf (buf, "%d", button1);
	u8g.drawStr( 98, 20, buf);

	u8g.drawStr( 80, 30, "B2");
	char buf2[2];
	sprintf (buf2, "%d", button2);
	u8g.drawStr( 98, 30, buf2);
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
	float batt_percentage = ((input_voltage - (BATTERY_MIN * cell_count)) * 100) / ((BATTERY_MAX - BATTERY_MIN) * cell_count);
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
		pinMode(RX_RSSI_PIN[i], OUTPUT);
	}
}

void rx_init()
{

}

void rx_set_channel()
{

}

void rx_set_band()
{

}

void rx_sample_rssi()
{

}

void rx_autoscan()
{

}

void rx_push_data()
{

}

void rx_decide_active()
{

}

void rx_calibration()
{

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

void alert_low_battery() 
{

}

void alert_low_signal()
{

}

void alert_rx_error()
{

}

void alert_channel_change()
{

}

void alert_band_change()
{

}

void alert_debug(String debugText)
{
	Serial.println(debugText);
	buzz_tone(440,100);		//A4 100ms
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
  // for(int i = 0; i < strip.numPixels(); i++) {

  // 	if(active_module-1 == i)
  // 	{
		// strip.setPixelColor(i, strip.Color(40,0,0));
  // 	}
  // 	else
  // 	{
		// strip.setPixelColor(i, strip.Color(0,0,0));
  // 	}

  // }
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

}

void buzz_band_change()
{

}

void buzz_freq_change()
{

}

void buzz_module_change()
{
	
}

void buzz_battery_low()
{

}

void buzz_battery_critical()
{

}

void buzz_startup()
{

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
button handling code checks

LED handling code
	startup animation
	channel indication led (in band colour)
	band indication flash (in band colour)
	fading between states
	error animation
	low battery animation
	rssi visualisation mode

buzzer code
	rssi sourd outputmode (make sound on change)
	complex tone generation

Battery Management
	Check battery cell count
	Estimate percentage full
	adjustable trigger points
	notification generation

EEPROM
	Save/recall functions
	Channel and band settings
	Store calibration data
	store user settings

Menu System
	Home Screen
	Settings Menu
	Overlay alert screens

General functions
	Calibration function
	module selection logic
	rssi read/processsing
	button handling
	non-blocking timers
	timer based callbacks

*/