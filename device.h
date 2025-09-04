#include <stdint.h>
#include <stdarg.h>

#define SECTOR_SIZE 512         // Bytes per sector for the sd card
#define MEMORY_BYTES 1024         //  Bytes of memory reserved for this device's memory table Do not pick a number too large or this will cause an exception

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
int write_block(const uint8_t* data, uint32_t sector, uint32_t offset, uint32_t len);


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
int read_block(uint8_t* data, uint32_t sector, uint32_t offset, uint32_t len);

/*
    Initilizes the hardware

    @param      args         Implementation dependent input argument

    @retval     0            Succuss
    @retval     others       Fail
*/
int hardware_init(void *args);

/*
    Ends hardware use

    @param      args         Implementation dependent input argument

    @retval     0            Succuss
    @retval     others       Fail
*/
int hardware_eject(void *args);
