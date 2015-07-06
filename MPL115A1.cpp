
#include <MPL115A1.h>
#include <Arduino.h>


#define delay_clk() delayMicroseconds(40); //__asm__("nop\n\t")
#define delay_device() delayMicroseconds(40); //__asm__("nop\n\t""nop\n\t")

MPL115A1_Class::MPL115A1_Class(int data_in_pin, 
                               int data_out_pin, 
                               int data_clock_pin, 
                               int select_pin, 
                               int shutdown_pin)
{
  int i;
  unsigned char calibration_data[8];
  
  _data_in_pin    = data_in_pin;
  _data_out_pin   = data_out_pin;
  _data_clock_pin = data_clock_pin;
  _select_pin     = select_pin;
  _shutdown_pin   = shutdown_pin;
  
  pinMode(_data_in_pin,    INPUT);
  pinMode(_data_out_pin,   OUTPUT);
  pinMode(_data_clock_pin, OUTPUT);
  pinMode(_select_pin,     OUTPUT);
  pinMode(_shutdown_pin,   OUTPUT);
  
  digitalWrite(_shutdown_pin, 1);
  
  /* The calibration data for the device needs to be read 
     and saved.
   */
  
  select_device();
  
  for (i = 0; i < 8; i++)
  {
    _SPI_IO(calibration_addresses[i]);
    calibration_data[i] = _SPI_IO(0x00);
    
    Serial.print(calibration_addresses[i], HEX);
    Serial.print(", ");
    Serial.println(calibration_data[i], HEX);
  }
  _SPI_IO(0x00);
  
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
  
  Serial.println(a0_coeff,HEX);
  Serial.println(b1_coeff,HEX);
  Serial.println(b2_coeff,HEX);
  Serial.println(c12_coeff,HEX);
  
}


MPL115A1_Class::~MPL115A1_Class()
{

}

void MPL115A1_Class::start_conversion()
{
  select_device();
  
  _SPI_IO(START_CONVERSION);
  _SPI_IO(0x00);
  
  deselect_device();
  
}


void MPL115A1_Class::read_conversion()
{
  int i;

  unsigned char temp_read[4];
  
  select_device();
  
  /* Send the addresses that will be returned in the
     subsequent exhchange. The last "send" is only 
     needed to get the previous responce.
   */
  (data_addresses[0]);
  temp_read[0] = _SPI_IO(data_addresses[1]);
  temp_read[1] = _SPI_IO(data_addresses[2]);
  temp_read[2] = _SPI_IO(data_addresses[3]);
  temp_read[3] = _SPI_IO(0x00);

  Tadc = (temp_read[2] << 8) | temp_read[3];
  Tadc = (Tadc >> 6) & 0x03FF;
  
  Padc = (temp_read[0] << 8) | temp_read[1];
  Padc = (Padc >> 6) & 0x03FF;
  
  deselect_device();

}

void MPL115A1_Class::select_device()
{
  /* Set the clock pin low and then wait the needed amount
     of time before selecting the chip.
   */

  digitalWrite(_data_clock_pin,0);
  
  delay_device();

  digitalWrite(_select_pin,0);

  delay_device();

}

void MPL115A1_Class::deselect_device()
{

  /* Set the clock pin low and then wait the needed amount of
     time before deselecting the chip.
   */
  digitalWrite(_data_clock_pin,0);
  
  delay_device();
  
  digitalWrite(_select_pin,1);
  digitalWrite(_data_out_pin, 0);

}

/* This function will use the values read from the device and perform the
   corrections specified in the datasheet. This is done in float space for
   simplicity and certainly not speed.
 */

int MPL115A1_Class::getPressure()
{
  double offset_a = (115.0 - 50.0) / 1023.0;
  double offset_y = 50.0;
  
  Pcomp = a0_coeff + ((b1_coeff + c12_coeff * (double)Tadc) * (double)Padc) + (b2_coeff * (double)Tadc);
  
  pressure = (Pcomp * offset_a) + offset_y;
  
  int return_value = (int)(pressure * 100.0);
  
  return return_value;
}

/* This fuction will use the values read from the device and perform a
   correction that is derived from information in the datasheet. There 
   is no specified temperature accuracy and it can be quite far off.
   Need to look more into how this would be calibrated.
 */

int MPL115A1_Class::getTemperature()
{
  return (int)((25.0 + ((double)Tadc - 472.0) / -5.35)*100.0);
}

/* This function will read and write to the SPI bus. It is intended
   to be called within a select_device and deselect_device block of
   code. Those functions have the job of waking up and selecting the
   device that this class is instansiated against.
 */

unsigned char MPL115A1_Class::_SPI_IO(unsigned char output)
{
  int i = 0;
  
  unsigned char input_read   = 0;
  unsigned char output_write = 0;
  unsigned char input        = 0;
  
  for (i = 0; i < 8; i++)
  {
    /* Set the data output pin and wait 62.5ns. The data
       sheet says 30ns is required but the smallest time
       measurable on the microcontroller is 62.5ns.
     
       So, using 62.5ns does not work at all... I do not have
       a scope so I can only guess as to what is going on.
       Using a longer delay of around 40us seems to be 
       about as fast as the bus can be clocked. This might
       be the breadboard or the wires between the breadboard
       and the Arduino, or something else completely. Maybe 
       this will inspire the purchase of a oscilloscope.
     */
    
    /* MSB first */
    output_write = (output & 0x80) > 0 ? 1 : 0;
    output = output << 1;
    
    /* LSB first
    output_write = (output & 0x01) > 0 ? 1 : 0;
    output = output >> 1;
    */
    
    digitalWrite(_data_out_pin, output_write);
    
    delay_clk();
    
    /* Set the clock pin high and wait the required amount
       of time for clock high. The datasheet says the clock
       high time is 62.5ns.
     */
    digitalWrite(_data_clock_pin, 1);
    delay_clk();
    digitalWrite(_data_out_pin,0);
  
    /* The slave device can also be outputing data while the
       uController is outputing data and clock pulses So, shift 
       the input value and read a new bit into the LSB of the 
       input value.
     */
    input = input << 1;
    input_read = digitalRead(_data_in_pin);
    input = input | (input_read & 0x01);

    /* Set the clock pin low and wait the needed amount of
     time for the device output to stabilize.
     The data sheet specifies this as 30ns but the smallest
     amount of time measurable is 62.5ns.
     */
    digitalWrite(_data_clock_pin, 0);
    
    /* I think this delay can be removed. Need to experiment
       with this not here and see what happens.
     */
    delay_clk();


  }
  
  
  
  return input;
  
}



