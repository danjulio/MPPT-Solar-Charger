/*
 * timer.h
 *
 * Header file for main code evaluation timer module.
 *
 * Copyright (c) 2018-2019 danjuliodesigns, LLC.  All rights reserved.
 *
 * SolarMpptCharger is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * SolarMpptCharger is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * See <http://www.gnu.org/licenses/>.
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
