// 这是 “混乱时钟” 游戏的 C++ 实现。
// 在这个游戏中，两个玩家通过移动棋子在棋盘上的 12 个位置上进行对决。
// “混乱时钟”游戏是一个二人对弈游戏，双方分别称作甲方和乙方。
// 棋盘是顺时针编号为1到12的圆形棋盘，初始状态下有12颗棋子随机分配在12个空位上，
// (程序中为了计算方便，用编号0表示编号12)
// 但不允许有棋子处于“正位”。
// 行棋顺序为乙方先行棋，双方轮流行棋。
// 行棋有三种类型：走子、落子和不走。
// “走子”是指从棋盘上拿起任意一颗棋子，顺时针走n步并吃掉停留位置上的棋子，
// 吃掉的棋子如果是奇数交到甲方手中，是偶数交到乙方手中。
// “落子”是指把手中的棋子放在它的正位上，吃掉对方棋子则己方多一次行棋机会。
// 当双方都不可能把棋子落到“正位”或连续放弃行棋时，为“双输”；
// 如果有一方把己方棋子全部放到“正位”，为“赢棋”；
// 如果对方接下来也达成将己方棋子放到“正位”，则为“双赢”；
// 如果某一方最后一步是落子吃棋，则是此方“赢棋”。

#include <iostream>     // std::cout, std::endl
#include <iomanip>      // std::setw
#include <random>
#include <vector>
#include <string>
#include <algorithm>

using namespace std;

//#define TEST_MODE

// 本游戏为二人游戏，游戏双方分别被称作甲方和乙方。
enum Color {
    YI = 0,     // 乙方
    JIA = 1,    // 甲方
};

// 行棋分3种类型
enum class MoveType {
    move,   // 走子
    place,  // 摆子
    pass,   // 不走，即放弃本次行棋机会，转由对方走棋。
};

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

static const string gameStatusStr[] = {
    "",
    "输入的范围错误。请输入 0-11，或者 -1，其中 0 表示 12，-1 表示放弃此轮行棋机会。",
    "对方上一步刚走过的棋子，己方在这一步不能再重复拿来走。",
    "不能拿对方的子来落子",
    "处于“正位”的棋子，不能再移动。",
    "处于“正位”的棋子，不能被吃掉。",
    "甲方赢棋。",
    "乙方赢棋。",
    "甲乙双方接连归于正位，双赢。",
    "甲乙双方接连放弃行棋，双输。",
    "未知",
};

enum class GameResult {
    none,
    jiaWin,
    yiWin,
    bothWin,
    bothLost,
};

void remove_first_element_with_value(std::vector<int>& arr, int n)
{
    auto new_end = std::remove(arr.begin(), arr.end(), n);
    arr.erase(new_end, arr.end());
}

class Piece {
public:
    int location {-1};
    int number {0};

    PieceStatus getStatus() {
        return location == -1 ? PieceStatus::inHand : PieceStatus::onBoard;
    }
};

// 用于存储棋盘的当前状态。
class Position {
public:
    // 时钟上 12 个点
    int board[12] { -1 };

    // 在双方手上的棋子
    vector<int> inHand;

    // 上次的着法
    int lastMove { -2 };

    // 双方走了多少步
    int step { 0 };

    // 当前该谁下棋
    // (行棋顺序: 先由乙方先行棋，之后双方轮流行棋。)
    Color sideToMove { YI };

    // 游戏结果
    GameResult result { GameResult::none };

    // 历史着法
    vector<int> moveList;

    // 是否有一方已经赢了
    bool jiaHasWon { false };
    bool yiHasWon { false };

    Position()
    {
        initBoard();
    }

    // 交换行棋方
    void changeSideToMove() {
        if (sideToMove == YI) {
            sideToMove = JIA;
        } else {
            sideToMove = YI;
        }
    }

