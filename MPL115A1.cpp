
#include <MPL115A1.h>
#include <Arduino.h>

#define delay_device() delayMicroseconds(40)
#define MPL115A1_READ_DELAY_TIME 10


typedef enum MPL115A1_State
{
  StartConversion,
  WaitToRead,
  ReadConversion,
};

MPL115A1::MPL115A1(Software_SPI* bus, 
                   int select_pin, 
                   int shutdown_pin)
{

  _bus            = bus;
  _select_pin     = select_pin;
  _shutdown_pin   = shutdown_pin;
  
  startConvertTime = 0;
  runState = StartConversion;
  
}

MPL115A1::~MPL115A1()
{

}

char MPL115A1::begin()
{
  int i;
  unsigned char calibration_data[8];  
  
  pinMode(_select_pin,     OUTPUT);
  pinMode(_shutdown_pin,   OUTPUT);
  
  digitalWrite(_shutdown_pin, 1);
  
  /* The calibration data for the device needs to be read 
   and saved.
   */
  
  select_device();
  
  for (i = 0; i < 8; i++)
  {
    _bus->SPI_Send_Receive(calibration_addresses[i]);
    calibration_data[i] = _bus->SPI_Send_Receive(0x00);
  }
  _bus->SPI_Send_Receive(0x00);
  
  deselect_device();
  
  a0_int    = ((int)calibration_data[0]) << 8 | ((int)calibration_data[1]);
  a0_coeff  = a0_int / 8.0;        /* 2 ^ 3 */
  b1_int    = ((int)calibration_data[2]) << 8 | ((int)calibration_data[3]);
  b1_coeff  = b1_int / 8192.0;     /* 2 ^ 13 */ 
  b2_int    = ((int)calibration_data[4]) << 8 | ((int)calibration_data[5]);
  b2_coeff  = b2_int / 16384.0;    /* 2 ^ 14 */
  c12_int   = ((int)calibration_data[6]) << 8 | ((int)calibration_data[7]);
  c12_int   = c12_int >> 2;
  c12_coeff = c12_int / 4194304.0; /* 2 ^ 22 */
  
  return 0;
}

void MPL115A1::run()
{
  switch (runState)
  {
    case StartConversion:
      startConvertTime = millis();
      start_conversion();
      break;
    case ReadConversion:
      read_conversion();
      break;
    default:
      break;
  }
  
  switch (runState)
  {
    case StartConversion:
      runState = WaitToRead;
      break;
    case WaitToRead:
      if (startConvertTime + MPL115A1_READ_DELAY_TIME < millis())
      {
        runState = ReadConversion;
      }
      break;
    case ReadConversion:
      runState = StartConversion;
      break;
  }
}

void MPL115A1::start_conversion()
{
  select_device();
  
  _bus->SPI_Send_Receive(START_CONVERSION);
  _bus->SPI_Send_Receive(0x00);
  
  deselect_device();
  
}


void MPL115A1::read_conversion()
{
  int i;

  unsigned char temp_read[4];
  
  select_device();
  
  /* Send the addresses that will be returned in the
     subsequent exchange. The last "send" is only 
     needed to get the previous response.
   */
  _bus->SPI_Send_Receive(data_addresses[0]);
  temp_read[0] = _bus->SPI_Send_Receive(data_addresses[1]);
  temp_read[1] = _bus->SPI_Send_Receive(data_addresses[2]);
  temp_read[2] = _bus->SPI_Send_Receive(data_addresses[3]);
  temp_read[3] = _bus->SPI_Send_Receive(0x00);

  Tadc = (temp_read[2] << 8) | temp_read[3];
  Tadc = (Tadc >> 6) & 0x03FF;
  
  Padc = (temp_read[0] << 8) | temp_read[1];
  Padc = (Padc >> 6) & 0x03FF;
  
  deselect_device();

}

void MPL115A1::select_device()
{
  /* Set the clock pin low and then wait the needed amount
     of time before selecting the chip.
   */

  _bus->reset();
  
  delay_device();

  digitalWrite(_select_pin,0);

  delay_device();

}

void MPL115A1::deselect_device()
{

  /* Set the clock pin low and then wait the needed amount of
     time before deselecting the chip.
   */
  _bus->reset();
  
  delay_device();
  
  digitalWrite(_select_pin,1);


}

/* This function will use the values read from the device and perform the
   corrections specified in the data sheet. This is done in float space for
   simplicity and certainly not speed.
 */

long MPL115A1::getPressure()
{
  double offset_a = (115.0 - 50.0) / 1023.0;
  double offset_y = 50.0;
  
  Pcomp = a0_coeff + ((b1_coeff + c12_coeff * (double)Tadc) * (double)Padc) + (b2_coeff * (double)Tadc);
  
  pressure = (Pcomp * offset_a) + offset_y;
  
  long return_value = (pressure * 1000.0);
  
  return return_value;
}

/* This function will use the values read from the device and perform a
   correction that is derived from information in the data sheet. There 
   is no specified temperature accuracy and it can be quite far off.
   Need to look more into how this would be calibrated.
 */

int MPL115A1::getTemperature()
{
  return (int)((25.0 + ((double)Tadc - 514.0) / -5.35)*10.0);
}
