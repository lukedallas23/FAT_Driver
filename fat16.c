#include "fat16.h"
#include "sd.h"
#include "MemoryTable.h"

///////////////////  FAT16 HELPER FUNCTIONS /////////////////////////////

/*
    Check if a string is compliant with a long directory entry.

    @param      IN  name            File/directory name
    @param      IN  len             Length of name
    @param      OUT new             File/directory to be returned

    @retval     TRUE                File and name are matching
    @retval     FALSE               File and name do not match
*/
bool fat16IsValidLongEntry(char *name, uint16_t len, FILE *file) {

    uint8_t pos = file->file.LongEntry.LDIR_Ord;
    
    if (pos & LAST_LONG_ENTRY) {

        // 43h for example, len must be between 27 and 39 chars exactly to be valid
        if ((len < (1 + 13*(pos & 0x3F))) || (len > (13*(1 + (pos & 0x3F))))) {
            return FALSE;
        }

        len = len % 13;
        if (len == 0) len = 13;

        return !(fat16StrCmp(&name[(pos & 0x3F)*13], len, file));

    } else {

        // 3 for example, len must be 40 chars or greater
        if (len > (pos * 3)) return FALSE;

        return !(fat16StrCmp(&name[pos*13], 13, file));
    }

    return FALSE;
}


/* 
    Check if a string is compliant with a short directory entry.

    @param      IN  name            File/directory name
    @param      IN  len             Length of name
    @param      OUT new             File/directory to be returned

    @retval     TRUE                File and name are matching
    @retval     FALSE               File and name do not match
*/
bool fat16IsValidShortEntry(char *name, uint16_t len, FILE *file) {

    if (!strcmp(".          ", name)) return TRUE;
    if (!strcmp("..         ", name)) return TRUE;

    if (len > 12) return FALSE;

    char shortName[11] = "           ";
    char shortIndex = 0;

    for (uint8_t i = 0; i < len; i++) {
        if (shortIndex == 11) return FALSE;
        if (shortIndex == 8 && i == shortIndex) return FALSE;
        char temp = name[i];
        if (name[i] >= 'a' && name[i] <= 'z') temp -= 32;
        if (name[i] == '.') {
            shortIndex = 8;
            continue;
        }
        shortName[shortIndex] = temp;
        shortIndex++;
    }

    for (uint8_t i = 0; i < 11; i++) {
        if (shortName[i] != file->file.ShortEntry.DIR_Name[i]) return FALSE;
    }
    return TRUE;
}


/*
    Check if a string matches a long directory entry

    @param      IN  file            File/directory entry
    @param      IN  str             ASCII Name to compare
    @param      IN  len             Length of string

    @retval     0                   Match
    @retval     1                   No Match
*/
uint8_t fat16StrCmp(char *str, uint16_t len, FILE *file) {

    for (uint8_t i = 0; i < 5; i++) {
        if (i >= len) {
            if (file->file.LongEntry.LDIR_Name1[i] != 0) return 1;
            return 0;
        }
        if (file->file.LongEntry.LDIR_Name1[i] != str[i]) return 1;
    }

    for (uint8_t i = 0; i < 6; i++) {
        if (i + 5 >= len) {
            if (file->file.LongEntry.LDIR_Name1[i] != 0) return 1;
            return 0;
        }
        if (file->file.LongEntry.LDIR_Name2[i] != str[i]) return 1;
    }

    for (uint8_t i = 0; i < 2; i++) {
        if (i + 11 >= len) {
            if (file->file.LongEntry.LDIR_Name1[i] != 0) return 1;
            return 0;
        }
        if (file->file.LongEntry.LDIR_Name3[i] != str[i]) return 1;
    }

    return 0;

}


/*
    Copy a string into a long directory entry up to 13 chars. If a null terminator
    is reached, then the rest of the LDE is padded.

    @param      IN  file            File/directory entry
    @param      IN  str             ASCII Name to write

    @retval     0                   Success
    @retval     1                   Failure
*/
uint8_t fat16StrCpy(char *str, FileEntry *file) {

    for (char i = 0; i < 2; i++) file->LongEntry.LDIR_Name1[i] = 0xFFFF;
    for (char i = 0; i < 6; i++) file->LongEntry.LDIR_Name2[i] = 0xFFFF;
    for (char i = 0; i < 5; i++) file->LongEntry.LDIR_Name3[i] = 0xFFFF;

    file->LongEntry.LDIR_Name1[0] = (uint16_t) str[0];
    if (!file->LongEntry.LDIR_Name1[0]) return 0;

    file->LongEntry.LDIR_Name1[1] = (uint16_t) str[1];
    if (!file->LongEntry.LDIR_Name1[0]) return 0;

    file->LongEntry.LDIR_Name2[0] = (uint16_t) str[2];
    if (!file->LongEntry.LDIR_Name2[0]) return 0;

    file->LongEntry.LDIR_Name2[1] = (uint16_t) str[3];
    if (!file->LongEntry.LDIR_Name2[0]) return 0;

    file->LongEntry.LDIR_Name2[2] = (uint16_t) str[4];
    if (!file->LongEntry.LDIR_Name2[0]) return 0;
    
    file->LongEntry.LDIR_Name2[3] = (uint16_t) str[5];
    if (!file->LongEntry.LDIR_Name2[0]) return 0;

    file->LongEntry.LDIR_Name2[4] = (uint16_t) str[6];
    if (!file->LongEntry.LDIR_Name2[0]) return 0;
    
    file->LongEntry.LDIR_Name2[5] = (uint16_t) str[7];
    if (!file->LongEntry.LDIR_Name2[0]) return 0;

    file->LongEntry.LDIR_Name3[0] = (uint16_t) str[8];
    if (!file->LongEntry.LDIR_Name3[0]) return 0;
    
    file->LongEntry.LDIR_Name3[0] = (uint16_t) str[9];
    if (!file->LongEntry.LDIR_Name3[1]) return 0;

    file->LongEntry.LDIR_Name3[0] = (uint16_t) str[10];
    if (!file->LongEntry.LDIR_Name3[2]) return 0;

    file->LongEntry.LDIR_Name3[0] = (uint16_t) str[11];
    if (!file->LongEntry.LDIR_Name3[3]) return 0;

    file->LongEntry.LDIR_Name3[0] = (uint16_t) str[12];
    
    return 0;

}

