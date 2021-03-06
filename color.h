#ifndef _COLOR_H
#define _COLOR_H

#include "math4.h"

#define rgb(v) xyz(v)
#define rgba(v) xyzw(v)
#define COLOR_FORMAT "r=%.1f g=%.1f b=%.1f a=%.1f"

extern const vec4 COLOR_RED;
extern const vec4 COLOR_GREEN;
extern const vec4 COLOR_BLUE;
extern const vec4 COLOR_WHITE;
extern const vec4 COLOR_BLACK;
extern const vec4 COLOR_YELLOW;
extern const vec4 COLOR_CYAN;
extern const vec4 COLOR_MAGENTA;

#endif
