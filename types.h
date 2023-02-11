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

constexpr int MAX_MOVES = 13; // 12 + 1 = 13
constexpr int MAX_PLY = 48;

enum Move : int8_t { MOVE_NONE = -2, MOVE_PASS = -1 };

enum MoveType { 
    MOVETYPE_MOVE,      // 走子
    MOVETYPE_PLACE,     // 摆子
    MOVETYPE_PASS       // 不走，即放弃本次行棋机会，转由对方走棋
};

// 本游戏为二人游戏，游戏双方分别被称作甲方和乙方。
enum Color : uint8_t {
    YI = 0, // 乙方
    JIA = 1, // 甲方
    COLOR_NB = 2,
};

enum class GameResult {
    none,
    jiaWin,
    yiWin,
    bothWin,
    bothLost,
};

enum Value : int8_t {
    VALUE_ZERO = 0,
    VALUE_INFINITE = INT8_MAX,
    VALUE_NONE = -VALUE_INFINITE,
    VALUE_LOSE = -120,
    VALUE_BOTH_LOSE = 0,
    VALUE_BOTH_WIN = 64,
    VALUE_UNIQUE = 100,
    VALUE_WIN = 120,
};

enum Rating : int8_t {
    RATING_ZERO = 0,
    RATING_MAX = INT8_MAX,
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

    SQUARE_NB = 12,
    SQ_BEGIN = 0,
    SQ_END = 12
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
ENABLE_INCR_OPERATORS_ON(Square)

#undef ENABLE_FULL_OPERATORS_ON
#undef ENABLE_INCR_OPERATORS_ON
#undef ENABLE_BASE_OPERATORS_ON

constexpr Color operator~(Color c)
{
    if (c == JIA) {
        return YI;
    } else {
        return JIA;
    }
}

constexpr Color color_of(Piece pc)
{
    if (pc % 2 == 0) {
        return YI;
    } else {
        return JIA;
    }
}

constexpr bool is_ok(Square s)
{
    if (s < SQ_BEGIN || s > SQ_END) {
        return false;
    }

    return true;
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
    resultJiaWin,
    resultYiWin,
    resultBothWin,
    resultBothLost,
    unknown,
};

#endif // #ifndef TYPES_H_INCLUDED
