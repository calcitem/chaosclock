// This file is part of Sanmill.
// Copyright (C) 2019-2023 The Sanmill developers (see AUTHORS file)
//
// Sanmill is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Sanmill is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#ifndef TYPES_H_INCLUDED
#define TYPES_H_INCLUDED

#include "config.h"

/// When compiling with provided Makefile (e.g. for Linux and OSX),
/// configuration is done automatically. To get started type 'make help'.
///
/// When Makefile is not used (e.g. with Microsoft Visual Studio) some switches
/// need to be set manually:
///
/// -DNDEBUG      | Disable debugging mode. Always use this for release.
///
/// -DNO_PREFETCH | Disable use of prefetch asm-instruction. You may need this
/// to
///               | run on some very old machines.
///
/// -DUSE_POPCNT  | Add runtime support for use of popcnt asm-instruction. Works
///               | only in 64-bit mode and requires hardware with popcnt
///               support.
///
/// -DUSE_PEXT    | Add runtime support for use of pext asm-instruction. Works
///               | only in 64-bit mode and requires hardware with pext support.

#include <algorithm>
#include <cassert>
#include <cctype>
#include <climits>
#include <cstdint>
#include <cstdlib>

#if defined(_MSC_VER)
// Disable some silly and noisy warning from MSVC compiler
#pragma warning(disable : 4127) // Conditional expression is constant
#pragma warning(disable : 4146) // Unary minus operator applied to unsigned type
#pragma warning(disable : 4800) // Forcing value to bool 'true' or 'false'
#endif

/// Predefined macros hell:
///
/// __GNUC__           Compiler is gcc, Clang or Intel on Linux
/// __INTEL_COMPILER   Compiler is Intel
/// _MSC_VER           Compiler is MSVC or Intel on Windows
/// _WIN32             Building on Windows (any)
/// _WIN64             Building on Windows 64 bit

#if defined(__GNUC__) && \
    (__GNUC__ < 9 || (__GNUC__ == 9 && __GNUC_MINOR__ <= 2)) && \
    defined(_WIN32) && !defined(__clang__)
#define ALIGNAS_ON_STACK_VARIABLES_BROKEN
#endif

#define ASSERT_ALIGNED(ptr, alignment) \
    assert(reinterpret_cast<uintptr_t>(ptr) % (alignment) == 0)

#if defined(_WIN64) && defined(_MSC_VER) // No Makefile used
#include <intrin.h> // Microsoft header for _BitScanForward64()
#define IS_64BIT
#endif

#if defined(USE_POPCNT) && (defined(__INTEL_COMPILER) || defined(_MSC_VER))
#include <nmmintrin.h> // Intel and Microsoft header for _mm_popcnt_u64()
#endif

#if !defined(NO_PREFETCH) && (defined(__INTEL_COMPILER) || defined(_MSC_VER))
#include <xmmintrin.h> // Intel and Microsoft header for _mm_prefetch()
#endif

#if defined(USE_PEXT)
#include <immintrin.h> // Header for _pext_u64() intrinsic
#define pext(b, m) _pext_u64(b, m)
#else
#define pext(b, m) 0
#endif

#ifdef USE_POPCNT
constexpr bool HasPopCnt = true;
#else
constexpr bool HasPopCnt = false;
#endif

#ifdef USE_PEXT
constexpr bool HasPext = true;
#else
constexpr bool HasPext = false;
#endif

#ifdef IS_64BIT
constexpr bool Is64Bit = true;
#else
constexpr bool Is64Bit = false;
#endif

#ifdef TRANSPOSITION_TABLE_64BIT_KEY
typedef uint64_t Key;
#else
using Key = uint32_t;
#endif /* TRANSPOSITION_TABLE_64BIT_KEY */

using Bitboard = uint64_t;

constexpr int MAX_MOVES = 13;
constexpr int MAX_PLY = 48;

enum Move : int8_t { MOVE_NONE = -2, MOVE_PASS = -1 };

enum MoveType { MOVETYPE_PLACE, MOVETYPE_MOVE, MOVETYPE_PASS };

enum Color : uint8_t {
    //NOCOLOR = 0,
    WHITE = 0,
    BLACK = 1,
    COLOR_NB = 2,
    DRAW = 4,   // TODO
    NOBODY = 8
};

enum class GameOverReason {
    none,
    win,
    lose,
    bothWin,
    bothLose,
};

enum Bound : uint8_t {
    BOUND_NONE,
    BOUND_UPPER,
    BOUND_LOWER,
    BOUND_EXACT = BOUND_UPPER | BOUND_LOWER
};

enum Value : int8_t {
    VALUE_UNKNOWN = INT8_MIN,
    VALUE_ZERO = 0,
    VALUE_INFINITE = INT8_MAX,
    VALUE_NONE = -VALUE_INFINITE,
    VALUE_UNIQUE = 11,
    VALUE_DRAW = -1,
    VALUE_LOSE = -100,
#ifdef CHAOS_CLOCK_CAN_DRAW
    VALUE_BOTH_LOSE = -50,
    VALUE_BOTH_WIN = 60,
#else
    VALUE_BOTH_LOSE = 0,
    VALUE_BOTH_WIN = 0,
#endif
    VALUE_WIN = 100,

