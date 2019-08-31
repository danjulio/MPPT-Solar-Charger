/*
 * buck.c
 *
 * Buck converter control module.
 *   - Controls an approximately 24 kHz PWM output for driving a buck-topology converter
 *   - Duty cycle computed to regulate input voltage (from solar panel) to MPPT set-point
 *     with output voltage equal to current measured battery voltage
 *   - Ability to limit based on upper battery voltage threshold (current charge threshold)
 *     and over-current conditions
 *
 */
#include "buck.h"
#include "adc.h"
#include "config.h"
#include "param.h"
#include "smbus.h"


//-----------------------------------------------------------------------------
// Efficiency lookup table - efficiency (percent) as a function of input power (mW)
//   determined via hardware measurement
//-----------------------------------------------------------------------------
#define BUCK_NUM_EFF_ENTRIES 22
code const uint16_t buckEffPowerPoints[BUCK_NUM_EFF_ENTRIES] = {
		250, 375, 500, 750, 1000, 1250, 1500, 1750, 2000, 2250, 2500, 2750, 3000, 3250, 3500, 4000, 5000, 6000, 7500, 11500, 25000, 35000
};
code const uint8_t buckEffValues[BUCK_NUM_EFF_ENTRIES] = {
		56,  62,  67,  71,  75,   77,   78,   80,   81,   83,   84,   85,   86,   87,   88,   89,   90,   91,   92,   93,    93,    92
};



//-----------------------------------------------------------------------------
// Variables
//-----------------------------------------------------------------------------
volatile uint16_t SI_SEG_IDATA buckCurVal;
volatile uint16_t SI_SEG_IDATA buckBattRegMv;
volatile uint16_t SI_SEG_IDATA buckSolarRegMv;
volatile bool buckEnable;
volatile bool buckLimitEnable;  // Enable for battery voltage limiting
volatile bool buckLimit1;       // Over-current or over-voltage limit
volatile bool buckLimit2;       // Regulated voltage above min



//-----------------------------------------------------------------------------
// Internal Routine forward declarations
//-----------------------------------------------------------------------------
void _BUCK_UpdatePwmOutput();



//-----------------------------------------------------------------------------
// API routines
//-----------------------------------------------------------------------------
void BUCK_Init()
{
	buckEnable = false;
	buckLimitEnable = true;
	buckCurVal = 0;
	buckLimit1 = false;
	buckLimit2 = false;
	buckBattRegMv = PARAM_GetFloatMv();
	buckSolarRegMv = V_MIN_GOOD_SOLAR;
	_BUCK_UpdatePwmOutput();
}


void BUCK_SetSolarVoltage(uint16_t v)
{
	// Atomically set input regulation voltage (since BUCK_Update is executed in Timer0 ISR)
	_ADC_DIS_TMR0();
	buckSolarRegMv = v;
	_ADC_EN_TMR0();
}


void BUCK_SetBattVoltage(uint16_t v)
{
	// Atomically set output regulation voltage
	_ADC_DIS_TMR0();
	buckBattRegMv = v;
	_ADC_EN_TMR0();
}


void BUCK_EnableRegulate(bool en)
{
	uint32_t t;

	if (en) {
		if (!buckEnable) {
			// Configure an initial regulator PWM guess based on a simple CCM
			// calculation using input and output voltages to get us in the ballpark
			t = (uint32_t) BUCK_PWM_MAX * (uint32_t) buckBattRegMv;
			buckCurVal = (uint16_t) (t / (uint32_t) buckSolarRegMv);
			if (buckCurVal > BUCK_PWM_MAX) {
				buckCurVal = BUCK_PWM_MAX;
			}
			_BUCK_UpdatePwmOutput();

			// Enable regulator last
			buckEnable = true;
		}
	} else {
		if (buckEnable) {
			// Disable regulator first
			buckEnable = false;

			// Disable output
			buckCurVal = 0;
			buckLimit1 = false;
			buckLimit2 = false;
			_BUCK_UpdatePwmOutput();

			// Note buck is off in BUCK STATUS
			SMB_SetBuckStatus(0);
		}
	}
}


void BUCK_EnableBatteryLimit(bool en) {
	buckLimitEnable = en;
}


void BUCK_Update()
{
	uint16_t solarMv;
	uint16_t battMv;
	uint16_t solarMa;

	if (buckEnable) {
		// Get the current system measurements
		solarMv = ADC_GetValueForIsr(ADC_MEAS_VS_INDEX);
		battMv = ADC_GetValueForIsr(ADC_MEAS_VB_INDEX);
		solarMa = ADC_GetValueForIsr(ADC_MEAS_IS_INDEX);

		// Compute limiting flags
		//  buckLimit1 : Over-voltage or Over-current condition (Decrease power transfer)
		//  buckLimit2 : Battery voltage at or above charge threshold (no need to increase power transfer)
		buckLimit1 = (buckLimitEnable && (battMv > (buckBattRegMv + V_BUCK_HYST))) || (solarMa > I_SOLAR_MAX);
		buckLimit2 = buckLimitEnable && (battMv >= (buckBattRegMv - V_BUCK_HYST));

		// Evaluate buck regulator
		if ((solarMv < buckSolarRegMv) || buckLimit1) {
			// Decrease power transfer to try to bring solarMv up
			if (buckCurVal > BUCK_PWM_MIN) {
				buckCurVal--;
				_BUCK_UpdatePwmOutput();
			}
		} else if ((solarMv > buckSolarRegMv) && !buckLimit2) {
			// Increase power transfer to try to bring solarMv down
			if (buckCurVal < BUCK_PWM_MAX) {
				buckCurVal++;
				_BUCK_UpdatePwmOutput();
			}
		}

		// Update SMBus BUCK STATUS
		//  Re-use SolarMv to build up register contents
		solarMv = buckCurVal << 6;
		if (buckLimit1) solarMv |= 0x0001;
		if (buckLimit2) solarMv |= 0x0002;
		SMB_SetBuckStatus(solarMv);
	}
}


uint8_t BUCK_GetEfficiency(uint16_t ps)
{
	uint8_t i = 0;

	// Find the efficiency value for the first power point immediately
	// above the current solar power input
	while (i < (BUCK_NUM_EFF_ENTRIES-1)) {
		if (ps < buckEffPowerPoints[i]) {
			break;
		}
		i++;
	}

	return(buckEffValues[i]);
}



//-----------------------------------------------------------------------------
// Internal Routines
//-----------------------------------------------------------------------------

//
// Input: buckCurVal
//
void _BUCK_UpdatePwmOutput()
{
	uint16_t pwmVal;

	pwmVal = BUCK_PWM_MAX - buckCurVal;

	// Load the PWM value
	PCA0CPL0 = pwmVal & 0xFF;
	PCA0CPH0 = pwmVal >> 8;

	// ECOM0 must be cleared if value is 0 to completely disable PWM
	if (buckCurVal == 0) {
		PCA0CPM0 &= ~0x40;         // Clear ECOM0 to disable output
	} else {
		if ((PCA0CPM0 & 0x40) == 0x00) {
			PCA0CPM0 |= 0x40;          // Set ECOM0 when BUCK is not 0
		}
	}
}
