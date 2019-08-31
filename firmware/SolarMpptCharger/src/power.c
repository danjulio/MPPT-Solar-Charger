/*
 * power.c
 *
 * Output power control module:
 *  1. Sense IO_PCTRL_I bit
 *   - IO_PCTRL_I = 1 => IO_PWR_EN_O enabled whenever battery good
 *   - IO_PCTRL_I = 0 => IO_PWR_EN_O only enabled at night if battery good
 *  2. Control IO_NIGHT_O bit
 *   - Based on Charge state
 *  3. Control IO_PWR_EN_O bit (external power enable)
 *   - Goes low when power-off timer expires (after IO_ALERT_N_O asserted)
 *   - Goes high when voltage crosses PowerOn threshold if turned off for low-voltage case
 *     or when IO_NIGHT_O asserted if IO_PCTRL_I = 0
 *   - Timer prevents power from being re-enabled after a low-battery shutdown and the
 *     battery recovers some (with no load) without being charged
 *  4. Control IO_ALERT_N_O bit
 *   - Goes low when timer to disable IO_PWR_EN_O started
 *   - Goes high when IO_PWR_EN_O re-asserted (stays low while IO_PWR_EN_O de-asserted
 *     to prevent sneak power path to any attached device - via the attached IO pin)
 *  5. Power down sequence
 *   - Timer triggered if low voltage triggered or IO_NIGHT_O de-asserted if IO_PCTRL_I = 0
 *     - IO_ALERT_N_O driven low when timer started
 *  6. Watchdog functionality
 *   - Cycles power when watchdog enabled and times out
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
#include "adc.h"
#include "power.h"
#include "charge.h"
#include "config.h"
#include "param.h"
#include "smbus.h"

//-----------------------------------------------------------------------------
// External IO bits
//-----------------------------------------------------------------------------
SI_SBIT(IO_ALERT_N_O, SFR_P1, 0);
SI_SBIT(IO_NIGHT_O, SFR_P0, 7);
SI_SBIT(IO_PCTRL_I, SFR_P1, 5);
SI_SBIT(IO_PWR_EN_O, SFR_P0, 6);



//-----------------------------------------------------------------------------
// Variables
//-----------------------------------------------------------------------------
bool POWER_badBatt;
bool POWER_enableAtNight;
bool POWER_isNight;
bool POWER_powerEnabled;
bool POWER_watchdogGlobalEnable;
bool POWER_watchdogCountWritten;
bool POWER_watchdogTriggered;
uint8_t POWER_state;
uint16_t POWER_offCount;
uint8_t POWER_watchdogCount;
uint16_t POWER_watchdogPwrOffTO;



//-----------------------------------------------------------------------------
// Internal Routine forward declarations
//-----------------------------------------------------------------------------
void _POWER_DisableWatchdog();



//-----------------------------------------------------------------------------
// API Routines
//-----------------------------------------------------------------------------
void POWER_Init()
{
	// Update state based on external information
	//   Use direct ADC Module values instead of CHARGE Module values because they
	//   haven't been updated yet.
	POWER_badBatt = (((uint16_t) ADC_GetValue(ADC_MEAS_VB_INDEX)) < V_BAD_BATTERY);
	POWER_enableAtNight = (IO_PCTRL_I == 0);
	POWER_isNight = (CHARGE_GetState() == CHG_ST_NIGHT);

	// Variable Init
	POWER_watchdogGlobalEnable = false;
	POWER_watchdogCountWritten = false;
	POWER_watchdogTriggered = false;
	POWER_offCount = 0;
	POWER_watchdogCount = 0;
	POWER_watchdogPwrOffTO = PWROFF_DEF_WD_TIMEOUT;

	// Initial power enable
	if (ADC_GetValue(ADC_MEAS_VB_INDEX) <= PARAM_GetPwrOffMv()) {
		// Low Battery: Startup off
		POWER_state = PWR_ST_OFF_LB;
		POWER_offCount = PWROFF_LB_CHG_TIMEOUT;
		POWER_powerEnabled = false;
	}
	else if (POWER_enableAtNight && !POWER_isNight) {
		// Battery OK but not night when we are configured to enable at night
		POWER_state = PWR_ST_OFF_DAY;
		POWER_powerEnabled = false;
	}
	else {
		// Turn on
		POWER_state = PWR_ST_ON;
		POWER_powerEnabled = true;
	}

	// Output Init
	IO_NIGHT_O = POWER_isNight;
	IO_PWR_EN_O = POWER_powerEnabled ? 1 : 0;
	IO_ALERT_N_O = POWER_powerEnabled ? 1 : 0;
}


void POWER_Update()
{
	bool watchdogTrigger = false;
	bool watchdogEnabled = POWER_WatchdogRunning();

	// Get external conditions
	POWER_badBatt = (CHARGE_GetVbMv() < V_BAD_BATTERY);
	POWER_enableAtNight = (IO_PCTRL_I == 0);
	POWER_isNight = (CHARGE_GetState() == CHG_ST_NIGHT);

	// Evaluate our state
	switch (POWER_state) {
	case PWR_ST_OFF_LB:
		// Evaluate minimum charge timer (to help try prevent a "bounced" battery from restarting
		// before it has had a change to charge some)
		if (POWER_offCount != 0) {
			if ((CHARGE_GetState() == CHG_ST_NIGHT) || (CHARGE_GetState() == CHG_ST_IDLE)) {
				// Hold counter while not charging
				POWER_offCount = PWROFF_LB_CHG_TIMEOUT;
			} else {
				// Decrement charger while charging
				POWER_offCount--;
			}
		}

		// Check for restart conditions
		if ((CHARGE_GetVbMv() >= PARAM_GetPwrOnMv()) && (POWER_offCount == 0)) {
			if (POWER_enableAtNight) {
				// Move to a state that can check if it's still day or not
				POWER_state = PWR_ST_OFF_DAY;
			}
			else {
				// Can restart immediately
				POWER_state = PWR_ST_ON;
			}
		}
		break;

	case PWR_ST_OFF_DAY:
		if (CHARGE_GetVbMv() <= PARAM_GetPwrOffMv()) {
			// Move immediately to battery discharged off state
			POWER_state = PWR_ST_OFF_LB;
			POWER_offCount = PWROFF_LB_CHG_TIMEOUT;
		}
		else if ((CHARGE_GetVbMv() >= (PARAM_GetPwrOffMv() + PWR_LB_HYST_MV)) && POWER_isNight) {
			// Battery is OK enough to turn on if we were off for day
			POWER_state = PWR_ST_ON;
		}
		break;

	case PWR_ST_ALERT_LB:
		if (--POWER_offCount == 0) {
			POWER_state = PWR_ST_OFF_LB;
			POWER_offCount = PWROFF_LB_CHG_TIMEOUT;
		}
		break;

	case PWR_ST_ALERT_DAY:
		if (--POWER_offCount == 0) {
			POWER_state = PWR_ST_OFF_DAY;
		}
		break;

	case PWR_ST_ON:
		// Evaluate the watchdog if it is running
		watchdogTrigger = false;
		if (watchdogEnabled) {
			if (POWER_watchdogCount != 0) {
				if (--POWER_watchdogCount == 0) {
					watchdogTrigger = true;
					POWER_watchdogTriggered = true;
					SMB_SetStatusBit(SMB_ST_PWD_TRIG_MASK, true);
				}
			}
		}

		// Start watchdog power reset if watchdog was triggered
		if (watchdogEnabled && watchdogTrigger) {
			POWER_state = PWR_ST_WD_ALERT;
			POWER_offCount = PWROFF_WARN_TIMEOUT;
		}
		// Otherwise start to turn off if battery voltage too low for longer than LOWPWR_TIMEOUT
		else if (CHARGE_GetVbMv() <= PARAM_GetPwrOffMv()) {
			if (--POWER_offCount == 0) {
				POWER_state = PWR_ST_ALERT_LB;
				POWER_offCount = PWROFF_WARN_TIMEOUT;
			}
		}
		// ...or if we're enabled for night operation and it isn't night
		else if ((POWER_enableAtNight && !POWER_isNight)) {
			POWER_state = PWR_ST_ALERT_DAY;
			POWER_offCount = PWROFF_WARN_TIMEOUT;
		} else {
			// Hold low-battery shutdown trigger timer in reset
			POWER_offCount = LOWPWR_TIMEOUT;
		}
		break;

	case PWR_ST_WD_ALERT:
		if (watchdogEnabled) {
			if (--POWER_offCount == 0) {
				POWER_state = PWR_ST_WD_OFF;
				POWER_offCount = POWER_watchdogPwrOffTO;
			}
		} else {
			// Safety clause in case we got to this state incorrectly
			POWER_state = PWR_ST_ON;
		}
		break;

	case PWR_ST_WD_OFF:
		if (watchdogEnabled) {
			if (--POWER_offCount == 0) {
				POWER_state = PWR_ST_ON;
				_POWER_DisableWatchdog();               // Watchdog disabled on restart
			}
		} else {
			// Safety clause in case we got to this state incorrectly
			POWER_state = PWR_ST_ON;
		}
		break;

	default:
		// Should never occur but turn on if we are above critical low battery
		if (CHARGE_GetVbMv() > PARAM_GetPwrOffMv()) {
			POWER_state = PWR_ST_ON;
		} else {
			POWER_state = PWR_ST_OFF_LB;
			POWER_offCount = PWROFF_LB_CHG_TIMEOUT;
		}
		_POWER_DisableWatchdog();
	}

	// Update our outputs
	POWER_powerEnabled = ((POWER_state == PWR_ST_OFF_LB) ||
	                      (POWER_state == PWR_ST_OFF_DAY) ||
					      (POWER_state == PWR_ST_WD_OFF)) ? 0 : 1;
	IO_PWR_EN_O = POWER_powerEnabled;
	IO_ALERT_N_O = (POWER_state == PWR_ST_ON) ? 1 : 0;
	IO_NIGHT_O = POWER_isNight;

	// Update STATUS register
	SMB_SetStatusBit(SMB_ST_BAD_BATT_MASK, POWER_badBatt);
	SMB_SetStatusBit(SMB_ST_WD_RUN_MASK, watchdogEnabled);
	SMB_SetStatusBit(SMB_ST_PWR_EN_MASK, POWER_powerEnabled);
	SMB_SetStatusBit(SMB_ST_ALERT_MASK, POWER_IsAlert());
	SMB_SetStatusBit(SMB_ST_PCTRL_MASK, POWER_enableAtNight);
	SMB_SetStatusBit(SMB_ST_NIGHT_MASK, POWER_isNight);
}


void POWER_EnableWatchdog(bool en)
{
	if (POWER_watchdogGlobalEnable) {
		if (en == false) {
			// Completely disable the watchdog
			_POWER_DisableWatchdog();
		}
	}
	POWER_watchdogGlobalEnable = en;
}


void POWER_SetWatchdogTimeout(uint8_t sec)
{
	if (sec == 0) {
		// Disable watchdog timer
		POWER_watchdogCountWritten = false;
	} else {
		// Enable timer
		POWER_watchdogCountWritten = true;
	}
	POWER_watchdogCount = sec;
}


void POWER_SetWatchdogPwrOffTO(uint16_t sec)
{
	if (sec == 0) {
		POWER_watchdogPwrOffTO = PWROFF_DEF_WD_TIMEOUT;
	} else {
		POWER_watchdogPwrOffTO = sec;
	}
}


bool POWER_WatchdogRunning()
{
	return (POWER_watchdogGlobalEnable && POWER_watchdogCountWritten);
}



//-----------------------------------------------------------------------------
// Internal Routines
//-----------------------------------------------------------------------------
void _POWER_DisableWatchdog()
{
	POWER_watchdogGlobalEnable = false;
	POWER_watchdogCountWritten = false;
	POWER_watchdogCount = 0;
	POWER_watchdogPwrOffTO = PWROFF_DEF_WD_TIMEOUT;
}
