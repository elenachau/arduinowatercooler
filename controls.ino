#define RDA 0x80
#define TBE 0x20
// useful for uart ^
//libraries vvv
#include <dht.h> //install the DHTLib library
#include <LiquidCrystal.h>
#include <Stepper.h> // Include the header file
#include <Wire.h>
#include <RTClib.h>
//pointers for uart
volatile unsigned char *myUCSR0A = (unsigned char *)0x00C0;
volatile unsigned char *myUCSR0B = (unsigned char *)0x00C1;
volatile unsigned char *myUCSR0C = (unsigned char *)0x00C2;
volatile unsigned int *myUBRR0 = (unsigned int *)0x00C4;
volatile unsigned char *myUDR0 = (unsigned char *)0x00C6;
//pointers for adc
volatile unsigned char *my_ADMUX = (unsigned char *)0x7C;
volatile unsigned char *my_ADCSRB = (unsigned char *)0x7B;
volatile unsigned char *my_ADCSRA = (unsigned char *)0x7A;
volatile unsigned int *my_ADC_DATA = (unsigned int *)0x78;
//GPIO pointers
volatile unsigned char *port_b = (unsigned char *)0x25;
volatile unsigned char *ddr_b = (unsigned char *)0x24;
volatile unsigned char *pin_b = (unsigned char *)0x23;
volatile unsigned char *port_c = (unsigned char *)0x28;
volatile unsigned char *ddr_c = (unsigned char *)0x27;
volatile unsigned char *pin_c = (unsigned char *)0x26;
volatile unsigned char *port_d = (unsigned char *)0x2B;
volatile unsigned char *ddr_d = (unsigned char *)0x2A;
volatile unsigned char *pin_d = (unsigned char *)0x29;
volatile unsigned char *port_j = (unsigned char *)0x105;
volatile unsigned char *ddr_j = (unsigned char *)0x104;
volatile unsigned char *pin_j = (unsigned char *)0x103;
volatile unsigned char *port_h = (unsigned char *)0x102;
volatile unsigned char *ddr_h = (unsigned char *)0x101;
volatile unsigned char *pin_h = (unsigned char *)0x100;
volatile unsigned char *port_l = (unsigned char *)0x10B;
volatile unsigned char *ddr_l = (unsigned char *)0x10A;
volatile unsigned char *pin_l = (unsigned char *)0x109;
//timer pointers
volatile unsigned char *myTCCR1A = (unsigned char *)0x80;
volatile unsigned char *myTCCR1B = (unsigned char *)0x81;
volatile unsigned char *myTCCR1C = (unsigned char *)0x82;
volatile unsigned char *myTIMSK1 = (unsigned char *)0x6F;
volatile unsigned int *myTCNT1 = (unsigned int *)0x84;
volatile unsigned char *myTIFR1 = (unsigned char *)0x36;
// interrupt pointers
volatile unsigned char *mySREG = (unsigned char *)0x5F;
volatile unsigned char *myEICRA = (unsigned char *)0x69;
volatile unsigned char *myEIMSK = (unsigned char *)0x3D;

/*char daysOfTheWeek[7][12] = {
  "Sunday",
  "Monday",
  "Tuesday",
  "Wednesday",
  "Thursday",
  "Friday",
  "Saturday"
};*/

// elena finished
//  b0 is water sensor enable pin 53
//  water sensor is analog pin 0
//  potentiometer for stepper motor is analog pin 1
//   liquidcrystal is using pb6, pb5, pe3, pg5, pe5, pe4, aka 12, 11, 5, 4, 3, 2
//  stepper motor is 22,23,24,25 aka pa0, pa1, pa2, pa3
//  b1 is humidity/temp enable pin 52
// dht is using pin 10, aka pb4
// fan motor is using pin 14, 15, 16, aka pj1, pj0, ph1 ,dira, enable, dirb respectively



// leds are pl0-pl3 yellow,green, blue, red aka 49-46
// enable button is pd2 19
// reset button is pd3 18
// rtc uses scl and sda that noo ne else can use

//other initializations
RTC_DS1307 rtc; //clock
#define DHT11_PIN 10 //hum/temp
#define STEPS 32 //stepper motor
const int rs = 12, //lcd
          en = 11, d4 = 5, d5 = 4, d6 = 3, d7 = 2, waterPin = 0;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7); //more lcd