/*
    Read from a cluster and put it in a buffer.

    @param      data        Buffer to place read data in
    @param      cluster     Cluster to read
    @param      offset      Bytes offset to read
    @param      len         Length in bytes to read

    @retval     0           Nothing was read
    @retval     > 0         Number of bytes read

*/
uint16_t fat16ReadCluster(uint8_t *data, uint16_t cluster, uint32_t offset, uint32_t len) {

    uint32_t sector = FSGetSector(cluster);

    if (sector == 0) return 0;

    uint32_t endSector = sector + BS->BPB_SecPerClus;
    uint32_t currOffset = offset;
    sector += offset / SECTOR_SIZE;

    if (sector > endSector) return 0;

    // Get the first sectors data
    if (len < SECTOR_SIZE - (offset % SECTOR_SIZE)) {
        currOffset += MT_DeviceRead(data, sector, offset % SECTOR_SIZE, len);
        return currOffset - offset;
    }
    else {
        currOffset += MT_DeviceRead(data, sector++, offset % SECTOR_SIZE, SECTOR_SIZE - (offset % SECTOR_SIZE));
    }

    // Continue to read sectors until length requirement is met
    while ((currOffset - offset) < len && sector < endSector) {

        if (len - (currOffset - offset) < SECTOR_SIZE) {
            
            currOffset += MT_DeviceRead(&data[currOffset - offset], sector++, 0, len - (currOffset - offset));
            return currOffset - offset;

        } else {

            currOffset += MT_DeviceRead(&data[currOffset - offset], sector++, 0, SECTOR_SIZE);

        }
    }

    return currOffset - offset; // Should be equal to len   


}


/*
    Write to a cluster from a buffer.

    @param      data        Buffer to read from to write
    @param      cluster     Cluster to write
    @param      offset      Bytes offset to write
    @param      len         Length in bytes to write

    @retval     0           Nothing was written
    @retval     > 0         Number of bytes written

*/
uint16_t fat16WriteCluster(uint8_t *data, uint16_t cluster, uint32_t offset, uint32_t len) {

    uint32_t sector = FSGetSector(cluster);
    if (sector == 0) return 0;

    uint32_t endSector = sector + BS->BPB_SecPerClus;
    uint32_t currOffset = 0;
    sector += offset / SECTOR_SIZE;

    if (sector > endSector) return 0;

    // Get the first sectors data
    uint32_t bytes;
    if (len < SECTOR_SIZE - (offset % SECTOR_SIZE)) {
        currOffset += MT_DeviceWrite(data, sector, offset % SECTOR_SIZE, len);
        return bytes;
    }
    else {
        currOffset += MT_DeviceWrite(data, sector++, offset % SECTOR_SIZE, SECTOR_SIZE - (offset % SECTOR_SIZE));
    }

    // Continue to read sectors until length requirement is met
    while ((currOffset - offset) < len && sector < endSector) {

        if (len - (currOffset - offset) < SECTOR_SIZE) {
            
            currOffset += MT_DeviceWrite(&data[currOffset - offset], sector++, 0, len - (currOffset - offset));
            return currOffset - offset;

        } else {

            currOffset += MT_DeviceWrite(&data[currOffset - offset], sector++, 0, SECTOR_SIZE);

        }
    }

    return currOffset - offset; // Should be equal to len
}


/*  
    Calculate the checksum value for a file name entry.

    @param      pFcbName    Pointer to an unsigned byte array assumed
                            to be 11 bytes long
    
    @returns    Sum         An 8-bit unsigned checksum of the array pointed
                            to by pFcbName


*/
uint8_t fat16ChkSum(uint8_t *pFcbName) {
    
    int16_t FcbNameLen;
    uint8_t Sum;

    Sum = 0;
    for (FcbNameLen=11; FcbNameLen!=0; FcbNameLen--) {
        // NOTE: The operation is an unsigned char rotate right
        Sum = ((Sum & 1) ? 0x80 : 0) + (Sum >> 1) + *pFcbName++;
    }
    return (Sum);

}

/*
    Write an updated short file entry back to the disk and update the checksum
    for long directory entries.

    @param      file            File to write back to disk

    @retval    EXIT_SUCCUSS     Succussful
    @retval    EXIT_WRITE_FAIL  Disk write failed.

*/
EXIT_STATUS fat16FileToDisk(FILE *file) {

    uint8_t chkSum = fat16ChkSum(file->file.ShortEntry.DIR_Name);
    uint32_t off = file->dirOffset;
    uint32_t attr = ATTR_LONG_NAME;

    if (sizeof(FileEntry) != FSWriteFile((char*)&file->file, file->dirOffset, sizeof(FileEntry), file->dir)) {
        return EXIT_WRITE_FAIL;
    }

    if (off == 0) return EXIT_SUCCESS;
    off -= 32;

    attr = FSReadFile (
        (char*)&attr,
        off + 11,
        1,
        file
    );

    // Modify long names and edit checksum for them
    while (attr == ATTR_LONG_NAME) {

        FSWriteFile (
            (char*)&attr,
            off + 11,
            1,
            file           
        );

        off -= 32;
        if (off == 0) return EXIT_SUCCESS;

        FSWriteFile (
            (char*)&attr,
            off + 11,
            1,
            file 
        );

    }

    return EXIT_SUCCESS;

}


/*
    Get the first cluster of the directory that a file is in.

    @param      file            File to write back to disk

    @retval    EXIT_SUCCUSS     Succussful
    @retval    EXIT_WRITE_FAIL  Disk write failed.

*/
uint16_t fat16GetDirCluster(FILE *file) {

    return (file->dir->file.ShortEntry.DIR_FstClusHI << 16) + file->dir->file.ShortEntry.DIR_FstClusLO;
}

///////////////////  FILE SYSTEM FUNCTIONS //////////////////////////////



/*
    Allocate a new cluster to the end of a file

    @param      from                File End Of Cluster file, or zero, if there is not a cluster allocated
    for a file.
    @param      args                Hardware Initilization arguments. Varies based on implementation

    @retval    > 0                      New Cluster Number
    @retval    0                        Fail   

*/
uint16_t FSAllocateCluster(uint16_t from) {

    // The provided FROM cluster must be EOC or unallocated
    if (FSGetFatTableEntry(from) & FAT_MASK >= BS->PAR_Max_Cluster &&
        FSGetFatTableEntry(from) & FAT_MASK < 2) {
        return 0;
    }

    // Full disk is error
    if (BS->FSI_Free_Count == 0) return 0; 

    uint16_t next_cluster = BS->FSI_Nxt_Free;  

    if (from != 0) FSFatTableUpdate(from, next_cluster);
    FSFatTableUpdate(next_cluster, FAT_EOC);

    BS->FSI_Free_Count--;

    // Iterate through the FAT until a free cluster is found
    while (1) {
        
        if (++BS->FSI_Nxt_Free >= BS->PAR_Max_Cluster) BS->FSI_Nxt_Free = 2;

        if (FSGetFatTableEntry(BS->FSI_Nxt_Free) == 0) break;

    }

    return next_cluster;
}