    // 判断编号为 n 的棋子是否处于 “正位”
    bool isFixed(int number) {
        // 编号为 n 的棋子被正好摆放到编号为 n 的空位的这种情况，
        // 称之为编号为 n 的棋子处于“正位”。
        return board[number] == number;
    }

    // 判断是否存在棋子处于正位
    bool hasFixedPiece()
    {
        for (int i = 0; i < 12; ++i) {
            if (isFixed(i)) {
                return true;
            }
        }
        return false;
    }

    // 乙方棋子全部放到“正位”
    bool yiIsFixed()
    {
        for (int i = 0; i < 12; i += 2) {
            if (!isFixed(i)) {
                return false;
            }
        }
        return true;
    }

    // 甲方棋子全部放到“正位”
    bool jiaIsFixed()
    {
        for (int i = 1; i < 12; i += 2) {
            if (!isFixed(i)) {
                return false;
            }
        }
        return true;
    }

    // 初始状态下，有编号为 1 到 12 的棋子随机地分配并摆放到棋盘上的 12 个空位上。
    void initBoard()
    {
        // 初始时手上无棋子
        inHand.clear();

        // 清空历史
        moveList.clear();

        // 初始化棋子
        for (int i = 0; i < 12; ++i) {
            board[i] = i;
        }

        // 初始状态下不允许任何棋子处于“正位”。如果存在，
        // 则需要重新随机分配直到没有棋子处于“正位”为止。
        // 此处实现为，随机交换棋子位置，直到没有棋子处于正位。
        std::mt19937 rng(std::random_device {}());
        std::uniform_int_distribution<> dist(0, 12 - 1);
        while (hasFixedPiece()) {
            int i = dist(rng);
            int j = dist(rng);
            std::swap(board[i],board[j]);
        }

#ifdef TEST_MODE
        int testBoard[] = {
            1,
            6,
            3,
            7,
            4,
            9,
            8,
            11,
            5,
            10,
            0,
            2,
        };

        // 载入写死的用于测试的棋盘
        for (int i = 0; i < 12; ++i) {
            board[i] = testBoard[i];
        }
#endif // TEST_MODE
    }

    PieceStatus getStatus(int number)
    {
        for (int i = 0; i < 12; ++i) {
            if (board[i] == number) {
                return PieceStatus::onBoard;
            }
        }

        return PieceStatus::inHand;
    }

    // “落子” 是指，一方把手中的己方棋子落在它的正位上，
    // 比如将编号为n的棋子放在编号为n的空位上。
    // 如果这个空位上有其它棋子，也要把它吃掉。
    GameStatus place(int number)
    {
        // 任何一方吃掉的棋子，如果这个棋子的编号是奇数，就交到甲方的手上，
        // 如果是偶数就交到乙方的手上。这些拿在手上的棋子将用于落子。
        // 因此，落子之前需要先判断奇偶性，不能拿对方的子来落子。
        if (number % 2 != sideToMove) {
            return GameStatus::errCannotPlaceOpponentsPiece;
        }

        // 把手中的己方棋子落在它的正位上。
        // 如果这个空位上有其它棋子，这个棋子就被吃掉了。
        // 如果落子时吃掉了对方的棋子，则己方多一次行棋的机会，否则就换对方继续行棋
        int location = number;
        if (remove(location, number) == false) {
            changeSideToMove();
        }

        lastMove = number;

        remove_first_element_with_value(inHand, number);

        return GameStatus::ok;
    }

    // 验证棋子号码是否合法
    bool isOk(int number) {
        if (number < -1 || number > 11) {
            return false;
        }

        return true;
    }

    // "走子" 是指，一方拿起棋盘上的任意一枚棋子，此棋子的编号是 n，则顺时针走 n 步，
    // 若走 n 步后停留的空位有其它棋子，则要把它吃掉。
    GameStatus move(int location, int number)
    {
        int newLocation = (location + number) % 12;

        // 处于“正位”的棋子，不能被吃掉。
        if (isFixed(newLocation)) {
            return GameStatus::errCannotRemoveFixedPiece;
        }

        remove(newLocation, number);
        lastMove = number;

        board[location] = -1;

        changeSideToMove();

        return GameStatus::ok;
    }

