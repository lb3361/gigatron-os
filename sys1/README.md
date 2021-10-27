# Readonly browser and loader

This version of 'SYSTEM.GT1' implements a file browser that can be used to navigate the 
directories and execute any GT1 file present on a SD card attached to the RAM & I/O 
expansion SPI0 port.  It supports FAT filesystems with long file names (up to 25 characters).

### 1. Compilation

Compile with GLCC (https://github.com/lb3361/gigatron-lcc) using the command 'make'.

### 1. Operation

Your gigatron must be equipped with a RAM and IO expansion card like the
one described in https://github.com/lb3361/gigatron-lb/tree/main/extension-retro.
Your gigatron must also be equipped with a recent version of the DEVROM such as
the one provided in the 'binaries' directory. 

* Format the SD card with a FAT32 filesystem (this is necessary for CardBoot)
* Copy 'system.gt1' into the root directory.
* Add GT1 files of interest to the SD card, possibly in subdirectories
* Insert the SD card in a SD breakout connected to port SPI0 of the RAM & IO expansion board.
* Reboot your gigatron.

![Screenshot1](images/shot1.png)

![Screenshot2](images/shot2.png)
