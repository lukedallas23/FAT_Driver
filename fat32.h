#include <stdint.h>
#include <string.h>
#include "MemoryTable.h"
#include "device.h"


/////////////////////// DATA STRUCTURES & CONSTANTS ////////////////

// Typedefs
typedef int8_t bool;
#define FALSE 0
#define TRUE 1

// 
// Status Register
//
#define FS_ACTIVE 0x0001
uint16_t flg;


//
// Master Boot Record
//
#define REFORMAT 8
#define MBR_SIGNATURE 0x55AA
#define MBR_FAT32_CHS 0x0B
#define MBR_FAT32_LBA 0x0C

//
// FS Info
//
#define FSI_LEAD_SIG    0x41615252
#define FSI_STR_SIG     0x61417272
#define FSI_TRAIL_SIG   0xAA550000


//
// FAT Tables
//
#define FAT_DEFECTIVE 0x0FFFFFF7
#define FAT_EOC 0x0FFFFFF8
#define FAT_MASK 0x0FFFFFFF
#define FAT_FREE 0x00000000

//
// Files & Directories
//
#define ATTR_READ_ONLY 0x01
#define ATTR_HIDDEN 0x02
#define ATTR_SYSTEM 0x04
#define ATTR_VOLUME_ID 0x08
#define ATTR_DIRECTORY 0x10
#define ATTR_ARCHIVE 0x20
#define ATTR_LONG_NAME ATTR_READ_ONLY|ATTR_HIDDEN|ATTR_SYSTEM|ATTR_VOLUME_ID
#define FREE_ENTRY 0xE5
#define REST_FREE_ENTRY 0x00
#define FREE_ENTRY_JP 0x05
#define LAST_LONG_ENTRY 0x40

#define MAX_FILE_SIZE 0xFFFFFFFFU   // 4 GB Max file size

//
// Struct for Partition within Master Boot Record
//
typedef struct MBR_partition_t {

    char        BootIndicator;
    char        StartHead;
    char        StartSector;
    char        StartTrack;
    char        OSType;
    char        EndHead;
    char        EndSector;
    char        EndTrack;
    uint32_t    StartingLBA;
    uint32_t    SizeInLBA;

} MBR_partition;


//
// Struct for Master Boot Record
//
typedef struct MBR_t {

    char            BootCode[424];
    uint32_t        UniqueMBRDiskSignature;
    uint16_t        Unknown;
    MBR_partition   PartitionRecord[4];
    uint16_t        Signature;

} MasterBootRecord;


//
// Struct for Partition Sector 0, Partition Boot Sector
//
typedef struct BootSector_t {

    char            BS_jmpBoot[3];
    char            BS_OEMName[8];
    uint16_t        BPB_BytesPerSec;
    uint8_t         BPB_SecPerClus;
    uint16_t        BPB_RsvdSecCnt;
    uint8_t         BPB_NumFATs;
    uint16_t        BPB_RootEntCnt;
    uint16_t        BPB_TotSec16;
    uint8_t         BPB_Media;
    uint16_t        BPB_FATSz16;
    uint16_t        BPB_SecPerTrk;
    uint16_t        BPB_NumHeads;
    uint32_t        BPB_HiddSec;
    uint32_t        BPB_TotSec32;
    uint32_t        BPB_FATSz32;
    uint16_t        BPB_ExtFlags;
    uint16_t        BPB_FSVer;
    uint32_t        BPB_RootClus;
    uint16_t        BPB_FSInfo;
    uint16_t        BPB_BkBootSec;
    char            BPB_Reserved[12];
    uint8_t         BS_DrvNum;
    uint8_t         BS_Reserved1;
    uint8_t         BS_BootSig;
    uint32_t        BS_VolID;
    char            BS_VolLab[11];
    char            BS_FilSysType[8];
    uint8_t         MBR_Part_No;        // Not part of windows spec
    MBR_partition   MBR_Part;           // Not part of windows spec
    uint32_t        FSI_Free_Count;     // Not part of windows spec
    uint32_t        FSI_Nxt_Free;       // Not part of windows spec
    uint32_t        PAR_Max_Cluster;    // Not part of windows spec
    char            Reserved[391];      // 420 bytes in windows spec
    uint16_t        Signature;

} BootSector;


