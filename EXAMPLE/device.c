/*

    Library which interfaces with SD cards using SPI for PIC24 Microcontrollers

*/

int module = 0;

//////////////////////////// HELPER FUNCTIONS START //////////////////////

// Calculates the CRC7 (x^7+x^3+1) for a given command and arg
unsigned char CRC7(unsigned char cmd, unsigned long arg) {
    unsigned char crc = 0;
    unsigned char crc7 = 0x89;
    unsigned char d[5] = {cmd, arg >> 24, arg >> 16, arg >> 8, arg};
    for (int i = 0; i < 5; i++) {
        unsigned char c = d[i];
        for (int b = 7; b >= 0; b--) {
            unsigned char val = (c >> b) & 1;
            unsigned char crc_msb = (crc >> 6) & 1;
            crc <<= 1;
            if (val^crc_msb) crc ^= crc7;
        }
        
    }
    return crc & 0x7f;
    
}

// Sends an SD card command. The CRC 7 is automatically calculated.
// The function will wait until a valid response token is received, then will copy
// it into the buffer dest.
void SD_send_CMD(unsigned char module, unsigned char cmd, unsigned long arg, unsigned char rBytes, unsigned char* dest) {
    
    cmd = (cmd & 0x7f) | 0x40;  // The first two bits must be 0b01
    unsigned char crc7 = (CRC7(cmd, arg) << 1) + 1;  // The last bit of CRC7 must be 1
    SPI_set_CS(module, 0);
    SPI_send_byte(module, cmd);
    SPI_send_byte(module, arg >> 24);
    SPI_send_byte(module, arg >> 16);
    SPI_send_byte(module, arg >> 8);
    SPI_send_byte(module, arg);
    unsigned char r = SPI_send_byte(module, crc7);
    for (unsigned char i = 0; i < 10 && r == 0xff; i++) r = SPI_send_byte(module, 0xff);
    if (r == 0xff) {
        dest[0] = 0xff;
    } else {
        dest[0] = r;
        for (unsigned char i = 1; i < rBytes; i++) {
            r = SPI_send_byte(module, 0xff);
            dest[i] = r;
        }
    }
}

void SD_send_app_CMD(unsigned char module, unsigned char acmd, unsigned long arg, unsigned char rBytes, unsigned char* cmddest, unsigned char* acmddest){
    SD_send_CMD(module, 55, 0l, 1, cmddest);
    SPI_set_CS(module, 1);
    SD_send_CMD(module, acmd, arg, rBytes, acmddest);
    SPI_set_CS(module, 1);
    
}

//////////////////////////// HELPER FUNCTIONS END ///////////////////////

/*
    Write data to a physical device sector. Will write up to the end of a sector
    and return (will not write beyond sector bouandry)

    @param      data            Buffer to write to device
    @param      sector          Physical device sector to write to
    @param      offset          Byte offset to on device to begin writing
    @param      len             Amount of bytes to write

    @retval     0               No bytes were written
    @retval     > 1             Amount of bytes written
*/
int write_block(const uint8_t* data, uint32_t sector, uint32_t offset, uint32_t len) {

    // Send CMD24 to initiate SD Write single block. Returns R1.
    // If this is not zero, there is an error and 0xff is returned
    unsigned char res;
    SD_send_CMD(module, 24, sector, 1, &res);
    if (res) {SPI_set_CS(module, 1); return 0;}
    
    // Send the start token to indicate transfer has started.
    // Transfers must equal exactly 512 bytes
    char d[512];
    if (SD_read_block(module, d, sector, 0, 512)) return 0;
    SPI_send_byte(module, 0xfe);            // Start token
    for (unsigned int i = 0; i < 512; i++){
        if (i >= offset && i < offset+len) SPI_send_byte(module, data[i-offset]);
        else SPI_send_byte(module, d[i]); 
    }
    
    // Send the CRC 16 bytes. This is disabled in SPI mode, but the bytes must be sent
    SPI_send_byte(module, 0x00);
    SPI_send_byte(module, 0x00);
    
    // The card responds with a 1-byte response token, which is returned
    unsigned char data_response = SPI_send_byte(module, 0xff);
    
    // If a proper token 0bXXX00101 is sent, the SD card holds the MISO line
    // LOW until done programming the data. Wait until this is done to return
    unsigned char j = 100;
    while (!SPI_send_byte(module, 0xff) && j-- > 0);
    if (!j) {SPI_set_CS(module, 1); return 0;}   // If after 100 cycles still low, send an error
    
    
    // Send SEND_STATUS (CMD13) and see if there was a programming error
    unsigned char response[2] = {0xff, 0xff};
    SD_send_CMD(module, 13, sector, 2, response);
    
    // Set CS to 1 and return the data response token if not valid
    SPI_set_CS(module, 1);
    if (response[0]) return 0; // CMD 13 R1 error
    if (response[1]) return 0; // CMD 13 programming error
    if ((data_response & 0x1f) != 0x05) return 0;
    
    return len;

}


