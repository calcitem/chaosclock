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

#include <algorithm>
#include <iomanip>
#include <sstream>
#include <string> // std::string, std::stoi

#include "bitboard.h"
#include "mills.h"
#include "option.h"
#include "position.h"
#include "thread.h"

using std::string;
using std::cout;
using std::endl;
using std::setfill;
using std::setw;

namespace Zobrist {
Key psq[SQUARE_NB];
Key side;
} // namespace Zobrist

namespace {
string PieceToChar(Piece p)
{
    if (p == NO_PIECE) {
        return "*";
    }

    if (p == PIECE_10) {
        return "A";
    }

    if (p == PIECE_11) {
        return "B";
    }

    if (p == PIECE_12) {
        return "C";
    }

    return string(1, (char)p + '0');
}

Piece CharToPiece(char ch) noexcept
{
    if (ch == '*') {
        return NO_PIECE;
    }

    if (ch == 'A' || ch == 'a') {
        return PIECE_10;
    }

    if (ch == 'B' || ch == 'b') {
        return PIECE_11;
    }

    if (ch == 'C' || ch == 'c') {
        return PIECE_12;
    }

    return Piece(ch - '0') ;
}

constexpr PieceType PieceTypes[] = {NO_PIECE_TYPE, WHITE_PIECE, BLACK_PIECE};
} // namespace

/// operator<<(Position) returns an ASCII representation of the position

std::ostream &operator<<(std::ostream &os, const Position &pos)
{

#define P(s) PieceToChar(pos.piece_on(Square(s)))

    os << "  " << std::setw(2) << std::setfill('0') << P(11) << " "
       << std::setw(2) << std::setfill('0') << P(0) << " " << std::setw(2)
       << std::setfill('0') << P(1) << " "
       << "\n";
    os << std::setw(2) << std::setfill('0') << P(10) << "        "
       << std::setw(2) << std::setfill('0') << P(2) << "\n";
    os << std::setw(2) << std::setfill('0') << P(9) << "        "
       << std::setw(2) << std::setfill('0') << P(3) << "\n";
    os << std::setw(2) << std::setfill('0') << P(8) << "        "
       << std::setw(2) << std::setfill('0') << P(4) << "\n";
    os << "  " << std::setw(2) << std::setfill('0') << P(7) << " "
       << std::setw(2) << std::setfill('0') << P(6) << " " << std::setw(2)
       << std::setfill('0') << P(5) << "  "
       << "\n";

#undef P

    const auto fill = os.fill();
    const auto flags = os.flags();

    os << "\nFen: " << pos.fen() << "\nKey: " << std::hex << std::uppercase
       << std::setfill('0') << std::setw(16) << pos.key() << std::endl;

    os.flags(flags);
    os.fill(fill);

    return os;
}

/// Position::init() initializes at startup the various arrays used to compute
/// hash keys

void Position::init()
{
    PRNG rng(1070372);

    for (Square s = SQ_BEGIN; s < SQ_END; ++s)
        Zobrist::psq[s] = rng.rand<Key>();

    Zobrist::side = rng.rand<Key>();
}

Position::Position()
{
    construct_key();

    reset();

    // TODO
    score[WHITE] = score[BLACK] = score_draw = gamesPlayedCount = 0;
}

/// Position::set() initializes the position object with the given FEN string.
/// This function is not very robust - make sure that input FENs are correct,
/// this is assumed to be the responsibility of the GUI.
// TODO
Position &Position::set(const string &fenStr, Thread *th)
{
    /*
       A FEN string defines a particular position using only the ASCII character
       set.

       A FEN string contains six fields separated by a space. The fields are:

       1) Piece placement. 

       2) Active color. "w" means white moves next, "b" means black.

       3) Last move.

       4) Rule50.

       5) gamePly.
    */

    unsigned char token = '\0';
    Square sq = SQ_0;
    std::istringstream ss(fenStr);

    std::memset(this, 0, sizeof(Position));
    std::memset(this->inHand, NO_PIECE, sizeof(this->inHand));
    this->winner = NOBODY;  // TODO: init to set

    ss >> std::noskipws;

    // 1. Piece placement
    while ((ss >> token) && !isspace(token)) {
        if (token == '*' || (token >= '0' && token <= '9') || token == 'A' ||
            token == 'B' ||
            token == 'C' || token == 'a' || token == 'b' || token == 'c') {
            put_piece_init(sq, CharToPiece(token)); // TODO
            ++sq;
        } else {
            assert(0);
        }
    }

    // TODO: Move to other place and calc piece count
    bool found = false;

    for (int i = 0; i < 12; ++i) {
        found = false;
        for (int j = 0; j < 12; ++j) {
            if (board[j] == i) {
                found = true;
            }
        }
        if (found == false) {
            //inHand.push_back((Piece)i);
            pushBackInHand((Piece)i);
        }
    }

    // 2. Active color
    ss >> token;
    sideToMove = (token == 'w' ? WHITE : BLACK);
    them = ~sideToMove; // Note: Stockfish do not need to set them

    // 3. Last move
    ss >> std::skipws >> token;

    if (token == '*') {
        lastMove = MOVE_NONE;
    } else {
        lastMove = (Move)CharToPiece(token);
    }

    // 4-5. Halfmove clock and fullmove number
    ss >> std::skipws >> st.rule50 >> gamePly;

    // Convert from fullmove starting from 1 to gamePly starting from 0,
    // handle also common incorrect FEN with fullmove = 0.
    gamePly = std::max(2 * (gamePly - 1), 0) + (sideToMove == BLACK);

    // For Mill only
    //check_if_game_is_over();

    thisThread = th;

    return *this;
}

