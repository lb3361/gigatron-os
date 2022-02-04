# Gigatron OS playground

This project is intended for operating-system-like code supporting SD cards 
plugged the SPI ports of a Gigatron RAM & IO expansion board.
At the moment, this is mostly a [readonly file browser and loader](sys1)
than can be executed when the Gigatron boots and allows you to 
run any GT1 file on the SD card. 

* Directory `sys1` contains the readonly browser.

* Directory `cardboot` contains the boot code that has been integrated into the devrom.

* Directory `stubs` contains small programs that launch programs available in the rom.
