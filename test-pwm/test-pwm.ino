/*
  Test PWM

  Outputs a dual PWM signal according to key pressed (and octave selected)

*/

#include "MyPWM.h" // dual PWM 16 bit via D9 and D10

/*
  The Keyboard is made of a matrix of 4 columns and 3 rows.
  
  Note  Row Col
  C     0   0
  C#    0   1
  D     0   2
  D#    0   3
  E     1   0
  F     1   1
  F#    1   2
  G     1   3
  G#    2   0
  A     2   1
  A#    2   2
  B     2   3

  Extra       Row Col
  Btn 1/LED 1 0   4
  LED 2       1   4
  LED 3       2   4

  To scan the keyboard (or light the corresponding LEDs):
  
  1) Power Columns sequentially
  10000
  01000
  00100
  00010
  00001
  
  Then
  
  2) Read Switch Rows (SRx)
  1 = closed switch (pushed)
  0 = open switch
  
  2 bis) Write on Led Rows (LRx)
  1 = LED is off
  0 = LED is on
*/


// Columns pin mapping
#define COL0_PIN A0
#define COL1_PIN A1
#define COL2_PIN A2
#define COL3_PIN A3
#define COL4_PIN A4

#define N_COLS 5
const uint8_t Column_pins[N_COLS] = {COL0_PIN, COL1_PIN, COL2_PIN, COL3_PIN, COL4_PIN};

// Key Rows pin mapping
#define SR0_PIN  2
#define SR1_PIN  3
#define SR2_PIN  4

// LEDs Rows pin mapping
#define LR0_PIN  5
#define LR1_PIN  6
#define LR2_PIN  7

#define N_ROWS 3
const uint8_t LED_row_pins[N_ROWS] = {LR0_PIN, LR1_PIN, LR2_PIN};
const uint8_t SW_row_pins[N_ROWS]  = {SR0_PIN, SR1_PIN, SR2_PIN};


// array to store the state of each individual LED, sorted by colmuns an rows. Columns and Rows are NOT pin numbers, only logical indexes (0 -> Max-1)
bool LED_states[N_COLS][N_ROWS];

// array to store the state of each individual switch, sorted by colmuns an rows. Columns and Rows are NOT pin numbers, only logical indexes (0 -> Max-1)
bool key_states[N_COLS][N_ROWS];


// only one key active at a time
uint8_t active_key_col = 0;
uint8_t active_key_row = 0;

// current settings:
uint8_t octave = 0;
uint8_t semitone = 0;

/*
 * 0xFFFF = 65535 is maximum output value which is 5V
 * 
 * 1 octave = 5V/5 -> 65535 / 5 = 13107 (0x3333)
 * 1 semitone = 1 / 12 -> 13107 / 12 = 1092.25 (~0x0444)
 */

const uint16_t pwm_semitone_step = 0x0444;

const uint8_t semitones_matrix[4][3] = {
  {0, 4, 8},    // C  E  G#
  {1, 5, 9},    // C# F  A
  {2, 6, 10},   // D  F# A#
  {3, 7, 11}    // D# G  B
};

// pwm output = semitones_matrix x pwm_semitone_step


// Other Input/outputs pin mappings
#define GATE_pin 8
//#define TRIG 9

// Connection to the DAC chip
//#define CS   10
//#define MOSI 11
//#define SCK  13


// LED state (Rows must be LOW to activate the corresponding LEDs)
#define LON  0
#define LOFF 1


// LED switching macros (the LED are active on LOW)
#define LED_ON(led_pin)  digitalWrite(led_pin, LOW)
#define LED_OFF(led_pin) digitalWrite(led_pin, HIGH)




void light_led(uint8_t state = LOFF, uint8_t col = 0, uint8_t row = 0);

bool read_keyboard(void);

void setup()
{
  // intialise states of LEDS and switches to 0 (logic OFF)
  for (uint8_t i = 0; i < N_COLS; i++)
  {
    for (uint8_t j = 0; j < N_ROWS; j++)
    {
      LED_states[i][j] = 0;
      key_states[i][j] = 0;
    }
  }

  // Keyboard/LED Matrix Columns are powered on demand. Define the corresponding pins as Outputs
  for (uint8_t i = 0; i < N_COLS; i++)
  {
    pinMode(Column_pins[i], OUTPUT);
  }

  for (uint8_t i = 0; i < N_ROWS; i++)
  {
    // Keyboard's Rows are scanned sequentially to detect key presses. The corresponding pins are Inputs
    // a pulldown resistor is hardwired on board
    pinMode(SW_row_pins[i], INPUT);

    // LED matrix Rows. Corresponding pins are Outputs
    pinMode(LED_row_pins[i], OUTPUT);

    // by default switch off the LEDs (LEDs active on LOW)
    LED_OFF(LED_row_pins[i]);
  }

  MyPWM::init();

  // debug through serial communication
  Serial.begin(115200);
  Serial.println("Hello Keyboard");
  Serial.println("Test PWM (20220630)");

  start_sequence();
}


