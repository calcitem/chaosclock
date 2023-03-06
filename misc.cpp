#include "piece.h"
#include "position.h"
#include "types.h"
#include "misc.h"

using namespace std;

int mod12(int x)
{
    if (x >= 12)
        x -= 12;
    return x;
}

void vectorCout(const std::vector<int8_t> &v,
                       const std::string &v_name = "ejso"
                                                   "on")
{
    std::ostringstream oss;
    oss << v_name << ": ";
    for (const auto &elem : v) {
        oss << (int)elem << ", ";
    }
    std::cout << oss.str() << std::endl;
}

void vectorCout(const std::vector<std::vector<int8_t>> &v,
                       const std::string &v_name = "ejsoon")
{
    std::cout << v_name << ":" << std::endl;
    for (size_t x = 0; x < v.size(); x++) {
        std::cout << x << ": ";
        for (const auto &elem : v[x]) {
            std::cout << elem << ", ";
        }
        std::cout << std::endl;
    }
}

void boardCout(const int8_t (&v)[12])
{
    std::ostringstream oss;
    for (const auto &elem : v) {
        oss << (int)elem << ", ";
    }
    std::cout << oss.str() << std::endl;
}

void vectorRemove(std::vector<int8_t> &v, int i)
{
    v.erase(std::remove(v.begin(), v.end(), i), v.end());
}

int vectorIndexOf(const std::vector<int8_t> &v, int i)
{
    auto iter = std::find(v.begin(), v.end(), i);
    if (iter == v.end())
        return -1;
    return std::distance(v.begin(), iter);
}

int vectorIndexOf(const int8_t (&v)[12], int8_t i)
{
    int vindex = find(v, v + 12, i) - v;
    if (vindex == 12)
        return -1;
    return vindex;
}

int vectorIndexOf(const int (&v)[12], int i)
{
    size_t vindex = find(v, v + 12, i) - v;
    if (vindex == 12)
        return -1;
    return vindex;
}

std::vector<int8_t> vectorMerge(const std::vector<int8_t> hand,
                                       const std::vector<int8_t> free)
{
    std::vector<int8_t> make_free(std::move(hand));
    make_free.insert(make_free.end(), std::make_move_iterator(free.begin()),
                     std::make_move_iterator(free.end()));
    return make_free;
}