//
// File System Info Sector
//
typedef struct FSInfo_t {

    uint32_t    FSI_LeadSig;
    char        FSI_Reserved1[480];
    uint32_t    FSI_StrucSig;
    uint32_t    FSI_Free_Count;
    uint32_t    FSI_Nxt_Free;
    char        FSI_Reserved2[12];
    uint32_t    FSI_TrailSig;

} FSInfo;

//
// Short Directory Entry
//
typedef struct DirEntry_t {

    char        DIR_Name[11];       // Short name entry
    uint8_t     DIR_Attr;           // See Flags
    uint8_t     DIR_NTRes;          // Always 0
    uint8_t     DIR_CrtTimeTenth;   // Tenths of second, 0-199
    uint16_t    DIR_CrtTime;        // Time of creation
    uint16_t    DIR_CrtDate;        // Date of creation
    uint16_t    DIR_LstAccDate;     // Last Access Date
    uint16_t    DIR_FstClusHI;      // High Word of entries 1st cluster
    uint16_t    DIR_WrtTime;        // Time of last write
    uint16_t    Dir_WrtDate;        // Date of last write
    uint16_t    DIR_FstClusLO;      // Low Word of entries 1st cluster
    uint32_t    DIR_FileSize;       // File size in bytes

} DirEntry;


//
// Long Directory Entry
//
typedef struct LongDirEntry_t {

    uint8_t     LDIR_Ord;           // Order of this entry
    uint16_t    LDIR_Name1[5];      // Chars 1 to 5
    uint8_t     LDIR_Attr;          // Always ATTR_LONG_NAME
    uint8_t     LDIR_Type;          // Should be 0
    uint8_t     LDIR_Checksum;      // Checksum of short dir entry
    uint16_t    LDIR_Name2[6];      // Chars 6-11
    uint16_t    LDIR_FstClusLO;     // Always 0
    uint16_t    LDIR_Name3[2];      // Chars 12-13

} LongDirEntry;

//
// File Entry
//
typedef union FileEntry_t {

    DirEntry        ShortEntry;    
    LongDirEntry    LongEntry;

} FileEntry;

//
// Working Block
//
typedef union Working_Block_t {

    MasterBootRecord        MBR;
    BootSector              BootSector;
    FSInfo                  File;
    FileEntry               Entry[SECTOR_SIZE/32];
    uint32_t                FAT[SECTOR_SIZE/4];
    char                    data[SECTOR_SIZE];

} Block;


BootSector *BS;


// File Name Rules
// 1. DIR_Name[0] = 0xE5 is illegal and this is free
// 2. DIR_Name[0] = 0x00 means that this and the whole rest of dir is free
// 3. DIR_Name[0] = 0x20 is illegal, but is legal for other spaces
// 4. All names must be unique
// 5. Lower Case are not allowed.
// 6. '.' and '..' are at the beggining of directories

char ShortNameIllegal[16] = {'"', '*', '+', ',', '.', '/', ':', 
';', '<', '=', '>', '?', '[', '\\', ']', '|'};

char LongNameIllegal[11] = {'"', '*', '.', '/', ':', '<', '>', '?', '\\', '|'};


//
//    Struct for File Date and Time Creation
//
typedef struct FileTime_t {

    uint16_t    createYear;
    uint8_t     createMonth;
    uint8_t     createDay;
    uint8_t     createHour;
    uint8_t     createMinute;
    uint8_t     createSecond;
    uint8_t     createTenthSec;
    uint16_t    wrtYear;
    uint8_t     wrtMonth;
    uint8_t     wrtDay;
    uint8_t     wrtHour;
    uint8_t     wrtMinute;
    uint8_t     wrtSecond;
    uint16_t    lstAccYear;
    uint8_t     lstAccMonth;
    uint8_t     lstAccDay;

} FileTime;


