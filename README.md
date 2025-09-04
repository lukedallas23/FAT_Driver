This project creates basic filesystem access to FAT16 and FAT32 filesystems partitioned using Master Boot Record partitioning.

This driver allows access using function calls to
1. Opening files
2. Reading files
3. Writing files
4. Updating a file's attributes
5. Creating files/directories
6. Removing files/directories
7. Formatting Partitions

To use this driver, the four functions in the file `device.h` must be created  
write_block - Which writes to a sector on the drive  
read_block - Which reads from a sector on the drive  
hardware_init - Initilizes the hardware  
hardware_eject - Preforms cleanup  

See the example, which interfaces with a SD Card on PIC24 microcontroller using SPI.