/// Position::fen() returns a FEN representation of the position.
/// This is mainly a debugging function.

string Position::fen() const
{
    std::ostringstream ss;

    for (int i = SQ_BEGIN; i != SQ_END; i++) {
        ss << PieceToChar(board[i]);
    }

    ss << " ";

    if (lastMove == MOVE_NONE) {
        ss << "* ";
    } else {
        ss << char(lastMove - '0');
    }

    ss << pieceOnBoardCount[WHITE] << " " << pieceInHandCount[WHITE] << " "
       << pieceOnBoardCount[BLACK] << " " << pieceInHandCount[BLACK] << " ";

    ss << st.rule50 << " " << 1 + (gamePly - (sideToMove == BLACK)) / 2;

    return ss.str();
}

/// Position::legal() tests whether a pseudo-legal move is legal

bool Position::legal(Move m) const
{
    // TODO
    return (is_ok((Square)m));
}

/// Position::do_move() makes a move, and saves all information necessary
/// to a StateInfo object. The move is assumed to be legal. Pseudo-legal
/// moves should be filtered out before this function is called.

GameStatus Position::do_move(Move m)
{
    move = m;

    // Verify that the chess piece number range is legal
    if (!is_ok((Square)m) && !(m == MOVE_PASS)) {
        return GameStatus::errorOutOfRange;
    }

    st.rule50++;

    // Either side can voluntarily give up the move of this round, and it is the
    // opponent's turn to move.
    if (m == MOVE_PASS) {
        // If Party A and Party B give up playing chess one after another, both
        // sides will be judged to lose, which is also a "lose-lose".
        if (lastMove == MOVE_PASS) {
            result = GameOverReason::bothLose;
            return GameStatus::resultBothLose;
        }

        change_side_to_move();
        lastMove = MOVE_PASS;
        return GameStatus::ok;
    }

    // The chess piece that the opponent has just passed in the last step, the
    // own side cannot repeat it in this step.
    if (m == lastMove) {
        return GameStatus::errCannotMoveLastMovedPiece;
    }

    // Pieces in the "upright" position cannot be moved.
    if (isFixed(m)) {
        return GameStatus::errCannotMoveFixedPiece;
    }

    // Find the piece to move
    for (Square i = SQ_BEGIN; i < SQ_END; ++i) {
        if (board[i] == m) {
            return move_piece(i, (Piece)m);
        }
    }

    return put_piece((Piece)m);

}

/// Position::undo_move() unmakes a move. When it returns, the position should
/// be restored to exactly the same state as before the move was made.

void Position::undo_move(Sanmill::Stack<Position> &ss)
{
    memcpy(this, ss.top(), sizeof(Position));
    ss.pop();
}

/// Position::key_after() computes the new hash key after the given move. Needed
/// for speculative prefetch. It doesn't recognize special moves like (need
/// remove)

Key Position::key_after(Move m) const
{
    Key k = st.key;

    // TODO
#if 0
    const auto s = to_sq(m);
    const MoveType mt = type_of(m);

    if (mt == MOVETYPE_REMOVE) {
        k ^= Zobrist::psq[~side_to_move()][s];
    } else {
        k ^= Zobrist::psq[side_to_move()][s];

        if (mt == MOVETYPE_MOVE) {
            k ^= Zobrist::psq[side_to_move()][from_sq(m)];
        }
    }
#endif

    k ^= Zobrist::side;

    return k;
}

int repetition;