/*
    Update a FAT table entry with a given status

    @param      cluster     Cluster which is to have FAT value updated
    @param      status      New FAT table entry. The first four bits will be unchanged

    @retval     EXIT_SUCCUSS            On Success
    @retval     EXIT_INVALID_PARAMETER  Cluster value provided was invalid
    @retval     EXIT_MEMORY_TABLE_FAIL  Memory table write failed

*/
EXIT_STATUS FSFatTableUpdate(uint16_t cluster, uint16_t status) {

    if (cluster & FAT_MASK >= BS->PAR_Max_Cluster) return EXIT_INVALID_PARAMETER;
    if (cluster < 2) return EXIT_INVALID_PARAMETER;

    // Iterate through all FAT tables and update all of the fat tables
    for (uint8_t FATTable = 0; FATTable < BS->BPB_FATSz32; FATTable++) {
        
        uint32_t sector = BS->BPB_HiddSec + BS->BPB_RsvdSecCnt + 
        (4*cluster)/SECTOR_SIZE + FATTable*BS->BPB_FATSz32;

        uint16_t offset = 4 * (cluster % (SECTOR_SIZE/4));

        if (sizeof(uint16_t) != MT_DeviceWrite((char*)&status, sector , offset & FAT_MASK, sizeof(uint16_t))) {
            return EXIT_MEMORY_TABLE_FAIL;
        }
    }

    return EXIT_SUCCESS;
}


/*
    Get the first physcial sector of a cluster given the cluster

    @param      cluster     Cluster which is to have FAT value updated
    @param      status      New FAT table entry. The first four bits will be unchanged

    @retval     sector number       On Success
    @retval     0                   Fail - Not valid cluster                       

*/
uint32_t FSGetSector(uint16_t cluster) {

    if (cluster > BS->PAR_Max_Cluster) return 0;
    if (cluster < 2) return 0;

    // Sector of cluster 2
    uint32_t first_sector = BS->BPB_HiddSec + BS->BPB_HiddSec + 
    BS->BPB_FATSz32 * BS->BPB_NumFATs;

    return (cluster - 2) * BS->BPB_SecPerClus;

}


/*
    Get the cluster that a physical sector is in. It is possible to get a cluster
    value equal to MAX_CLUSTER because of the final incomplete cluster on a partition.

    @param      cluster     Cluster which is to have FAT value updated
    @param      status      New FAT table entry. The first four bits will be unchanged

    @retval     cluster number    On Success
    @retval     0                 Fail - Sector not in partition data area                       

*/
uint16_t FSGetCluster(uint32_t sector) {


    // Sector of cluster 2
    uint16_t first_cluster = BS->BPB_HiddSec + BS->BPB_HiddSec + 
    BS->BPB_FATSz32 * BS->BPB_NumFATs;

    if (sector < first_cluster) return 0;
    if (sector >= BS->MBR_Part.StartingLBA + BS->MBR_Part.SizeInLBA) return 0;

    return ((sector - first_cluster) / BS->BPB_SecPerClus) + 2;

}


/*
    Get a value in the FAT table given a cluster entry

    @param      cluster     Cluster which is to have FAT value updated

    @retval     0           Fail or invalid
    @retval     > 0         Sector number in table                       

*/
uint16_t FSGetFatTableEntry(uint16_t cluster) {

    if (cluster & FAT_MASK >= BS->PAR_Max_Cluster) return 0;
    if (cluster & FAT_MASK < 2) return 0;

    uint32_t sector = BS->BPB_HiddSec + BS->BPB_RsvdSecCnt + 
    (4*cluster)/SECTOR_SIZE;

    uint16_t offset = 4 * (cluster % (SECTOR_SIZE/4));

    uint32_t status;
    if (sizeof(uint16_t) != MT_DeviceRead((char*)&status, sector, offset, sizeof(uint16_t))) {
        return 0;
    }

    return status;

}


/*
    Search a directory for a file name and return it if found.

    @param      IN  name            File/directory to search for
    @param      IN  directory       Directory which is to be searched
    @param      OUT new             File/directory to be returned

    @retval     EXIT_SUCCESS            Directory succussfully found
    @retval     EXIT_NOT_FOUND          Directory not found
    @retval     EXIT_INVALID_PARAMETER  Device Error
    @retval     EXIT_MEMORY_TABLE_FAIL  Memory table driver failed
*/
EXIT_STATUS FSDirectorySearch(char *name, FILE *directory, FILE *new) {
    
    // Length of string name
    uint16_t len = strlen(name);

    // Max directory entry value (first to search for)
    uint8_t MaxLDEntry = LAST_LONG_ENTRY | ((uint8_t) (len / 13) + 1);

    // Current entry
    uint8_t LDEntry = MaxLDEntry;

    // Storage container for files
    FILE File;

    // Current cluster that is being searched
    uint16_t cluster = (directory->file.ShortEntry.DIR_FstClusHI << 16) + directory->file.ShortEntry.DIR_FstClusLO;

    // Current sector to search through
    uint16_t sector = FSGetSector(cluster);
    if (sector == 0) {
        return EXIT_INVALID_PARAMETER;
    }

    // Byte offset in directory
    uint32_t byteOffset = 0;
     
    // Search file entries within the directory
    while (cluster < FAT_DEFECTIVE) {

        // Search within a cluster
        for (uint16_t sec = 0; sec < BS->BPB_SecPerClus; sec++) {

            // Search within a sector
            for (uint16_t off = 0; off < SECTOR_SIZE; off += sizeof(FileEntry)) {

                // Read the file entry
                if (0 != MT_DeviceRead((char*)&(File.file), sector, off, sizeof(FileEntry))) {
                    return EXIT_MEMORY_TABLE_FAIL;
                }

                // If LDEntry is 0, the file has been found. Return the File entry
                // in the corresponding short entry
                if (LDEntry == 0) {

                    if (new != NULL) {
                        memcpy(&(new->file), &File, sizeof(FileEntry));
                        new->len = len;
                        new->dir = directory;
                        new->dirCluster = directory->file.ShortEntry.DIR_FstClusHI << 16 +
                        directory->file.ShortEntry.DIR_FstClusLO;
                        new->dirOffset = byteOffset; 
                    }
                    
                    return EXIT_SUCCESS;
                }

                // If attr is 0, then no need to search anymore, file is not present
                if (File.file.LongEntry.LDIR_Ord == 0) return EXIT_NOT_FOUND;

                // Short name checks if possible - if it matches the short name, then this is 
                // a short entry and this can be used
                if (MaxLDEntry == LAST_LONG_ENTRY + 1) {
                    if (fat16IsValidShortEntry(name, len, &File)) {
                        LDEntry = 0;
                        byteOffset += 32;
                        continue;
                    }
                }

                // If attr is not as expected, then we are going down the wrong path
                // Reset to look for beggining LDR entry and continue search
                if (File.file.LongEntry.LDIR_Ord != LDEntry) {
                    LDEntry = MaxLDEntry;
                    byteOffset += 32;
                    continue;
                }

                // If attr matches, see if this could be the valid file. If not, then reset
                // the LDEntry to look for beggining again and continue
                if (!fat16IsValidLongEntry(name, len, &File)) {
                    LDEntry = MaxLDEntry;
                    byteOffset += 32;
                    continue; 
                }

                // If we have gotten to here, the path is consistant with the file needed,
                // reduce LDEntry to indicate this and continue.
                LDEntry = (LDEntry & 0x3F) - 1;
                byteOffset += 32;

            }

        }

        // When the end of a cluster has been reached, then load the next.
        cluster = FSGetFatTableEntry(cluster);
        if (cluster == 0) return EXIT_INVALID_PARAMETER;
    }

    // Will only get here if the loaded fat value indicates end of directory was reached
    return EXIT_NOT_FOUND;
}


