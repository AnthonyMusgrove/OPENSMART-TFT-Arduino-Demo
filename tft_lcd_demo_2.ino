/*
 *    OpenSmart TFT sheild (ILI9320 controller IC)
 *    
 *    For use with Arduino Uno.  This demonstration was written due to the severe lack of documentation with this cheap shield available
 *    from Wish (for around $5 AUD).   After various and extensive research, this sketch demonstrates this sheild, utilising the following :-
 *    
 *    * LM75 Temperature Sensor (onboard of TFT shield, upper corner), via I2C at default address 0x48.
 *    * Micro SD slot (onboard of TFT shield, upper middle), via SPI - SD Chip Select on Digital Pin 5, utilising the standard arduino SD library.
 *    * TFT Screen - 320x240, 2.8" OpenSmart LCD screen via a modified version of the MCUFRIEND library.
 *    * TFT Touch Screen - Resistive touch screen included with TFT shield - via Adafruit Touch Screen Library.
 *    
 *    Note; this demonstration utilises the built-in arduino SD library.  By default, this library modifies the SPI configuration for 
 *    every call utilising this library; so you MUST call the RESTORE_SPI_GPIO() macro AFTER utilising SD library functions, otherwise
 *    the TFT screen will not function correctly/at all.
 *    
 *    Any questions, queries, comments, suggestions, please feel free to contact me at the following;
 * 
 *    (C) 2019 Anthony Musgrove
 *    anthony.musgrove@newymods.com.au 
 *    
 *    This demonstration is OPEN SOURCE, Free to distribute, free to use in your own projects - all I ask is that
 *    you leave my contact details and this header intact.  Thank you.
 *    
 */

#include <Adafruit_GFX.h>    // Core graphics library
#include <MCUFRIEND_kbv.h>
#include <TouchScreen.h>
#include <M2M_LM75A.h> // temperature sensor uses I2C, address 0x48
#include <SPI.h> // for SD
#include <SD.h>

MCUFRIEND_kbv tft_screen;

M2M_LM75A temperature_sensor;

#define RESTORE_SPI_GPIO() SPCR = 0

/* colours */
#define BLACK 0x0000
#define NAVY 0x000Fa
#define DARKGREEN 0x03E0
#define DARKCYAN 0x03EF
#define MAROON 0x7800
#define PURPLE 0x780F
#define OLIVE 0x7BE0
#define LIGHTGREY 0xC618
#define DARKGREY 0x7BEF
#define BLUE 0x001F
#define GREEN 0x07E0
#define CYAN 0x07FF
#define RED 0xF800
#define MAGENTA 0xF81F
#define YELLOW 0xFFE0
#define WHITE 0xFFFF
#define ORANGE 0xFD20
#define GREENYELLOW 0xAFE5
#define PINK 0xF81F

/* Basic debug datetime stuff */
#define SECS_PER_MIN  (60UL)
#define SECS_PER_HOUR (3600UL)
#define SECS_PER_DAY  (SECS_PER_HOUR * 24L)

/* Useful Macros for getting elapsed time */
#define numberOfSeconds(_time_) (_time_ % SECS_PER_MIN)  
#define numberOfMinutes(_time_) ((_time_ / SECS_PER_MIN) % SECS_PER_MIN)
#define numberOfHours(_time_) (( _time_% SECS_PER_DAY) / SECS_PER_HOUR)
#define elapsedDays(_time_) ( _time_ / SECS_PER_DAY) 

// Touch screen pins for ILI9320:
// ILI9320: YP=A2, XM=A3, YM=8, XP=9

uint8_t YP = A1;  // must be an analog pin, use "An" notation!
uint8_t XM = A2;  // must be an analog pin, use "An" notation!
uint8_t YM = 7;   // can be a digital pin
uint8_t XP = 6;   // can be a digital pin
uint8_t SwapXY = 0;

uint16_t TS_LEFT = 880;
uint16_t TS_RT  = 170;
uint16_t TS_TOP = 950;
uint16_t TS_BOT = 180;

// NOTE: check out the constructor for TouchScreen, the value '250' is calculated by 
//       testing the resistance between specific pins on the touchscreen.  

TouchScreen touch_driver = TouchScreen(XP, YP, XM, YM, 250);
TSPoint tp;

// NOTE: these values can be changed to adjust the touch screen sensitivity; you will
//       notice these values being utilised in detect_touch().

#define MINPRESSURE 100
#define MAXPRESSURE 1000

unsigned long duration_up = 0;

int TFT_CS = A3; // chip select for SPI bus for TFT screen
int SD_CS = 5; // chip select for SPI bus for SD card

struct gui_button {