//
// Default time to use
//
FileTime DefaultTime = {
    1980,
    1,
    1,
    0,
    0,
    0,
    0,
    1980,
    1,
    1,
    0,
    0,
    0,
    1980,
    1,
    1
};


//
// Struct which holds file information
//
typedef struct FILE_t {

    FileEntry           file;           // Short File Entry
    uint32_t            len;            // File Name Length
    struct FILE_t       *dir;           // Dir that file is in
    uint32_t            dirCluster;     // Cluster directory is in
    uint32_t            dirOffset;      // Dir Entry Offset 

} FILE;



//
//  Exit Status Codes
//
typedef enum {
    EXIT_SUCCESS,               // Completed succussfully
    EXIT_HARDWARE_FAIL,         // Hardware failed
    EXIT_INCORRECT_FORMAT,      // Not FAT32 formatted
    EXIT_ALREADY_INIT,          // Trying to init twice
    EXIT_MEMORY_TABLE_FAIL,     // Memory table failed
    EXIT_READ_FAIL,             // Device read failed
    EXIT_WRITE_FAIL,            // DEvice write failed
    EXIT_INVALID_DEVICE,        // Device is invalid
    EXIT_NOT_FOUND,             // A search did not find
    EXIT_INVALID_PARAMETER,     // Invalid parameter passed
    EXIT_FAIL,                  // Unknown failure
    EXIT_INVALID_TIME,          // Date/Time is invalid
    EXIT_NOT_EXIST              // Struct does not exist

} EXIT_STATUS;



///////////////////  FAT32 HELPER FUNCTIONS /////////////////////////////

/*
    Check if a string is compliant with a long directory entry.

    @param      IN  name            File/directory name
    @param      IN  len             Length of name
    @param      OUT new             File/directory to be returned

    @retval     TRUE                File and name are matching
    @retval     FALSE               File and name do not match
*/
bool fat32IsValidLongEntry(char *name, uint16_t len, FILE *file);


/*
    Check if a string is compliant with a short directory entry.

    @param      IN  name            File/directory name
    @param      IN  len             Length of name
    @param      OUT new             File/directory to be returned

    @retval     TRUE                File and name are matching
    @retval     FALSE               File and name do not match
*/
bool fat32IsValidShortEntry(char *name, uint16_t len, FILE *file);


/*
    Check if a string matches a long directory entry

    @param      IN  file            File/directory entry
    @param      IN  str             ASCII Name to compare
    @param      IN  len             Length of string

    @retval     0                   Match
    @retval     1                   No Match
*/
uint8_t fat32StrCmp(char *str, uint16_t len, FILE *file);

/*
    Read from a cluster and put it in a buffer.

    @param      data        Buffer to place read data in
    @param      cluster     Cluster to read
    @param      offset      Bytes offset to read
    @param      len         Length in bytes to read

    @retval     0           Nothing was read
    @retval     > 0         Number of bytes read

*/
uint16_t fat32ReadCluster(uint8_t *data, uint32_t cluster, uint32_t offset, uint32_t len);



/*
    Write to a cluster from a buffer.

    @param      data        Buffer to read from to write
    @param      cluster     Cluster to write
    @param      offset      Bytes offset to write
    @param      len         Length in bytes to write

    @retval     0           Nothing was written
    @retval     > 0         Number of bytes written

*/
uint16_t fat32WriteCluster(uint8_t *data, uint32_t cluster, uint32_t offset, uint32_t len);


