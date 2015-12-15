# EmuNAND9Tool
EmuNAND SD formatter & manager for the 3DS console
aka "The last nail in the coffin of GW software for non GW users"

## How to run this?
You can only use this if your 3DS FW is <= 9.2. There are multiple ways of running this:
* CakeHax - use EmuNAND9Tool.dat, EmuNAND9Tool.nds for MSET is included
* Homebrew Launcher - use EmuNAND9Tool.3dsx to run this from Homebrew Launcher
* Gateway 4.x browser exploit - use Launcher.dat
* Bootstrap / Brahma - use Decrypt9.bin

## What can this do?
Actually, a lot! See this incomplete list:
* Completely setup a fresh SD card for EmuNAND, including cloning SysNAND to EmuNAND and autosetup of a homebrew starter pack
* Clone your SysNAND to your EmuNAND, at any time, without wiping SD data
* Make backups of your SysNAND and EmuNAND
* Inject these backups back into EmuNAND (without wiping SD data)
* Convert a RedNAND to a GW type EmuNAND (without wiping SD data)
* Format an SD card without EmuNAND / remove an existing EmuNAND

## But why is this better than GW software?
EmuNAND9Tool is better than GW software for a number of reasons:
* Open source, licensed under the GPL v2
* Formats SD cards according to official recommendations for better performance & longer lifetime
* Bigger functionality - see the list above
* Includes a number of safety measures to make it idiot proof
* Has more ways of running than GW software (HB Launcher, anyone?)
* Doesn't require a PC to setup a fresh SD card

## Additional info
As written above, this includes a number of safety measures to make it as idiot proof as possible. However, this is a powerful tool. It can wipe your SD card and it can overwrite your EmuNAND. You will be warned every step along the way, but it lies in the hands of the user to actually read the text on screen and to react accordingly.

## Starter pack contents
As a new feature, EmuNAND9Tool contains the ability to transfer a starter pack to your newly formatted SD card. The starter pack is in the file called 'starter.bin'. This file can be either a boot.3dsx or a Launcher.dat, but you have to rename it to 'starter.bin' for it work. EmuNAND9Tool will detect the type of 'starter.bin' automatically and name it accordingly when transfering to the formatted SD card. The maximum size of 'starter.bin' is 16MB.

The 'starter.bin' included with EmuNAND9Tool is an extended version of smealums homebrew starter pack (from https://smealum.github.io/ninjhax2/). It was converted to a 3DS compatible self extracting ZIP archive using ZIP3DSFX (https://github.com/d0k3/ZIP3DSFX). ZIP3DSFX doesn't have a graphical user interface at the moment, but you can just rename the included starter.bin to starter.zip and edit it in a standard ZIP archiver.

The starter.bin contains the following homebrew software:
* Gridlauncher by mashers (instead of regular HB launcher)
* MenuHax Manager by Yellows8
* HANS by smealum
* CHMM2 by Rinnegatamante
* ftBrony by mtheall
* mGBA by endrift
* Playcoin Setter by MrCheeze
* FBI by SteveIce10
* svdt by meladroit
* uncart by Archshift & others
* CTRXplorer by d0k3
* Decrypt9 by Archshift & d0k3
* EmuNAND9Tool by d0k3
* ReiNAND CFW by reisyukaku (without the evil firmware.bin file)
* CakesFW by mid-kid (you need to get some files yourself)
* MiniPasta by zoogie

## Credits (EmuNAND9Tool)
* Archshift for the basic code behind this
* Cha(N), Kane49, and all other FatFS contributors for FatFS
* Normmatt for `sdmc.s` as well as project infrastructure (Makefile, linker setup, etc)
* Shadowtrance, Datalogger and countless others for helping me test this