Stepper stepper(STEPS, 22, 24, 23, 25); //stepper
dht DHT; //hum/temp
int Pval = 0; //stepper
int potVal = 0; //stepper
volatile int state = 0; // 0 is disabled, 1 is idle, 2 is running, 3 is error
volatile int old = 0;
void setup()
{
   
  state = 0; //initialize
  old = 0;
  *mySREG &= 0b01111111; // turn off global interrupt
  U0init(9600);          // initialize
  adc_init();            // initialize ADC
  *ddr_b |= 0b00000011;  // set b0/b1 to be configurable on/off for water sensor
  *ddr_c &= 0b11111101;  // pc1 is reset button
  *port_b &= 0b11111100; // set b0/b1 to be off by default
  *port_c |= 0b00000010; // pc1 gets pull up resistor
  *ddr_j |= 0b00000010;  // set enable to be output
  *ddr_j |= 0b00000001;  // set dira to be output
  *ddr_h |= 0b00000010;  // set dirb to be output
  *ddr_l |= 0b00001111;  // set pl0-pl3 as output for led
  *port_l &= 0b11110001; // set leds to be off by default
  *port_l |= 0b00000001; // set except yellow :)
 // *port_j &= 0b11111101; // fan is off by default
  *port_d |= 0b00001100; // set d0/d1 to have pull up resistor
  *ddr_d &= 0b11110011;  // set pd0/d1 as input for button

  stepper.setSpeed(1100); // set up stepper
  lcd.begin(16, 2);
  *myEICRA |= 0b10100000; // falling edge mode for interrupt 2/3
  *myEICRA &= 0b10101111; // falling edge mode for interrupt 2/3
  *myEIMSK |= 0b00001100; // turn on interrupt 2/3
  *mySREG |= 0b10000000;  // turn on global interrupt
  lcd.setCursor(0, 0); //initialize lcd as disabled
  lcd.print("Machine is off.");

        //  U0putchar('e');

  if(!rtc.begin()){ //check
  char printarray[18] = "Couldn't find RTC";
    for(int i = 0;i < 18;i++)
    {
      U0putchar(printarray[i]);
    }
  }
}

bool enabled = false;
bool error = true;
bool fanon = false;

