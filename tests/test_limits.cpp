#include "HeatingLimits.hpp"
#include <cassert>
int main(){HeatingLimits l{40,80};float o=0;assert(l.bound(39,o)&&o==40);assert(l.bound(81,o)&&o==80);assert(l.bound(40,o)&&o==40);assert(l.bound(80,o)&&o==80);assert(!l.bound(NAN,o));assert(!l.bound(INFINITY,o));assert((!HeatingLimits{39,80}.valid()));assert((!HeatingLimits{40,81}.valid()));assert((!HeatingLimits{60,50}.valid()));assert((HeatingLimits{40,65}.bound(80,o)&&o==65));}
