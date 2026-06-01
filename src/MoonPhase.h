#ifndef MOON_PHASE_H
#define MOON_PHASE_H

#include <arduino.h>

double Julian(int year, int month, double day);
double sun_position(double j);
double moon_position(double j, double ls);
uint8_t moon_phase(int year, int month, int day, double hour, int* ip);

#endif // MOON_PHASE_H
