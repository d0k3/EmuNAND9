# EmuNAND9
_Open source EmuNAND SD formatter & manager for the 3DS console
aka. "The final nail in the coffin of GW software for GW non-owners"_

## What can this do?
Actually, a lot! See this incomplete list:
* Completely setup a fresh SD card for EmuNAND, including cloning SysNAND to EmuNAND and autosetup of a homebrew starter pack
* Setup a minimum size RedNAND, saving space on your SD card
* Clone your SysNAND to your EmuNAND, at any time, without wiping SD data
* Make backups of your SysNAND and EmuNAND
* Convert GW type EmuNANDs to RedNAND and vice versa 
* Inject these backups back into EmuNAND (without wiping SD data)
* Convert a RedNAND to a GW type EmuNAND (without wiping SD data)
* Format an SD card without EmuNAND / remove an existing EmuNAND

## Why is this better than GW software?
EmuNAND9 is better than GW software for a number of reasons:
* Open source, licensed under the GPL v2
* Formats SD cards according to official recommendations for better performance & longer lifetime
* Bigger functionality - see the list above
* Includes a number of safety measures to make it idiot proof
* Has more ways of running than GW software (HB Launcher, anyone?)
* Doesn't require a PC to setup a fresh SD card

## Warning
As written above, this includes a number of safety measures to make it as idiot proof as possible. However, this is a powerful tool. It can wipe your SD card and it can overwrite your EmuNAND. You will be warned every step along the way, but it lies in the hands of the user to actually read the text on screen and to react accordingly.

