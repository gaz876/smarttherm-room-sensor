#include "HeatingLimits.hpp"
#include <cassert>
int main(){HeatingLimits l{40,82};float o=0;assert(l.bound(39,o)&&o==40);assert(l.bound(83,o)&&o==82);assert(l.bound(40,o)&&o==40);assert(l.bound(82,o)&&o==82);assert(!l.bound(NAN,o));assert(!l.bound(INFINITY,o));assert((!HeatingLimits{39,82}.valid()));assert((!HeatingLimits{40,83}.valid()));assert((!HeatingLimits{60,50}.valid()));assert((HeatingLimits{40,65}.bound(80,o)&&o==65));}