/*
int state = 0; 0= disabled 1 = idle 2= enabled 3 = error
bool trigger = 0;
interrupt button -> trigger = 0, if state = 0 then state -> 1
if state != 0, then state -> 0
interrupt
void loop()
{
  if(!trigger && state = 0)
  {
    bla bla bla print the date staate turn leds on/off whatever
    trigger = true;
  }
  else if(!trigger && state = 1)
  { print the date state turn leds on/off whatever
    bla bla bal
    trigger = true;
  }
}
*/
unsigned int watervalue = 0; //store water adc reading
void loop()
{


  if (state != 3) // if NOT ERROR 
  {               // adjust vent position, when no error
    potVal = map(adc_read(1), 0, 1024, 0, 300);
    int potValhBound = potVal+10;
    int potVallBound = potVal-10;
    // Serial.print(potVal);
    // U0putchar('\n');
    // Serial.print(Pval);
    // U0putchar('\n');
    // U0putchar('a');
    // U0putchar('\n');
    if (!(potVallBound < Pval && potValhBound > Pval)) //margin of error so it doesnt change to noise
    {
    if (potVal > Pval)
    {

      stepper.step(5);
    }
    if (potVal < Pval)
    {
      stepper.step(-5);
  
    }
        if (!(potVallBound-30 < Pval && potValhBound+30 > Pval)) //margin of error so it doesnt spam serial monitor
    {
      char printarray[18] = "Vent adjusted at "; 
       for (int i = 0; i < 18; i++)
      {
        U0putchar(printarray[i]); //replacement for serial println
      }
      printTime();
      U0putchar('.');
      U0putchar('\n');
    }
    Pval = potVal;
  }
  }

  switch (state)
  {       // transitions in and out of disabled
  case 0: // disabled
    if (old != 0)
    {
      old = 0;
      char printarray[23] = "Machine turned off at ";

      for (int i = 0; i < 23; i++)
      {
        U0putchar(printarray[i]);
      }
      printTime();
      U0putchar('.');
      U0putchar('\n');
      // turn yellow on turn all others off
      // Serial.println("TURNED OFF AT DISABLE");
      
      *port_l &= 0b11110001;
      *port_l |= 0b00000001;
      *port_b &= 0b11111100; // disable sensors
      lcd.setCursor(0, 0);
      lcd.print("Machine is off. ");
      lcd.setCursor(0, 1);
      lcd.print("                ");
    }
    break;
  default: // any other state
    if (old == 0)
    {
      // old will be set next switch statement
      *port_b |= 0b00000011; // enable sensors
    }
  }
    *mySREG &= 0b01111111; // turn off global interrupt
  my_delay(1);
    *mySREG |= 0b10000000;  // turn on global interrupt

  if (state != 0)                    // if NOT DISABLED,
  {          
                            // read
    *port_b |= 0b00000011; // enable sensors
    int chk = DHT.read11(DHT11_PIN); // read humid/temp
    watervalue = adc_read(waterPin); // read the analog value from sensor
    //Serial.println("a");
  }
// Serial.print(watervalue);
//  U0putchar('\n');
  // monitoring
  if (watervalue < 50 && state != 0 && state != 3) // if water is low and not disabled or error
  {
    state = 3; // set state to error
  }
  if (DHT.temperature >= 23 && state == 1) // if temp is high and state is idle
  {
    state = 2; // set state to enabled
  }
  if (DHT.temperature < 23 && state == 2) // if temp is low and state is enabled
  {
    state = 1; // set state to idle
  }

  switch (state)
  {       // transitions for 1-3 and lcd
  case 0: // disabled
    // transition in previous switch statement
    lcd.setCursor(0, 0);

    lcd.print("Machine is off");
   *port_j &= 0b11111101; // turn off fan (might already be off oh well)

    break;
  case 1: // idle
    if (old != 1)
    {
      old = 1;
      char printarray[33] = "Machine idled/fan turned off at ";
      for (int i = 0; i < 33; i++)
      {
        U0putchar(printarray[i]);
      }
      printTime();
      U0putchar('.');
      U0putchar('\n');
      // Serial.println("TURNED OFF AT IDLE");
      *port_j &= 0b11111101; // turn off fan (might already be off oh well)
      *port_l &= 0b11110010; // turn on green
      *port_l |= 0b00000010;
    }
    lcd.setCursor(0, 0);
    lcd.print("Temp = ");
    lcd.print(DHT.temperature);
    lcd.print("   ");
    lcd.setCursor(0, 1);
    lcd.print("Hmty = ");
    lcd.print(DHT.humidity);
    lcd.print("   ");
    break;
  case 2: // running
    if (old != 2)
    {
      old = 2;
      char printarray[34] = "Machine enabled/fan turned on at ";
      for (int i = 0; i < 34; i++)
      {
        U0putchar(printarray[i]);
      }
      printTime();
      U0putchar('.');
      U0putchar('\n');
      // Serial.println("TURNED ON ");
      *port_j |= 0b00000001; // set direction fan 1
      *port_h &= 0b11111101; // set direction fan 2
      *port_j |= 0b00000010; // turn on fan
      *port_l &= 0b11110100; // turn on blue
      *port_l |= 0b00000100;
    }
    lcd.setCursor(0, 0); //lcd stuff
    lcd.print("Temp = ");
    lcd.print(DHT.temperature);
    lcd.print("   ");
    lcd.setCursor(0, 1);
    lcd.print("Hmty = ");
    lcd.print(DHT.humidity);
    lcd.print("   ");
    break;
  case 3: // error
    if (old != 3)
    {
      old = 3;
      char printarray[33] = "Machine error at ";
      for (int i = 0; i < 33; i++)
      {
        U0putchar(printarray[i]);
      }
      printTime();
      U0putchar('.');
      U0putchar('\n');
      // Serial.println("TURNED OFF AT ERROR");
      *port_j &= 0b11111101; // turn off fan (might already be off oh well)
      *port_l &= 0b11111000; // turn on red
      *port_l |= 0b00001000;
      lcd.setCursor(0, 0);
      lcd.print("Water level low.");
      lcd.setCursor(0, 1);
      lcd.print("                ");
    }
    break;
  }

     *mySREG &= 0b01111111; // turn off global interrupt
  my_delay(300);
    *mySREG |= 0b10000000;  // turn on global interrupt
}

ISR(INT2_vect) //set button is pressed
{
//  U0putchar('d');
//  U0putchar('\n');
  if (state == 0)
  {
    old = 0;
    state = 1;
  }
  else
  {
    old = state;
    state = 0;
  }
}
ISR(INT3_vect) //reset button is pressed
{
//   U0putchar('e');
//  U0putchar('\n');
    if (state == 0)
  {
//wow look, nothing!
  }
  else
  {
    old= state;
    state = 1;
  }
}

