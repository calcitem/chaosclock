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

#include "evaluate.h"
#include "bitboard.h"
#include "option.h"
#include "thread.h"

namespace {

class Evaluation
{
public:
    Evaluation() = delete;

    explicit Evaluation(Position &p) noexcept
        : pos(p)
    { }

    Evaluation &operator=(const Evaluation &) = delete;
    [[nodiscard]] Value value() const;

private:
    Position &pos;
};

// Evaluation::value() is the main function of the class. It computes the
// various parts of the evaluation and returns the value of the position from
// the point of view of the side to move.

Value Evaluation::value() const
{
    Value value = VALUE_ZERO;

#if 0
    // TODO: Disable for performance
    if (pos.has_repeat()) {
        return VALUE_DRAW;
    }
#endif

    switch (pos.result) {
    case GameOverReason::bothLose:
        value = VALUE_BOTH_LOSE;
        break;
    case GameOverReason::bothWin:
        value = VALUE_BOTH_WIN;
        break;
    case GameOverReason::win:
        value = VALUE_WIN;
        break;
    case GameOverReason::lose:
        value = VALUE_LOSE;
        break;
    case GameOverReason::none:
        // TODO: Implement it
        value = VALUE_ZERO;
        break;
    }

#if 0
    // TODO: Need?
    if (pos.side_to_move() == BLACK) {
        value = -value;
    }
#endif

    return value;
}

} // namespace

/// evaluate() is the evaluator for the outer world. It returns a static
/// evaluation of the position from the point of view of the side to move.

Value Eval::evaluate(Position &pos)
{
    return Evaluation(pos).value();
}
