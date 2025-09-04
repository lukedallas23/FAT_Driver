

// This is a guard condition so that contents of this file are not included
// more than once.  
#ifndef XC_HEADER_TEMPLATE_H
#define	XC_HEADER_TEMPLATE_H

#include <xc.h> // include processor files - each processor file is guarded.  
#include "device.h"

#ifdef	__cplusplus
extern "C" {
#endif /* __cplusplus */
extern volatile char CS1;
extern volatile char CS2;

// Initilize SPI1 module to proper pins in standard mode.
int SPI_init(unsigned char module, unsigned char CS, unsigned char MOSI, unsigned char CLK, unsigned char MISO);
unsigned char SPI_send_byte(unsigned char module, char b);
void SPI_set_CS(unsigned char module, unsigned char val);
char SD_start_seq();

#ifdef	__cplusplus
}
#endif /* __cplusplus */

#endif	/* XC_HEADER_TEMPLATE_H */
