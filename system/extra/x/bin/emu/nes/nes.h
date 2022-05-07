/*
 *   This file is part of nes_emu.
 *   Copyright (c) 2019 Franz Flasch.
 *
 *   nes_emu is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   nes_emu is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with nes_emu.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef NES_H
#define NES_H

#define DEBUG_CPU 0
#define DEBUG_PPU 0
#define DEBUG_MEMORY 0
#define DEBUG_MAIN 0

#include <stdarg.h>
#include <stdlib.h>
#define die(...)	do{printf(__VA_ARGS__);exit(-1);}while(0)

#endif /* NES_H */
