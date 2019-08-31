/*
 * charge.c
 *
 * Charge control logic:
 *   1. Implement MPPT control
 *     - Occasional MPPT Scan from near battery voltage to solar panel voltage to find current Vmpp
 *     - MPPT P&O algorithm with dynamic step size and partially shaded panel recovery capabilities
 *   2. Implement Charge control
 *     - Night/Day detection
 *     - Charge state management: BULK, ABSORPTION, FLOAT
 *     - Temperature compensation and over/under-temperature charge suspension management
 *     - Bulk/Absorption over-charge timer
 *     - Battery disconnect handling
 *     - Update SMBus registers with charge related values
 *
 */
#include "charge.h"
#include "adc.h"
#include "buck.h"
#include "config.h"
#include "param.h"
#include "power.h"
#include "smbus.h"
#include "temp.h"


//-----------------------------------------------------------------------------
// Variables
//-----------------------------------------------------------------------------

//
// Charge Management variables
//
bool chargeMpptEnable;
bool chargeMpptScanEnable;
bool chargeTempLimited;
uint8_t chargeState;
uint8_t chargeLowProdCount;
uint8_t chargeAbsTermCount;
uint16_t chargeSolarRegMv;
uint16_t chargeMpptStepMv;
uint16_t chargeHighCount;
uint16_t chargeTimeoutCount;
uint16_t chargeCompThreshMv;
uint16_t chargeScanEndMv;
uint16_t chargeMaxPower;
uint16_t chargeMaxScanVSetpoint;

//
// Current measurement values
//
uint16_t v_s_mv;
uint16_t i_s_ma;
uint16_t v_b_mv;
uint16_t i_b_ma;
int16_t i_c_ma;        // Battery charge current estimate
uint16_t solarPowerMw;

//
// Code-optimizing temporary used by _CHARGE_ComputePower and _CHARGE_ComputeChargeCurrent
//
uint32_t solarPowerUw;



//-----------------------------------------------------------------------------
// Internal Routine forward declarations
//-----------------------------------------------------------------------------
void _CHARGE_SetState(uint8_t newSt);
uint16_t _CHARGE_ComputePower();
int16_t _CHARGE_ComputeChargeCurrent();
void _CHARGE_IncMpptRegVoltage();
void _CHARGE_DecMpptRegVoltage();
void _CHARGE_SetRegulate(bool en);
void _CHARGE_AdjustCompBattV();
void _CHARGE_StartScan(uint16_t lowMv, uint16_t highMv);


//-----------------------------------------------------------------------------
// API Routines
//-----------------------------------------------------------------------------
void CHARGE_Init() {
	chargeMpptEnable = false;
	chargeMpptScanEnable = false;
	chargeTempLimited = false;
	chargeSolarRegMv = V_MIN_GOOD_SOLAR;
	chargeMpptStepMv = MPPT_LO_I_STEP_MV;
	chargeLowProdCount = 0;
	chargeAbsTermCount = 0;
	chargeHighCount = 0;
	chargeTimeoutCount = 0;
	chargeCompThreshMv = PARAM_GetFloatMv();

	// Select entry state based on initial solar voltage
	if (((uint16_t) ADC_GetValue(ADC_MEAS_VS_INDEX)) < (V_NIGHT_THRESH + V_DELTA_CHANGE)) {
		chargeState = CHG_ST_NIGHT;
	} else {
		chargeState = CHG_ST_IDLE;
	}
}