/*
    Initializes the drive and memory table for which the hardware driver is configured.

    @param      partition       Physical MBR partition number which is to be formatted with 
    corresponding flags (usually 0)
    @param      args            Hardware Init. arguments. Varies based on implementation

    @retval     EXIT_SUCCESS            Succuss
    @retval     EXIT_HARDWARE_FAIL      Hardware Driver Failed
    @retval     EXIT_INCORRECT_FORMAT   Partition was not formatted because its not fat16
    @retval     EXIT_ALREADY_INIT       Drive is already initilized
    @retval     EXIT_MEMORY_TABLE_FAIL  The memory table driver failed
    @retval     EXIT_READ_FAIL          Device Read Failed
    @retval     EXIT_WRITE_FAIL         Device Write Failed
    @retval     EXIT_INVALID_DEVICE     Device is unrecognizable
    @retval     Others                  Fail
*/
EXIT_STATUS FSInit(uint8_t partition, void *args) {

    flg = 0;
    Block *buf = (Block*) DeviceMemory[0];

    // Call this only once. Calling eject will reset so this can be called again
    if (flg & FS_ACTIVE) return EXIT_ALREADY_INIT;

    if (MT_TableInit() != 0) return EXIT_MEMORY_TABLE_FAIL;
    
    
    // Call the hardare init function
    if (hardware_init(args)) return EXIT_HARDWARE_FAIL;

    // Load the Master Boot Record into the memory table
    if (MT_DeviceRead((uint8_t*)buf, 0, 0, SECTOR_SIZE) != SECTOR_SIZE) {
        return EXIT_READ_FAIL;
    }

    flg |= FS_ACTIVE;
    EXIT_STATUS stat = FSMount(partition);
    if (stat == EXIT_SUCCESS) flg |= FS_ACTIVE;
    return stat;

}


/*
    Eject a fat16 File System. This method should be called to ensure that all
    sectors are written back to the device and that hardware is deinitilized

    @param      args            Hardware eject arguments. Varies based on implementation

    @retval     EXIT_SUCCESS            Succuss
    @retval     EXIT_HARDWARE_FAIL      Hardware Driver Failed
    @retval     EXIT_MEMORY_TABLE_FAIL  The memory table driver failed
    @retval     Others                  Fail    

*/
EXIT_STATUS FSEject(void *args) {

    if (0 != MT_TableUnload()) {
        return EXIT_MEMORY_TABLE_FAIL;
    }

    if (0 != hardware_eject(args)) {
        return EXIT_HARDWARE_FAIL;
    }

    flg &= ~FS_ACTIVE;

    return EXIT_SUCCESS;

}