/*  
    Calculate the checksum value for a file name entry.

    @param      pFcbName    Pointer to an unsigned byte array assumed
                            to be 11 bytes long
    
    @returns    Sum         An 8-bit unsigned checksum of the array pointed
                            to by pFcbName


*/
uint8_t fat32ChkSum(uint8_t *pFcbName);

/*
    Write an updated short file entry back to the disk and update the checksum
    for long directory entries.

    @param      file            File to write back to disk

    @retval    EXIT_SUCCUSS     Succussful
    @retval    EXIT_WRITE_FAIL  Disk write failed.

*/
EXIT_STATUS fat32FileToDisk(FILE *file);


/*
    Get the first cluster of the directory that a file is in.

    @param      file            File to write back to disk

    @retval    EXIT_SUCCUSS     Succussful
    @retval    EXIT_WRITE_FAIL  Disk write failed.

*/
uint32_t fat32GetDirCluster(FILE *file);

///////////////////  FILE SYSTEM FUNCTIONS //////////////////////////////



/*
    Allocate a new cluster to the end of a file

    @param      from                File End Of Cluster file, or zero, if there is not a cluster allocated
    for a file.
    @param      args                Hardware Initilization arguments. Varies based on implementation

    @retval    > 0                      New Cluster Number
    @retval    0                        Fail   

*/
uint32_t FSAllocateCluster(uint32_t from);



/*
    Update a FAT table entry with a given status

    @param      cluster     Cluster which is to have FAT value updated
    @param      status      New FAT table entry. The first four bits will be unchanged

    @retval     EXIT_SUCCUSS            On Success
    @retval     EXIT_INVALID_PARAMETER  Cluster value provided was invalid
    @retval     EXIT_MEMORY_TABLE_FAIL  Memory table write failed

*/
EXIT_STATUS FSFatTableUpdate(uint32_t cluster, uint32_t status);


/*
    Get the first physcial sector of a cluster given the cluster

    @param      cluster     Cluster which is to have FAT value updated
    @param      status      New FAT table entry. The first four bits will be unchanged

    @retval     sector number       On Success
    @retval     0                   Fail - Not valid cluster                       

*/
uint32_t FSGetSector(uint32_t cluster);


/*
    Get the cluster that a physical sector is in. It is possible to get a cluster
    value equal to MAX_CLUSTER because of the final incomplete cluster on a partition.

    @param      cluster     Cluster which is to have FAT value updated
    @param      status      New FAT table entry. The first four bits will be unchanged

    @retval     cluster number    On Success
    @retval     0                 Fail - Sector not in partition data area                       

*/
uint32_t FSGetCluster(uint32_t sector);


/*
    Get a value in the FAT table given a cluster entry

    @param      cluster     Cluster which is to have FAT value updated

    @retval     0           Fail or invalid
    @retval     > 0         Sector number in table                       

*/
uint32_t FSGetFatTableEntry(uint32_t cluster);


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
EXIT_STATUS FSDirectorySearch(char *name, FILE *directory, FILE *new);



/*
    Initializes the drive and memory table for which the hardware driver is configured.

    @param      partition       Physical MBR partition number which is to be formatted with 
    corresponding flags (usually 0)
    @param      args            Hardware Init. arguments. Varies based on implementation

    @retval     EXIT_SUCCESS            Succuss
    @retval     EXIT_HARDWARE_FAIL      Hardware Driver Failed
    @retval     EXIT_INCORRECT_FORMAT   Partition was not formatted because its not FAT32
    @retval     EXIT_ALREADY_INIT       Drive is already initilized
    @retval     EXIT_MEMORY_TABLE_FAIL  The memory table driver failed
    @retval     EXIT_READ_FAIL          Device Read Failed
    @retval     EXIT_WRITE_FAIL         Device Write Failed
    @retval     EXIT_INVALID_DEVICE     Device is unrecognizable
    @retval     Others                  Fail
*/
EXIT_STATUS FSInit(uint8_t partition, void *args);


