#pragma once
#include <iostream>

struct point{
    int x;
    int y;
};

namespace std {

ostream & operator<<(ostream & os, point const & p)
{
    os << "(" << p.x << ", " << p.y << ")";
    return os;
}
}
