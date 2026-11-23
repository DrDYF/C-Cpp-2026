#pragma once
#include "core/Types.h"
#include <vector>

namespace FourThreeFixtures {
struct Case {
    const char* name;
    std::vector<Point> prefix;
    Point losing, attack, block, fork;
    std::vector<Point> ends;
    int attackRank;
};

inline const Case Cases[]={
    {"28-22",
     {{7,7},{11,7},{4,10},{8,5},{7,8},{8,6},{7,10},{7,6},{5,10},{6,10},{5,9},
      {9,6},{6,8},{3,11},{7,9},{7,11},{10,6},{10,7},{7,4},{9,7},{8,7}},
     {9,4},{6,9},{4,11},{4,9},{{3,9},{8,9}},14},
    {"2-17",
     {{7,7},{10,10},{10,3},{8,6},{11,9},{9,10},{11,10},{9,8},{9,7},{8,9},
      {11,8},{11,7},{10,11},{7,10},{10,7},{8,11}},
     {12,9},{8,10},{6,10},{8,8},{{8,7},{8,12}},13},
};
}
