#ifndef _IO_CNTR_H
#define _IO_CNTR_H

#include "pcf8574_esp.h"
#include <Wire.h>

#define addr_output_PCF8574 0x38

class io_cntr {

public:
  io_cntr(TwoWire *UseWire);
  void begin(void);

private:
  PCF857x* pcf8574_output;

};


#endif
