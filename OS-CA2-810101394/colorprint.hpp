#include <iostream>

using namespace std;

enum class Color {
    BLK = 30,
    RED = 31,
    GRN = 32,
    YEL = 33,
    BLU = 34,
    MAG = 35,
    CYN = 36,
    WHT = 37,
    DEF = 39,
    RST = 0,
};

inline std::ostream& operator<<(std::ostream& os, Color clr) {
    return os << "\x1B[" << static_cast<int>(clr) << 'm';
}