void printTime() //reads and prints the time
{
  DateTime now = rtc.now(); //crashes program if called too often
  int year = now.year();
  int month = now.month();
  int day = now.day();
  int hour = now.hour();
  int minute = now.minute();
  int second = now.second();
  char time[22] = {
      month / 10 + '0',
      month % 10 + '0',
      '/',
      day / 10 + '0',
      day % 10 + '0',
      '/',
      (year / 1000) + '0',
      (year % 1000 / 100) + '0',
      (year % 100 / 10) + '0',
      (year % 10) + '0',
      ' ',
      'a',
      't',
      ' ',
      hour / 10 + '0',
      hour % 10 + '0',
      ':',
      minute / 10 + '0',
      minute % 10 + '0',
      ':',
      second / 10 + '0',
      second % 10 + '0',
  };
  for (int i = 0; i < 22; i++)
  {
    U0putchar(time[i]);
  }
}
void U0init(int U0baud) //serial.begin
{
  unsigned long FCPU = 16000000;
  unsigned int tbaud;
  tbaud = (FCPU / 16 / U0baud - 1);
  *myUCSR0A = 0x20;
  *myUCSR0B = 0x18;
  *myUCSR0C = 0x06;
  *myUBRR0 = tbaud;
}

void adc_init() //initialize adc
{
  // setup the A register
  *my_ADCSRA |= 0b10000000; // set bit   7 to 1 to enable the ADC
  *my_ADCSRA &= 0b11011111; // clear bit 6 to 0 to disable the ADC trigger mode
  *my_ADCSRA &= 0b11110111; // clear bit 5 to 0 to disable the ADC interrupt
  *my_ADCSRA &= 0b11111000; // clear bit 0-2 to 0 to set prescaler selection to slow reading
  // setup the B register
  *my_ADCSRB &= 0b11110111; // clear bit 3 to 0 to reset the channel and gain bits
  *my_ADCSRB &= 0b11111000; // clear bit 2-0 to 0 to set free running mode
  // setup the MUX Register
  *my_ADMUX &= 0b01111111; // clear bit 7 to 0 for AVCC analog reference
  *my_ADMUX |= 0b01000000; // set bit   6 to 1 for AVCC analog reference
  *my_ADMUX &= 0b11011111; // clear bit 5 to 0 for right adjust result
  *my_ADMUX &= 0b11100000; // clear bit 4-0 to 0 to reset the channel and gain bits
}

unsigned int adc_read(unsigned char adc_channel_num) //adc read
{
  // clear the channel selection bits (MUX 4:0)
  *my_ADMUX &= 0b11100000;
  // clear the channel selection bits (MUX 5)
  *my_ADCSRB &= 0b11110111;
  // set the channel number
  if (adc_channel_num > 7)
  {
    // set the channel selection bits, but remove the most significant bit (bit 3)
    adc_channel_num -= 8;
    // set MUX bit 5
    *my_ADCSRB |= 0b00001000;
  }
  // set the channel selection bits
  *my_ADMUX += adc_channel_num;
  // set bit 6 of ADCSRA to 1 to start a conversion
  *my_ADCSRA |= 0x40;
  // wait for the conversion to complete
  while ((*my_ADCSRA & 0x40) != 0)
    ;
  // return the result in the ADC data register
  return *my_ADC_DATA;
}

unsigned char U0kbhit() //reading from serial monitor
{
  return *myUCSR0A & RDA;
}
unsigned char U0getchar() //reading from serial monitor
{
  return *myUDR0;
}
void U0putchar(unsigned char U0pdata) //outputting to serial monitor
{
  while ((*myUCSR0A & TBE) == 0)
    ;
  *myUDR0 = U0pdata;
}

// void U0putstr(un)

void my_delay(unsigned int freq) //takes frequency to make a delay
{
  // calc period
  double period = 1.0 / double(freq);
  // 50% duty cycle
  double half_period = period / 2.0f;
  // clock period def
  double clk_period = 0.0000000625;
  // calc ticks
  unsigned int ticks = half_period / clk_period;
  // stop the timer
  *myTCCR1B &= 0xF8;
  // set the counts
  *myTCNT1 = (unsigned int)(65536 - ticks);
  // start the timer
  *myTCCR1B |= 0b00000001;
  // wait for overflow
  while ((*myTIFR1 & 0x01) == 0)
    ; // 0b 0000 0000
  // stop the timer
  *myTCCR1B &= 0xF8; // 0b 0000 0000
  // reset TOV
  *myTIFR1 |= 0x01;
}