// CHARGE_MpptUpdate should be called prior to CHARGE_StateUpdate
void CHARGE_MpptUpdate() {
	static uint16_t lastPowerMw = 0;
	static uint16_t lastSolarMv= 0;
	bool deltaVpos;

	// Update current operating measurement values for use during this evaluation
	v_s_mv = (uint16_t) ADC_GetValue(ADC_MEAS_VS_INDEX);
	i_s_ma = (uint16_t) ADC_GetValue(ADC_MEAS_IS_INDEX);
	v_b_mv = (uint16_t) ADC_GetValue(ADC_MEAS_VB_INDEX);
	i_b_ma = (uint16_t) ADC_GetValue(ADC_MEAS_IB_INDEX);
	solarPowerMw = _CHARGE_ComputePower();  // Call before _CHARGE_ComputeChargeCurrent
	i_c_ma = _CHARGE_ComputeChargeCurrent();

	// Execute MPPT Scan algorithm if enabled
	if (chargeMpptScanEnable) {
		// Update maxima
		if (solarPowerMw > chargeMaxPower) {
			chargeMaxPower = solarPowerMw;
			chargeMaxScanVSetpoint = v_s_mv;
		}

		// Update for next evaluation
		chargeSolarRegMv -= MPPT_SCAN_STEP_MV;
		BUCK_SetSolarVoltage(chargeSolarRegMv);

		// Check for completion
		if (chargeSolarRegMv < chargeScanEndMv) {
			chargeMpptScanEnable = false;
		}
	}

	// Execute MPPT PO algorithm if enabled and regulator isn't limiting (since
	// it would cause calculations to be incorrect)
	else if (chargeMpptEnable) {
		if (BUCK_IsLimiting()) {
			// Let chargeSolarRegMv follow the solar panel output as a starting
			// point for when the algorithm starts executing again
			chargeSolarRegMv = v_s_mv;
		} else {
			// Compute current step voltage
			if (i_s_ma > MPPT_HI_I_STEP_MA) {
				chargeMpptStepMv = MPPT_HI_I_STEP_MV;
			} else if (i_s_ma > MPPT_LO_I_STEP_MA) {
				chargeMpptStepMv = MPPT_MID_I_STEP_MV;
			} else {
				chargeMpptStepMv = MPPT_LO_I_STEP_MV;
			}

			// Look for special case where MPPT algorithm has wandered upward causing the BUCK
			// to stop operating.  This seems to happen when a panel is partially obscured and
			// the light level changes rapidly.  Force a condition that decreases the regulation
			// voltage until the BUCK restarts.
			if ((chargeSolarRegMv >= V_MAX_SOLARV) && (BUCK_GetPwm() == 0)) {
				chargeSolarRegMv = v_s_mv - chargeMpptStepMv;
				if (chargeSolarRegMv < V_MIN_SOLARV) chargeSolarRegMv = V_MIN_SOLARV;
			} else {
				// Compute sign of deltaV
				deltaVpos = v_s_mv >= lastSolarMv;

				// Evaluate Perturb and Observe Algorithm
				if (solarPowerMw > lastPowerMw) {
					// [Ideally] Climbing the curve toward MPP
					//   - solar V delta pos -> solar voltage increased and power increased so more
					//     power must be at higher voltage
					//   - solar V delta neg -> solar voltage decreased and power increased so more
					//     power must be at lower voltage
					if (deltaVpos) {
						_CHARGE_IncMpptRegVoltage();
					} else {
						_CHARGE_DecMpptRegVoltage();
					}
				} else {
					// [Ideally] Descending the curve past MPP
					//   - solar V delta pos -> solar voltage increased but power went down so more
					//     power must be at lower voltage
					//   - solar V delta neg -> solar voltage decreased but power went down so more
					//     power must be at higher voltage
					if (deltaVpos) {
						_CHARGE_DecMpptRegVoltage();
					} else {
						_CHARGE_IncMpptRegVoltage();
					}
				}
			}

			// Update BUCK regulation voltage
			BUCK_SetSolarVoltage(chargeSolarRegMv);
		}
	}

	// Store previous values
	lastPowerMw = solarPowerMw;
	lastSolarMv = v_s_mv;

	// Update SMBus registers
	SMB_SetIndexedValue(SMB_INDEX_VS, v_s_mv);
	SMB_SetIndexedValue(SMB_INDEX_IS, i_s_ma);
	SMB_SetIndexedValue(SMB_INDEX_VB, v_b_mv);
	SMB_SetIndexedValue(SMB_INDEX_IB, i_b_ma);
	SMB_SetIndexedValue(SMB_INDEX_IC, i_c_ma);
	SMB_SetIndexedValue(SMB_INDEX_VM, chargeSolarRegMv);
}


