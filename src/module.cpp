#include <algorithm>
#include <cfloat>
#include "module.h"
using namespace std;

size_t Block::_maxX = 0;
size_t Block::_maxY = 0;

double Net::calcHPWL() {
    double min_x = DBL_MAX, max_x = 0.0;
    double min_y = DBL_MAX, max_y = 0.0;
    for (const auto& terminal : _termList) {
        double center_x = (terminal->getX1() + terminal->getX2())/2.0;
        double center_y = (terminal->getY1() + terminal->getY2())/2.0;
        min_x = min(min_x, center_x);
        max_x = max(max_x, center_x);
        min_y = min(min_y, center_y);
        max_y = max(max_y, center_y);
    }
    double hpwl = (max_x-min_x) + (max_y-min_y);
    return hpwl;
}
