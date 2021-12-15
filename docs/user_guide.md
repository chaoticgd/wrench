# Wrench User Guide

## Extracting an ISO file

Use the File->Extract ISO or the icon on the Start Screen. Depending on the game, this tool may be the only ISO extractor that will work.

## Level Editor

The editing tools that are currently available are quite primitive. For now, you can move objects about and modify their properties via the inspector.

## Building an ISO file

To build an ISO file, use the File->Build ISO menu. The following settings are available:

- Input directory: The directory to build from.
- Output ISO: The file path of the ISO file to be created.
- Launch emulator after building: Boot the newly created ISO file in the emulator specified in the settings dialog.
- Only write out single level: All level table entries are pointed at a single level. This dramatically reduces build times.
- Skip writing out MPEG cutscenes: Cutscenes are skipped and don't play ingame. This also dramatically reduces build times.

## Command Line Tools

These tools are used for development.

### iso

This let you extract/build Ratchet & Clank ISO files. The games use raw disk I/O for loading assets, so a custom tool like this is required for extracting assets and making valid images. Sample usage:

	$ ./bin/iso extract game.iso outdir
	LBA             Size (bytes)    Filename
	---             ------------    --------
	0x3e8           0x3b            system.cnf
	0x3e9           0x63b7c         rc2.hdr
	0x4b1           0x2bea84        sces_516.07
	0xa2f           0x40901         ioprp255.img
	...

### lz

Decompresses and recompresses LZ compressed segments. These are not to be confused with the WAD files on the game's filesystem. The decompressor must be directed to look at the beginning of the WAD header which starts with "WAD" in ASCII. Sample usage:

	$ ./bin/lz decompress LEVEL4.WAD gameplay.bin -o 0x1078800

### memmap

Given an image of the game's memory (the eeMemory.bin file from an unzipped PCSX2 save state), determine its memory layout. Sample usage:

	$ ./bin/memmap eeMemory.bin
	--- Detected R&C2.
	001bfac0 OS                     0     ??? k
	001bfac4 Code              100000    3664 k
	001bfac8                   494000       0 k
	001bfacc                   494000    1408 k
	001bfad0                   5f4000    1408 k
	001bfad4                   754000    1024 k
	001bfad8                   854000       0 k
	001bfadc Tfrag Geometry    854000    2788 k
	001bfae0 Occlusion         b0d2c0     346 k
	001bfae4 Sky               b63c40     477 k
	...

### vif

Disassembles VIF command lists. This is useful for inspecting model data. Sample usage:

	$ ./bin/vif ARMOR.WAD -o 0xd80
	d84 756080c2 UNPACK vnvl=V2_16 num=60 flg=USE_VIF1_TOPS usn=SIGNED addr=c2 interrupt=0 SIZE=184
	f08 6e1a812d UNPACK vnvl=V4_8 num=1a flg=USE_VIF1_TOPS usn=SIGNED addr=12d interrupt=0 SIZE=6c
	f74 00000000 NOP interrupt=0 SIZE=4
	f78 00000000 NOP interrupt=0 SIZE=4
	f7c 00000000 NOP interrupt=0 SIZE=4
	f80 6c048147 UNPACK vnvl=V4_32 num=4 flg=USE_VIF1_TOPS usn=SIGNED addr=147 interrupt=0 SIZE=44
	fc4 00000001 NOP interrupt=0 SIZE=4
	fc8 00410000 NOP interrupt=0 SIZE=4
	fcc 0060001f NOP interrupt=0 SIZE=4
	fd0 00000060 NOP interrupt=0 SIZE=4
	fd4 0000004b NOP interrupt=0 SIZE=4
	fd8 00000000 NOP interrupt=0 SIZE=4
	fdc failed to parse VIF code

### wad

Extract and build WAD files. Sample usage:

	$ ./bin/wad extract level.wad outdir/