  int16_t x;
  int16_t y;
  int16_t height;
  int16_t width;
  String label;
  int command_on_press;
  int command_on_release;
  int data_on_press;
  int data_on_release;
  bool is_depressed = false;
  
};

gui_button gui_buttons[5];
int total_buttons = 0;

void calculate_uptime()
{
  //eventually handle rollover.  for now, this is purely for demonstration purposes only.
  duration_up = millis();
}

String get_uptime() // get fancy uptime format 00:00:00  (HH:mm:ss)
{
  long total_sec_up = duration_up / 1000;

  String cur_hours = String(numberOfHours(total_sec_up));
  String cur_min = String(numberOfMinutes(total_sec_up));
  String cur_sec = String(numberOfSeconds(total_sec_up));

  if(cur_hours.length() < 2)
    cur_hours = "0" + cur_hours;

  if(cur_min.length() < 2)
    cur_min = "0" + cur_min;

  if(cur_sec.length() < 2)
    cur_sec = "0" + cur_sec;

  return(cur_hours + ":" + cur_min + ":" + cur_sec);
}

/* tft identifier */
uint16_t g_identifier;

/* just a simple flag for if the device powered on with a valid SD card */
bool has_valid_sd = false;

void setup() {

  Serial.begin(9600);
    
  temperature_sensor.begin();

  g_identifier = tft_screen.readID();

  if (g_identifier == 0x00D3 || g_identifier == 0xD3D3) g_identifier = 0x9481; // write-only shield
    if (g_identifier == 0xFFFF) g_identifier = 0x9341; // serial

  tft_screen.begin(g_identifier);
  tft_screen.fillScreen(BLUE);

  if(!SD.begin(SD_CS))
  {
    has_valid_sd = false;
  }
  else
  {
    has_valid_sd = true;
  }

  RESTORE_SPI_GPIO();
}


bool initial_display_debug = false;

int current_screen_id = 0;

void update_screen()
{
  switch(current_screen_id)
  {
    case 0:
    //update home screen
    tft_screen.setTextColor(WHITE, DARKCYAN);
    tft_screen.setTextSize(2);
    tft_screen.setCursor(74, 285);
    tft_screen.println(get_uptime());
  
    tft_screen.setTextColor(GREEN, DARKGREY);
    tft_screen.setCursor(78, 240);
    tft_screen.println("NORMAL.");

    tft_screen.setTextColor(WHITE, DARKGREY);
    tft_screen.setCursor(191, 285);
    tft_screen.println(temperature_sensor.getTemperature(), 0);

    if(has_valid_sd)
    {
      tft_screen.setTextColor(WHITE, DARKGREY);
      tft_screen.setCursor(20, 285);
      tft_screen.println("SD");
    }
    else
    {
      tft_screen.setTextColor(WHITE, RED);
      tft_screen.setCursor(20, 285);
      tft_screen.println("!SD");

    }
    break;
  }
}

void clear_buttons()
{
  for(int t=0; t<5; t++)
  {
    gui_buttons[t].x = 0;
    gui_buttons[t].y = 0;
    gui_buttons[t].label = "";
  }

  total_buttons = 0;
}

void change_screen(int screen_id)
{

   tft_screen.fillScreen(BLUE);
  
   //screen border
   tft_screen.drawLine(10, 10, 10, 310, WHITE);
   tft_screen.drawLine(10, 310, 230, 310, WHITE);
   tft_screen.drawLine(230, 310, 230, 10, WHITE);
   tft_screen.drawLine(230, 10, 10, 10, WHITE);
  
   //heading underline
   tft_screen.drawLine(11, 40, 230, 40, WHITE);
  
  tft_screen.setTextColor(WHITE, RED);
  tft_screen.setTextSize(2);
  tft_screen.setCursor(27, 18);
  tft_screen.println("FIRE ALARM PANEL");


  switch(screen_id)
  {

    case 0: // HOME SCREEN
  
    tft_screen.setTextColor(WHITE, BLUE);
    tft_screen.setTextSize(1);

    tft_screen.setCursor(38, 38);
    tft_screen.println("VERSION 1.0.0 - BUILD 20190928");


    tft_screen.setCursor(84, 225);
    tft_screen.println("SYSTEM STATUS");
  
    tft_screen.setCursor(84, 270);
    tft_screen.println("SYSTEM UPTIME");

    tft_screen.setCursor(192, 270);
    tft_screen.println("(*C)");

    clear_buttons();
    add_button(35, 55, 40, 170, "ISOLATE", 0x2, 0x1, 0x0, 0x1);
    add_button(35, 110, 40, 170, "RESET", 0x2, 0x1, 0x0, 0x2);
    add_button(35, 165, 40, 170, "SETTINGS", 0x2, 0x3, 0x0, 0x0);
    break;

  case 0x1: // ISOLATE

    tft_screen.setTextColor(WHITE, BLUE);
    tft_screen.setTextSize(1);
  
    tft_screen.setCursor(48, 38);
    tft_screen.println("SELECT ZONE(S) TO ISOLATE");

    clear_buttons();
    add_button(35, 250, 40, 170, "< BACK", 0x2, 0x1, 0x0, 0x0);
    
  break;

  case 0x2: // RESET

    tft_screen.setTextColor(WHITE, BLUE);
    tft_screen.setTextSize(1);
  
    tft_screen.setCursor(52, 38);
    tft_screen.println("SELECT ZONE(S) TO RESET");

    clear_buttons();
    add_button(35, 250, 40, 170, "< BACK", 0x2, 0x1, 0x0, 0x0);
  
  break;

  }

  draw_buttons();
  current_screen_id = screen_id;
}

