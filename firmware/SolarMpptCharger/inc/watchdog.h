/*
 * watchdog.h
 *
 * Header file for main system watchdog timer module.
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

#ifndef INC_WATCHDOG_H_
#define INC_WATCHDOG_H_

//-----------------------------------------------------------------------------
// Constants
//-----------------------------------------------------------------------------

//
// Set the (maximum) period of the watchdog reset with the value loaded into
// PCA0CPL2.  This value is the number of times PCA0L overflows before a
// WDT is detected.  PCA0L is incremented with SYSCLK (24.5 MHz) so each
// count in PCA0CPL2 is about 10.449 uSec.  The watchdog period may vary
// some depending on where PCA0L is when the watchdog is reloaded.  The
// formula from the reference manual is
//
//   Period (in PCA clocks) = (256 x PCA0CPL2) + (256 - PCA0L)
//
// We choose a value of around 2.5 mSec (2.51 - 2.52 mSec) because this is
// longer than the maximum measured time through the watchdog protected main
// loop but still very short in real-time.
//
#define WD_PCA0CPL2_RELOAD 240



//-----------------------------------------------------------------------------
// Externs
//-----------------------------------------------------------------------------
extern bool WD_Detected;



//-----------------------------------------------------------------------------
// API Routines
//-----------------------------------------------------------------------------
void WD_Disable();
void WD_Init();
void WD_Reset();

#define WD_WatchdogWasTriggered()   WD_Detected
#define WD_ClearWatchdogTriggered() WD_Detected = false

#endif /* INC_WATCHDOG_H_ */
