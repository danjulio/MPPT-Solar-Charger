/*
 * timer.c
 *
 * Main code evaluation timer.  Generates fast and low tick values to cause
 * activity by the main evaluation thread.
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
#include "timer.h"

//-----------------------------------------------------------------------------
// Variables
//-----------------------------------------------------------------------------
volatile bool TIMER_IsrTick;
bool TIMER_fastTick;
bool TIMER_slowTick;
uint8_t TIMER_FastCount;
uint8_t TIMER_SlowCount;



//-----------------------------------------------------------------------------
// API Routines
//-----------------------------------------------------------------------------
void TIMER_Init()
{
	TIMER_IsrTick = false;
	TIMER_FastCount = 0;
	TIMER_SlowCount = 0;
}


// Designed to be called once per system evaluation interval
void TIMER_Update()
{
	// Flags set as necessary for this evaluation interval
	TIMER_fastTick = false;
	TIMER_slowTick = false;

	if (TIMER_IsrTick) {
		TIMER_IsrTick = false;

		if (++TIMER_FastCount == (TIMER_FAST_TICK_MS/TIMER_CORE_TICK_MS)) {
			TIMER_FastCount = 0;
			TIMER_fastTick = true;
		}

		if (++TIMER_SlowCount == (TIMER_SLOW_TICK_MS/TIMER_CORE_TICK_MS)) {
			TIMER_SlowCount = 0;
			TIMER_slowTick = true;
		}
	}
}



//-----------------------------------------------------------------------------
// TIMER2 ISR
//-----------------------------------------------------------------------------
SI_INTERRUPT (TIMER2_ISR, TIMER2_IRQn)
{
    // Overflows every 10 ms
    TMR2CN0 &= ~TMR2CN0_TF2H__BMASK;

    TIMER_IsrTick = true;
}
