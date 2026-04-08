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
  float fval[2];
  int32_t ival[2];
} data_t;
#define DATA_SZ sizeof(data_t)

data_t dat = { 0 };
uint32_t baseline = 10;
uint32_t delta = 50;

uint32_t n = 0;
bool toggle_OUT = false;

typedef struct
{
  uint32_t ival;
} read_data_t;
read_data_t input_data = { 0 };

//
void setup()
{
  Serial.begin(115200);
  // initialize GPIO 2 as an output.
  pinMode(OUT, OUTPUT);

  strcpy(dat.id, "ESPD");
  dat.fval[0] = 50.0f;
  dat.fval[1] = 20.0f;
  dat.ival[0] = 1;
  dat.ival[1] = 0;
}

// the loop function runs over and over again forever
void loop()
{
  // read from usb
  if (Serial.available())
  {
    Serial.readBytes((char *)&input_data, sizeof(read_data_t));
    dat.ival[0] = input_data.ival;
    
  }

  if (n > 200)
  {
    dat.ival[1] = (toggle_OUT ? 5 : 0);
    delta = (toggle_OUT ? 1 : 0);
    toggle_OUT = !toggle_OUT;

    n = 0;
    digitalWrite(OUT, (uint8_t)toggle_OUT);
  }
  
  dat.fval[0] = 50.0f + delta * 50.0f + baseline;
  dat.fval[1] = 20.0f + delta * 20.0f + baseline * 0.5;

  uint8_t *bytes = (uint8_t *)&dat;
  Serial.write(bytes, DATA_SZ);
  
  delay(10);

  n++;

}