/*
    Mount a fat16 File System. This is also reformat a partition if set

    @param      partition           Physical MBR partition number which is to be used. Set REFORMAT
                                    flag to reformat the partition

    @retval    EXIT_SUCCESS             Succuss
    @retval    EXIT_INCORRECT_FORMAT    Partition is not fat16
    @retval    EXIT_ALREADY_INIT        Init already called
    @retval    EXIT_MEMORT_TABLE_FAIL   The memory table failed
    @retval    EXIT_READ_FAIL           Device Read failed
    @retval    EXIT_WRITE_FAIL          Device write failed
    @retval    EXIT_INVALID_DEVICE      Device is unrecknoizable
    @retval    others                   Fail      

*/
EXIT_STATUS FSMount(uint8_t partition) {

    Block buf;
    for (uint16_t i = 0; i < sizeof(Block); i++) buf.data[i] = 0;
    
    // Read the Master Boot Record
    if (MT_DeviceRead((uint8_t*)&buf, 0, 0, SECTOR_SIZE) != 0) {
        return EXIT_READ_FAIL;
    }

    // Check if MBR is valid
    if (buf.MBR.Signature != MBR_SIGNATURE) return EXIT_INVALID_DEVICE;

    // Check if the partition number points to a valid partition
    for (uint8_t i = 0; i < 16; i++) {
        if (((char*)&buf.MBR.PartitionRecord[partition])[i]) break;
        if (i == 15) return EXIT_INCORRECT_FORMAT;
    }

    // Check if valid partition is fat16 (if REFORMAT is not set)
    if (!(partition & REFORMAT)) {

        if (buf.MBR.PartitionRecord[partition].OSType != MBR_fat16_CHS &&
            buf.MBR.PartitionRecord[partition].OSType != MBR_fat16_LBA) {
            return EXIT_INCORRECT_FORMAT;
        }

    } else {
        buf.MBR.PartitionRecord[partition].OSType = MBR_fat16_LBA;
    }


    // If REFORMAT is set, write the PBS
    BS = (BootSector*) MT_SetPermanent(buf.MBR.PartitionRecord[partition].StartingLBA);
    if (BS == NULL) return EXIT_MEMORY_TABLE_FAIL;

    if (partition & REFORMAT) {
        
        BS->BS_jmpBoot[0] = 0xEB;
        BS->BS_jmpBoot[1] = 0x00; // Not important
        BS->BS_jmpBoot[2] = 0x90;
        strcpy(BS->BS_OEMName, "MSWIN4.1");
        BS->BPB_BytesPerSec = SECTOR_SIZE;
        uint32_t size_MB = (buf.MBR.PartitionRecord[partition].SizeInLBA * SECTOR_SIZE) / (1024 * 1024);
        
        if (size_MB < 9) BS->BPB_SecPerClus = 16;
        else if (size_MB < 1025) BS->BPB_SecPerClus = 32;
        else BS->BPB_SecPerClus = 64;
        if (BS->BPB_SecPerClus * SECTOR_SIZE > 0x8000) {
            BS->BPB_SecPerClus = 0x8000 / SECTOR_SIZE;
        }

        BS->BPB_RsvdSecCnt = 1;
        BS->BPB_NumFATs = 2;
        BS->BPB_RootEntCnt = 512;
        BS->BPB_TotSec16 = 0;
        BS->BPB_Media = 0xF0;
        BS->BPB_FATSz16 = 1 + (BS->BPB_TotSec32 / BS->BPB_SecPerClus) / (SECTOR_SIZE / 4);
        
        if (size_MB < 3) BS->BPB_SecPerTrk = 16;
        else if (size_MB < 65) BS->BPB_SecPerTrk = 32;
        else BS->BPB_SecPerTrk = 64;

        if (size_MB < 129) BS->BPB_NumHeads = 128;
        else BS->BPB_NumHeads = 255;

        BS->BPB_HiddSec = buf.MBR.PartitionRecord[partition].StartingLBA;
        BS->BPB_TotSec32 = buf.MBR.PartitionRecord[partition].SizeInLBA;
        
        BS->BS_DrvNum = 0x80;
        BS->BS_Reserved1 = 0;
        BS->BS_BootSig = 0x29;
        BS->BS_VolID = 0x12345678; // Randomly generate
        strcpy(BS->BS_VolLab, "fat16 PART ");
        strcpy(BS->BS_FilSysType, "fat16   ");

        if (BS->BPB_TotSec32 <= 0xFFFF) {
            BS->BPB_TotSec16 = BS->BPB_TotSec32;
            BS->BPB_TotSec32 = 0;
        }
    }

    // General PBS setting
    BS->MBR_Part = buf.MBR.PartitionRecord[partition];
    BS->MBR_Part_No = partition;
    BS->PAR_Max_Cluster = (BS->BPB_TotSec32 - (BS->BPB_NumFATs * BS->BPB_FATSz32) - BS->BPB_RsvdSecCnt) / BS->BPB_SecPerClus;
    if (SECTOR_SIZE != MT_DeviceWrite((char*)BS, buf.MBR.PartitionRecord[partition].StartingLBA, 0, SECTOR_SIZE)) {
        return EXIT_WRITE_FAIL;
    }

    
    BS->FSI_Free_Count = buf.File.FSI_Free_Count;
    BS->FSI_Nxt_Free = buf.File.FSI_Nxt_Free;
    
    // REFORMAT FAT setting
    if (partition & REFORMAT) {

        for (uint16_t i = 0; i < SECTOR_SIZE; i++) buf.data[i] = 0;

        // Zero out the FAT table
        for (uint32_t sec = BS->BPB_HiddSec + BS->BPB_RsvdSecCnt;
        sec < BS->BPB_HiddSec + BS->BPB_RsvdSecCnt + BS->BPB_FATSz32 * BS->BPB_NumFATs;
        sec++) {

            if (SECTOR_SIZE != MT_DeviceWrite((char*)&buf, sec, 0, SECTOR_SIZE)) {
                return EXIT_WRITE_FAIL;
            }

        }

        buf.FAT[0] = 0x0F + BS->BPB_Media;
        buf.FAT[1] = 0x0FFF;
        buf.FAT[2] = 0xFFF8;     // Root directory

        // Write to the first sector of each FAT
        for (uint8_t i = 0; i < BS->BPB_NumFATs; i++) {
            if (SECTOR_SIZE != MT_DeviceWrite((char*)&buf, BS->BPB_HiddSec + BS->BPB_RsvdSecCnt + i * BS->BPB_FATSz32, 0, SECTOR_SIZE)) {
                return EXIT_WRITE_FAIL;
            }
        }

        // Zero out the root directory
        buf.FAT[0] = 0;
        buf.FAT[1] = 0;
        buf.FAT[2] = 0;
        for (uint32_t sec = BS->BPB_HiddSec + BS->BPB_RsvdSecCnt + BS->BPB_FATSz32 * BS->BPB_NumFATs;
        sec < BS->BPB_HiddSec + BS->BPB_RsvdSecCnt + BS->BPB_FATSz32 * BS->BPB_NumFATs + BS->BPB_SecPerClus;
        sec++) {

            if (SECTOR_SIZE != MT_DeviceWrite((char*)&buf, sec, 0, SECTOR_SIZE)) {
                return EXIT_WRITE_FAIL;
            }

        }

    }

    return EXIT_SUCCESS;
}


/*
    Read the contents from a file and place them in a buffer

    @param          data                Buffer to place data in
    @param          offset              File offset to begin reading
    @param          len                 Length to read file
    @param          file                FAT File entry to read

    @return         bytes               Amount of bytes read

*/
uint32_t FSReadFile(uint8_t *data, uint32_t offset, uint32_t len, FILE *file) {


    uint32_t bytesPerCluster = SECTOR_SIZE * BS->BPB_SecPerClus;
    uint32_t bytes = 0;
    uint64_t currOffset = offset;
    uint32_t fileSize = file->file.ShortEntry.DIR_FileSize;
    uint16_t currCluster = file->file.ShortEntry.DIR_FstClusHI << 16 + file->file.ShortEntry.DIR_FstClusLO;

    if (offset > fileSize) return 0;

    // Traverse to the starting cluster
    for (uint32_t i = 0; i < offset/bytesPerCluster; i++) {

        currCluster = FSGetFatTableEntry(currCluster) & FAT_MASK;
        if ((currCluster & FAT_MASK) > FAT_DEFECTIVE) return 0;

    }

    if (len < bytesPerCluster - (offset % bytesPerCluster)) {

        if (fileSize - offset < len) {
            currOffset += fat16ReadCluster(data, currCluster, offset % bytesPerCluster, fileSize - offset);
            return currOffset - offset;
        }

        currOffset += fat16ReadCluster(data, currCluster, offset % bytesPerCluster, len);
        return currOffset - offset;

    } else {
        
        if (fileSize - offset < bytesPerCluster - (offset % bytesPerCluster)) {
            currOffset += fat16ReadCluster(data, currCluster, offset % bytesPerCluster, fileSize - offset);
            return currOffset - offset;  
        }

        currOffset += fat16ReadCluster(data, currCluster, offset % bytesPerCluster, bytesPerCluster - (offset % bytesPerCluster));
    }

    currCluster = FSGetFatTableEntry(currCluster);

    // Keep reading clusters until the EOC cluster is reached or length reached OR end of file reached
    while ((currOffset - offset) < len && currCluster < FAT_DEFECTIVE && currOffset < fileSize) {

        if (len - (currOffset - offset) < bytesPerCluster) {

            if (fileSize - currOffset < len - (currOffset - offset)) {
                currOffset += fat16ReadCluster(&data[(currOffset - offset)], currCluster, 0, fileSize - currOffset);
                return currOffset - offset;
            }

            currOffset += fat16ReadCluster(&data[(currOffset - offset)], currCluster, 0, len - (currOffset - offset));
            return currOffset - offset;

        } else {

            if (fileSize - currOffset < bytesPerCluster) {
                bytes = fat16ReadCluster(&data[(currOffset - offset)], currCluster, 0, fileSize - currOffset);
                return bytes + (currOffset - offset);
            }

            currOffset += fat16ReadCluster(&data[(currOffset - offset)], currCluster, 0, bytesPerCluster);

        }

        currCluster = FSGetFatTableEntry(currCluster);

    }

    return (currOffset - offset); // should always equal len if the function exits here


}