void loop()
{
  // First, scan keyboard
  if (read_keyboard() == true)
  {
    // activate gate output
    digitalWrite(GATE_pin, HIGH);
    //Serial.println("G1");
  }
  else
  {
    // DEactivate gate output
    digitalWrite(GATE_pin, LOW);
    //Serial.println("G0");
  }

  // Then display results

  for (uint8_t c = 0; c < N_COLS; c++)        // Scan the columns sequentially
  {
    digitalWrite(Column_pins[c], HIGH);       // activate the COLUMN

    for (uint8_t r = 0; r < N_ROWS; r++)
    {
      if (LED_states[c][r] == 1)
      {
        digitalWrite(LED_row_pins[r], LON);   // turn the LED ROW on
      }
    }
    
    // do something here to keep LEDs ON
    delay(2);
    
    for (uint8_t r = 0; r < N_ROWS; r++)
    {
      if (LED_states[c][r] == 1)
      {
        digitalWrite(LED_row_pins[r], LOFF);   // turn the LED ROW off
      }
    }

    digitalWrite(Column_pins[c], LOW);         // finally, turn the COLUMN off
  }
}


void start_sequence(void)
// just for showing off...
{
  uint8_t seq[15][2] = {
    {0, 0},   // C
    {1, 0},   // C#
    {2, 0},   // D
    {3, 0},   // D#
    {0, 1},   // E
    {1, 1},   // F
    {2, 1},   // F#
    {3, 1},   // G
    {0, 2},   // G#
    {1, 2},   // A
    {2, 2},   // A#
    {3, 2},   // B
    {4, 1},   // "D12"
    {4, 0},   // "D11"
    {4, 2}    // "D13"
  };

  for (uint8_t i = 0; i < 15; i++)
  {
    light_led(LON, seq[i][0], seq[i][1]);
    delay(10);
    light_led(LOFF, seq[i][0], seq[i][1]);
    delay(100);
  }
}

void light_led(uint8_t state = LOFF, uint8_t col, uint8_t row)
{
  digitalWrite(Column_pins[col], !state);   // turn the COLUMN on or off
  digitalWrite(LED_row_pins[row], state);   // turn the ROW on or off
}


bool read_keyboard(void)
{
  uint8_t key_state = 0;
  bool a_key_is_pressed = false;

  // Scan the columns sequentially
  for (uint8_t c = 0; c < N_COLS; c++)
  {
    digitalWrite(Column_pins[c], HIGH);   // activate the COLUMN

    // Then scan the rows
    for (uint8_t r = 0; r < N_ROWS; r++)
    {
      key_state = digitalRead(SW_row_pins[r]);  // read ROW (key state)

      if (key_state == HIGH)        // if a key is pressed, no matter how it was previously, then
      {
        a_key_is_pressed = true;    // keep track of it
      }
      
      // if the current scanned key is pressed, and the previous state of that key was NOT pressed, then
      if (key_state == HIGH && key_states[c][r] == LOW)
      {
        // immediately update the status of the corresponding LED for visual feedback
        //LED_states[c][r] = !LED_states[c][r]; // TOGGLE

        // if the current column is not Col4 (the one with the "Mode" button)
        if (c != 4)
        {
          LED_states[active_key_col][active_key_row] = LOW;   // shutdown previous LED
          // keep track of the new key
          active_key_col = c;
          active_key_row = r;
          LED_states[active_key_col][active_key_row] = HIGH;   // activate new LED

          // update CV output HERE
          semitone = semitones_matrix[c][r];
          Serial.print("semitone=");
          Serial.println(semitone);
        }
        else
        {
          // "Mode" button is pressed
          octave++;
          if (octave > 4) octave = 0;

          Serial.print("octave=");
          Serial.println(octave);
        }
        Serial.print(" pwm=");
        Serial.println((octave * ((pwm_semitone_step * 12) + 3)) + (semitone * pwm_semitone_step));

        MyPWM::write((octave * ((pwm_semitone_step * 12) + 3)) + (semitone * pwm_semitone_step)); // write result to DAC

        // do something else HERE if needed...

        // and debounce a little...
        delay(1);
      }

      // store the new state
      key_states[c][r] = key_state;
    }
    digitalWrite(Column_pins[c], LOW);    // turn the COLUMN off
  }

  return a_key_is_pressed;
}
