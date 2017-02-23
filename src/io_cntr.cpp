#include "io_cntr.h"

io_cntr::io_cntr(TwoWire *UseWire){
  pcf8574_output = new PCF857x(addr_output_PCF8574, UseWire);
}

void io_cntr::begin(void){

  pcf8574_output->write8(0);

}