/*
    Read data from a physical device sector. Will read up to the end of a sector
    and return (will not read beyond sector bouandry)

    @param      data            Buffer to place sector contents
    @param      sector          Physical device sector to read from
    @param      offset          Byte offset to on device to begin reading
    @param      len             Amount of bytes to read

    @retval     0               No bytes were read
    @retval     > 1             Amount of bytes read
*/
int read_block(uint8_t* data, uint32_t sector, uint32_t offset, uint32_t len) {
    
    // Send CMD17 to initiate SD Read single block. Returns R1.
    // If this is not zero, there is an error and 1 is returned
    unsigned char res;
    SD_send_CMD(module, 17, sector, 1, &res);
    if (res) {SPI_set_CS(module, 1); return 0;}
    
    // Wait until the card send the token 0xfe which preceeds the transmission
    // After 100 sends, assume and error and return 2.
    unsigned char counter = 100;
    while (SPI_send_byte(module, 0xff) != 0xfe && counter--);
    if (!counter) {SPI_set_CS(module, 1); return 0;}
    
    // Place received results into buffer
    for (int i = 0; i < 512; i++){
        if (i >= offset && i < offset+len) rcv[i-offset] = SPI_send_byte(module, 0xff);
        else SPI_send_byte(module, 0xff);
    }
    
    // UNIMPLEMENTED (CRC16 is disabled by defualt) Receive CRC16 bytes
    unsigned char CRC16[2];
    CRC16[0] = SPI_send_byte(module, 0xff);
    CRC16[1] = SPI_send_byte(module, 0xff);
    
    // Reset CS and return
    SPI_set_CS(module, 1);
    return len;
}

/*
    Initilizes the hardware

    @param      args         Pointer to int type determining module

    @retval     0            Succuss
    @retval     others       Fail
*/
int hardware_init(void *args) {
    
    module = *(int*)(args);

    unsigned char res[6];
    SPI_set_CS(module, 1);
    for (int i = 0; i < 10; i++) SPI_send_byte(module, 0xff);
    
    SD_send_CMD(module, 0, 0l, 1, res);
    SPI_set_CS(module, 1);
    if (res[0] != 0x01) {
        if (res[0] == 0xff)   return 1;   // Card not responding
        if (!(res[0] & 0x04)) return 2;   // Illegal Command
        if (!(res[0] & 0x08)) return 3;   // CRC error
        if (!(res[0] & 0x40)) return 4;   // Parameter error
        else                  return 5;   // Other CMD0 error
    }
    
    SD_send_CMD(module, 8, 0x000001aal, 5, res);
    SPI_set_CS(module, 1);
    if (res[0] != 0x01){
        if (res[0] == 0xff)   return 17;   // Card not responding
        if (!(res[0] & 0x04)) return 18;   // Illegal Command
        if (!(res[0] & 0x08)) return 19;   // CRC error
        if (!(res[0] & 0x40)) return 20;   // Parameter error
        else                  return 21;   // Other CMD8 R1 error
    } 
    else if (res[1] || res[2] || res[3] & 0xf0) return 22;  // Reserved bits error
    else if ((res[3] & 0x0f) != 0x01) return 23;            // Invalid Voltage
    else if (res[4] != 0xaa) return 24;                     // Check pattern invalid
    
    res[1] = 0xff;
    for (int i = 0; i < 1000 && res[1]; i++){
        SD_send_app_CMD(module, 41, 0x40000000l , 1, res, &res[1]); // res[0] = 0x01, res[1] = 0x00 (wait until equals 0x00)
        SPI_set_CS(module, 1);
    }
    
    if (res[0] != 0x01){
        if (res[0] == 0xff)   return 33;   // Card not responding
        if (!(res[0] & 0x04)) return 34;   // Illegal Command
        if (!(res[0] & 0x08)) return 35;   // CRC error
        if (!(res[0] & 0x40)) return 36;   // Parameter error
        else                  return 37;   // Other CMD55 R1 error        
    }
    
    if (res[1]) {
        if (res[1] == 0x01)   return 49;   // Card did not leave idle state in time
        if (res[1] == 0xff)   return 50;   // Card did not respond
        if (!(res[1] & 0x04)) return 51;   // Illegal Command
        if (!(res[1] & 0x08)) return 52;   // CRC error
        if (!(res[1] & 0x40)) return 53;   // Parameter error
        else                  return 54;   // Other ACMD41 R1 error
    }
    
    SD_send_CMD(module, 58, 0l, 5, res);
    SPI_set_CS(module, 1);
    
    if (res[0]) {
        if (res[0] == 0x01)   return 65;   // Card in idle state in time
        if (res[0] == 0xff)   return 66;   // Card did not respond
        if (!(res[0] & 0x04)) return 67;   // Illegal Command
        if (!(res[0] & 0x08)) return 68;   // CRC error
        if (!(res[0] & 0x40)) return 69;   // Parameter error
        else                  return 70;   // Other CMD58 R1 error
    }
    
    if (!(res[1] & 0x80)) return 71; // OCR read error
    
    return 0;
}

/*
    Ends hardware use

    @param      args         Implementation dependent input argument

    @retval     0            Succuss
    @retval     others       Fail
*/
int hardware_eject(void *args) {
    return 0;
}
