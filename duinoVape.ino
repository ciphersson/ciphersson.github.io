GNU GPLv3 license
*/
//Libraries Required
#include <SPI.h>  // Library for SPI interface to OLED
#include <Wire.h> // Library for duno, but something that is needed lol
#include <ClickButton.h> // library for multi function button 
#include <avr/sleep.h>  // library so it can use various sleep states
#include <Adafruit_GFX.h> // library for the driver of the oled
#include <Adafruit_SSD1306.h>  // library for the interfacing with the driver

// Pin Declerations
const int firebuttonPin = 7;     // Connected to fire switch on the Box from pin 7
ClickButton button1(firebuttonPin, HIGH); // sets the clickbutton library to look at firebuttonPin for the button presses and sets it to HIGH as firing
//const int fireLED = 10;          // LED which lights when firing atomizer // remove the // before this if you wire in a fire button LED
const int powerConverter = 6;    // Connected to inhibit/enable/on-off of the DC DC Converter connected to pin 6
const int analogPin = A0;        // potentiometer wiper (middle terminal) connected to analog pin 0, outside leads to ground and +5V
const int atomizerVoltage = A1;  // declares the analog pin 1 as the voltage output of the DC-DC converter to be read
const int batteryVoltage = A2;   // declares the analog pin 2 as the voltage output of the battery pack
//const int ledPin = 4;           // Internal LED to verifyt he circuit is activating for trouble shooting connected to pin 4

// Display setup for software SPI
#define OLED_MOSI   9
#define OLED_CLK   10
#define OLED_DC    14
#define OLED_CS    15
#define OLED_RESET 16
Adafruit_SSD1306 display(OLED_MOSI, OLED_CLK, OLED_DC, OLED_RESET, OLED_CS);

#define NUMFLAKES 10
#define XPOS 0
#define YPOS 1
#define DELTAY 2

#if (SSD1306_LCDHEIGHT != 64)
#error("Height incorrect, please fix Adafruit_SSD1306.h!");
#endif

// Variables
//int buttonState = 0;         // State of Fire Button
float R2 = .90;                // variable to store the R2 value which is the resistance of the atomizer coil, for now you change the .90 to whatever your attomizer is if you want accurate wattage numbers.
float buffer = 0;            // buffer variable for calculation
float Vbatt = 0;             // Assigns a variable for battery voltage raw read
int Vatty = 0;              // Assigns a variable for atomizer voltage raw read
int count = 0;               // counter for sleep activation
int countLimit = 0;          // counter to limit fire time
float Amps = 0;                // amps output to atomizer
float Avoltage = 0;            //variable for calculating raw voltage value from raw input
float Avoltage1 = 0;           // doubles avoltage and is displayed to account for voltage divider
int Watts = 0;                 // watts output to atomizer
float Bvoltage2 = 0;           // used for voltage divider, allows for over 5v input safely, compenstates for 1/2 voltage used in calculation
int Bpercent = 0;             // battery percent remaining
boolean ninjaMode = false;    // stealth mode, fires with screen staying off
//boolean fireLED = false;
boolean noAutofire = false;  // used in autofire prevention, so you dont get a fire or melted atty like I did :P
int puffCounter = 0;         // puff counter
float puffTimer = 0;         // used to calculate putt timer and store the result until next time the mod is fired
float totalPuffs = 0;          // puffs since last full charge
float avgPuff = 0;           // average time of puffs
boolean buttonLastState = false; // for trackign when button is pressed and held
float VTime = 0;
float VTime2 = 0;
float VTimeD = 0;
boolean invertedDisplay = false;

// Arbitrary LED function 
int PowerConvFunction = 0;  // ignore this, but dont change it used in button press code.

// Sleep Code
 void wakeUpNow() {             // here the interrupt is handled after wakeup
 // Wake up actions
  digitalWrite (powerConverter, HIGH);
  delay(50);  
  }

void setup()
{

 pinMode (firebuttonPin, INPUT);     // Set pin 7 as fire button input
 //digitalWrite (firebuttonPin, HIGH);
 pinMode (powerConverter, OUTPUT);   // Set Pin 6 as DC-DC converter output
 // Display
  display.begin(SSD1306_SWITCHCAPVCC);
  display.display(); // show splashscreen
  delay(500);
  display.clearDisplay();   // clears the screen and buffer
  
   // Setup button timers (all in milliseconds / ms)
   // (These are default if not set, but changeable for convenience)
   button1.debounceTime   = 10;   // Debounce timer in ms
   button1.multiclickTime = 400;  // Time limit for multi clicks
   button1.longClickTime  = 200; // time until "held-down clicks" register
   }
 
  // Sleep Setup for IDLE mode
  void sleepNow() {                      // here we put the arduino to sleep
  display.ssd1306_command(SSD1306_DISPLAYOFF);
  set_sleep_mode(SLEEP_MODE_IDLE);       // sleep mode is set here from the 5 available: SLEEP_MODE_IDLE, SLEEP_MODE_ADC, SLEEP_MODE_PWR_SAVE, SLEEP_MODE_STANDBY,  SLEEP_MODE_PWR_DOWN
  //sleep_enable();                        // enables the sleep bit in the mcucr register so sleep is possible. just a safety pin
  //attachInterrupt(0, wakeUpNow, LOW);  // use interrupt 0 (pin 2) and run function wakeUpNow when pin 2 gets LOW
  sleep_mode();                          // here the device is actually put to sleep!!  THE PROGRAM CONTINUES FROM HERE AFTER WAKING UP
  //sleep_disable();                       // first thing after waking from sleep is disable sleep.
  //detachInterrupt(0);                  // disables interrupt 0 on pin 2 so the wakeUpNow code will not be executed during normal running time.
  }


