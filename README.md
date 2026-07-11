# derailleur

A Mario Party randomizer *(working title)*.

> ### Visit [**our website**](https://www.doggylongface.com/derailleur/) for the download and lists of currently supported games, or the [**Discord**](https://discord.gg/UdYNx447R5) for setup help or to find net players.

**derailleur** allows a Mario Party board context to be played over the Internet while injecting custom mini-games from other games. Theoretically, it can support any game across series or platform, currently targeting Nintendo 64, GameCube, and GBA.

This program was created as a novel use of my [QRetro](https://github.com/classicslive/QRetro) emulation library, a libretro frontend (think RetroArch) that can load and manage multiple cores simulataneously. Like a derailleur on a bicycle, this functionality allows a master program to "shift gears" between different games at a point in the emulation cycle where it is safe to do so.

<img width="1363" height="1015" alt="image" src="https://github.com/user-attachments/assets/a0ce2dc4-aaf6-4d0a-a5a5-b7d84f4537dc" />

## Licensing

derailleur is built using open-source software and technology:

| Component | License |
| --- | --- |
| [libretro-common](https://github.com/libretro/libretro-common) | MIT |
| [QRetro](https://github.com/classicslive/QRetro) | MIT |
| [SDL3](https://github.com/libsdl-org/SDL) | zlib |
| [Qt 6](https://www.qt.io/) | LGPL-3.0 |

The following emulator binaries are distributed with the program with either no edits, or with edits commited upstream:

| Emulator | License |
| --- | --- |
| [mupen64plus-libretro-nx](https://github.com/libretro/mupen64plus-libretro-nx) | GPL-2.0 |
| [mGBA](https://github.com/mgba-emu/mgba) | MPL-2.0 |
| [Dolphin](https://github.com/dolphin-emu/dolphin) | GPL-2.0-or-later |

derailleur itself is distributed under the MIT license.
