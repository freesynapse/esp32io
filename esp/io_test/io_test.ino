/*---------------------------------------------------------------------------------------
|  Date:       2026-02-23
|  Auth:       Måns Edström
|  Desc.:      Testing out the ESP32 microcontroller's I/O capabilities by serial port.
|
----------------------------------------------------------------------------------------*/

#define OUT     4
#define C_READ

// cmd key codes
#define KEY_S   83
#define KEY_W   87



typedef struct data_t_
{
  char id[4];
  float fval;
  uint32_t ival;
} data_t;
#define DATA_SZ sizeof(data_t)

data_t dat;
uint32_t baseline = 10;
uint32_t delta = 50;

uint32_t n = 0;
bool toggle_OUT = false;

char input_cmd = 0;

//
void setup()
{
  Serial.begin(115200);
  // initialize GPIO 2 as an output.
  pinMode(OUT, OUTPUT);

  strcpy(dat.id, "ESPD");
  dat.fval = 50.0f;
  dat.ival = 0;

}

// the loop function runs over and over again forever
void loop()
{
  // read from usb
  if (Serial.available())
  {
    input_cmd = Serial.read();
    switch (input_cmd)
    {
      case KEY_W:
        if (baseline < 90)
          baseline += 10; 
        break;
      
      case KEY_S:
        if (baseline > 10)
          baseline -= 10;
        break;
      
      default:
        break;
    }
  }

  if (n > 200)
  {
    dat.ival = (dat.ival + 1) % 100;
    delta = (toggle_OUT ? 50 : 0);
    toggle_OUT = !toggle_OUT;

    n = 0;
    digitalWrite(OUT, (uint8_t)toggle_OUT);
  }
  
  dat.fval = delta + baseline;

  #ifdef C_READ
  uint8_t *bytes = (uint8_t *)&dat;
  Serial.write(bytes, DATA_SZ);
  #else
  Serial.println(val);
  #endif

  delay(10);

  n++;

}



