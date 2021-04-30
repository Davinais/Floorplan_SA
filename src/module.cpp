#include <algorithm>
#include <cstdint>
#include "module.h"
using namespace std;

size_t Block::_maxX = 0;
size_t Block::_maxY = 0;

double Net::calcHPWL() {
    size_t min_x = SIZE_MAX, max_x = 0;
    size_t min_y = SIZE_MAX, max_y = 0;
    for (const auto& terminal : _termList) {
        min_x = min(min_x, terminal->getX1());
        max_x = max(max_x, terminal->getX2());
        min_y = min(min_y, terminal->getY1());
        max_y = max(max_y, terminal->getY2());
    }
    double hpwl = (double)((max_x-min_x) + (max_y-min_y));
    return hpwl;
}
