// File which allows memory to be loaded and unloaded
// from an external storage device.
#include <stdint.h>
#include <string.h>
#include "device.h"


/*
    Flag for indicating that a sector has been written to.
*/
#define WRITE_SECTOR    0x100000000LL

/*
    Flag indicating that a sector is dirty and is queued to be replaced
*/
#define DIRTY           0x200000000LL

/*
    Flag indicating that a memory table entry is unallocated
*/
#define UNALLOCATED     0x400000000LL

/*
    Flag indicating that a memory table entry is permanent
*/
#define PERMANENT       0x800000000LL

/*
    Maximum number of sectors on common devices
*/
#define MAX_SECTORS     0xffffffffLL

/*
    Specifies to load a block as permanent
*/
#define LOAD_AS_PERMANENT       1

/*
    Specifies to not load a block as permanent
*/
#define NO_LOAD_AS_PERMANENT    0





/*
    Array which contains the memory for the device
*/
uint8_t DeviceMemory[MEMORY_BYTES/SECTOR_SIZE][SECTOR_SIZE];


/*
    Array containing sectors for the memory table
*/
uint64_t DeviceSectors[MEMORY_BYTES/SECTOR_SIZE];

/*
    Index for clock table replacement
*/
uint32_t SectorIndex;

/*
    Number of table entries
*/
uint32_t TABLE_ENTRIES;

/*
    Initilizes the memory table. This should be called before using other 
    MemoryTable functions.

    @returns    0   on succuss.
    @returns    1   on failure.

*/
int MT_TableInit ();


/*
    Unloads the memory table and writes back any changed memory to the disk

    @returns    0   on succuss.
    @returns    1   on failure.

*/
int MT_TableUnload ();


/*
    Loads a device sector into the memory table. This function uses the clock
    page replacement algorithm to determine which sector gets replaced to load
    the new sector into memory, if nessesary.

    @param      sector      Sector to load into memory
    @param      permanent   Set to 1 if loaded sector is to be set as permanent

    @returns                On succuss, a pointer to the first byte of the sector in memory.
    @returns                On failure, NULL 

*/
uint8_t* MT_LoadMemory(uint32_t sector);


/*
    Unsets a sector in memory table as permeanent, allowing it to be written back

    @param      sector      Sector to unset if permanent

    @returns    0           On succuss
    @returns    1           Sector was not in memory table

*/
int MT_UnsetPermanent(uint32_t sector);


/*
    Sets a sector as permanent, loading the sector if nessesary

    @param      sector      Sector to set permanent

    @returns    0           On succuss
    @returns    1           On failure

*/
uint8_t *MT_SetPermanent(uint32_t sector);


/*
    Write data to the memory table of given length and offset to a sector. This function
    will not write beyond a sector bouandry and will stop writing if the end of a sector
    is reached. 

    @param      data        Pointer to data to be written
    @param      sector      Device sector to be written to
    @param      offset      Starting byte offset to begin writing
    @param      len         Amount of bytes to write
    @param      mem_table   OPTIONAL: If sector is stored at the address provided,
                            memory table will not be scanned. Set to NULL if not using

    @returns    > 0         On Succuss, the amount of bytes written. 
    @returns    0           No bytes were written, function considered to succeed
    @returns    -1          Load Memory Failed
*/
int MT_DeviceWrite(uint8_t* data, uint32_t sector, uint32_t offset, uint32_t len);


/*
    Read data of given length of a sector starting at a given offset and returns pointer 
    to memory table. This function will not read beyond a sector bouandry and will stop 
    reading if the end of a sector is reached.

    @param      sector      Device sector to be read from
    @param      offset      Starting byte offset to begin reading. Must be between 0 and
                            SECTOR_SIZE - 1
    @param      mem_table   OPTIONAL: If sector is stored at the address provided,
                            memory table will not be scanned. Set to NULL if not using
    
    @returns                On Succuss, a pointer to the memory table at the specified offset
    @returns                On Failure, NULL.
*/
int MT_DeviceRead(uint8_t *data, uint32_t sector, uint32_t offset, uint32_t len);
