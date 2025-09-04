#include "xc.h"

volatile char CS1 = 16;
volatile char CS2 = 16;

// Sets CS pin to val if valid. Otherwise, nothing will happen
void SPI_set_CS(unsigned char module, unsigned char val){
    if (!val) {
        if (module == 1)      LATB &= ~(1 << CS1);
        else if (module == 2) LATB &= ~(1 << CS2);
    } else {
        if (module == 1)      LATB |= 1 << CS1;
        else if (module == 2) LATB |= 1 << CS2;        
    }
}

// Initilize SPI1 module to proper pins in standard mode.
// Pass the RP pherphrial pin (or RB pin for CS) for the module
// (RP, pin) -> 
//( 0, 4),( 1, 5),( 2, 6),( 3, 7),
//( 4,11),( 5,14),( 6,15),( 7,16),
//( 8,17),( 9,18),(10,21),(11,22),
//(12,23),(13,24),(14,25),(15,26)
int SPI_init(unsigned char module, unsigned char CS, unsigned char MOSI, unsigned char CLK, unsigned char MISO) {
    
    // See if values are valid
    if (module > 2 || !module || CS > 15 || MOSI > 15 || CLK > 15 || MISO > 15) return 1;
    if (CS == MOSI || CS == CLK || CS == MISO || MOSI == CLK || MOSI == MISO || CLK == MISO) return 1;
    // Set the relevant pin to proper I/O.
    TRISB &= ~(1 << CS);    // CS output
    TRISB &= ~(1 << MOSI);  // MOSI output
    TRISB &= ~(1 << CLK);   // CLK output
    TRISB |= 1 << MISO;     // MISO input
    
    // Turn the proper pins to digital if nessesary
    if (CS ==  0 || MOSI ==  0 || CLK ==  0 || MISO ==  0) AD1PCFG |= 0x0040;
    if (CS ==  1 || MOSI ==  1 || CLK ==  1 || MISO ==  1) AD1PCFG |= 0x0080;
    if (CS ==  2 || MOSI ==  2 || CLK ==  2 || MISO ==  2) AD1PCFG |= 0x0100;
    if (CS ==  3 || MOSI ==  3 || CLK ==  3 || MISO ==  3) AD1PCFG |= 0x0200;
    if (CS == 12 || MOSI == 12 || CLK == 12 || MISO == 12) AD1PCFG |= 0x0800;
    if (CS == 13 || MOSI == 13 || CLK == 13 || MISO == 13) AD1PCFG |= 0x0400;
    if (CS == 14 || MOSI == 14 || CLK == 14 || MISO == 14) AD1PCFG |= 0x0200;
    if (CS == 15 || MOSI == 15 || CLK == 15 || MISO == 15) AD1PCFG |= 0x0100;
    
    __builtin_write_OSCCONL(OSCCON & 0xbf); // unlock PPS
    if (module == 1) RPINR20bits.SDI1R = MISO;  // Assign Output for SPI 1
    else RPINR22bits.SDI2R = MISO;              // Assign Output for SPI 2
    
    // Assign MOSI to RP pin based on module and RP value
         if (MOSI == 0) RPOR0bits.RP0R  = 4 + 3*module;
    else if (MOSI == 1) RPOR0bits.RP1R  = 4 + 3*module;
    else if (MOSI == 2) RPOR1bits.RP2R  = 4 + 3*module;
    else if (MOSI == 3) RPOR1bits.RP3R  = 4 + 3*module;
    else if (MOSI == 4) RPOR2bits.RP4R  = 4 + 3*module;
    else if (MOSI == 5) RPOR2bits.RP5R  = 4 + 3*module;
    else if (MOSI == 6) RPOR3bits.RP6R  = 4 + 3*module;
    else if (MOSI == 7) RPOR3bits.RP7R  = 4 + 3*module;
    else if (MOSI == 8) RPOR4bits.RP8R  = 4 + 3*module;
    else if (MOSI == 9) RPOR4bits.RP9R  = 4 + 3*module;
    else if (MOSI ==10) RPOR5bits.RP10R = 4 + 3*module;
    else if (MOSI ==11) RPOR5bits.RP11R = 4 + 3*module;
    else if (MOSI ==12) RPOR6bits.RP12R = 4 + 3*module;
    else if (MOSI ==13) RPOR6bits.RP13R = 4 + 3*module;
    else if (MOSI ==14) RPOR7bits.RP14R = 4 + 3*module;
    else if (MOSI ==15) RPOR7bits.RP15R = 4 + 3*module;
    
    // Assign CLK to RP pin based on module and RP value
         if (CLK == 0) RPOR0bits.RP0R  = 5 + 3*module;
    else if (CLK == 1) RPOR0bits.RP1R  = 5 + 3*module;
    else if (CLK == 2) RPOR1bits.RP2R  = 5 + 3*module;
    else if (CLK == 3) RPOR1bits.RP3R  = 5 + 3*module;
    else if (CLK == 4) RPOR2bits.RP4R  = 5 + 3*module;
    else if (CLK == 5) RPOR2bits.RP5R  = 5 + 3*module;
    else if (CLK == 6) RPOR3bits.RP6R  = 5 + 3*module;
    else if (CLK == 7) RPOR3bits.RP7R  = 5 + 3*module;
    else if (CLK == 8) RPOR4bits.RP8R  = 5 + 3*module;
    else if (CLK == 9) RPOR4bits.RP9R  = 5 + 3*module;
    else if (CLK ==10) RPOR5bits.RP10R = 5 + 3*module;
    else if (CLK ==11) RPOR5bits.RP11R = 5 + 3*module;
    else if (CLK ==12) RPOR6bits.RP12R = 5 + 3*module;
    else if (CLK ==13) RPOR6bits.RP13R = 5 + 3*module;
    else if (CLK ==14) RPOR7bits.RP14R = 5 + 3*module;
    else if (CLK ==15) RPOR7bits.RP15R = 5 + 3*module;
    __builtin_write_OSCCONL(OSCCON | 0x40); // lock   PPS
    
    if (module == 1) {
        CS1 = CS;
        SPI_set_CS(1, 1);
        SPI1STAT = 0;           // Reset status/control register
        SPI1CON1 = 0;           // Reset con1 register
        SPI1CON2 = 0;           // Reset con2 register
        SPI1CON1bits.SMP = 1;   // SD card uses mode 0
        SPI1CON1bits.CKE = 1;   // SD card uses mode 0
        SPI1CON1bits.CKP = 0;   // Sample at edge
        SPI1CON1bits.MSTEN = 1; // Enable to master mode
        SPI1CON1bits.SPRE = 5;//5;  // 333kHz
        SPI1CON1bits.PPRE = 3;//2;  // 333kHz
        SPI1STATbits.SPIEN = 1; // Enable SPI1
    } else {
        CS2 = CS;
        SPI_set_CS(2, 1);
        SPI2STAT = 0;           // Reset status/control register
        SPI2CON1 = 0;           // Reset con1 register
        SPI2CON2 = 0;           // Reset con2 register
        SPI2CON1bits.SMP = 1;   // SD card uses mode 0
        SPI2CON1bits.CKE = 1;   // SD card uses mode 0
        SPI2CON1bits.CKP = 0;   // Sample at edge
        SPI2CON1bits.MSTEN = 1; // Enable to master mode
        SPI2CON1bits.SPRE = 5;  // 333kHz
        SPI2CON1bits.PPRE = 1;  // 333kHz
        SPI2STATbits.SPIEN = 1; // Enable SPI1
    }
    return 0;
}

// Returns received byte, must enable CS seperatly
unsigned char SPI_send_byte(unsigned char module, char b) {
    if (module == 1) {
        SPI1BUF = b;                    // Place char in array, initiate transfer
        while(!SPI1STATbits.SPIRBF);    // Wait until T/R is complete
        unsigned char c = SPI1BUF;
        return c; 
    } else {
        SPI2BUF = b;                    // Place char in array, initiate transfer
        while(!SPI2STATbits.SPIRBF);    // Wait until T/R is complete 
        unsigned char c = SPI2BUF;
        return c; 
    }
    return 0;
}

// Puts start sequence on the SD card
char SD_start_seq() {
    for (int i = 0; i < 10; i++) SPI_send_byte(1, 0xff);
    SPI_set_CS(1, 0);
    SPI_send_byte(1, 0x40);
    SPI_send_byte(1, 0x00);
    SPI_send_byte(1, 0x00);
    SPI_send_byte(1, 0x00);
    SPI_send_byte(1, 0x00);
    char r = SPI_send_byte(1, 0x95);
    while (r == (char) 0xff) r = SPI_send_byte(1, 0xff);
    SPI_set_CS(1, 1);
    return r;
    
}