// Position::has_repeated() tests whether there has been at least one repetition
// of positions since the last remove.

bool Position::has_repeated(Sanmill::Stack<Position> &ss) const
{
    for (int i = static_cast<int>(posKeyHistory.size()) - 2; i >= 0; i--) {
        if (key() == posKeyHistory[i]) {
            return true;
        }
    }

    const int size = ss.size();

    for (int i = size - 1; i >= 0; i--) {
        if (key() == ss[i].st.key) {
            return true;
        }
    }

    return false;
}

/// Position::has_game_cycle() tests if the position has a move which draws by
/// repetition.

bool Position::has_game_cycle() const
{
    for (const auto i : posKeyHistory) {
        if (key() == i) {
            repetition++;
            if (repetition == 3) {
                repetition = 0;
                return true;
            }
        }
    }

    return false;
}

/// Mill Game

bool Position::reset()
{
    repetition = 0;

    gamePly = 0;
    st.rule50 = 0;

    set_side_to_move(WHITE);

    winner = NOBODY;
    gameOverReason = GameOverReason::none;

    memset(board, NO_PIECE, sizeof(board));
    memset(byTypeBB, 0, sizeof(byTypeBB));
    memset(byColorBB, 0, sizeof(byColorBB));

    st.key = 0;

    pieceOnBoardCount[WHITE] = pieceOnBoardCount[BLACK] = 6;
    pieceInHandCount[WHITE] = pieceInHandCount[BLACK] = 0;

    mobilityDiff = 0;

    // TODO
    MoveList<LEGAL>::create();

    // Chaos Clock
    lastMove = MOVE_NONE;
    result = GameOverReason::none;
    //inHand.clear();
    clearInHand();
    historyList.clear();
    haveWon[BLACK] = haveWon[WHITE] = false;

#ifdef ENDGAME_LEARNING
    if (gameOptions.isEndgameLearningEnabled() && gamesPlayedCount > 0 &&
        gamesPlayedCount % SAVE_ENDGAME_EVERY_N_GAMES == 0) {
        Thread::saveEndgameHashMapToFile();
    }
#endif /* ENDGAME_LEARNING */

    return false;
}

bool Position::start()
{
    gameOverReason = GameOverReason::none;

    if (result != GameOverReason::none) {
        reset();
    }

    return true;
}

Piece init_board[SQUARE_NB];

void Position::initBoard()
{
    // Initially there are no chess pieces in hand
    //inHand.clear();
    clearInHand();

    // TODO
    // clear history
    historyList.clear();

#ifdef RANDOM_POSITION
    // Initialize chess pieces
    for (int i = 0; i < 12; ++i) {
        board[i] = i;
    }

    // In the initial state, no chess piece is allowed to be in "upright
    // position". if it exists, You need to re-allocate randomly until no chess
    // piece is in the "right position". The implementation here is to randomly
    // exchange the positions of the pieces until no pieces are in the upright
    // position.
    std::mt19937 rng(std::random_device {}());
    std::uniform_int_distribution<> dist(0, 12 - 1);
    while (hasFixedPiece()) {
        int i = dist(rng);
        int j = dist(rng);
        std::swap(board[i], board[j]);
    }
#else
    // Load hard-coded board for testing
    for (int i = 0; i < 12; ++i) {
        board[i] = init_board[i];
    }

    bool found = false;

    for (int i = 0; i < 12; ++i) {
        found = false;
        for (int j = 0; j < 12; ++j) {
            if (board[j] == i) {
                found = true;
            }
        }
        if (found == false) {
            //inHand.push_back((Piece)i);
            pushBackInHand((Piece)i);
        }
    }
#endif // RANDOM_POSITION
}

PieceStatus Position::getStatus(Square s)
{
    for (Square i = SQ_BEGIN; i < SQ_END; ++i) {
        if (board[i] == s) {
            return PieceStatus::onBoard;
        }
    }

    return PieceStatus::inHand;
}

GameStatus Position::put_piece_init(Square s, Piece pc)
{
    if (!(SQ_BEGIN <= s && s < SQ_END)) {
        return GameStatus::errorOutOfRange;
    }

    // TODO: Need bb and hash?

    // Place piece
    board[s] = pc;
    //byColorBB[color_of(pc)] |= s;

    return GameStatus::ok;
}