void CHARGE_StateUpdate() {
	static uint8_t scanExitChargeState;  // Used to restore state after MPPT scan
	int16_t t_c10;

	// Evaluate specific charge-state items
	if ((chargeState == CHG_ST_BULK) || (chargeState == CHG_ST_ABS)) {
		// Set temperature compensated charge voltage for bulk/abs
		chargeCompThreshMv = TEMP_GetCompBulkMv();

		// Evaluate Bulk/Absorption over-charge timer
		if (v_b_mv > (TEMP_GetCompFloatMv() + V_DELTA_CHANGE)) {
			if (++chargeHighCount >= HIGH_CHARGE_TIMEOUT) {
				_CHARGE_SetState(CHG_ST_FLT);
			}
		} else {
			chargeHighCount = 0;
		}
	} else {
		// Set temperature compensated charge voltage for float (default)
		chargeCompThreshMv = TEMP_GetCompFloatMv();
	}

	// Evaluate temperature limits
	t_c10 = TEMP_GetCurTempC10();
	if (chargeTempLimited) {
		// Look for temperature to return to within charging limits
		if ((t_c10 < ((TEMP_LIMIT_HIGH - TEMP_LIMIT_HYST)*10)) && (t_c10 > (TEMP_LIMIT_LOW + TEMP_LIMIT_HYST)*10)) {
			chargeTempLimited = false;
		}
	} else {
		// Look for temperature outside of charging limits
		if ((t_c10 > (TEMP_LIMIT_HIGH*10)) || (t_c10 < (TEMP_LIMIT_LOW*10))) {
			chargeTempLimited = true;
			if (!((chargeState == CHG_ST_NIGHT) || (chargeState == CHG_ST_IDLE))) {
				_CHARGE_SetState(CHG_ST_IDLE);
			}
		}
	}

	// Evaluate various exits (in priority order) from one charge state to another state
	if ((chargeState == CHG_ST_BULK) || (chargeState == CHG_ST_ABS) || (chargeState == CHG_ST_FLT)) {
		// Immediately transition to CHG_ST_IDLE for a disconnected battery which will switch off the buck to
		// kill power to the system (which may not be able to be regulated anymore depending on the load).
		if (v_b_mv < V_BAD_BATTERY) {
			_CHARGE_SetState(CHG_ST_IDLE);
		}

		// Evaluate periodic scan timer
		else if (++chargeTimeoutCount == MPPT_SCAN_TIMEOUT) {
			// Entering scan isn't necessary if we are limiting since we're already getting enough
			// power.  Plus we don't want to create a condition where the converter supplies so
			// much power during the scan that the battery voltage rises significantly above
			// the current charge threshold.
			if (!BUCK_IsLimiting()) {
				scanExitChargeState = chargeState;    // Save current state to restore after scan
				_CHARGE_SetState(CHG_ST_VSRCV);       // Turn off charging and start a short idle period to let VS recover to OC voltage
			} else {
				// Restart the timer
				chargeTimeoutCount = 0;
			}
		}

		// Detect low power production after an interval qualified by making sure the buck
		// converter isn't limiting due to a charged battery (and therefore consuming no power).
		// The interval allows us to weather short declines in solar voltage (e.g. passing
		// clouds) without unnecessarily going back through idle.
		else if ((solarPowerMw < P_MIN_THRESH) && !BUCK_IsLimiting()) {
			if (++chargeLowProdCount >= LOW_PROD_TIMEOUT) {
				_CHARGE_SetState(CHG_ST_IDLE);
			}
		} else {
			// Reset low solar production timer
			chargeLowProdCount = 0;
		}
	}

	// Evaluate charge state machine
	switch (chargeState) {
	case CHG_ST_NIGHT:
		if (v_s_mv > (V_NIGHT_THRESH + V_DELTA_CHANGE)) {
			// Light above night threshold.  Evaluate transition to IDLE based on PCTRL input:
			//   1. PCTRL set (power only at night): Wait WAKE_TIMEOUT to prevent temporary panel illumination
			//      from turning off power.
			//   2. PCTRL clear (power always): Can move to IDLE immediately
			if (!POWER_PctrlSet() || (++chargeTimeoutCount == WAKE_TIMEOUT)) {
				_CHARGE_SetState(CHG_ST_IDLE);
			}
		} else {
			// Hold timer in reset while voltage below night threshold
			chargeTimeoutCount = 0;
		}
		break;

	case CHG_ST_IDLE:
		if (v_s_mv < (V_NIGHT_THRESH - V_DELTA_CHANGE)) {
			if (++chargeTimeoutCount == NIGHT_TIMEOUT) {
				_CHARGE_SetState(CHG_ST_NIGHT);
			}
		} else {
			chargeTimeoutCount = 0; // Reset night detection

			// See if we can enter a charging state
			if ((v_s_mv > V_MIN_GOOD_SOLAR) && (v_b_mv >= V_BAD_BATTERY) && !chargeTempLimited) {
				_CHARGE_SetState(CHG_ST_SCAN);
				if (v_b_mv < V_MIN_IDLE_2_FLT) {
					scanExitChargeState = CHG_ST_BULK;
				} else {
					scanExitChargeState = CHG_ST_FLT;
				}
			}
		}
		break;

    case CHG_ST_VSRCV: // Allow VS to recover after charging before starting a scan
    	if (++chargeTimeoutCount == CHG_RCVR_PERIOD) {
    		_CHARGE_SetState(CHG_ST_SCAN);
    	}
    	break;

    case CHG_ST_SCAN:
    	if (!chargeMpptScanEnable) {
    		// Scan done, restart charging
       		BUCK_EnableRegulate(false);                   // Disable BUCK to force re-initialization at start of charge
    		chargeSolarRegMv = chargeMaxScanVSetpoint;    // Set voltage for maximum power
    		if (scanExitChargeState != CHG_ST_FLT) {
    			// During scan, the compensation logic reset the threshold voltage to FLOAT
    			// so we reset it here to BULK/ABSORPTION if necessary
    			chargeCompThreshMv = TEMP_GetCompBulkMv();
    		}
    		_CHARGE_SetState(scanExitChargeState);
    	}
    	break;

	case CHG_ST_BULK:
		_CHARGE_AdjustCompBattV();

		// Look for termination condition - move to Absorption charge when battery voltage rises to Bulk limit
		if (v_b_mv >= chargeCompThreshMv) {
			_CHARGE_SetState(CHG_ST_ABS);
		}
		break;

	case CHG_ST_ABS:
		_CHARGE_AdjustCompBattV();

		// Look for termination condition - only terminate if current falls off while voltage is at
		// absorption setpoint.  Timer prevents transient conditions (such as restarting after scan)
		// from prematurely triggering exit.
		if ((i_c_ma < I_ABS_CUTOFF) && BUCK_IsLimit2()) {
			if (++chargeAbsTermCount == ABS_TERM_TIMEOUT) {
				_CHARGE_SetState(CHG_ST_FLT);
			}
		} else {
			// Hold timer in reset
			chargeAbsTermCount = 0;
		}
		break;

	case CHG_ST_FLT:  // Maintain float voltage until conditions evaluated above move us to IDLE
		_CHARGE_AdjustCompBattV();
		break;

	default:
		_CHARGE_SetState(CHG_ST_IDLE);
		break;
	}

	// Update SMBus registers
	SMB_SetStatusBit(SMB_ST_T_LIM_MASK, chargeTempLimited);
	SMB_SetIndexedValue(SMB_INDEX_TH, chargeCompThreshMv);
}



