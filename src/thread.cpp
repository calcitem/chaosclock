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

#include <iomanip>
#include <utility>

#include "mills.h"
#include "option.h"
#include "thread.h"
#include "uci.h"

#ifdef MADWEASEL_MUEHLE_PERFECT_AI
#include "perfect/perfect.h"
#endif

#ifdef FLUTTER_UI
#include "engine_main.h"
#endif

#ifdef OPENING_BOOK
#include <deque>
#endif

using std::cout;
using std::string;

ThreadPool Threads; // Global object

/// Thread constructor launches the thread and waits until it goes to sleep
/// in idle_loop(). Note that 'searching' and 'exit' should be already set.

Thread::Thread(size_t n
#ifdef QT_GUI_LIB
               ,
               QObject *parent
#endif
               )
    :
#ifdef QT_GUI_LIB
    QObject(parent)
    ,
#endif
    idx(n)
    , stdThread(&Thread::idle_loop, this)
    , timeLimit(3600)
{
    wait_for_search_finished();
}

/// Thread destructor wakes up the thread in idle_loop() and waits
/// for its termination. Thread should be already waiting.

Thread::~Thread()
{
    assert(!searching);

    exit = true;
    start_searching();
    stdThread.join();
}

/// Thread::clear() reset histories, usually before a new game

void Thread::clear() noexcept
{
    // TODO(calcitem): Reset histories
    return;
}

/// Thread::start_searching() wakes up the thread that will start the search

void Thread::start_searching()
{
    std::lock_guard lk(mutex);
    searching = true;
    cv.notify_one(); // Wake up the thread in idle_loop()
}

void Thread::pause()
{
    std::lock_guard lk(mutex);
    searching = false;
    cv.notify_one(); // Wake up the thread in idle_loop()
}

/// Thread::wait_for_search_finished() blocks on the condition variable
/// until the thread has finished searching.

void Thread::wait_for_search_finished()
{
    std::unique_lock lk(mutex);
    cv.wait(lk, [&] { return !searching; });
}

#ifdef NNUE_GENERATE_TRAINING_DATA
extern Value nnueTrainingDataBestValue;
#endif /* NNUE_GENERATE_TRAINING_DATA */

/// Thread::idle_loop() is where the thread is parked, blocked on the
/// condition variable, when it has no work to do.

void Thread::idle_loop()
{
    while (true) {
        std::unique_lock lk(mutex);
        // CID 338451: Data race condition(MISSING_LOCK)
        // missing_lock : Accessing this->searching without holding lock
        // Thread.mutex. Elsewhere, Thread.searching is accessed with
        // Thread.mutex held 2 out of 3 times(2 of these accesses strongly imply
        // that it is necessary).
        searching = false;

        cv.notify_one(); // Wake up anyone waiting for search finished
        cv.wait(lk, [&] { return searching; });

        if (exit)
            return;

        lk.unlock();

#if 0
        // TODO: Why Sanmill need this?
        // Note: Stockfish doesn't have this
        if (rootPos == nullptr || rootPos->side_to_move() != us) {
            continue;
        }
#endif

#ifdef MADWEASEL_MUEHLE_PERFECT_AI
        if (gameOptions.getPerfectAiEnabled()) {
            bestMove = perfect_search();
            assert(bestMove != MOVE_NONE);
            strCommand = next_move();
            if (strCommand != "" && strCommand != "error!") {
                emitCommand();
            }
        } else {
#endif // MADWEASEL_MUEHLE_PERFECT_AI
#ifdef OPENING_BOOK
            // gameOptions.getOpeningBook()
            if (!openingBookDeque.empty()) {
                char obc[16] = {0};
                sq2str(obc);
                strCommand = obc;
                emitCommand();
            } else {
#endif
                const int ret = search();

#ifdef NNUE_GENERATE_TRAINING_DATA
                nnueTrainingDataBestValue = rootPos->sideToMove == WHITE ? bestvalue :
                                                              -bestvalue;
#endif /* NNUE_GENERATE_TRAINING_DATA */

                if (ret == 3 || ret == 50 || ret == 10) {
                    debugPrintf("Draw\n\n");
                    strCommand = "draw";
                    emitCommand();
                } else {
                    strCommand = next_move();
                    if (strCommand != "" && strCommand != "error!") {
                        emitCommand();
                    }
                }
#ifdef OPENING_BOOK
            }
#endif
#ifdef MADWEASEL_MUEHLE_PERFECT_AI
        }
#endif // MADWEASEL_MUEHLE_PERFECT_AI
    }
}

////////////////////////////////////////////////////////////////////////////

void Thread::setAi(Position *p)
{
    std::lock_guard lk(mutex);

    this->rootPos = p;

#ifdef TRANSPOSITION_TABLE_ENABLE
#ifdef CLEAR_TRANSPOSITION_TABLE
    TranspositionTable::clear();
#endif
#endif
}

void Thread::setAi(Position *p, int time)
{
    setAi(p);

    timeLimit = time;
}

void Thread::emitCommand()
{
#ifdef QT_GUI_LIB
    emit command(strCommand);
#else
    sync_cout << "bestmove " << strCommand.c_str();
    std::cout << sync_endl;

#ifdef FLUTTER_UI
    println("bestmove %s", strCommand.c_str());
#endif

#ifdef UCI_DO_BEST_MOVE
    //rootPos->command(strCommand.c_str());
    rootPos->do_move(bestMove);
    us = rootPos->side_to_move();

    if (strCommand.size() > strlen("-(1,2)")) {
        posKeyHistory.push_back(rootPos->key());
    } else {
        posKeyHistory.clear();
    }
#endif

#ifdef ANALYZE_POSITION
    analyze(rootPos->side_to_move());
#endif
#endif // QT_GUI_LIB
}