void loop()
  {
    
if (countLimit <= 199) noAutofire = false;  // if the atty has fired for less than 12 seconds noAutofire will not be true
if (countLimit >= 200) noAutofire = true;   // if the atty has fired for more than 12 seconds noAutofire will  be true
         
  // Update button state
    button1.Update();

  // Click codes are reset at next Update()
if (button1.clicks != 0) PowerConvFunction = button1.clicks;  // sets powerconvfunction equal to the number of times the button is pressed as long as the press count is not zero
  
// Ensure the converter stays OFF when not doing anything
  if(button1.clicks == 0) { 
    digitalWrite (powerConverter, HIGH);  
    //digitalWrite (ohmPin, LOW);                  // Disables the Ohm Meter circuit  
    }
  
if (button1.clicks == 1) { // if button is pressed one time, this turns off the display and ensures the atty is not firing
    display.ssd1306_command(SSD1306_DISPLAYOFF);
    digitalWrite (powerConverter, HIGH);  
    //digitalWrite (ohmPin, LOW);                  // Disables the Ohm Meter circuit  
    }
   
  // Ninja Mode Engage and disengage command
  if(PowerConvFunction == 3) {
  ninjaMode = !ninjaMode; 
  } 
  
  // Display Inversion
  if(PowerConvFunction == 4) {
  invertedDisplay = !invertedDisplay; 
  } 
  
  // Go to sleep and sends it into a sleep state
  if(PowerConvFunction == 5) {
    count = 125;
    }
    
// Fires the converter pin
  if(PowerConvFunction == -1 && button1.depressed == true && ninjaMode == false) {
    switch (noAutofire) { 
    case false: {  //if the noAutofire variable is false, so the atty has been firing less than 12 seconds it uses the command which first turns on the display adds +1 to the countLimit variable, resets the puff timer, reads the power converter output, and turns on the powerconverter, then setst he sleep timer to zero.
     display.ssd1306_command(SSD1306_DISPLAYON);
     totalPuffs ++;
     countLimit ++;
     puffTimer = 0;
     //fireLED = true;
     Vatty = analogRead(atomizerVoltage);
     digitalWrite (powerConverter, LOW);         // Turn on the DC-DC converter output
     count = 0;
     break;
    }
     case true: { // if the atty has been on for over 12 seconds this does the same as the above except turns the converter off to prevent saftey issues.
     display.ssd1306_command(SSD1306_DISPLAYON);
     countLimit ++;
     //fireLED = true;
     Vatty = analogRead(atomizerVoltage);
     digitalWrite (powerConverter, HIGH);         // Turn on the DC-DC converter output
     count = 0;
     break;
     }
    }
  }
// Fires the converter pin, these are the same commands as the above, except in stealth mode so display stays off
   if(PowerConvFunction == -1 && button1.depressed == true && ninjaMode == true) {
    switch (noAutofire) {
    case false: {
     display.ssd1306_command(SSD1306_DISPLAYOFF);
     totalPuffs ++;
     countLimit ++;
     puffTimer = 0;
     //fireLED = true;
     Vatty = analogRead(atomizerVoltage);
     digitalWrite (powerConverter, LOW);         // Turn on the DC-DC converter output
     count = 0;
     break;
    }
     case true: {
     display.ssd1306_command(SSD1306_DISPLAYOFF);
     countLimit ++;
     //fireLED = true;
     Vatty = analogRead(atomizerVoltage);
     digitalWrite (powerConverter, HIGH);         // Turn on the DC-DC converter output
     count = 0;
     break;
     }
    }
   }
   
   
if (invertedDisplay == true) display.invertDisplay(true);
 else  display.invertDisplay(false);
if (countLimit >= 1) puffTimer = countLimit;   // used to store the pufftimer that is displayed while the button is not pressed down until the next press.
if (button1.depressed == false) { // if the button is not being pressed it resets the variable used in calculating the puff timer and autofire safety feature
  countLimit = 0;
  }
           
// Calculation code
 float Avoltage = Vatty * (5.0 / 1023);             // Convert the raw value being read from analog pin 1 into a voltage value
 float Avoltage1 = Avoltage * 2;                     // used to double the voltage input so you can use over a 5v input, needed for the safety of the arduino.
 int Watts = (Avoltage1 * Avoltage1) / R2;            // Convert the power going the atty to watts using voltage squared divided by resistance formula using the value of resistance found above in ohm meter
 float Amps = Avoltage1 / R2;                        // Convert the power going the atty to Amps using voltage divided by resistance formula using the value of resistance found above in ohm meter
 float pTime = puffTimer / 17;                        //converts the pufftimer raw count into seconds, this is an approximation and is fairly accurate but not exact.
 float VTime = totalPuffs / 17;
 float VTime2 = (totalPuffs / 17) / 60;

 // Votlage remaining in battery code
  Vbatt = analogRead(batteryVoltage);               // Reads the voltage of analog pin 2 to determin the voltage going to the battery
  float Bvoltage1 = Vbatt * (5.0 / 1023);      // Convert the raw value being read from analog pin 1 into a voltage value
  float Bvoltage2 = 2 * Bvoltage1;             //doubles the initial battery voltage calculation, required because the battery voltage is read using a voltage divider so is 1/2 what it really is, needed for safety of arduino since it will be over 5 volts
  float Bpercent1 = (Bvoltage2 - 6.4);         // creats a value to use in calculating battery percetn remaining.
  int Bpercent = (Bpercent1 * 100) / 2;        // this is a straight linear calculation and does not account for battery discharge curves so it is more of an idea of whats left than a super accurate percent most accurate between 30-90 percent
 
if (Bvoltage2 >= 8.4) {
  totalPuffs = 0;   
  VTime = 0;
  VTime2 = 0;
}
if (VTime <= 60) VTimeD = VTime;
  else VTimeD = VTime2;
   
// Low Voltage Lockout Code   
  if (Bvoltage2 < 6.39) {                        
    digitalWrite (powerConverter, HIGH);  //Set the Fire pin to off
    // Low Batt Warning Display
    display.clearDisplay(); 
    display.setTextColor(WHITE);
    display.setTextSize(2);
    display.setCursor(22,9);
    display.print("LOW BAT");
    display.setTextColor(WHITE);
    display.setTextSize(2);
    display.setCursor(16,34);
    display.print("RECHARGE");
    display.setTextColor(WHITE);
    display.setTextSize(1);
    display.setCursor(40,51);
    display.print("BV: ");
    display.print(Bvoltage2);
    display.display();
    delay (4000);
    display.ssd1306_command(SSD1306_DISPLAYOFF);
    sleepNow();
    }
    else { //these items below are where you display the information. The text color must be white on a black and white oled or you cant see it. the size is either 1 2 or 4, the dimensions of the text are 6x8, 12x16, 24x32 for each size. the set cursor is where to position the top left corner of the letters. the top left of the screen is 0,0.
     //digitalWrite (ledBattwarn, LOW);
      display.clearDisplay();
      // Watt Meter Display
      display.setTextColor(WHITE);
      display.setTextSize(4);
      display.setCursor(2,0);
      display.print("W: ");
      display.print(Watts);
      // AMP Meter Display
      display.setTextColor(WHITE);
      display.setTextSize(1);
      display.setCursor(2,32);
      display.print("Amps: ");
      display.print(Amps);  
      // Volte Meter Display
      display.setTextColor(WHITE);
      display.setTextSize(1);
      display.setCursor(2,40);
      display.print("AV: ");
      display.print(Avoltage1);
      // Battery Voltage Meter
      display.setTextColor(WHITE);
      display.setTextSize(1);
      display.setCursor(64,40);
      display.print("BV: ");
      display.print(Bvoltage2);
      // OHM Meter Display
      display.setTextColor(WHITE);
      display.setTextSize(1);
      display.setCursor(2,48);
      display.print("Ohms: ");
      display.print(R2);
      //Battery Percentage remaining meter display
      display.setTextColor(WHITE);
      display.setTextSize(1);
      display.setCursor(64,48);
      display.print("Batt %: ");
      display.print(Bpercent);
      display.setTextColor(WHITE);
      display.setTextSize(1);
      display.setCursor(64,32);
      display.print("Psec:");
      display.print(pTime);
      display.setTextColor(WHITE);
      display.setTextSize(1);
      display.setCursor(2,56);
      display.print("VTime:");
      display.print(VTimeD);
      display.setTextColor(WHITE);
      display.setTextSize(1);
      display.setCursor(76,56);
      display.print("Claviger");
      display.display(); 
      count++;
      //countLimit ++;
      }
     
     
    if (count >=125) {  //if the count variable is greater than 125 this turns off the display and puts the module to sleep. This is used when you release the fire button to auto turn off the display and put it to sleep for battery savings.
      display.ssd1306_command(SSD1306_DISPLAYOFF);
      delay(100);     // this delay is needed, the sleep
      count = 0;
      sleepNow();     // sleep function called here
  }}
