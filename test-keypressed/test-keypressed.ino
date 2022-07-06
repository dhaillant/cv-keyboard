/*
  Test Key pressed

  Turns LEDs on and off based on the keys pressed

*/


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



// Other Input/outputs pin mappings
#define GATE 8
#define TRIG 9

// Connection to the DAC chip
#define CS   10
#define MOSI 11
#define SCK  13


// LED state (Rows must be LOW to activate the corresponding LEDs)
#define LON  0
#define LOFF 1


// LED switching macros (the LED are active on LOW)
#define LED_ON(led_pin)  digitalWrite(led_pin, LOW)
#define LED_OFF(led_pin) digitalWrite(led_pin, HIGH)




void light_led(uint8_t state = LOFF, uint8_t col = 0, uint8_t row = 0);

void read_keyboard(void);

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


  // debug through serial communication
  Serial.begin(115200);
  Serial.println("Hello Keyboard");
  Serial.println("Test Key pressed (20220630)");

  start_sequence();
}


void loop()
{
  // First, scan keyboard
  read_keyboard();


  // Then display results

  
  #define FALSE 0
  #define TRUE 1
  bool is_there_light = FALSE;
  
  for (uint8_t c = 0; c < N_COLS; c++)        // Scan the columns sequentially
  {
    digitalWrite(Column_pins[c], HIGH);       // activate the COLUMN
    is_there_light = FALSE;

    for (uint8_t r = 0; r < N_ROWS; r++)
    {
      if (LED_states[c][r] == 1)
      {
        digitalWrite(LED_row_pins[r], LON);   // turn the LED ROW on
        is_there_light = TRUE;
      }
    }
    
    // do something here to keep LEDs ON
    //if (is_there_light == TRUE)
    //{
      delay(1);
    //}
    
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
    delay(50);
  }
}

void light_led(uint8_t state = LOFF, uint8_t col, uint8_t row)
{
  digitalWrite(Column_pins[col], !state);   // turn the COLUMN on or off
  digitalWrite(LED_row_pins[row], state);   // turn the ROW on or off
}


void read_keyboard(void)
{
  uint8_t key_state = 0;

  // Scan the columns sequentially
  for (uint8_t c = 0; c < N_COLS; c++)
  {
    digitalWrite(Column_pins[c], HIGH);   // activate the COLUMN

    // Then scan the rows
    for (uint8_t r = 0; r < N_ROWS; r++)
    {
      key_state = digitalRead(SW_row_pins[r]);  // read ROW (key state)

      // if the current scanned key is pressed, and the previous state of that key was NOT pressed, then
      if (key_state == HIGH && key_states[c][r] == LOW)
      {
        // immediately update the status of the corresponding LED for visual feedback
        LED_states[c][r] = !LED_states[c][r];

        // do something else HERE if needed...

        // and debounce a little...
        delay(1);
      }

      // store the new state
      key_states[c][r] = key_state;
    }
    digitalWrite(Column_pins[c], LOW);    // turn the COLUMN off
  }

}
