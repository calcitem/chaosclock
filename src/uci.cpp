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

#include <sstream>
#include <vector>

#include "thread.h"
#include "uci.h"

#ifdef FLUTTER_UI
#include "base.h"
#include "command_channel.h"
#endif

using std::cin;
using std::istream;
using std::istringstream;
using std::skipws;
using std::string;
using std::stringstream;
using std::vector;

extern vector<string> setup_bench(Position *, istream &);

extern int repetition;

namespace {

// FEN string of the initial position, normal mill game
// TODO
// 10A, 11B, 12C,
//const char *StartFEN0 = "6B89541A7023 w * 0 1";
const char *StartFEN0 = "6B8954*17023 b 1 0 1";

char StartFEN[BUFSIZ];

// position() is called when engine receives the "position" UCI command.
// The function sets up the position described in the given FEN string ("fen")
// or the starting position ("startpos") and then makes the moves given in the
// following move list ("moves").

void position(Position *pos, istringstream &is)
{
    Move m;
    string token, fen;

    is >> token;

    if (token == "startpos") {
        fen = StartFEN;
        is >> token; // Consume "moves" token if any
    } else if (token == "fen") {
        while (is >> token && token != "moves") {
            fen += token + " ";
        }
    } else {
        return;
    }

    repetition = 0;
    posKeyHistory.clear();

    pos->set(fen, Threads.main());

    // Parse move list (if any)
    while (is >> token && (m = UCI::to_move(pos, token)) != MOVE_NONE) {
        pos->do_move(m);        
        posKeyHistory.push_back(pos->key());      
    }

    // TODO(calcitem): Stockfish does not have this
    Threads.main()->us = pos->sideToMove;
}

// setoption() is called when engine receives the "setoption" UCI command. The
// function updates the UCI option ("name") to the given value ("value").

void setoption(istringstream &is)
{
    string token, name, value;

    is >> token; // Consume "name" token

    // Read option name (can contain spaces)
    while (is >> token && token != "value")
        name += (name.empty() ? "" : " ") + token;

    // Read option value (can contain spaces)
    while (is >> token)
        value += (value.empty() ? "" : " ") + token;

    if (Options.count(name))
        Options[name] = value;
    else
        sync_cout << "No such option: " << name << sync_endl;
}

// go() is called when engine receives the "go" UCI command. The function sets
// the thinking time and other parameters from the input string, then starts
// the search.

void go(Position *pos)
{
#ifdef UCI_AUTO_RE_GO
begin:
#endif

    repetition = 0;

    Threads.start_thinking(pos);

    if (pos->result != GameOverReason::none) {
#ifdef UCI_AUTO_RESTART
        // TODO(calcitem)
        while (true) {
            if (Threads.main()->searching == true) {
                continue;
            }

            pos->set(StartFEN, Threads.main());
            Threads.main()->us = WHITE; // WAR
            break;
        }
#else
        return;
#endif
    }

#ifdef UCI_AUTO_RE_GO
    goto begin;
#endif
}

} // namespace

/// UCI::loop() waits for a command from stdin, parses it and calls the
/// appropriate function. Also intercepts EOF from stdin to ensure gracefully
/// exiting if the GUI dies unexpectedly. When called with some command line
/// arguments, e.g. to run 'bench', once the command is executed the function
/// returns immediately. In addition to the UCI ones, also some additional debug
/// commands are supported.

void UCI::loop(int argc, char *argv[])
{
    const auto pos = new Position;
    string token, cmd;

#ifdef _MSC_VER
    strncpy_s(StartFEN, BUFSIZ, StartFEN0, BUFSIZ - 1);
#else
    strncpy(StartFEN, StartFEN0, BUFSIZ - 1);
#endif

    StartFEN[BUFSIZ - 1] = '\0';

    pos->set(StartFEN, Threads.main());

    for (int i = 1; i < argc; ++i)
        cmd += std::string(argv[i]) + " ";

    do {
#ifdef FLUTTER_UI
        static const int LINE_INPUT_MAX_CHAR = 4096;
        char line[LINE_INPUT_MAX_CHAR];
        CommandChannel *channel = CommandChannel::getInstance();
        while (!channel->popupCommand(line))
            Idle();
        cmd = line;
        LOGD("[uci] input: %s\n", line);
#else
        if (argc == 1 && !getline(cin, cmd)) // Block here waiting for input or
                                             // EOF
            cmd = "quit";
#endif

        istringstream is(cmd);

        token.clear(); // Avoid a stale if getline() returns empty or blank line
        is >> skipws >> token;

        if (token == "quit" || token == "stop")
            Threads.stop = true;

        // The GUI sends 'ponderhit' to tell us the user has played the expected
        // move. So 'ponderhit' will be sent if we were told to ponder on the
        // same move the user has played. We should continue searching but
        // switch from pondering to normal search.
        else if (token == "ponderhit")
            Threads.main()->ponder = false; // Switch to normal search

        else if (token == "uci")
            sync_cout << "id name " << engine_info(true) << "\n"
                      << Options << "\nuciok" << sync_endl;

        else if (token == "setoption")
            setoption(is);
        else if (token == "go")
            go(pos);
        else if (token == "position")
            position(pos, is);
        else if (token == "ucinewgame")
            Search::clear();
        else if (token == "isready")
            sync_cout << "readyok" << sync_endl;

        // Additional custom non-UCI commands, mainly for debugging.
        // Do not use these commands during a search!
        else if (token == "d")
            sync_cout << *pos << sync_endl;
        else if (token == "compiler")
            sync_cout << compiler_info() << sync_endl;
        else
            sync_cout << "Unknown command: " << cmd << sync_endl;
    } while (token != "quit" && argc == 1); // Command line args are one-shot

    delete pos;
}

/// UCI::move() converts a Move to a string in algebraic notation ((1,2), etc.).

string UCI::move(Move m)
{
    const Square to = Square(m);

    if (m == MOVE_NONE)
        return "(none)";

    if (m == MOVE_PASS)
        return "(pass)";

    return std::to_string(m);
}

/// UCI::to_move() converts a string representing a move in coordinate notation
/// to the corresponding legal Move, if any.

Move UCI::to_move(Position *pos, const string &str)
{
    for (const auto &m : MoveList<LEGAL>(*pos))
        if (str == move(m))
            return m;

    return MOVE_NONE;
}