GameStatus Position::put_piece(Piece pc)
{
    const Color us = sideToMove;

    if (!(SQ_BEGIN <= pc && pc < SQ_END)) {
        return GameStatus::errorOutOfRange;
    }

    if (color_of(pc) != sideToMove) {
        return GameStatus::errCannotPlaceOpponentsPiece;
    }

    // TODO: Need bb and hash?

    Square s = (Square)pc;
    if (remove_piece(s, pc) == false) {
        change_side_to_move();
    }
   
    lastMove = (Move)s;

    // TODO: Performance
    //inHand.remove((Piece)s);
    removePieceInHand((Piece)s);
    //inHand.erase(std::remove(inHand.begin(), inHand.end(), s), inHand.end());

    return GameStatus::ok;
}

inline GameStatus Position::move_piece(Square from, Piece pc)
{
    Square to = Square((int)from + pc);

    if (to >= SQ_END) {
        to = Square((int)to - SQ_END);
    }

    // Pieces in the "upright" position cannot be captured.
    if (isFixed(to)) {
        return GameStatus::errCannotRemoveFixedPiece;
    }

    remove_piece(to, pc);
    lastMove = (Move)pc;

    board[from] = NO_PIECE;

    change_side_to_move();

    // Increment ply counters. In particular
    // TODO: Remove() has it. No need?
    ++gamePly;
    ++st.pliesFromNull;

    return GameStatus::ok;
}

bool Position::remove_piece(Square s, Piece pc)
{
    bool ret = false;
    revert_key(s);   // TODO

    Piece rm = board[s];

    CLEAR_BIT(byColorBB[color_of(pc)], s);  // TODO

    if (rm != NO_PIECE) {
        //inHand.push_back(rm);
        pushBackInHand(rm);

        if ((rm & 1) != sideToMove) {
            ret = true;
            st.rule50 = 0;
        }        
    }

    // Place piece
    board[s] = pc;
    byColorBB[color_of(pc)] |= s;

    pieceOnBoardCount[them]--;

    return ret;
}

bool Position::isFixed(int number)
{
    // The situation where the chess piece numbered n is placed exactly in the
    // empty space numbered n, Call the pawn number n in the "upright position".
    return board[number] == number;
}

bool Position::isAllFixed(Color c)
{
    if (c == WHITE) {
        for (int i = 0; i < 12; i += 2) {
            if (!isFixed(i)) {
                return false;
            }
        }
        return true;
    } else {
        for (int i = 1; i < 12; i += 2) {
            if (!isFixed(i)) {
                return false;
            }
        }
        return true;
    }
}

bool Position::hasFixedPiece()
{
    for (int i = 0; i < 12; ++i) {
        if (isFixed(i)) {
            return true;
        }
    }
    return false;
}

bool Position::bothWin()
{
    for (int i = 0; i < 12; ++i) {
        if (board[i] != i) {
            return false;
        }
    }

    return true;
}

bool Position::bothLost()
{
    // TODO: The current judgment is BOTH_LOSE_THRESHOLD
    // If the game is not over yet, both sides will lose. Is it possible to
    // judge in advance that neither side can win?
    return st.rule50 > BOTH_LOSE_THRESHOLD;
}

Color Position::get_winner() const noexcept
{
    return winner;
}

void Position::set_gameover(Color w, GameOverReason reason)
{
    gameOverReason = reason;
    winner = w;

    update_score();
}

void Position::update_score()
{
    if (result != GameOverReason::none) {
        if (winner == DRAW) {
            score_draw++;
            return;
        }

        score[winner]++;
    }
}

GameStatus Position::check_if_game_is_over(GameStatus status)
{
    // TODO: The performance of this method of judging the end of the chess game
    // is not good and needs to be optimized

    if (bothWin()) {
        result = GameOverReason::bothWin;
        return GameStatus::resultBothWin;
    }

    if (bothLost()) {
        result = GameOverReason::bothLose;
        return GameStatus::resultBothLose;
    }

    // TODO: This paragraph is not concise and needs to be refactored
    if (isAllFixed(sideToMove)) {
        if (haveWon[sideToMove] == true) {
            // If Party A has already won but Party B failed to win, then only
            // Party A wins, it will not be a win-win situation
            result = GameOverReason::win;
            return GameStatus::resultWin;
        } else {
            haveWon[sideToMove] = true;
        }
    }

    return status;
}

// Output chess game status information
void Position::showGameStatus(GameStatus status)
{
    cout << "\n" << gameStatusStr[int(status)] << endl;
}