#define DEBOUNCE_PERIOD_MS 100
unsigned long last_touch = 0;

void add_button(int16_t x, int16_t y, int16_t height, int16_t width, String label, int cmd_on_press, int cmd_on_release, int data_on_press, int data_on_release)
{
  //check if max buttons reached for interface.
  if(total_buttons == 4) //maximum of 5.
    return;
    
  gui_button btn;
  btn.x = x;
  btn.y = y;
  btn.height = height;
  btn.width = width;
  btn.label = label;
  btn.command_on_press = cmd_on_press;
  btn.command_on_release = cmd_on_release;
  btn.data_on_press = data_on_press;
  btn.data_on_release = data_on_release;
  btn.is_depressed = false;

  gui_buttons[total_buttons] = btn;
  total_buttons++;
  
}


void draw_buttons()
{
  for(int t=0; t<total_buttons; t++)
  {
    draw_button(gui_buttons[t].x, gui_buttons[t].y, gui_buttons[t].height, gui_buttons[t].width, gui_buttons[t].label, gui_buttons[t].is_depressed);
  }
}


void redraw_button(int button_to_redraw_id)
{
      //draw a BLUE box (background colour) over x-width+5, y-height+5 first (incorporates shadow), which will clear the space 
    //this will clear the space for depressing the button.
      
     tft_screen.fillRect(gui_buttons[button_to_redraw_id].x - 1, gui_buttons[button_to_redraw_id].y - 1, gui_buttons[button_to_redraw_id].width+8, gui_buttons[button_to_redraw_id].height+8, BLUE);
     draw_button(gui_buttons[button_to_redraw_id].x, gui_buttons[button_to_redraw_id].y, gui_buttons[button_to_redraw_id].height, gui_buttons[button_to_redraw_id].width, gui_buttons[button_to_redraw_id].label, gui_buttons[button_to_redraw_id].is_depressed);
}

// text size 2, char height = 8, char width = 12
void draw_button(int16_t x, int16_t y, int16_t height, int16_t width, String text, bool is_depressed)
{   
    int16_t draw_button_at_x = x;
    int16_t draw_button_at_y = y;
    
    if(is_depressed)
    {
      draw_button_at_x = draw_button_at_x + 2;
      draw_button_at_y = draw_button_at_y + 2;
    }

    //shadow
    tft_screen.fillRect(x + 5, y+height+2, width, 5, DARKGREY); 
    tft_screen.fillRect(x + width + 2, y + 5, 5, height, DARKGREY);

  
    tft_screen.drawLine(draw_button_at_x-1, draw_button_at_y-1, draw_button_at_x-1, ((draw_button_at_y+height) + 1), WHITE);
    tft_screen.drawLine(draw_button_at_x-1, (draw_button_at_y+height) + 1, (draw_button_at_x+1+width), (draw_button_at_y+height)+1, WHITE);    
    tft_screen.drawLine((draw_button_at_x+1+width), (draw_button_at_y+height)+1, (draw_button_at_x+1+width), (draw_button_at_y-1), WHITE);
    tft_screen.drawLine((draw_button_at_x+1+width), (draw_button_at_y-1), draw_button_at_x-1, draw_button_at_y-1, WHITE);
    tft_screen.fillRect(draw_button_at_x, draw_button_at_y, width, height, DARKCYAN);

    //draw text
    tft_screen.setTextColor(WHITE, DARKCYAN);
    tft_screen.setTextSize(2);
    tft_screen.setCursor(draw_button_at_x + width/2 - ((text.length()*12) / 2), draw_button_at_y + height / 2 - 8);
    tft_screen.println(text);
}