//-----------------------------------------------------------------------------
// Internal Routines
//-----------------------------------------------------------------------------

void _CHARGE_SetState(uint8_t newSt)
{
	chargeState = newSt;
	switch (chargeState) {
	case CHG_ST_NIGHT:
		_CHARGE_SetRegulate(false);
		break;

	case CHG_ST_IDLE:
		chargeTimeoutCount = 0;  // Setup timer for night detection
		_CHARGE_SetRegulate(false);
		break;

	case CHG_ST_VSRCV:
		chargeTimeoutCount = 0;  // Setup timer for recovery period
		_CHARGE_SetRegulate(false);
		break;

	case CHG_ST_SCAN:
		chargeTimeoutCount = 0;  // Setup timer for next scan
		_CHARGE_StartScan(v_b_mv + CHG_SCAN_END_DELTA, v_s_mv);
		break;

	case CHG_ST_BULK:
		chargeLowProdCount = 0;  // Always init low production timer when we enter a new charge state
		chargeHighCount = 0;     // Initialize high charge count when we enter the first of the high-charge states
		_CHARGE_SetRegulate(true);
		break;

	case CHG_ST_ABS:
		chargeLowProdCount = 0;  // Always init low production timer when we enter a new charge state
		chargeAbsTermCount = 0;  // Initialize Absorption termination condition reached timer
		_CHARGE_SetRegulate(true);
		break;

	case CHG_ST_FLT:
		chargeLowProdCount = 0;  // Always init low production timer when we enter a new charge state
		_CHARGE_SetRegulate(true);
		break;
	}

	// Update STATUS
	SMB_SetStatusChargeState(chargeState);
}