void Position::printClock()
{
    // The chessboard is circular, looks like a clock, and has 12 spaces
    // numbered clockwise from 1 to 12.
    cout << "  " << setw(2) << setfill('0') << board[11] << " " << setw(2)
         << setfill('0') << board[0] << " " << setw(2) << setfill('0')
         << board[1] << " " << endl;
    cout << setw(2) << setfill('0') << board[10] << "        " << setw(2)
         << setfill('0') << board[2] << endl;
    cout << setw(2) << setfill('0') << board[9] << "        " << setw(2)
         << setfill('0') << board[3] << endl;
    cout << setw(2) << setfill('0') << board[8] << "        " << setw(2)
         << setfill('0') << board[4] << endl;
    cout << "  " << setw(2) << setfill('0') << board[7] << " " << setw(2)
         << setfill('0') << board[6] << " " << setw(2) << setfill('0')
         << board[5] << "  " << endl;

    cout << endl;
}

void Position::printPiecesOnBoard()
{
    // cout << "On board: ";

    for (int i = 0; i < 12; i++) {
        if (board[i] == i) {
            cout << "[" << board[i] << "] ";
        } else {
            cout << board[i] << " ";
        }
    }

    // cout << endl;
}

void Position::printPiecesInHand()
{
    // cout << "In hand: ";

    for (int i = SQ_BEGIN; i < SQ_END; i++) {
        auto pc = inHand[i];

        if (pc == NO_PIECE) {
            //cout << "{" << pc << "} ";
        } else if (pc > 0 && pc % 2 == 1) {
            cout << "(" << pc << ") ";
        } else if (pc >= 0 && pc % 2 == 0) {
            cout << "<" << pc << "> ";
        } else {
            cout << pc << "? ";
        }
    }

    cout << endl;
}

void Position::printMoveList()
{
    cout << "\nMove list: ";

    int size = historyList.size();

    for (int i = 0; i < size; i++) {
        if (i == size - 1) {
            cout << historyList[i] << endl;
        } else {
            cout << historyList[i] << ", ";
        }
    }
}

void Position::printLastMove()
{
    cout << endl;
    cout << "Last move: " << lastMove << endl;
}

void Position::printSideToMove()
{
    cout << "\n----------------------------------------------------------"
         << endl
         << endl;

    if (sideToMove == BLACK) {
        cout << "Odd";
        cout << endl;
    } else {
        cout << "Even";
        cout << endl;
    }
}

// Output the current game state
void Position::print_board()
{
    if (sideToMove == WHITE) {
        // cout << "\033[33m";
    }

    printClock();
    printPiecesOnBoard();
    cout << ": ";
    printPiecesInHand();
    printLastMove();
    printMoveList();
    printSideToMove();

    if (sideToMove == WHITE) {
        // cout << "\033[0m";
    }
}

int Position::calculate_mobility_diff()
{
    // TODO(calcitem): Deal with rule is no ban location
    int mobilityWhite = 0;
    int mobilityBlack = 0;

    // TODO

    return mobilityWhite - mobilityBlack;
}

inline void Position::set_side_to_move(Color c)
{
    sideToMove = c;
    // us = c;
    them = ~sideToMove;
}

inline void Position::change_side_to_move()
{
    set_side_to_move(~sideToMove);
    st.key ^= Zobrist::side;
}

inline Key Position::update_key(Square s)
{
    st.key ^= Zobrist::psq[s];

    return st.key;
}

inline Key Position::revert_key(Square s)
{
    return update_key(s);
}

///////////////////////////////////////////////////////////////////////////////

#include "misc.h"
#include "movegen.h"

Color Position::color_on(Square s) const
{
    return color_of(board[s]);
}

bool Position::bitboard_is_ok()
{
#ifdef BITBOARD_DEBUG
    Bitboard whiteBB = byColorBB[WHITE];
    Bitboard blackBB = byColorBB[BLACK];

    for (Square s = SQ_BEGIN; s < SQ_END; ++s) {
        if (empty(s)) {
            if (whiteBB & (1 << s)) {
                return false;
            }

            if (blackBB & (1 << s)) {
                return false;
            }
        }

        if (color_of(board[s]) == WHITE) {
            if ((whiteBB & (1 << s)) == 0) {
                return false;
            }

            if (blackBB & (1 << s)) {
                return false;
            }
        }

        if (color_of(board[s]) == BLACK) {
            if ((blackBB & (1 << s)) == 0) {
                return false;
            }

            if (whiteBB & (1 << s)) {
                return false;
            }
        }
    }
#endif

    return true;
}

void Position::reset_bb()
{
    memset(byTypeBB, 0, sizeof(byTypeBB));
    memset(byColorBB, 0, sizeof(byColorBB));

    for (Square s = SQ_BEGIN; s < SQ_END; ++s) {
        const Piece pc = board[s];
        byColorBB[color_of(pc)] |= s;
    }
}
