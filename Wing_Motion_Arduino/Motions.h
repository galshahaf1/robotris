#ifndef MOTIONS_H
#define MOTIONS_H

#include "Config.h"

// Calculate target angles for the 8 servos based on current mode and time
void calculateTargets(unsigned long time);

// Move the servos towards target angles smoothly using easing
void moveServosSmoothly();

// Initialize or toggle the one-time wave direction
void triggerOnetimeWave();

#endif // MOTIONS_H
