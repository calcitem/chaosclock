// This file is part of ChaosClock.
// Copyright (C) 2023 The ChaosClock developers (see AUTHORS file)
//
// ChaosClock is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// ChaosClock is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#ifdef _WIN32
#if _WIN32_WINNT < 0x0601
#undef _WIN32_WINNT
#define _WIN32_WINNT 0x0601 // Force to include needed API prototypes
#endif

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <windows.h>
// The needed Windows API for processor groups could be missed from old Windows
// versions, so instead of calling them directly (forcing the linker to resolve
// the calls at compile time), try to load them at runtime. To do this we need
// first to define the corresponding function pointers.
extern "C" {
using fun1_t = bool (*)(LOGICAL_PROCESSOR_RELATIONSHIP,
                        PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX, PDWORD);
using fun2_t = bool (*)(USHORT, PGROUP_AFFINITY);
using fun3_t = bool (*)(HANDLE, CONST GROUP_AFFINITY *, PGROUP_AFFINITY);
}
#endif

#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <vector>

#include "misc.h"