uint16_t _CHARGE_ComputePower()
{
	uint32_t p;

	solarPowerUw = (uint32_t) v_s_mv * (uint32_t) i_s_ma;
	p = solarPowerUw / (uint32_t) 1000;
	return ((uint16_t) p);
}


// _CHARGE_ComputePower must be called before this
int16_t _CHARGE_ComputeChargeCurrent()
{
	int32_t t;

	// Estimate using the following equations:
	//   EstimateBuckOutputCurrent = SolarCurrent * (BUCK_INPUT_V / BUCK_OUTPUT_V) * BuckEfficiency (while charging)
	//     (Optimization: We use solarPowerUw for SolarCurrent * BUCK_INPUT_V)
	//   BatteryChargeCurrent = EstimateBuckOutputCurrent - MeasuredLoadCurrent
	if (BUCK_IsEnabled()) {
		t = (int32_t) solarPowerUw / (int32_t) v_b_mv;
		t = t * (int32_t) BUCK_GetEfficiency(solarPowerMw);
		t = t / 100;
	} else {
		t = 0;
	}
	t = t - (int32_t) i_b_ma;

	return ((int16_t) t);
}


void _CHARGE_IncMpptRegVoltage()
{
	chargeSolarRegMv += chargeMpptStepMv;
	if (chargeSolarRegMv > V_MAX_SOLARV) chargeSolarRegMv = V_MAX_SOLARV;
}


void _CHARGE_DecMpptRegVoltage()
{
	chargeSolarRegMv -= chargeMpptStepMv;
	if (chargeSolarRegMv < V_MIN_SOLARV) chargeSolarRegMv = V_MIN_SOLARV;
}


void _CHARGE_SetRegulate(bool en)
{
	if (en) {
		// (Re)configure the BUCK whenever MPPT regulation is enabled or re-enabled
		BUCK_SetSolarVoltage(chargeSolarRegMv);
		BUCK_SetBattVoltage(chargeCompThreshMv);
		BUCK_EnableBatteryLimit(true);
	}
	BUCK_EnableRegulate(en);
	chargeMpptEnable = en;
}


void _CHARGE_AdjustCompBattV()
{
	if (chargeCompThreshMv != BUCK_GetCurBattRegMv()) {
		BUCK_SetBattVoltage(chargeCompThreshMv);
	}
}


void _CHARGE_StartScan(uint16_t lowMv, uint16_t highMv)
{
	chargeMpptScanEnable = true;
	chargeScanEndMv = lowMv;
	chargeSolarRegMv = highMv;
	chargeMaxPower = 0;
	chargeMpptEnable = false;
	BUCK_SetSolarVoltage(highMv);
	BUCK_EnableBatteryLimit(false);  // Force regulation on solar side only
	BUCK_EnableRegulate(true);
}

