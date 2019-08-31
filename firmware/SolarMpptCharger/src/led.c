/*
 * led.c
 *
 * Status LED Module.  Implements the following functionality:
 *   - Power off: Short blink every 10 seconds
 *   - Power on/Not charging: Short blink every 5 seconds
 *   - Charging: PWM "breathing", max brightness based on charge current
 *     PWM period ~ 24 kHz
 *   - Fault indication (series of blinks every 5 seconds) (highest->lowest priority)
 *     1. Bad Battery (2 blinks)
 *     2. Missing External Temperature Sensor (3 blinks)
 *     3. Operating Temperature range currently exceeded (4 blinks)
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
#include "led.h"
#include "charge.h"
#include "config.h"
#include "power.h"
#include "temp.h"


//-----------------------------------------------------------------------------
// Variables
//-----------------------------------------------------------------------------
uint8_t LED_blinkState;
uint8_t LED_curState;
uint8_t LED_curFault;
uint8_t LED_curPwm;
uint8_t LED_totBlinkCount;
uint8_t LED_curBlinkCount;
uint8_t LED_pulseMax;
uint16_t LED_curPeriod;
uint16_t LED_maxPeriod;
uint16_t LED_pulseValue;     // Fractional pulsed LED value - upper 8 bits is used as PWM value
uint16_t LED_pulseInc;       // Fractional increment/decrement value per step {IIIIIIII.FFFFFFFF}



//-----------------------------------------------------------------------------
// Internal Routine forward declarations
//-----------------------------------------------------------------------------
void _LED_SetNormalState(uint8_t s);
uint8_t _LED_ReturnCurNormalState();
void _LED_SetupFault(uint8_t f);
uint8_t _LED_ReturnFaultInfo();
void _LED_SetCurPwm2Led();
void _LED_SetConstIntensity(uint8_t intensity);
void _LED_SetupBlink(uint16_t blinkPeriod, uint8_t blinkCount);
void _LED_SetupPulse();
void _LED_DoBlink();
void _LED_DoPulse();



//-----------------------------------------------------------------------------
// API Routines
//-----------------------------------------------------------------------------
void LED_Init()
{
	uint8_t t;

	LED_curState = LED_ST_INIT;
	LED_curFault = LED_FAULT_NONE;
	_LED_SetConstIntensity(0);

	// Look for an initial fault condition
	t = _LED_ReturnFaultInfo();
	if (t != LED_FAULT_NONE) {
		_LED_SetupFault(t);
	} else {
		// Get normal state
		t = _LED_ReturnCurNormalState();
		_LED_SetNormalState(t);
	}

}


// Designed to be called every LED_EVAL_MSEC mSec
//   Requires 6-36 uSec, up to ~232 uSec when (re)computing pulse parameters
void LED_Update()
{
	uint8_t t;

	// Fault condition has highest priority
	t = _LED_ReturnFaultInfo();
	if (LED_curState == LED_ST_FAULT) {
		// Remain in fault state until there are no faults
		if (t == LED_FAULT_NONE) {
			t = _LED_ReturnCurNormalState();
			_LED_SetNormalState(t);
		} else {
			if (t < LED_curFault) {
				// Higher priority fault
				_LED_SetupFault(t);
			} else {
				// Continue blinking this fault code
				_LED_DoBlink();
			}
		}
	} else {
		if (t != LED_FAULT_NONE) {
			// Enter fault state
			_LED_SetupFault(t);
		} else {
			// Look for a change of normal state
			t = _LED_ReturnCurNormalState();
			if (LED_curState != t) {
				_LED_SetNormalState(t);
			} else {
				// Just update the LED
				if (LED_curState == LED_ST_CHARGE) {
					_LED_DoPulse();
				} else {
					_LED_DoBlink();
				}
			}
		}
	}
}



//-----------------------------------------------------------------------------
// Internal Routines
//-----------------------------------------------------------------------------
void _LED_SetNormalState(uint8_t s)
{
	LED_curFault = LED_FAULT_NONE;

	if (s != LED_curState) {
		LED_curState = s;

		switch(s) {
		case LED_ST_IDLE_CHG:
			// Setup blink
			_LED_SetupBlink(LED_IDLE_PER_MSEC, 1);
			break;
		case LED_ST_LOW_BATT:
			// Setup blink
			_LED_SetupBlink(LED_PO_PER_MSEC, 1);
			break;
		case LED_ST_CHARGE:
			// Setup pulse
			_LED_SetupPulse();
			break;
		}

		// Update LED
		_LED_SetCurPwm2Led();
	}
}


uint8_t _LED_ReturnCurNormalState()
{
	// Priority detection of normal operating states
	if (POWER_LowBattDisabled()) {
		return LED_ST_LOW_BATT;
	} else if ((CHARGE_GetState() == CHG_ST_NIGHT) || (CHARGE_GetState() == CHG_ST_IDLE)) {
		return LED_ST_IDLE_CHG;
	} else {
		return LED_ST_CHARGE;
	}
}


void _LED_SetupFault(uint8_t f)
{
	uint8_t t;

	LED_curState = LED_ST_FAULT;

	if (f != LED_curFault) {
		// Setup blink
		LED_curFault = f;

		switch(f) {
		case LED_FAULT_BAD_BATT:
			t = LED_BAD_BATT_BLNKS;
			break;
		case LED_FAULT_MISS_ET:
			t = LED_MISS_ET_BLNKS;
			break;
		case LED_FAULT_TEMP_RNG:
			t = LED_TEMP_RNG_BLNKS;
			break;
		default:
			t = LED_UNKNOWN_BLNKS;
		}
		_LED_SetupBlink(LED_FAULT_PER_MSEC, t);

		// Update LED
		_LED_SetCurPwm2Led();
	}
}


uint8_t _LED_ReturnFaultInfo()
{
	// Check for fault conditions in priority order
	if (POWER_badBattery()) {
		return LED_FAULT_BAD_BATT;
	} else if (TEMP_ExtSensorMissing()) {
		return LED_FAULT_MISS_ET;
	} else if (CHARGE_IsTempLimited()) {
		return LED_FAULT_TEMP_RNG;
	} else {
		return LED_FAULT_NONE;
	}
}


void _LED_SetCurPwm2Led()
{
	uint16_t pwmVal;

	// Convert 8-bit LED_curPwm to 10-bit PWM count
	pwmVal = 1023 - (LED_curPwm << 2);

	// Load the PWM value
	PCA0CPL1 = pwmVal & 0xFF;
	PCA0CPH1 = pwmVal >> 8;

	// ECOM0 must be cleared if value is 0 to completely disable PWM
	if (LED_curPwm == 0) {
		PCA0CPM1 &= ~0x40;         // Clear ECOM0 to disable output
	} else {
		if ((PCA0CPM1 & 0x40) == 0x00) {
			PCA0CPM1 |= 0x40;          // Set ECOM0 when output is not 0
		}
	}
}


void _LED_SetConstIntensity(uint8_t intensity)
{
	LED_curPwm = intensity;
	_LED_SetCurPwm2Led();
}


void _LED_SetupBlink(uint16_t blinkPeriod, uint8_t blinkCount)
{
	LED_curPwm = 0;
	LED_totBlinkCount = blinkCount;
	LED_curBlinkCount = 0;
	LED_maxPeriod = blinkPeriod/LED_EVAL_MSEC;
	LED_curPeriod = 0;
	LED_blinkState = LED_BLINK_OFF_ST;
}


void _LED_SetupPulse()
{
	uint32_t t;

	// Scale solar current between 0 and I_SOLAR_MAX to LED_MIN_PWM to 255
	t = (255 - LED_MIN_PWM) * (uint32_t) CHARGE_GetIsMa();
	t = t / I_SOLAR_MAX;
	t = t + LED_MIN_PWM;

    // Maximum pulse brightness
	if (t > 255) {
		LED_pulseMax = 255;
	} else {
		LED_pulseMax = t & 0xFF;
	}

	// Compute fractional increment value based on number of steps
	LED_pulseInc = (uint16_t) (LED_pulseMax << 8) / LED_NUM_STEPS;

	// Start with LED off
	LED_curPwm = 0;
	LED_pulseValue = 0;
	LED_blinkState = LED_PWM_UP_ST;
}


void _LED_DoBlink()
{
	switch (LED_blinkState) {
	case LED_BLINK_OFF_ST:
		if (++LED_curPeriod == (LED_BLINK_OFF_MSEC/LED_EVAL_MSEC)) {
			// Change to ON part of blink
			LED_curPeriod = 0;
			LED_blinkState = LED_BLINK_ON_ST;
			_LED_SetConstIntensity(LED_BLINK_PWM);
		}
		break;

	case LED_BLINK_ON_ST:
		if (++LED_curPeriod == (LED_BLINK_ON_MSEC/LED_EVAL_MSEC)) {
			LED_curPeriod = 0;
			_LED_SetConstIntensity(0);

			// See if we have more blinks or are in the period between sets of blinks
			if (++LED_curBlinkCount == LED_totBlinkCount) {
				LED_blinkState = LED_BLINK_WAIT_ST;
			} else {
				LED_blinkState = LED_BLINK_OFF_ST;
			}
		}
		break;

	case LED_BLINK_WAIT_ST:
		if (++LED_curPeriod == LED_maxPeriod) {
			// Done with the wait period between sets of blinks, start a blink
			LED_curPeriod = 0;
			LED_blinkState = LED_BLINK_OFF_ST;
			LED_curBlinkCount = 0;
		}
		break;

	default:
		// Should never get here so setup something sane(ish)
		LED_blinkState = LED_BLINK_OFF_ST;
		LED_curPeriod = 0;
		LED_curBlinkCount = 0;
		_LED_SetConstIntensity(0);
	}
}


void _LED_DoPulse()
{
	switch (LED_blinkState) {
	case LED_PWM_UP_ST:
		LED_pulseValue += LED_pulseInc;
		LED_curPwm = LED_pulseValue >> 8;
		if (LED_curPwm >= LED_pulseMax) {
			LED_curPwm = LED_pulseMax;
			LED_blinkState = LED_PWM_DN_ST;
		}
		break;

	case LED_PWM_DN_ST:
		LED_pulseValue -= LED_pulseInc;
		LED_curPwm = LED_pulseValue >> 8;
		if (LED_pulseValue == 0) {
			LED_blinkState = LED_PWM_UP_ST;

			// Update power level once per pulse
			_LED_SetupPulse();
		}
		break;

	default:
		_LED_SetupPulse();
	}

	_LED_SetCurPwm2Led();
}