    // 使用 number 的己方棋子吃掉对方位于 location 的棋子, 如确实吃掉了对方子，返回 true
    bool remove(int location, int number)
    {
        bool ret = false;
        int pc = board[location];

        // 如果目标位置有子，即此次行棋确实是吃子，而不是放在空位上
        if (pc != -1) {
            inHand.push_back(pc);

            // 如果吃掉的不是自己的棋子
            if (pc % 2 != sideToMove) {
                ret = true;
            }
        }

        board[location] = number;

        return ret;
    }

    // 用于执行玩家的行棋操作，包括走子、落子、放弃。
    GameStatus doMove(int number)
    {
        // 验证棋子号码范围是否合法
        if (!isOk(number))
        {
            return GameStatus::errorOutOfRange;
        }

        // 何一方都可以主动放弃本回合的行棋，轮到对方行棋。
        if (number == -1) {
            // 若甲乙双方接连放弃行棋，则判双方都输棋，即也是“双输”。
            if (lastMove == -1) {
                result = GameResult::bothLost;
                return GameStatus::resultBothLost;
            }

            changeSideToMove();
            lastMove = -1;
            return GameStatus::ok;
        }

        // 对方上一步刚走过的棋子，己方在这一步不能再重复拿来走。
        if (number == lastMove) {
            return GameStatus::errCannotMoveLastMovedPiece;
        }

        // 处于“正位”的棋子，不能再移动。
        if (isFixed(number)) {
            return GameStatus::errCannotMoveFixedPiece;
        }

        // 找到要移动的棋子
        for (int i = 0; i < 12; ++i) {
            if (board[i] == number) {
                return move(i, number);
            }
        }

        return place(number);
    }

    // 双方的棋子都在正位上，游戏结束
    bool bothWin()
    {
        for (int i = 0; i < 12; ++i) {
            if (board[i] != i) {
                return false;
            }
        }

        return true;
    }

    bool bothLost()
    {
        // TODO: 当前是判断 100 步还未结束棋局就算双输，是否有提前判断双方都不可能赢？
        return step > 100;
    }

    GameStatus checkIfGameIsOver(GameStatus status)
    {
        // TODO: 这段判断棋局结束的方式性能不佳，需优化

        if (bothWin()) {
            result = GameResult::bothWin;
            return GameStatus::resultBothWin;
        }

        if (bothLost()) {
            result = GameResult::bothLost;
            return GameStatus::resultBothLost;
        }

        // TODO: 这段很不简洁，需要重构
        if (jiaIsFixed()) {
            if (jiaHasWon == true) {
                // 如果甲方早就赢了但乙方没能赢，那么就只有甲方赢，不会是双赢
                result = GameResult::jiaWin;
                return GameStatus::resultJiaWin;
            } else {
                jiaHasWon = true;
            }

            if (sideToMove == JIA) {
                // 如甲赢了且接下来还是甲走棋，甲肯定单独赢了，不会是双赢
                result = GameResult::jiaWin;
                return GameStatus::resultJiaWin;
            }
        }

        if (yiIsFixed()) {
            if (yiHasWon == true) {
                // 如果乙方早就赢了但甲方没能赢，那么就只有乙方赢，不会是双赢
                result = GameResult::yiWin;
                return GameStatus::resultYiWin;
            } else {
                yiHasWon = true;
            }

            if (sideToMove == YI) {
                // 如乙赢了且接下来还是乙走棋，乙肯定单独赢了，不会是双赢
                result = GameResult::yiWin;
                return GameStatus::resultYiWin;
            }
        }

        return status;
    }

    // 输出棋局状态信息
    void showGameStatus(GameStatus status)
    {
        cout << "\n" << gameStatusStr[int(status)] << endl;
    }