    // TODO
    VALUE_MTDF_WINDOW = 5,
    VALUE_PVS_WINDOW = 5,
};

enum Rating : int8_t {
    RATING_ZERO = 0,

    // TODO

    RATING_TT = 100,
    RATING_MAX = INT8_MAX,
};

enum PieceType : uint16_t {
    NO_PIECE_TYPE = 0,
    WHITE_PIECE = 1,
    BLACK_PIECE = 2,
    ALL_PIECES = 0, // TODO
    PIECE_TYPE_NB = 3,

    IN_HAND = 0x10,
    ON_BOARD = 0x20,
};

enum Piece : int8_t {
    NO_PIECE = -1,
    PIECE_0 = 0,
    PIECE_1 = 1,
    PIECE_2 = 2,
    PIECE_3 = 3,
    PIECE_4 = 4,
    PIECE_5 = 5,
    PIECE_6 = 6,
    PIECE_7 = 7,
    PIECE_8 = 8,
    PIECE_9 = 9,
    PIECE_10 = 10,
    PIECE_11 = 11,
    PIECE_12 = 0,
    PIECE_NB = 12,
};

using Depth = int8_t;

enum : int { DEPTH_NONE = 0, DEPTH_OFFSET = DEPTH_NONE };

enum Square : int {
    SQ_NONE = -1,

    SQ_0 = 0,
    SQ_1 = 1,
    SQ_2 = 2,
    SQ_3 = 3,
    SQ_4 = 4,
    SQ_5 = 5,
    SQ_6 = 6,
    SQ_7 = 7,
    SQ_8 = 8,
    SQ_9 = 9,
    SQ_10 = 10,
    SQ_11 = 11,
    SQ_12 = 0,

    // The board consists of 12 intersections or points.
    SQUARE_NB = 12,

    SQ_BEGIN = SQ_0,
    SQ_END = SQUARE_NB
};

#define ENABLE_BASE_OPERATORS_ON(T) \
    constexpr T operator+(T d1, int d2) { return T(int(d1) + d2); } \
    constexpr T operator-(T d1, int d2) { return T(int(d1) - d2); } \
    constexpr T operator-(T d) { return T(-int(d)); } \
    inline T &operator+=(T &d1, int d2) { return d1 = d1 + d2; } \
    inline T &operator-=(T &d1, int d2) { return d1 = d1 - d2; }

#define ENABLE_INCR_OPERATORS_ON(T) \
    inline T &operator++(T &d) { return d = T(int(d) + 1); } \
    inline T &operator--(T &d) { return d = T(int(d) - 1); }

#define ENABLE_FULL_OPERATORS_ON(T) \
    ENABLE_BASE_OPERATORS_ON(T) \
    constexpr T operator*(int i, T d) noexcept { return T(i * int(d)); } \
    constexpr T operator*(T d, int i) noexcept { return T(int(d) * i); } \
    constexpr T operator/(T d, int i) noexcept { return T(int(d) / i); } \
    constexpr int operator/(T d1, T d2) noexcept { return int(d1) / int(d2); } \
    inline T &operator*=(T &d, int i) noexcept { return d = T(int(d) * i); } \
    inline T &operator/=(T &d, int i) noexcept { return d = T(int(d) / i); }

ENABLE_FULL_OPERATORS_ON(Value)

ENABLE_INCR_OPERATORS_ON(Piece)
ENABLE_INCR_OPERATORS_ON(PieceType)
ENABLE_INCR_OPERATORS_ON(Square)

#undef ENABLE_FULL_OPERATORS_ON
#undef ENABLE_INCR_OPERATORS_ON
#undef ENABLE_BASE_OPERATORS_ON

constexpr Color operator~(Color c)
{
    return static_cast<Color>(c ^ 1); // Toggle color
}

constexpr Color color_of(Piece pc)
{
    return static_cast<Color>(pc & 1);
}

constexpr bool is_ok(Square s)
{
    return (s >= SQ_BEGIN && s < SQ_END);
}

/// Based on a congruential pseudo random number generator
constexpr Key make_key(uint64_t seed)
{
    return static_cast<Key>(seed * 6364136223846793005ULL +
                            1442695040888963407ULL);
}

enum class PieceStatus {
    onBoard,
    inHand,
};

enum class GameStatus {
    ok,
    errorOutOfRange,
    errCannotMoveLastMovedPiece,
    errCannotPlaceOpponentsPiece,
    errCannotMoveFixedPiece,
    errCannotRemoveFixedPiece,
    resultWin,
    resultLose,
    resultBothWin,
    resultBothLose,
    unknown,
};

#endif // #ifndef TYPES_H_INCLUDED