/*
    Write to a file and place the contents in a buffer

    @param          data                Buffer to read data from
    @param          offset              File offset to begin writing
    @param          len                 Length to write file
    @param          file                FAT File entry to write

    @return         bytes               Amount of bytes read

*/
uint32_t FSWriteFile(uint8_t *data, uint32_t offset, uint32_t len, FILE *file) {

    if (offset > file->file.ShortEntry.DIR_FileSize) return 0;

    uint32_t fileSize = file->file.ShortEntry.DIR_FileSize;
    uint64_t currOffset = offset;
    uint16_t currCluster = file->file.ShortEntry.DIR_FstClusHI << 16 + file->file.ShortEntry.DIR_FstClusLO;
    uint32_t bytesPerCluster = SECTOR_SIZE * BS->BPB_SecPerClus;
    uint32_t bytesWritten = 0;

    // Traverse to the starting cluster
    for (uint32_t i = 0; i < offset/bytesPerCluster; i++) {

        currCluster = FSGetFatTableEntry(currCluster) & FAT_MASK;
        if ((currCluster & FAT_MASK) > FAT_DEFECTIVE) return 0;

    }

    if (len <= bytesPerCluster - (offset % bytesPerCluster) || 
        MAX_FILE_SIZE - offset < bytesPerCluster - (offset % bytesPerCluster)) {

        if (MAX_FILE_SIZE - offset < bytesPerCluster - (offset % bytesPerCluster)) {
            currOffset += fat16WriteCluster(data, currCluster, offset % bytesPerCluster, MAX_FILE_SIZE - offset);
        }
        
        currOffset += fat16WriteCluster(data, currCluster, offset % bytesPerCluster, len);
       
        // If the file size was increased, update the file size
        if (currOffset > fileSize) {
            file->file.ShortEntry.DIR_FileSize = currOffset;
        }
        return currOffset - offset;

    } else {
        currOffset += fat16WriteCluster(data, currCluster, offset % bytesPerCluster, bytesPerCluster - (offset % bytesPerCluster));
    }


    while (currOffset - offset < len && currOffset < MAX_FILE_SIZE) {

        // Get next cluster or allocate a cluster, if nessesary
        if (FSGetFatTableEntry(currCluster) >= FAT_DEFECTIVE) {
            currCluster = FSAllocateCluster(currCluster);
        } else {
            currCluster = FSGetFatTableEntry(currCluster);
        }

        if (len - (currOffset - offset) < bytesPerCluster) {

            currOffset += fat16WriteCluster(&data[currOffset - offset], currCluster, 0, len - (currOffset - offset));
            if (currOffset > fileSize) {
                file->file.ShortEntry.DIR_FileSize = currOffset;
            }
            return currOffset - offset;

        } else {

            currOffset += fat16WriteCluster(&data[currOffset - offset], currCluster, 0, bytesPerCluster);

        }
    }

    if (currOffset > fileSize) {
        file->file.ShortEntry.DIR_FileSize = currOffset;
    }

    // If MAX_SIZE is reached
    if (currOffset > MAX_FILE_SIZE) {
        file->file.ShortEntry.DIR_FileSize = MAX_FILE_SIZE;
        currOffset -= 1;
    }
    
    return currOffset - offset;

}


