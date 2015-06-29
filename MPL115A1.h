
#ifndef MPL115A1_H
#define MPL115A1_H

#define CONVERSION_WAIT_TIME_MS       3

#define START_CONVERSION           0x24

#define READ_PRESSURE_HIGH_BYTE    0x80
#define READ_PRESSURE_LOW_BYTE     0x82
#define READ_TEMPERATURE_HIGH_BYTE 0x84
#define READ_TEMPERATURE_LOW_BYTE  0x86

#define READ_A0_HIGH_BYTE          0x88
#define READ_A0_LOw_BYTE           0x8A
#define READ_B1_HIGH_BYTE          0x8C
#define READ_B1_LOW_BYTE           0x8E
#define READ_B2_HIGH_BYTE          0x90
#define READ_B2_LOW_BYTE           0x92
#define READ_C12_HIGH_BYTE         0x94
#define READ_C12_LOW_BYTE          0x96

static const unsigned char calibration_addresses[] = {READ_A0_HIGH_BYTE,
                                                      READ_A0_LOw_BYTE,
                                                      READ_B1_HIGH_BYTE,
                                                      READ_B1_LOW_BYTE,
                                                      READ_B2_HIGH_BYTE,
                                                      READ_B2_LOW_BYTE,
                                                      READ_C12_HIGH_BYTE,
                                                      READ_C12_LOW_BYTE};

static const unsigned char data_addresses[] = {READ_PRESSURE_HIGH_BYTE,
                                               READ_PRESSURE_LOW_BYTE,
                                               READ_TEMPERATURE_HIGH_BYTE,
                                               READ_TEMPERATURE_LOW_BYTE};

class MPL115A1_Class
{
public:
  MPL115A1_Class(int data_in_pin, 
                 int data_out_pin, 
                 int data_clock_pin, 
                 int select_pin, 
                 int shutdown_pin);

  ~MPL115A1_Class();
  
  void start_conversion();
  void read_conversion();
  void read_conversion_int();
  
  void select_device();
  void deselect_device();
  
  int getPressure();
  int getTemperature();
  
private:
  
  int _data_in_pin;
  int _data_out_pin;
  int _data_clock_pin;
  int _select_pin;
  int _shutdown_pin;
  
  double pressure;
  double temperature;
  
  double Pcomp; /* The datasheet defines how to compute the compensated pressure. */
  double Tcomp; /* There is no mention of a compensated temperature in the datasheet.
                   I am hoping that this value can also be used. 
                 */
  
  /* The datasheet for the part describes how these values are
   stored on the device. For the purpose of this library the
   values will be stored so that any "unused" bits are at the
   most significant bit end of the value. This is different 
   than the internal storage on the device but it will make
   the code simpler for the microcontroller.
   */  
  
  unsigned int Tadc; /* 10 bit value */
  unsigned int Padc; /* 10 bit value */
  
  int a0_int;
  int b1_int;
  int b2_int;
  int c12_int;
  
  double a0_coeff;  /* 16 bit value */
  double b1_coeff;  /* 16 bit value */
  double b2_coeff;  /* 16 bit value */
  double c12_coeff; /* 14 bit value */
  
  unsigned char serialIO(unsigned char output);

};


#endif