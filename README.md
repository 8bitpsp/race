RACE! PSP
=========

NeoGeo Pocket and Pocket Color emulator

&copy; 2008-2009 Akop Karapetyan  
&copy; 2008 Flavor  
&copy; 2006 Judge\_  

New Features
------------

#### Version 2.16 (April 04 2009)

*   Time Rewind feature: map ‘Special: Rewind’ to any PSP button in the Controls menu to enable. See documentation for more information
*   Save state format has changed: RACE! PSP will still read the older save state format, but loading will be slightly slower
*   Not a new feature, but the documentation now includes a section on how to have RACE! PSP load a BIOS ROM file (instead of using the customized hardcoded version)

Installation
------------

Unzip `race.zip` into `/PSP/GAME/` folder on the memory stick.

Game ROM’s may reside anywhere (the GAMES subdirectory is recommended, but not necessary). ROM files can also be loaded from ZIP files.

Controls
--------

During emulation:

| PSP controls       | Emulated controls           |
| ------------------ | --------------------------- |
| Analog stick       | D-pad up/down/left/right    |
| D-pad pad          | D-pad up/down/left/right    |
| [ ] (square)       | Button A                    |
| O (circle)         | Button B                    |
| Select             | Option                      |
| [L] + [R]          | Return to the emulator menu |

By default, button configuration changes are not retained after button mapping is modified. To save changes, press X (cross) after desired mapping is configured. To load the default (“factory”) mapping press ^ (triangle).

BIOS ROM Support
----------------

To have RACE! PSP use an actual BIOS ROM, place the ROM file in the same directory as EBOOT.PBP, under the name _NPBIOS.BIN_. Note that RACE! PSP has not been thoroughly tested with the real BIOS ROM, and may not work correctly – for example, the Test Mode button does not work correctly in Card Fighters’ Clash.

Flash Memory
------------

Flash memory is saved to memory stick when the game is reset, a new game is launched, or at program exit.

Rewinding
---------

Starting with version 2.16, RACE! PSP includes the Time Rewind feature: map ‘Special: Rewind’ to any PSP button in the Controls menu to enable it. Press the Rewind button to rewind emulation (approximately 18 seconds of recent gameplay are recorded).

Enabling this feature may result in slight performance degradation, so if you don’t plan to use it, don’t leave it enabled.

Compiling
---------

To compile, ensure that [zlib](svn://svn.pspdev.org/psp/trunk/zlib) and [libpng](svn://svn.pspdev.org/psp/trunk/libpng) are installed, and run make:

`make -f Makefile.psp`

Version History
---------------

#### 2.15 (October 14 2008)

*   Fixed state auto-selection bug; when switching games, the latest save state will now be highlighted
*   Added support for NGC and NGPC file extensions
*   To improve menu entrance/exit time, reduced the number of times the game will save flash RAM data. Flash data will now be saved when resetting a game, loading a new game, and exiting emulator
*   Rapid fire support – map any button to A or B autofire (‘Controls’ tab). You can also change the rate of autofire (‘Options’ tab)
*   Snapshots are now saved into PSP’s own PHOTO directory (/PSP/PHOTO), and can be viewed in PSP’s image viewer
*   File selector snapshots – while browsing for games with the file selector, pause momentarily to display the first snapshot for the game (name must end with ‘-00.png’)

#### 2.1 (September 21 2008)

*   Emulation state saving implemented
*   Added a test switch that works in Card Fighters Clash, Card Fighters Clash 2, Dokodemo Mahjong, possibly other games (see ‘Controls‘ tab)

#### 2.0 (September 10 2008)

*   Initial release

Credits
-------

Flavor (original PSP port, optimizations)  
Thor (GP2X port, optimizations)  
Judge_ (The emulator is based on MHE emulator by Judge_)  