/*
    Create a file or directory.

    @param      file                Directory to add file to
    @param      flags               File flags to add

    @return


*/
EXIT_STATUS FSCreateFile(FILE *file, FILE *dir, uint8_t *name, uint8_t flags, FileTime *time) {

    uint16_t cluster = dir->file.ShortEntry.DIR_FstClusHI << 16 + dir->file.ShortEntry.DIR_FstClusLO;
    uint16_t oldCluster = cluster;
    uint32_t totalOffset = 0;
    uint16_t off = 0;
    uint32_t bytesPerCluster = SECTOR_SIZE * BS->BPB_SecPerClus;
    uint16_t longEntries = 0;
    EXIT_STATUS status;
    
    if (name != NULL) {
        longEntries = ((strlen(name) - 1) / 13) + 1;
    }
    
    // Iterate through directory and see where the first free entry is
    while (cluster & FAT_MASK != FAT_EOC) {

        oldCluster = cluster;

        for (off = 0; off < bytesPerCluster; off+=32) {
            uint8_t stat = 1;
            status = fat16ReadCluster(&stat, cluster, off, 1);
            if (status != EXIT_SUCCESS) return status;
            if (stat == REST_FREE_ENTRY) goto endFound;
            totalOffset += 32;
        }

        cluster = FSGetFatTableEntry(cluster);
    }

    // If reached the end of dir and need a new cluster allocated
    cluster = FSAllocateCluster(oldCluster);
    oldCluster = cluster;

  endFound:

    // Create the long directory entries
    file->file.LongEntry.LDIR_Attr = ATTR_LONG_NAME;
    file->file.LongEntry.LDIR_Checksum = 0xFF;
    file->file.LongEntry.LDIR_FstClusLO = 0;
    file->file.LongEntry.LDIR_Ord = longEntries | LAST_LONG_ENTRY;
    file->file.LongEntry.LDIR_Type = 0;
    fat16StrCpy(&name[13 * longEntries], &file->file);

    if (off == 0) {
        cluster = FSGetFatTableEntry(cluster);
        if (cluster == FAT_EOC) {
            cluster = FSAllocateCluster(oldCluster);
            oldCluster = cluster;
        }
    }
    off += 32;
    if (off == bytesPerCluster) off = 0;

    status = fat16WriteCluster((char*)&file->file, cluster, off, sizeof(FileEntry));
    if (status != EXIT_SUCCESS) return status;

    while (--longEntries) {

        file->file.LongEntry.LDIR_Ord = longEntries;
        fat16StrCpy(&name[13 * longEntries], &file->file);

        if (off == 0) {
            cluster = FSGetFatTableEntry(cluster);
            if (cluster == FAT_EOC) {
                cluster = FSAllocateCluster(oldCluster);
                oldCluster = cluster;
            }
        }
        off += 32;
        if (off == bytesPerCluster) off = 0;

        status = fat16WriteCluster((char*)&file->file, cluster, off, sizeof(FileEntry));
        if (status != EXIT_SUCCESS) return status;
    }
    

    // Create the short directory entry with FILE WRITE
    file->file.ShortEntry.DIR_Attr = flags;
    file->file.ShortEntry.DIR_CrtDate = 0b000000000100001; // 1/1/1980
    file->file.ShortEntry.DIR_CrtTime = 0;
    file->file.ShortEntry.DIR_CrtTimeTenth = 0;
    file->file.ShortEntry.DIR_FileSize = 0;
    file->file.ShortEntry.DIR_FstClusHI = 0;
    file->file.ShortEntry.DIR_FstClusLO = 0;
    file->file.ShortEntry.DIR_LstAccDate = 0b000000000100001;
    strcpy((char*)&file->file.ShortEntry.DIR_Name, "ABCDEFGHIJK");
    file->file.ShortEntry.DIR_NTRes = 0;
    file->file.ShortEntry.Dir_WrtDate = 0b000000000100001;
    file->file.ShortEntry.DIR_WrtTime = 0;

    if (off == 0) {
        cluster = FSGetFatTableEntry(cluster);
        if (cluster == FAT_EOC) {
            cluster = FSAllocateCluster(oldCluster);
            oldCluster = cluster;
        }
    }
    off += 32;
    if (off == bytesPerCluster) off = 0;

    status = fat16WriteCluster((char*)&file->file, cluster, off, sizeof(FileEntry));
    if (status != EXIT_SUCCESS) return status;
    
    file->dirOffset = totalOffset;
    file->dir = dir;
    file->len = strlen(name);

    // Update the time
    FSChangeAttribues(file, flags, time, NULL);

    // If DIR is set, create the '.' and '..' entries
    if (flags & ATTR_DIRECTORY) {

        // Allocate a cluster to the directory
        oldCluster = FSAllocateCluster(0);
        file->file.ShortEntry.DIR_FstClusHI = oldCluster >> 16;
        file->file.ShortEntry.DIR_FstClusLO = oldCluster & 0xFFFF;
        status = fat16WriteCluster((char*)&file->file, cluster, off, sizeof(FileEntry));
        if (status != EXIT_SUCCESS) return status;

        // Create '.' entry and write it to new file
        FILE entry;
        entry.dir = file;
        entry.dirOffset = 0;
        entry.len = 1;
        entry.file.ShortEntry.DIR_Attr = flags | ATTR_HIDDEN;
        entry.file.ShortEntry.DIR_CrtDate = 0b000000000100001;
        entry.file.ShortEntry.DIR_CrtTime = 0;
        entry.file.ShortEntry.DIR_CrtTimeTenth = 0;
        entry.file.ShortEntry.DIR_FileSize = 0;
        entry.file.ShortEntry.DIR_FstClusHI = oldCluster >> 16;
        entry.file.ShortEntry.DIR_FstClusLO = oldCluster & 0xFFFF;
        entry.file.ShortEntry.DIR_LstAccDate = 0b000000000100001;
        strcpy(entry.file.ShortEntry.DIR_Name, ".          ");
        entry.file.ShortEntry.DIR_NTRes = 0;
        entry.file.ShortEntry.Dir_WrtDate = 0b000000000100001;
        entry.file.ShortEntry.DIR_WrtTime = 0;
        status = FSChangeAttribues(&entry, flags, time, NULL);
        if (status != EXIT_SUCCESS) return status;
        
        status = fat16WriteCluster((char*)&entry.file, oldCluster, 0, sizeof(FileEntry));
        if (status != EXIT_SUCCESS) return status;

        // Create '..' Entry and write it to new file
        entry.dirOffset = 32;
        entry.len = 2;
        entry.file.ShortEntry.DIR_FstClusHI = entry.dir->file.ShortEntry.DIR_FstClusHI;
        entry.file.ShortEntry.DIR_FstClusLO = entry.dir->file.ShortEntry.DIR_FstClusLO;
        strcpy(entry.file.ShortEntry.DIR_Name, "..         ");

        status = FSChangeAttribues(&entry, flags, time, NULL);
        if (status != EXIT_SUCCESS) return status;

        status = fat16WriteCluster((char*)&entry.file, oldCluster, 32, sizeof(FileEntry));
        if (status != EXIT_SUCCESS) return status;
    }

    return EXIT_SUCCESS;
}


/*
    Remove a file or directory.

    @param      name                ASCII string for directory name
    @param      file                Directory to remove file from

    @return     EXIT_SUCCESS        File was succussfully removed
    @return     EXIT_NOT_EXIST      The file has already been removed
    @return     

*/
EXIT_STATUS FSRemoveFile(FILE *file) {

    char entry;
    char attr = ATTR_LONG_NAME;
    char off = 32;
    char freeEntry = FREE_ENTRY;
    EXIT_STATUS status;
    uint32_t bytesPerCluster = BS->BPB_SecPerClus * SECTOR_SIZE;
    uint16_t cluster = 0;

    status = FSReadFile(
        &entry,
        file->dirOffset,
        1,
        file->dir
    );

    if (status != EXIT_SUCCESS) return status;

    if (entry == FREE_ENTRY || entry == REST_FREE_ENTRY) return EXIT_NOT_EXIST;

    status = FSWriteFile(
        &freeEntry,
        file->dirOffset,
        1,
        file->dir
    );

    if (status != EXIT_SUCCESS) return status;

    if (file->dirOffset == 0) return EXIT_SUCCESS;

    // Read previous long entry names until a short name is found
    status = FSReadFile(
        &attr,
        file->dirOffset - off + 11,
        1,
        file->dir
    );

    if (status != EXIT_SUCCESS) return status;

    while (attr == ATTR_LONG_NAME) {

        status = FSWriteFile(
            &freeEntry,
            file->dirOffset,
            1,
            file->dir
        );

        status = FSReadFile(
            &attr,
            file->dirOffset + 11,
            1,
            file->dir
        );

        if (status != EXIT_SUCCESS) return status;

        off -= 32;
        if (file->dirOffset == 0) return EXIT_SUCCESS;
    }

    // Remove the FAT cluster chain
    if (fat16GetDirCluster(file) == 0) return EXIT_SUCCESS;
    cluster = fat16GetDirCluster(file);

    // Free the cluster chain
    while (cluster & FAT_MASK != FAT_EOC) {

        uint16_t tempCluster = FSGetFatTableEntry(cluster);
        FSFatTableUpdate(cluster, FAT_FREE);
        cluster = tempCluster;
        BS->FSI_Free_Count += 1;

    }
    

    return EXIT_SUCCESS;
}


