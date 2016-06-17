#include <Adafruit_NeoPixel.h>
#include <EEPROM.h>

#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// Settings and such
#define VERSION_NUMBER 1.0
#define RX_TYPE 1 //0 = RX5808, 1 = RX158

// Pinout mapping
#define VOLTAGE_SENSE A6  //ADC6, 19
#define BUZZER  3 //PD3, 01
#define WS2812  5  //PB5, 17
#define BTN_1 2 //PD2, 32
#define BTN_2 4 //PD4, 02

#define SPI_DIO 10  //PB2, 14
#define SPI_CS  11  //PB3, 15
#define SPI_CLK 12  //PB4, 1
#define UART_RX 0 //PD0, 12
#define UART_TX 1 //PD1, 31
#define I2C_SDA A4  //PC4, 27
#define I2C_SCL A5  //PC5, 28

#define RSSI1 A2  //PC2, 25
#define RSSI2 A0  //PC0, 23
#define RSSI3 A3  //PC3, 26
#define RSSI4 A1  //PC1, 24

#define MODULE_COUNT 4	//number of RX modules
int RX_SEL_PIN[MODULE_COUNT] = {6, 7, 8, 9};
//#define RXSEL1  6 //PD6, 10
//#define RXSEL2  7 //PD7, 11
//#define RXSEL3  8 //PB0, 12
//#define RXSEL4  9 //PB1, 13


// Driver Object Definitions
Adafruit_NeoPixel strip = Adafruit_NeoPixel(4, WS2812, NEO_GRB + NEO_KHZ800);
Adafruit_SSD1306 display = Adafruit_SSD1306();


// Global Defines
int rssi_module[] = {0, 0, 0, 0};
int active_module = 0;
int button1, button2 = 0;

void setup_pins()
{
  sm_setup_pinMode();

  pinMode(SPI_CLK, OUTPUT);
  pinMode(SPI_CS, OUTPUT);
  pinMode(SPI_DIO, OUTPUT);

  pinMode(BUZZER, OUTPUT);

  pinMode(RSSI1, INPUT);
  pinMode(RSSI2, INPUT);
  pinMode(RSSI3, INPUT);
  pinMode(RSSI4, INPUT);

  pinMode(BTN_1, INPUT);
  pinMode(BTN_2, INPUT);
  pinMode(VOLTAGE_SENSE, INPUT);

}

void setup_peripherals()
{
  //i2c display init
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);  // initialize with the I2C addr 0x3C (for the 128x32)
  display.setTextColor(WHITE);
  display.clearDisplay();

  //WS2812/SK6812 LED Setup
  strip.begin();
  strip.clear();	//using clear() ensures we wipe any colour held over a reset
  strip.show();
    
  //rx module init


}

void setup() 
{
  Serial.begin(115200);
  Serial.println("setup: entry");
  
  setup_pins();
  setup_peripherals();

  draw_init();		//content on screen ASAP
  status_setup();	//cool startup animation


  
  Serial.println("setup: exit");

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
	}

	status_dominant_rx();
  // button1 =  digitalRead(BTN_1);
  // button2 = digitalRead(BTN_1);

  // if(button1 == 0)
  // {
  // 	Serial.println("Button 1");
  // 		sm_set_active(1);

  // }

  // if(button2 == 0)
  // {
  // 	Serial.println("Button 2");
  // 		sm_set_active(2);

  // }

  Serial.println("loop: exit");

  strip.show();
  display.display();

  delay(400);

}




//------- Display Handling --------

void draw_init()
{
  display.clearDisplay();
  display.setTextColor(WHITE);

  //title block
  display.setTextSize(2);
  display.setCursor(0,0);
  display.print("QUAD 5.8RX");
  display.setCursor(0,24);
  display.print("Ver 1.0-A");
 

  //detected battery voltage
  display.setTextSize(2);
  display.setCursor(0,48);
  display.print("Batt: ");
  
  display.print("?");
 
  display.display();
}

void draw_splash()
{

}

void draw_update()
{

}

void menu_manager()
{

} 

//------- Battery Handling ---------

void determine_cell_count()
{
  //how the fuck do we do this reliably


}

void measure_battery()
{
  
}

//---------- RX Functions ----------

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
	int col = 0;

	for(int i = 0; i < 254; i++)
	{
		for(int j = 0; j < strip.numPixels(); j++) {
	    	strip.setPixelColor(j, strip.Color(col,col,col));
	    	col++;
  		}
  		strip.show();
  		delay(10);
	}

	strip.clear();
	strip.show();
}

void status_autoscan()
{
  
}

void status_dominant_rx()
{
  for(int i = 0; i < strip.numPixels(); i++) {

  	if(active_module-1 == i)
  	{
		strip.setPixelColor(i, strip.Color(40,0,0));
  	}
  	else
  	{
		strip.setPixelColor(i, strip.Color(0,0,0));
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
	migrate to fastLED
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
	Boot screen
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