/*
    Eject a FAT32 File System. This method should be called to ensure that all
    sectors are written back to the device and that hardware is deinitilized

    @param      args            Hardware eject arguments. Varies based on implementation

    @retval     EXIT_SUCCESS            Succuss
    @retval     EXIT_HARDWARE_FAIL      Hardware Driver Failed
    @retval     EXIT_MEMORY_TABLE_FAIL  The memory table driver failed
    @retval     Others                  Fail    

*/
EXIT_STATUS FSEject(void *args);


/*
    Mount a FAT32 File System. This is also reformat a partition if set

    @param      partition           Physical MBR partition number which is to be used. Set REFORMAT
                                    flag to reformat the partition

    @retval    EXIT_SUCCESS             Succuss
    @retval    EXIT_INCORRECT_FORMAT    Partition is not FAT32
    @retval    EXIT_ALREADY_INIT        Init already called
    @retval    EXIT_MEMORT_TABLE_FAIL   The memory table failed
    @retval    EXIT_READ_FAIL           Device Read failed
    @retval    EXIT_WRITE_FAIL          Device write failed
    @retval    EXIT_INVALID_DEVICE      Device is unrecknoizable
    @retval    others                   Fail      

*/
EXIT_STATUS FSMount(uint8_t partition);


/*
    Read the contents from a file and place them in a buffer

    @param      data                Buffer to place data in
    @param      offset              File offset to begin reading
    @param      len                 Length to read file
    @param      file                FAT File entry to read

    @return     bytes               Amount of bytes read

*/
uint32_t FSReadFile(uint8_t *data, uint32_t offset, uint32_t len, FILE *file);


/*
    Write to a file and place the contents in a buffer

    @param      data                Buffer to read data from
    @param      offset              File offset to begin writing
    @param      len                 Length to write file
    @param      file                FAT File entry to write

    @return     bytes               Amount of bytes read

*/
uint32_t FSWriteFile(uint8_t *data, uint32_t offset, uint32_t len, FILE *file);


/*
    Create a file or directory.

    @param      name                ASCII string for directory name
    @param      file                Directory to add file to
    @param      flags               File flags to add

    @return     EXIT_SUCCUSS            Function exited succussfully
    @return     EXIT_INVALID_PARAMETER  Invalid parameter


*/
EXIT_STATUS FSCreateFile(FILE *file, FILE *dir, uint8_t *name, uint8_t flags, FileTime *time);


/*
    Remove a file or directory.

    @param      file                Directory to remove file from

    @return     EXIT_SUCCUSS            Function exited succussfully
    @return     EXIT_INVALID_PARAMETER  Invalid parameter

*/
EXIT_STATUS FSRemoveFile(FILE *file);

/*
    Change the attributes of a file or directory.

    @param      flags               File/directory flags
    @param      dir                 Directory which holds the file
    @param      time                Time to use. NULL means use no time

    @return     EXIT_SUCCUSS            Function exited succussfully
    @return     EXIT_INVALID_PARAMETER  Invalid parameter
    @return     EXIT_INVALID_TIME       Time passed was not valid
*/
EXIT_STATUS FSChangeAttribues(FILE *file, uint8_t flags, FileTime *time, uint8_t *name);


/*
    Open a file or directory.
    OPEN FILES MUST BE CLOSED TO AVOID UNDEFINED BEHAVIOUR

    @param      dir                 Directory which file is in
    @param      file                Empty file for the new file

    @return     EXIT_SUCCESS        File was opened
    @return     EXIT_NOT_FOUND      File did not exist in directory
    @return     others              Other failure

*/
EXIT_STATUS FSOpen(uint8_t *name, FILE *dir, FILE *file);


/*
    Close a file (or directory) and write it back to the disk.

    @param      file                File which is to be closed

    @return     EXIT_SUCCESS        File was closed

*/
EXIT_STATUS FSClose(FILE *file);