#ifdef OPENING_BOOK
deque<int> openingBookDeque({
    /* B W */
    21,
    23,
    19,
    20,
    17,
    18,
    15,
});

deque<int> openingBookDequeBak;

void sq2str(char *str)
{
    int sq = openingBookDeque.front();
    openingBookDeque.pop_front();
    openingBookDequeBak.push_back(sq);

    File file = FILE_A;
    Rank rank = RANK_1;
    int sig = 1;

    if (sq < 0) {
        sq = -sq;
        sig = 0;
    }

    file = file_of(sq);
    rank = rank_of(sq);

    if (sig == 1) {
        snprintf(str, Position::RECORD_LEN_MAX, 16, "(%d,%d)", file, rank);
    } else {
        snprintf(str, Position::RECORD_LEN_MAX, "-(%d,%d)", file, rank);
    }
}
#endif // OPENING_BOOK

void Thread::analyze(Color c) const
{
    // TODO
    static float nWhiteWin = 0;
    static float nBlackWin = 0;
    static float nDraw = 0;
#ifndef QT_GUI_LIB
    float total;
    float blackWinRate, whiteWinRate, drawRate;
#endif // !QT_GUI_LIB

    const int d = originDepth;
    const int v = bestvalue;
    const int lv = lastvalue;

    cout << std::endl << std::endl;
}

Depth Thread::get_depth() const
{
    return Mills::get_search_depth(rootPos);
}

string Thread::next_move() const
{
#ifdef ENDGAME_LEARNING
    // Check if very weak
    if (gameOptions.isEndgameLearningEnabled()) {
        if (bestvalue <= -VALUE_KNOWN_WIN) {
            Endgame endgame;
            endgame.type = rootPos->side_to_move() == WHITE ?
                               EndGameType::blackWin :
                               EndGameType::whiteWin;
            Key endgameHash = rootPos->key(); // TODO(calcitem): Do not generate
                                              // hash repeatedly
            saveEndgameHash(endgameHash, endgame);
        }
    }
#endif /* ENDGAME_LEARNING */

#ifdef TRANSPOSITION_TABLE_ENABLE
#ifdef TRANSPOSITION_TABLE_DEBUG
    size_t hashProbeCount = ttHitCount + ttMissCount;
    if (hashProbeCount) {
        debugPrintf("[posKey] probe: %llu, hit: %llu, miss: %llu, hit rate: "
                    "%llu%%\n",
                    hashProbeCount, ttHitCount, ttMissCount,
                    ttHitCount * 100 / hashProbeCount);
    }
#endif // TRANSPOSITION_TABLE_DEBUG

#endif // TRANSPOSITION_TABLE_ENABLE

    return UCI::move(bestMove);
}

#ifdef ENDGAME_LEARNING
bool Thread::probeEndgameHash(Key posKey, Endgame &endgame)
{
    return endgameHashMap.find(posKey, endgame);
}

int Thread::saveEndgameHash(Key posKey, const Endgame &endgame)
{
    Key hashValue = endgameHashMap.insert(posKey, endgame);
    unsigned addr = hashValue * (sizeof(posKey) + sizeof(endgame));

    debugPrintf("[endgame] Record 0x%08I32x (%d) to Endgame hash map, TTEntry: "
                "0x%08I32x, Address: 0x%08I32x\n",
                posKey, endgame.type, hashValue, addr);

    return 0;
}

void Thread::clearEndgameHashMap()
{
    endgameHashMap.clear();
}

void Thread::saveEndgameHashMapToFile()
{
    const string filename = "endgame.txt";
    endgameHashMap.dump(filename);

    debugPrintf("[endgame] Dump hash map to file\n");
}

void Thread::loadEndgameFileToHashMap()
{
    const string filename = "endgame.txt";
    endgameHashMap.load(filename);
}

#endif // ENDGAME_LEARNING

/// ThreadPool::set() creates/destroys threads to match the requested number.
/// Created and launched threads will immediately go to sleep in idle_loop.
/// Upon resizing, threads are recreated to allow for binding if necessary.

void ThreadPool::set(size_t requested)
{
    if (!empty()) {
        // destroy any existing thread(s)
        main()->wait_for_search_finished();

        while (!empty()) {
            delete back();
            pop_back();
        }
    }

    if (requested > 0) {
        // create new thread(s)
        push_back(new MainThread(0));

        while (size() < requested)
            push_back(new Thread(size()));
        clear();

#ifdef TRANSPOSITION_TABLE_ENABLE
        // Reallocate the hash with the new thread pool size
        TT.resize(static_cast<size_t>(Options["Hash"]));
#endif

        // Init thread number dependent search params.
        Search::init();
    }
}

/// ThreadPool::clear() sets threadPool data to initial values.

void ThreadPool::clear() const
{
    for (const Thread *th : *this)
        th->clear();
}

/// ThreadPool::start_thinking() wakes up main thread waiting in idle_loop() and
/// returns immediately. Main thread will wake up other threads and start the
/// search.

void ThreadPool::start_thinking(Position *pos, bool ponderMode)
{
    main()->wait_for_search_finished();

    main()->stopOnPonderhit = stop = false;
    increaseDepth = true;
    main()->ponder = ponderMode;

    // We use Position::set() to set root position across threads.
    for (Thread *th : *this) {
        // Fix CID 338443: Data race condition (MISSING_LOCK)
        // missing_lock: Accessing th->rootPos without holding lock
        // Thread.mutex. Elsewhere, Thread.rootPos is accessed with Thread.mutex
        // held 1 out of 2 times (1 of these accesses strongly imply that it is
        // necessary).
        std::lock_guard lk(th->mutex);
        th->rootPos = pos;
    }

    main()->start_searching();
}