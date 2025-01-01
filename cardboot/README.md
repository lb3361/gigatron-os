# CardBoot

This program is a derivative of the original CardBoot.gcl by MarcelK.
It is normally integrated to the ROM and is called when Reset.gt1
detects the presence of a SD card in the first SPI port of 
a RAM & IO expansion board. Its only purpose is to load
and execute file SYSTEM.GT1 on the SD card.

Cardboot remains quite limited about what it can load.
- It cannot load anything into page zero without crashing
- It cannot load anything into the visible screen buffer because it clears the screen before executing.
- It cannot load anything into the screen buffers hole above 0x72a0.

The reliable solution to start an arbitrary program when the Gigatron boots
is to make CardBoot load the [browser](../sys1) which then autoloads 
any program named 'autoexec.gt1' without such restrictions.

Although the main structure of the program is inherited from MarcelK's version,
it is focused towards the boot objective intead of being a preparation for 
a generic OS.  That made it much easier to make some of the changes
Marcel had envisioned such as buffering the sectors (for speed),
and being more compliant with the SD and FAT specifications.

Update: a version of this program is now integrated in the Gigatron DevRom, ROMv6, DEV7ROM..

Update:
- Version `CardBoot_v2.gt1` is for DEV6ROM or ROMv6. It searches and loads `system.gt1`.
- Version `CardBoot.gt1` is for DEV7ROM. It first reads `system7.gt1` then falls back to `system.gt1`. 
  It only prints the file list during the fallback search but without date to save code space for
  the new search strategy. This is becoming obsolete as DEV7ROM has system7.gt1 in ROM.