    void printClock()
    {
        // 棋盘是圆形的，长得像钟表的，有从顺时针方向编号 1 到 12 共 12 个空位。
        cout << "  " << setw(2) << setfill('0') << board[11] << " "
             << setw(2) << setfill('0') << board[0] << " "
             << setw(2) << setfill('0') << board[1] << " " << endl;
        cout << setw(2) << setfill('0') << board[10] << "        "
             << setw(2) << setfill('0') << board[2] << endl;
        cout << setw(2) << setfill('0') << board[9] << "        "
             << setw(2) << setfill('0') << board[3] << endl;
        cout << setw(2) << setfill('0') << board[8] << "        "
             << setw(2) << setfill('0') << board[4] << endl;
        cout << "  " << setw(2) << setfill('0') << board[7] << " "
             << setw(2) << setfill('0') << board[6] << " "
             << setw(2) << setfill('0') << board[5] << "  " << endl;

        cout << endl;

    }

    void printPiecesOnBoard()
    {
        cout << "棋盘上有的棋子: ";

        for (int i = 0; i < 12; i++) {
            if (board[i] == i) {
                cout << "[" << board[i] << "] ";
            } else {
                cout << board[i] << " ";
            }
        }

        cout << endl;
    }

    void printPiecesInHand()
    {
        cout << "双方手中的棋子: ";

        for (const auto& element : inHand) {
            if (element == -1) {
                cout << "{" << element << "} ";
            } else if (element > 0 && element % 2 == 1) {
                cout << "(" << element << ") ";
            } else if (element >= 0 && element % 2 == 0) {
                cout << "[" << element << "] ";
            } else {
                cout << element << "? ";
            }
        }

        cout << endl;
    }

    void printMoveList()
    {
        cout << "历史着法: ";

        for (const auto& element : moveList) {
            cout << element << " ";
        }

        cout << endl;
    }

    void printSideToMove()
    {
        cout << "\n--------------------------------------------------" << endl
             << endl;

        if (sideToMove == JIA) {
            cout << "下面轮到“甲方”行棋。" << endl;
        } else {
            cout << "下面轮到“乙方”行棋。" << endl;
        }
    }

    // 输出当前棋局状态
    void print()
    {
        printClock();
        printPiecesOnBoard();
        printPiecesInHand();
        printMoveList();
        printSideToMove();
    }
};

// 在 main 函数中，使用循环不断执行玩家的落子操作，直到游戏结束。
// 如果玩家的输入不合法，则给出错误提示。
int main()
{
    Position position;
    cout << "混乱时钟游戏开始。" << endl << endl;
    position.print();
    position.step = 0;

    for (;;) {
        position.step++;
        int number;

        cout << "请输入要移动或摆放的棋子编号，不走请输入 -1： ";
        cin >> number;

        // 处理输入非法的情况
        if (cin.fail()) {
            cout << "\n输入格式错误！" << endl;
            cin.clear();
            // 在读入时忽略掉输入流中的前 10000 个字符，直到遇到换行符为止。
            // 这个函数通常用于在读入之后清除输入缓存，以便在读入失败的情况下继续执行程序。
            // 例如，如果在读入整数时读入了一个字符串，则可能会导致该字符串仍然在缓存中，
            // 并且后续读入操作仍然会失败。在这种情况下，
            // 使用 cin.ignore 函数来清除缓存中的数据是很有用的。
            cin.ignore(10000, '\n');
        }

        GameStatus status = position.doMove(number);
        position.moveList.push_back(number);
        status = position.checkIfGameIsOver(status);

        if (status != GameStatus::ok) {
            position.showGameStatus(status);
        }

        if (position.result != GameResult::none) {
            break;
        }

        cout << endl
             << endl;

        position.print();
    }

    cout << "恭喜完成游戏！" << endl;

    system("pause");
    return 0;
}
