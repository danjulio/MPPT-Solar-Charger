/*
 * watchdog.c
 *
 * Main system watchdog timer control module. WD_Reset() must be called more
 * frequently than the minimum watchdog timeout or the module will reset the
 * processor.  Make a wathdog detected status available to the SMBus module.
 *
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

#include <SI_EFM8SB1_Register_Enums.h>
#include "watchdog.h"
#include "smbus.h"

//-----------------------------------------------------------------------------
// Variables
//-----------------------------------------------------------------------------
bool WD_Detected;



//-----------------------------------------------------------------------------
// API routines
//-----------------------------------------------------------------------------
void WD_Disable()
{
	// Disable the watchdog
	PCA0MD &= ~PCA0MD_WDTE__BMASK;
}


void WD_Init()
{
	// Detect if we have been reset because of a watchdog timeout
	WD_Detected = ((RSTSRC & RSTSRC_WDTRSF__BMASK) == RSTSRC_WDTRSF__SET);

	// Configure our watchdog timeout
	PCA0CPL2 = WD_PCA0CPL2_RELOAD;

	// Enable and lock-enabled the watchdog timer
	PCA0MD |= PCA0MD_WDLCK__BMASK;

	// Reset the watchdog
	PCA0CPH2 = 0;
}


void WD_Reset()
{
	// Reset the timeout value
	PCA0CPL2 = WD_PCA0CPL2_RELOAD;

	// Reset the timer itself
	PCA0CPH2 = 0;
}