## How to run this / entry points
EmuNAND9 can be built to run from a number of entry points, descriptions are below. Note that you need to be on or below 3DS firmware version v9.2 or have ARM9loaderhax installed for any of these to work.
* __Gateway Browser Exploit__: Copy `Launcher.dat` to your SD card root and run this via http://go.gateway-3ds.com/ from your 3DS browser. Build this with `make gateway`.
* __A9LH & Brahma__: Copy `EmuNAND9.bin` to somewhere on your SD card and run it via either [Brahma](https://github.com/delebile/Brahma2) or [arm9loaderhax](https://github.com/Plailect/Guide/wiki). Brahma derivatives / loaders (such as [BrahmaLoader](https://gbatemp.net/threads/release-easily-load-payloads-in-hb-launcher-via-brahma-2-mod.402857/), [BootCTR](https://gbatemp.net/threads/re-release-bootctr-a-simple-boot-manager-for-3ds.401630/) and [CTR Boot Manager](https://gbatemp.net/threads/ctrbootmanager-3ds-boot-manager-loader-homemenuhax.398383/)) and A9LH chainloaders (such as [Luma3DS](https://github.com/AuroraWright/Luma3DS) and [BootCTR9](https://github.com/hartmannaf/BootCtr9)) will work with this as well. Build this with `make a9lh`.
* __Homebrew Launcher__: Copy `EmuNAND9.3dsx` & `EmuNAND9.smdh` into `/3DS/EmuNAND9` on your SD card. Run this via [Smealums Homebrew Launcher](http://smealum.github.io/3ds/), [Mashers Grid Launcher](https://gbatemp.net/threads/release-homebrew-launcher-with-grid-layout.397527/) or any other compatible software. Build this with `make brahma`.
* __CakeHax Browser__: Copy `EmuNAND9.dat` to the root of your SD card. You can then run it via http://dukesrg.github.io/?EmuNAND9.dat from your 3DS browser. Build this via `make cakehax`.
* __CakeHax MSET__: Copy `EmuNAND9.dat` to the root of your SD card and `EmuNAND9.nds` to anywhere on the SD card. You can then run it either via MSET and EmuNAND9.nds. Build this via `make cakerop`.

If you are a developer and you are building this, you may also just run `make release` to build all files at once. If you are a user, all files are already included in the release archive.

## EmuNAND9 controls
The most important controls are displayed on screen, here is a list of all:
* __DOWN__/__UP__ - Navigate menus, select between options.
* __A__ - Enter submenu or confirm action.
* __B__ - Depending on location, leave submenu or cancel.
* __X__ - Make a screenshot (works in menu only).
* __X + LEFT/RIGHT__ - Batch screenshot all submenus / entries (only on menu)
* __SELECT__ - Unmount SD card (only on menu).
* __START (+ LEFT)__ - Reboot (START only) / Poweroff (with LEFT).

There are some features (NAND backup and restore, f.e.), that require the user to choose a filename. In these cases, use the arrow keys to select and A / B to confirm and cancel.

## EmuNAND9 features description
See below for a quick descriction of features in EmuNAND9.
* __Complete EmuNAND Setup__: The recommended option, with the highest comaptibility, for everyone. Works on any CFW and/or hardware. This will format your SD card, clone your SysNAND to EmuNAND and setup a base starter pack (see below). _The contents of your SD card & your EmuNAND (if existing) will be wiped_, so keep backups. You are given the possibility to switch the SD card after starting this, just in case you want to format a different SD card than the one EmuNAND9 is running from.
* __Complete RedNAND Setup__: Same as above, but will setup a RedNAND instead of a standard GW type EmuNAND. RedNANDs are at the moment only compatible with [CakesFW](https://github.com/mid-kid/CakesForeveryWan) and [AuReiNAND CFW](https://github.com/AuroraWright/AuReiNand).
* __SD Format Options__: This lets you format your SD card, with various options. _Each of these options will wipe your SD card_, but you will get a warning and a chance to switch SD cards before it actually starts.
  * __with / without starter pack__: Choose to setup the base starter pack after formatting or to skip it.
  * __without EmuNAND__: This also _removes your EmuNAND from the SD card_, freeing up the space it took.
  * __for EmuNAND (default/legacy/minsize)__: Choose default to avoid wasting space and for best compatibility. For some outdated tools to keep working with your EmuNAND, you may need to choose legacy, but know that this will waste a lot of space on some systems and is not recommended. Choose minsize for a minimum size RedNAND partition, but know that you will only be able to setup a RedNAND on this (see above/below). _Choosing minsize will damage your GW type EmuNAND_ (RedNAND will be untouched), while the other two options will leave your existing EmuNAND / RedNAND untouched.
* __EmuNAND Manager Options__: This includes various options to manage your EmuNAND. Note that _the writing options (clone / restore) will overwrite your existing EmuNAND_. You can...
  * __Clone your SysNAND to EmuNAND / RedNAND__: Use this to directly copy SysNAND to EmuNAND.
  * __Backup your SysNAND / EmuNAND to a file__: This should be self explaining. There is no difference in GW type EmuNAND backups / RedNAND backups - EmuNAND9 will handle this for you.
  * __Restore your EmuNAND from a backup__: This should be self explaining as well. Again note that backups for GW type EmuNANDs / RedNANDs are the same - EmuNAND9 handles the conversion for you.
  * __Convert your EmuNAND -> RedNAND or vice versa__: Convert an existing GW type EmuNAND to RedNAND or vice versa. Do not turn off your system while doing this, or your EmuNAND will be damaged. It is recommend to make a backup before the conversion.

## Why should I use RedNAND?
Read more about the [RedNAND / GW EmuNAND difference here](https://gbatemp.net/threads/emunand-rednand-technical-implementation.401969/#post-5783813). RedNANDs are, by now, only supported by CakesFW and AuReiNAND, and technically a superior alternative to GW type EmuNANDs. For N3DS consoles with a 1.8GB NAND chip, choosing RedNAND over EmuNAND will create an EmuNAND that is a whopping 560MB smaller than a standard one, with other console types your mileage may vary. If you want to convert an existing GW type EmuNAND to RedNAND, do this:
* Backup your current EmuNAND and SD card contents (via EmuNAND Manager Options).
* Either use 'Complete RedNAND Setup' or 'Format SD for EmuNAND (min size)' to setup your SD card.
* Restore your EmuNAND and copy back the contents of your SD card.

Just doing the conversion (in EmuNAND Manager Options) will not save any space on your SD card.

## Starter pack contents
As a new feature, EmuNAND9 contains the ability to transfer a starter pack to your newly formatted SD card. The starter pack is in the file called 'starter.bin'. This file can be either a boot.3dsx or a Launcher.dat, but you have to rename it to 'starter.bin' for it work. EmuNAND9 will detect the type of 'starter.bin' automatically and name it accordingly when transferring to the formatted SD card. The maximum size of 'starter.bin' is 16MB.

The 'starter.bin' included with EmuNAND9 is an extended version of smealums homebrew starter pack (from https://smealum.github.io/ninjhax2/). It was converted to a 3DS compatible self extracting ZIP archive using ZIP3DSFX (https://github.com/d0k3/ZIP3DSFX). ZIP3DSFX doesn't have a graphical user interface at the moment, but you can use the simple batch script included in the release archive to convert any ZIP archive to your personal .3DSX self extracting ZIP archive. You can also open the starter.bin in any ZIP archiver.

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
* EmuNAND9 by d0k3
* GodMode9 by d0k3
* Luma3DS by AuroraWright
* ReiNAND CFW by reisyukaku
* CakesFW by mid-kid (you need to get some files yourself)
* MiniPasta by zoogie
* ... and more!

## Credits
* Archshift for the basic code behind this
* Cha(N), Kane49, and all other FatFS contributors for FatFS
* Normmatt for `sdmmc.c` as well as the project infrastructure (Makefile, linker setup, etc)
* Shadowtrance, Datalogger and countless others for helping me test and develop this
* The fine folks on freenode #Cakey
