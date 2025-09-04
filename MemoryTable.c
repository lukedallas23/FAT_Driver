// File which allows memory to be loaded and unloaded
// from an external storage device.
#include "MemoryTable.h"



/*
    Initilizes the memory table. This should be called before using other 
    MemoryTable functions.

    @returns    0   on succuss.
    @returns    1   on failure.

*/
int MT_TableInit () {

    TABLE_ENTRIES = MEMORY_BYTES/SECTOR_SIZE;
    SectorIndex = 0;

    for (int i = 0; i < TABLE_ENTRIES; i++) {
        DeviceSectors[i] = UNALLOCATED | DIRTY;
    }

    return 0;

}


/*
    Unloads the memory table and writes back any changed memory to the disk

    @returns    0   on succuss.
    @returns    1   on failure.

*/
int MT_TableUnload () {

    for (int i = 0; i < TABLE_ENTRIES; i++) {
        if (DeviceSectors[i] & WRITE_SECTOR) {
            if (write_block(DeviceMemory[i], DeviceSectors[i] & MAX_SECTORS, 0, SECTOR_SIZE) != SECTOR_SIZE) return 1;
        }
    }

    return 0;

}


/*
    Loads a device sector into the memory table. This function uses the clock
    page replacement algorithm to determine which sector gets replaced to load
    the new sector into memory, if nessesary.

    @param      sector      Sector to load into memory
    @param      permanent   Set to 1 if loaded sector is to be set as permanent

    @returns                On succuss, a pointer to the first byte of the sector in memory.
    @returns                On failure, NULL 

*/
uint8_t* MT_LoadMemory(uint32_t sector) {

    // Check memory table to see if sector is already in memory table
    for (int i = 0; i < TABLE_ENTRIES; i++) {

        if (DeviceSectors[i] & MAX_SECTORS == sector) {

            if (DeviceSectors[i] & UNALLOCATED) continue;

            DeviceSectors[i] &= ~DIRTY;
            return DeviceMemory[i];

        }
    }

    // Find memory table location to load sector into using memory
    // Cycle around the clock to see non DIRTY bits. Set dirty bits along the way
    for (uint32_t i = 0; i < (2*TABLE_ENTRIES); i++) {

        uint32_t index = (i + SectorIndex) % TABLE_ENTRIES;

        // If dirty bit set, replace this block and write old sector back if nessesary
        if (DeviceSectors[index] & DIRTY) {

            if (DeviceSectors[index] & PERMANENT) continue;

            if (DeviceSectors[index] & WRITE_SECTOR) {
                if (write_block(DeviceMemory[index], DeviceSectors[index] & MAX_SECTORS, 0, SECTOR_SIZE) != SECTOR_SIZE) return NULL;
            }

            DeviceSectors[index] = sector;
            if (read_block(DeviceMemory[index], sector, 0, SECTOR_SIZE) != SECTOR_SIZE) return NULL;
            SectorIndex = index;

            return DeviceMemory[index];

        }

        DeviceSectors[index] |= DIRTY;

    }

    return NULL;    // If all blocks are permanent, will reach here

}


/*
    Unsets a sector in memory table as permeanent, allowing it to be written back

    @param      sector      Sector to unset if permanent

    @returns    0           On succuss
    @returns    1           Sector was not in memory table

*/
int MT_UnsetPermanent(uint32_t sector) {

    for (int i = 0; i < TABLE_ENTRIES; i++) {

        if (DeviceSectors[i] == sector) {
            DeviceSectors[i] &= ~PERMANENT;
            return 0;
        }

    }

    return 1;
}


/*
    Sets a sector as permanent, loading the sector if nessesary. Only permanent
    sectors can be written to or read from directly.

    @param      sector      Sector to set permanent

    @returns    A pointer to the memory table               On succuss
    @returns    NULL                                        On failure

*/
uint8_t *MT_SetPermanent(uint32_t sector) {

    for (int i = 0; i < TABLE_ENTRIES; i++) {

        if (DeviceSectors[i] == sector) {
            DeviceSectors[i] |= PERMANENT;
            return DeviceMemory[i];
        }
    }

    uint8_t* sector_contents = MT_LoadMemory(sector);
    if (sector_contents == NULL) return NULL;
    DeviceSectors[((int)sector_contents - (int)(DeviceMemory[0]))/SECTOR_SIZE] |= PERMANENT | WRITE_SECTOR;
    return sector_contents;

}


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
int MT_DeviceWrite(uint8_t* data, uint32_t sector, uint32_t offset, uint32_t len) {

    // If mem table address points to the start of the sector specified in the memory table, skip iterating
    // through the table.
    uint8_t* sector_contents;

    sector_contents = MT_LoadMemory(sector);

    if (sector_contents == NULL) return -1;

    DeviceSectors[((int)sector_contents - (int)(DeviceMemory[0]))/SECTOR_SIZE] |= WRITE_SECTOR;

    int i = offset;
    for (; i < offset + len && i < SECTOR_SIZE; i++) sector_contents[i] = data[i-offset];
    
    return i - offset; 

}


/*
    Read data of given length of a sector starting at a given offset and returns pointer 
    to memory table. This function will not read beyond a sector bouandry and will stop 
    reading if the end of a sector is reached.

    @param      sector      Device sector to be read from
    @param      offset      Starting byte offset to begin reading. Must be between 0 and
                            SECTOR_SIZE - 1
    @param      mem_table   OPTIONAL: If sector is stored at the address provided,
                            memory table will not be scanned. Set to NULL if not using
    
    @retval     > 0           On Succuss, the number of bytes written
    @retval     0             On Failure
*/
int MT_DeviceRead(uint8_t *data, uint32_t sector, uint32_t offset, uint32_t len) {

    if (offset >= SECTOR_SIZE) return 0;

    uint8_t *mem_table = &(MT_LoadMemory(sector)[offset]);

    int i = offset;
    for (; i < offset + len && i < SECTOR_SIZE; i++) data[i-offset] = mem_table[i-offset];
    
    return i - offset; 
}
