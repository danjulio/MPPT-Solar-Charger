/*
 * led.h
 *
 * Header for for status LED module
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
#ifndef INC_LED_H_
#define INC_LED_H_

#include "SI_EFM8SB1_Register_Enums.h"
#include "timer.h"

//-----------------------------------------------------------------------------
// Constants
//-----------------------------------------------------------------------------

// Main State
#define LED_ST_INIT        0
#define LED_ST_IDLE_CHG    1
#define LED_ST_LOW_BATT    2
#define LED_ST_CHARGE      3
#define LED_ST_FAULT       4


// Blinking LED State
#define LED_BLINK_OFF_ST   0
#define LED_BLINK_ON_ST    1
#define LED_BLINK_WAIT_ST  2
#define LED_PWM_UP_ST      3
#define LED_PWM_DN_ST      4


// Fault types
#define LED_FAULT_NONE     0
#define LED_FAULT_BAD_BATT 1
#define LED_FAULT_MISS_ET  2
#define LED_FAULT_TEMP_RNG 3


// Fault blink counts
#define LED_BAD_BATT_BLNKS 2
#define LED_MISS_ET_BLNKS  3
#define LED_TEMP_RNG_BLNKS 4
#define LED_UNKNOWN_BLNKS  5


// Time intervals (must fit in a uint16_t)
#define LED_EVAL_MSEC      TIMER_FAST_TICK_MS
#define LED_PO_PER_MSEC    10000
#define LED_IDLE_PER_MSEC  5000
#define LED_FAULT_PER_MSEC 5000
#define LED_BLINK_ON_MSEC  60
#define LED_BLINK_OFF_MSEC 240


// Blink PWM brightness
#define LED_BLINK_PWM      160

// Pulsed PWM brightness parameters : The maximum LED brightness is scaled, based on
// solar current, between LED_MIN_PWM and 255 for 0 - I_SOLAR_MAX.  LED_NUM_STEPS
// sets the number of increments or decrements between minimum and maximum brightness
// (number of eval cycles for each half of the pulse).  It is an 8-bit value and should
// be a power-of-2 to prevent an overflow in the interpolator logic.
#define LED_MIN_PWM        32
#define LED_NUM_STEPS      64



//-----------------------------------------------------------------------------
// API Routines
//-----------------------------------------------------------------------------
void LED_Init();
void LED_Update();


#endif /* INC_LED_H_ */
