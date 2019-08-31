/*
 * timer.h
 *
 * Header file for main code evaluation timer module.
 *
 */

#ifndef INC_TIMER_H_
#define INC_TIMER_H_

#include "SI_EFM8SB1_Register_Enums.h"


//-----------------------------------------------------------------------------
// Constants
//-----------------------------------------------------------------------------
// Fast and slow ticks should be a multiple of TIMER_CORE_TICK_MS (hardware timer rate)
#define TIMER_CORE_TICK_MS  10
#define TIMER_FAST_TICK_MS  10
#define TIMER_SLOW_TICK_MS  250



//-----------------------------------------------------------------------------
// Externs
//-----------------------------------------------------------------------------
extern bool TIMER_fastTick;
extern bool TIMER_slowTick;



//-----------------------------------------------------------------------------
// API Routines
//-----------------------------------------------------------------------------
void TIMER_Init();
void TIMER_Update();

#define TIMER_FastTick() TIMER_fastTick
#define TIMER_SlowTick() TIMER_slowTick


#endif /* INC_TIMER_H_ */