TSPoint current_touch_point;
long touch_first_detected = 0;
int last_pressure = 0;
int total_pressure_samples_for_release = 0;
bool is_touching = false;

void detect_touch()
{
    tp = touch_driver.getPoint();

    pinMode(XM, OUTPUT);
    pinMode(YP, OUTPUT);
    pinMode(XP, OUTPUT);
    pinMode(YM, OUTPUT);

    if(tp.z > MINPRESSURE && tp.z < MAXPRESSURE)
    {

        if(is_touching == false)
          touch_first_detected = millis();
 
        current_touch_point = tp; //touch_driver.getPoint();

        total_pressure_samples_for_release = 0;

        long touch_begin_duration = (millis() - touch_first_detected);

        process_touch(touch_begin_duration, tp);


        is_touching = true;
    }
  else
  {
    if(is_touching == true)
    {
      //sample last pressure, check if last pressure was 0.  We need to see 5 consecutive zeros to 
      //confirm a release.
      total_pressure_samples_for_release++;

      if(tp.z == 0 && last_pressure == 0 && total_pressure_samples_for_release == 5)
      {
        //release!
        last_pressure = 0;
        total_pressure_samples_for_release = 0;
        is_touching = false;

        long touch_duration = (millis() - touch_first_detected);
        touch_first_detected = 0;
        
        process_touch_release(touch_duration, current_touch_point);
      }
    }
  }
}

void process_touch_release(long touch_duration, TSPoint touch_point)
{
    process_gui_functions(0x1, touch_point, touch_duration);
}

void process_touch(long touch_duration, TSPoint touch_point) //touch detected, not release. touch duration for this touch.
{
    process_gui_functions(0x0, touch_point, touch_duration);
}

void process_gui_functions(int touch_action, TSPoint touch_point, long touch_duration)
{
      // BUTTONS
      long mapped_x = map(touch_point.x, 932, 167, 0, 240);
      long mapped_y = map(touch_point.y, 947, 195, 0, 320);

      for(int k=0; k<total_buttons; k++)
      {
          if(mapped_x >= gui_buttons[k].x && mapped_x <= gui_buttons[k].x + gui_buttons[k].width)
          {
            if(mapped_y >= gui_buttons[k].y && mapped_y <= gui_buttons[k].y + gui_buttons[k].height)
            {
              process_button_action(k, touch_action, touch_duration);
            }
          }
      }
}

void process_button_action(int btn_pressed_id, int touch_action, long touch_duration)
{

  switch(touch_action)
  {
      case 0x0:
      //touch
      switch(gui_buttons[btn_pressed_id].command_on_press)
      {
        case 0x0: //NOOP - Do nothing
        break;
  
        case 0x1: //Navigate to new menu, menu ID stored in data_on_press
        change_screen(gui_buttons[btn_pressed_id].data_on_press);
        break;

        case 0x2: // depress

        if(!gui_buttons[btn_pressed_id].is_depressed)
        {
          gui_buttons[btn_pressed_id].is_depressed = true;
          redraw_button(btn_pressed_id);
        }
        
        break;

        case 0x3: //unpress
  
        if(gui_buttons[btn_pressed_id].is_depressed)
        {
          gui_buttons[btn_pressed_id].is_depressed = false;
          redraw_button(btn_pressed_id);
        }
        
        break;

      }

      break;

      case 0x1:
      //release
      switch(gui_buttons[btn_pressed_id].command_on_release)
      {
        case 0x0: //NOOP - Do nothing
        break;
  
        case 0x1: //Navigate to new menu, menu ID stored in data_on_press
        change_screen(gui_buttons[btn_pressed_id].data_on_release);
        break;

        case 0x2: // depress

        if(!gui_buttons[btn_pressed_id].is_depressed)
        {
          gui_buttons[btn_pressed_id].is_depressed = true;
          redraw_button(btn_pressed_id);
        }
        
        break;

        case 0x3: //unpress
  
        if(gui_buttons[btn_pressed_id].is_depressed)
        {
          gui_buttons[btn_pressed_id].is_depressed = false;
          redraw_button(btn_pressed_id);
        }
        
        break;
      }
      
      break;
  }
  
  switch(gui_buttons[btn_pressed_id].command_on_press)
  {
      case 0x0: //NOOP - Do nothing
      break;

      case 0x1: //Navigate to new menu, menu ID stored in data_on_press
      change_screen(gui_buttons[btn_pressed_id].data_on_press);
      break;    
  }
}

void loop() {

  calculate_uptime();

  if(!initial_display_debug)
  {
    change_screen(0);
    initial_display_debug = true;
  }

  detect_touch();
  update_screen();
}