/*
    Change the attributes of a file or directory.

    @param      flags               File/directory flags
    @param      dir                 Directory which holds the file
    @param      time                Time to use. NULL means use no time

    @return     EXIT_SUCCUSS            Function exited succussfully
    @return     EXIT_INVALID_PARAMETER  Invalid parameter
    @return     EXIT_INVALID_TIME       Time passed was not valid
*/
EXIT_STATUS FSChangeAttribues(FILE *file, uint8_t flags, FileTime *time, uint8_t *name) {

    uint16_t    cDate;
    uint8_t     cTime;
    uint8_t     cTimeTenth;
    uint16_t    wDate;
    uint8_t     wTime;
    uint8_t     wTimeTenth;
    uint8_t     chkSum;

    FILE        currFile;

    // A file cannot be turned into a directory and vice versa
    if ((flags & ATTR_DIRECTORY) != (file->file.ShortEntry.DIR_Attr & ATTR_DIRECTORY)) {
        return EXIT_INVALID_PARAMETER;
    }

    // Check for valid date/time values
    if (time != NULL) {

        if (time->createYear < 1979 || time->createYear > 2107) return EXIT_INVALID_TIME;
        if (time->wrtYear < 1979 || time->wrtYear > 2107) return EXIT_INVALID_TIME;
        if (time->lstAccYear < 1979 || time->lstAccYear > 2107) return EXIT_INVALID_TIME;
        if (time->createMonth < 1 || time->createMonth > 12) return EXIT_INVALID_TIME;
        if (time->wrtMonth < 1 || time->wrtMonth > 12) return EXIT_INVALID_TIME;
        if (time->lstAccMonth < 1 || time->lstAccMonth > 12) return EXIT_INVALID_TIME;
        if (time->createDay < 1 || time->createDay > 31) return EXIT_INVALID_TIME;
        if (time->wrtDay < 1 || time->wrtDay > 31) return EXIT_INVALID_TIME;
        if (time->lstAccDay < 1 || time->lstAccDay > 31) return EXIT_INVALID_TIME;
        if (time->createHour > 23) return EXIT_INVALID_TIME;
        if (time->wrtHour > 23) return EXIT_INVALID_TIME;
        if (time->createMinute > 59) return EXIT_INVALID_TIME;
        if (time->wrtMinute > 59) return EXIT_INVALID_TIME;
        if (time->createSecond > 59) return EXIT_INVALID_TIME;
        if (time->wrtSecond > 59) return EXIT_INVALID_TIME;
        if (time->createTenthSec > 9) return EXIT_INVALID_TIME;

        // 30 Day Months
        if (time->createMonth == 4 || time->createMonth == 6 || time->createMonth == 9 ||
            time->createMonth == 11) {

            if (time->createDay == 31) return EXIT_INVALID_TIME;
        }

        if (time->wrtMonth == 4 || time->wrtMonth == 6 || time->wrtMonth == 9 ||
            time->wrtMonth == 11) {

            if (time->wrtDay == 31) return EXIT_INVALID_TIME;
        }

        if (time->lstAccMonth == 4 || time->lstAccMonth == 6 || time->lstAccMonth == 9 ||
            time->lstAccMonth == 11) {

            if (time->lstAccMonth == 31) return EXIT_INVALID_TIME;
        }

        if (time->createMonth == 2) {

            if (time->createYear % 4 && time->createYear != 2100) {
                if (time->createDay > 29) return EXIT_INVALID_TIME;
            } else {
                if (time->createDay > 28) return EXIT_INVALID_TIME;
            }

        }

        // February
        if (time->wrtMonth == 2) {

            if (time->wrtMonth % 4 && time->wrtMonth != 2100) {
                if (time->wrtMonth > 29) return EXIT_INVALID_TIME;
            } else {
                if (time->wrtMonth > 28) return EXIT_INVALID_TIME;
            }

        }

        if (time->lstAccMonth == 2) {

            if (time->lstAccMonth % 4 && time->lstAccMonth != 2100) {
                if (time->lstAccMonth > 29) return EXIT_INVALID_TIME;
            } else {
                if (time->lstAccMonth > 28) return EXIT_INVALID_TIME;
            }

        }
    }

    // 1. Change name if nessesary
    if (name != NULL) {

        // See if file is a duplicate
        switch (FSDirectorySearch(name, file->dir, NULL)) {
            case EXIT_SUCCESS:
                return EXIT_INVALID_PARAMETER;
            case EXIT_NOT_FOUND:
                break;
            default:
                return EXIT_FAIL;
        }

        // Remove the old file and create a new file TODO ERROR CHECKING
        memcpy(&currFile, &file, sizeof(FILE));
        FSRemoveFile(file);
        FSCreateFile(file, currFile.dir, name, currFile.file.ShortEntry.DIR_Attr, NULL);

    }

    // 2. Change date/time if nessesary
    if (time != NULL) {

        file->file.ShortEntry.DIR_CrtDate = time->createDay;
        file->file.ShortEntry.DIR_CrtDate += time->createMonth << 5;
        file->file.ShortEntry.DIR_CrtDate += (time->createYear - 1980) << 9;

        file->file.ShortEntry.Dir_WrtDate = time->wrtDay;
        file->file.ShortEntry.Dir_WrtDate += time->wrtMonth << 5;
        file->file.ShortEntry.Dir_WrtDate += (time->wrtYear - 1980) << 9;

        file->file.ShortEntry.DIR_LstAccDate = time->lstAccDay;
        file->file.ShortEntry.DIR_LstAccDate += time->lstAccMonth << 5;
        file->file.ShortEntry.DIR_LstAccDate += (time->lstAccYear - 1980) << 9;

        file->file.ShortEntry.DIR_CrtTime = time->createSecond / 2;
        file->file.ShortEntry.DIR_CrtTime = time->createMinute << 5;
        file->file.ShortEntry.DIR_CrtTime = time->createHour << 11;

        file->file.ShortEntry.DIR_WrtTime = time->wrtSecond / 2;
        file->file.ShortEntry.DIR_WrtTime = time->wrtMinute << 5;
        file->file.ShortEntry.DIR_WrtTime = time->wrtHour << 11;

        file->file.ShortEntry.DIR_CrtTimeTenth = time->createTenthSec + 10 * (time->createSecond % 2);
    }


    // 3. Change flags
    file->file.ShortEntry.DIR_Attr = flags;

    // Recalculate checksum and populate long directory entries with it
    if (file->len <= 12) {

        // If the entry before is a short name, then this is over.
        uint8_t attr;
        FSReadFile(&attr, file->dirOffset - 32 + 11, 1, file->dir);
        if (attr != ATTR_LONG_NAME) return EXIT_SUCCESS;

    }
    
    if (EXIT_SUCCESS != fat16FileToDisk(file)) {
        return EXIT_WRITE_FAIL;
    }

    return EXIT_SUCCESS;
}


/*
    Open a file or directory.
    OPEN FILES MUST BE CLOSED TO AVOID UNDEFINED BEHAVIOUR

    @param      dir                 Directory which file is in
    @param      file                Empty file for the new file

    @return     EXIT_SUCCESS        File was opened
    @return     EXIT_NOT_FOUND      File did not exist in directory
    @return     others              Other failure

*/
EXIT_STATUS FSOpen(uint8_t *name, FILE *dir, FILE *file) {

    return FSDirectorySearch(name, dir, file);
}


/*
    Close a file (or directory) and write it back to the disk.

    @param      file                File which is to be closed

    @return     EXIT_SUCCESS        File was closed

*/
EXIT_STATUS FSClose(FILE *file) {

    return fat16FileToDisk(file);
}
