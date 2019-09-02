/*
 * adc.c
 *
 * ADC Measurement module
 *  Implement an interrupt-driven sampling system for solar voltage and current, battery
 *  voltage and current and internal/external temperature sensors.  TIMER0 IRQ triggers ADC
 *  readings.  ADC IRQ used to update values.
 *
 * Functions:
 *  1. Initialization
 *    - Initialize filter used for noisy voltage and current measurements
 *    - Read temp - initialize averaging array
 *    - Enable timer + adc interrupts
 *  2. Digitally filtered measurements for voltage and current to handle varying values that
 *     make it through the external low-pass hardware filters.  Measurements made for each
 *     voltage and current channel at approximately 1 kHz.
 *  3. Skew TIMER0 reload value back and forth around the center sample rate so ADC samples
 *     entire PWM waveform over the filter time constants.
 *  4. Temperature sensor measurements every second and averaging over a several second period.
 *  5. Value access routines for other modules and BattChargeCurrent estimation
 *  6. BUCK regulator evaluation every ADC_BUCK_EVAL_MSEC mSec
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
#include <SI_EFM8SB1_Register_Enums.h>
#include "intrins.h"
#include "adc.h"
#include "buck.h"
#include "config.h"


//-----------------------------------------------------------------------------
// Module constants
//-----------------------------------------------------------------------------

// ADC Inputs
#define _ADC_VS_CH 5
#define _ADC_IS_CH 1
#define _ADC_VB_CH 2
#define _ADC_IB_CH 3
#define _ADC_TE_CH 4
#define _ADC_TI_CH 27

// Timer0 reload min/max counts
//  Chosen to allow the sample point to skew around the entire PWM period to
//  more accurately measure a varying signal during that time.
//    - Skew around timer 0 reload base value of 0x80 (~250 uSec/sample)
//    - Timer 0 count = 1.96 uSec
//    - PWM Period = 41.8 uSec
//    => Minimum of 41.8/1.96 = 21.3 counts ==> ~ +/- 11 Timer 0 counts
#define _ADC_TH0_MIN 0x75
#define _ADC_TH0_MAX 0x8B



//-----------------------------------------------------------------------------
// Internal Reference Calibration value - stored at the top of code memory,
// below the bootloader signature byte and the lock byte, so a calibrated
// value can be loaded into the processor by a production programmer.
//-----------------------------------------------------------------------------
SI_SEG_CODE int16_t adcVRefMv _at_ 0x1FFC;



//-----------------------------------------------------------------------------
// Variables
//-----------------------------------------------------------------------------

// Filtered measurement variables
volatile uint32_t SI_SEG_IDATA adcFilterSum[4];
volatile uint16_t SI_SEG_IDATA adcFilteredAdcVal[4];

// Temperature measurement variables
volatile uint8_t SI_SEG_IDATA adcTempSensorIndex;
volatile uint16_t SI_SEG_IDATA adcTempAvgArray[2][ADC_NUM_TEMP_SMPLS];
volatile uint8_t SI_SEG_IDATA adcTempAvgIndex;
volatile uint16_t SI_SEG_IDATA adcTempExtAvgAdcVal;
volatile uint16_t SI_SEG_IDATA adcTempIntAvgAdcVal;

// State management variables
volatile uint8_t SI_SEG_IDATA adcMeasIndex;			// Current index being measured
volatile uint8_t SI_SEG_IDATA adcStoredMeasIndex;   // Index to restore after temperature measurement "interruption"
volatile uint16_t SI_SEG_IDATA adcTempMeasCount;	// Counts down series between temperature measurements (slow)
volatile uint8_t SI_SEG_IDATA adcBuckEvalCount;		// Counts down series of measurements between BUCK evals

// Timer 0 sample control
volatile uint8_t SI_SEG_IDATA adcTimer0Reload;
volatile bit adcTimer0ReloadInc;



//-----------------------------------------------------------------------------
// Forward declarations for internal functions
//-----------------------------------------------------------------------------
void _ADC_DelayMsec(uint8_t mSec);
uint16_t _ADC_GetSingleReading(uint8_t adcChannel);
void _ADC_PushFilteredVal(uint16_t val, uint8_t index);
void _ADC_PushTemp(uint16_t val, uint8_t Tindex);
uint16_t _adc2mV(uint16_t adcVal);
uint16_t _adc2mA(uint16_t adcVal);
int16_t _adc2ExtT10(uint16_t adcVal);
int16_t _adc2IntT10(uint16_t adcVal);
uint16_t _adcIsr2mV(uint16_t adcVal);
uint16_t _adcIsr2mA(uint16_t adcVal);



//-----------------------------------------------------------------------------
// API Routines
//-----------------------------------------------------------------------------
void ADC_Init()
{
	uint8_t i;

	// Make sure ADC Interrupts are disabled so ISRs don't fire while we're initializing
	_ADC_DIS_INT();

	// Delay to allow voltages to stabilize
	_ADC_DelayMsec(50);

	// Initialize variables
	adcMeasIndex = 0;
	adcTempSensorIndex = ADC_TEMP_INT;
	adcTempMeasCount = ADC_TEMP_EVAL_COUNT;
	adcBuckEvalCount = ADC_BUCK_EVAL_COUNT;
	adcTimer0Reload = 0x80;
	adcTimer0ReloadInc = 1;

	// Setup the filtered ADC value filters
	adcFilteredAdcVal[ADC_MEAS_VS_INDEX] = _ADC_GetSingleReading(_ADC_VS_CH);
	adcFilterSum[ADC_MEAS_VS_INDEX] = (adcFilteredAdcVal[ADC_MEAS_VS_INDEX] << ADC_V_FILTER_SHIFT);
	adcFilteredAdcVal[ADC_MEAS_IS_INDEX] = _ADC_GetSingleReading(_ADC_IS_CH);
	adcFilterSum[ADC_MEAS_IS_INDEX] = (adcFilteredAdcVal[ADC_MEAS_IS_INDEX] << ADC_I_FILTER_SHIFT);
	adcFilteredAdcVal[ADC_MEAS_VB_INDEX] = _ADC_GetSingleReading(_ADC_VB_CH);
	adcFilterSum[ADC_MEAS_VB_INDEX] = (adcFilteredAdcVal[ADC_MEAS_VB_INDEX] << ADC_V_FILTER_SHIFT);
	adcFilteredAdcVal[ADC_MEAS_IB_INDEX] = _ADC_GetSingleReading(_ADC_IB_CH);
	adcFilterSum[ADC_MEAS_IB_INDEX] = (adcFilteredAdcVal[ADC_MEAS_IB_INDEX] << ADC_I_FILTER_SHIFT);

	// Load the temperature averaging arrays
	adcTempExtAvgAdcVal = _ADC_GetSingleReading(_ADC_TE_CH);
	for (i=0; i<ADC_NUM_TEMP_SMPLS; i++) {
		adcTempAvgArray[ADC_TEMP_EXT][i] = adcTempExtAvgAdcVal;
	}
	adcTempIntAvgAdcVal = _ADC_GetSingleReading(_ADC_TI_CH);
	for (i=0; i<ADC_NUM_TEMP_SMPLS; i++) {
		adcTempAvgArray[ADC_TEMP_INT][i] = adcTempIntAvgAdcVal;
	}

	// Configure the ADC input for the initial reading
	ADC0MX = _ADC_VS_CH;

	// Enable Timer0
	TCON_TR0 = 1;

	// Enable ADC Interrupts
	_ADC_EN_INT();
}


// Designed to be called by code from the main loop thread
int16_t ADC_GetValue(uint8_t index)
{
	uint16_t t1;

	// Atomically sample current value to prevent ISR from updating it halfway
	// through our access
	switch (index) {
	case ADC_MEAS_VS_INDEX:
		_ADC_DIS_INT();
		t1 = adcFilteredAdcVal[ADC_MEAS_VS_INDEX];
		_ADC_EN_INT();
		return(_adc2mV(t1));
	case ADC_MEAS_IS_INDEX:
		_ADC_DIS_INT();
		t1 = adcFilteredAdcVal[ADC_MEAS_IS_INDEX];
		_ADC_EN_INT();
		return(_adc2mA(t1));
	case ADC_MEAS_VB_INDEX:
		_ADC_DIS_INT();
		t1 = adcFilteredAdcVal[ADC_MEAS_VB_INDEX];
		_ADC_EN_INT();
		return(_adc2mV(t1));
	case ADC_MEAS_IB_INDEX:
		_ADC_DIS_INT();
		t1 = adcFilteredAdcVal[ADC_MEAS_IB_INDEX];
		_ADC_EN_INT();
		return(_adc2mA(t1));
	case ADC_MEAS_TI_INDEX:
		_ADC_DIS_INT();
		t1 = adcTempIntAvgAdcVal;
		_ADC_EN_INT();
		return(_adc2IntT10(t1));
	case ADC_MEAS_TE_INDEX:
		_ADC_DIS_INT();
		t1 = adcTempExtAvgAdcVal;
		_ADC_EN_INT();
		return(_adc2ExtT10(t1));
	default:
		return(0);
	}
}


// Designed to be called by BUCK module executing in Timer0 ISR (minimal functionality necessary)
uint16_t ADC_GetValueForIsr(uint8_t index)
{
	switch (index) {
		case ADC_MEAS_VS_INDEX:
			return(_adcIsr2mV(adcFilteredAdcVal[ADC_MEAS_VS_INDEX]));
		case ADC_MEAS_IS_INDEX:
			return(_adcIsr2mA(adcFilteredAdcVal[ADC_MEAS_IS_INDEX]));
		case ADC_MEAS_VB_INDEX:
			return(_adcIsr2mV(adcFilteredAdcVal[ADC_MEAS_VB_INDEX]));
		default:
			return(0);
	}
}



//-----------------------------------------------------------------------------
// Interrupt Handlers
//-----------------------------------------------------------------------------
// TIMER0_IRQ requires ~1.3 - 1.5 uSec typically, ~148-163 uSec when BUCK_Update runs
SI_INTERRUPT (TIMER0_ISR, TIMER0_IRQn)
{
	// Evaluate the BUCK Buck regulator if time
	if (--adcBuckEvalCount == 0) {
		adcBuckEvalCount = ADC_BUCK_EVAL_COUNT;
		BUCK_Update();
	}

	// Trigger an ADC reading
	ADC0CN0_ADBUSY = 1;

	// Update the reload value to skew the period between samples
	if (adcTimer0ReloadInc == 1) {
		if (++adcTimer0Reload == _ADC_TH0_MAX) {
			adcTimer0ReloadInc = 0;
		}
	} else {
		if (--adcTimer0Reload == _ADC_TH0_MIN) {
			adcTimer0ReloadInc = 1;
		}
	}
	TH0 = adcTimer0Reload;

	// Clear TCON::TF0 (Timer 0 Overflow Flag) - done by HW on entry to ISR
}


// ADC0E0C_IRQ requires ~19-32 uSec
SI_INTERRUPT (ADC0EOC_ISR, ADC0EOC_IRQn)
{
	// Store the ADC result
	if (adcMeasIndex <= ADC_MEAS_IB_INDEX) {
		_ADC_PushFilteredVal(ADC0, adcMeasIndex);
	} else {
		// Temperature
		_ADC_PushTemp(ADC0, adcTempSensorIndex);
	}

	// Setup the next ADC reading
	if ((adcMeasIndex == ADC_MEAS_TI_INDEX) || (adcMeasIndex == ADC_MEAS_TE_INDEX)) {
		// Restore normal operation
		adcMeasIndex = adcStoredMeasIndex;
	} else {
		// Setup for next channel
		if (++adcMeasIndex > ADC_MEAS_IB_INDEX) {
			adcMeasIndex = 0;
		}
		if (--adcTempMeasCount == 0) {
			// Setup special case of occasional temperature measurement
			adcTempMeasCount = ADC_TEMP_EVAL_COUNT; // Reset temperature timer
			adcStoredMeasIndex = adcMeasIndex;      // Save current normal channel to restore

			// Setup temperature channel for next measurement (alternate between temp channels)
			if (adcTempSensorIndex == ADC_TEMP_EXT) {
				adcMeasIndex = ADC_MEAS_TI_INDEX;   // Internal temperature sensor
				adcTempSensorIndex = ADC_TEMP_INT;  // Next temp will be internal
			} else {
				adcMeasIndex = ADC_MEAS_TE_INDEX;   // External temperature sensor
				adcTempSensorIndex = ADC_TEMP_EXT;  // Next temp will be external
			}
		}
	}

	// Configure the ADC input for the next reading
	switch (adcMeasIndex) {
	case ADC_MEAS_VS_INDEX:
		ADC0MX = _ADC_VS_CH;
		break;
	case ADC_MEAS_IS_INDEX:
		ADC0MX = _ADC_IS_CH;
		break;
	case ADC_MEAS_VB_INDEX:
		ADC0MX = _ADC_VB_CH;
		break;
	case ADC_MEAS_IB_INDEX:
		ADC0MX = _ADC_IB_CH;
		break;
	case ADC_MEAS_TI_INDEX:
		ADC0MX = _ADC_TI_CH;
		break;
	default:
		ADC0MX = _ADC_TE_CH;
	}

	// Clear ADC0CN0::ADINT (Conversion Complete Interrupt Flag)
	ADC0CN0_ADINT = 0;
}



//-----------------------------------------------------------------------------
// Internal Routines
//-----------------------------------------------------------------------------

// Delay function designed for use during initialization - ADC/Timer0 interrupts must be disabled
void _ADC_DelayMsec(uint8_t mSec)
{
	uint8_t i;

	// Uses Timer0 overflow at ~250 uSec
	TCON_TF0 = 0;  // Clear overflow flag
	TL0 = TH0;     // Manually set timer for first time
	TCON_TR0 = 1;  // Enable Timer 0
	while (mSec--) {
		for (i=0; i<4; i++) {
			// Spin until timer overflows
			while (TCON_TF0 != 1) {};

			// Reset for next period
			TCON_TF0 = 0;
		}
	}

	// Disable Timer 0
	TCON_TR0 = 0;
	TCON_TF0 = 0;
}


// Single ADC measurement - ADC Interrupts must be disabled
uint16_t _ADC_GetSingleReading(uint8_t adcChannel)
{
	// Set the ADC Channel
	ADC0MX = adcChannel;

	// Wait >5 uSec for input to settle
	_ADC_DelayMsec(10);

	// Trigger ADC
	ADC0CN0_ADINT = 0;
	ADC0CN0_ADBUSY = 1;

	// Wait for ADC to finish
	while (ADC0CN0_ADINT != 1) {};
	ADC0CN0_ADINT = 0;

	return(ADC0);
}


void _ADC_PushFilteredVal(uint16_t val, uint8_t index)
{
	// Even indices are voltage, odd are current
	if (index & 0x01) {
		// Update current filter with current sample
		adcFilterSum[index] = adcFilterSum[index] - (adcFilterSum[index] >> ADC_I_FILTER_SHIFT) + val;

		// Scale for unity gain
		adcFilteredAdcVal[index] = adcFilterSum[index] >> ADC_I_FILTER_SHIFT;
	} else {
		// Update current filter with current sample
		adcFilterSum[index] = adcFilterSum[index] - (adcFilterSum[index] >> ADC_V_FILTER_SHIFT) + val;

		// Scale for unity gain
		adcFilteredAdcVal[index] = adcFilterSum[index] >> ADC_V_FILTER_SHIFT;
	}
}


void _ADC_PushTemp(uint16_t val, uint8_t Tindex)
{
	uint8_t i;
	uint16_t tempAvgAdcVal = 0;

	// Push current value
	adcTempAvgArray[Tindex][adcTempAvgIndex] = val;
	if (Tindex == ADC_TEMP_EXT) {
		// Only increment after second sensor measured
		if (++adcTempAvgIndex == ADC_NUM_TEMP_SMPLS) adcTempAvgIndex = 0;
	}

	// Compute current averaged ADC value
	tempAvgAdcVal = 0;
	for (i=0; i<ADC_NUM_TEMP_SMPLS; i++) {
		tempAvgAdcVal += adcTempAvgArray[Tindex][i];
	}

	// Round up the pre-divided value if necessary and then scale to divide
	if (tempAvgAdcVal & ADC_TEMP_RND_MASK) {
		// Round up unshifted sum
		tempAvgAdcVal += ADC_TEMP_RND_MASK;
	}
	tempAvgAdcVal = tempAvgAdcVal >> ADC_TEMP_SHIFT;

	if (Tindex == ADC_TEMP_INT) {
		adcTempIntAvgAdcVal = tempAvgAdcVal;
	} else {
		adcTempExtAvgAdcVal = tempAvgAdcVal;
	}
}


// Compute a voltage reading in mV from the ADC raw count
//  mV = (ADC_COUNT * VREF * V_SF) / 4092
//
uint16_t _adc2mV(uint16_t adcVal)
{
	uint32_t t = (uint32_t) adcVal * adcVRefMv * V_SF;
	t = t / 4092;
	return (t);
}


// Compute a voltage reading in mV from the ADC raw count (ISR version)
//  mV = (ADC_COUNT * VREF * V_SF) / 4092
//
uint16_t _adcIsr2mV(uint16_t adcVal)
{
	uint32_t t = (uint32_t) adcVal * adcVRefMv * V_SF;
	t = t / 4092;
	return (t);
}


// Compute a current reading in mA from the ADC raw count
//  mA = (ADC_COUNT * VREF) / (4092 * I_GAIN * I_RESISTOR) = (ADC_COUNT * VREF) / I_DIVISOR
//
uint16_t _adc2mA(uint16_t adcVal)
{
	uint32_t t = (uint32_t) adcVal * adcVRefMv;
	t = t / I_DIVISOR;
	return (t);
}


// Compute a current reading in mA from the ADC raw count (ISR version)
//  mA = (ADC_COUNT * VREF) / (4092 * I_GAIN * I_RESISTOR) = (ADC_COUNT * VREF) / I_DIVISOR
//
uint16_t _adcIsr2mA(uint16_t adcVal)
{
	uint32_t t = (uint32_t) adcVal * adcVRefMv;
	t = t / I_DIVISOR;
	return (t);
}


// Return temp in units of C * 10
//
// Internal Temperature Sensor spec says slope is 3.4 mV/C and a 10-bit
// measurement for each device is held in the TOFF register (measured
// at 25C).  An offset for 0C can be calculated by subtracting the
// computed ADC count for 25 degrees C from the TOFF register and
// scaling to 12-bits.
int16_t _adc2IntT10(uint16_t adcVal)
{
	int32_t t;

	// Offset for 0-degree with internal calibration offset
	//   - Offset @ 0C is 940 mV nominal (12-bit ADC count = (940/ADC_VREF_MV)*4092 = 2331 ideal
	//   - {TOFFH[7:0], TOFFL[7:6]} contain 10-bit calibrated offset voltage
	//   - Adjust ADC count to 0-degrees with calibration: adcVal - 2331 - 4*TOFF
	//
	t = 3846480 / adcVRefMv;
	t = (int32_t) adcVal - t;;
	t = t - (((int32_t) TOFFH << 4) | ((int32_t) TOFFL >> 4));

	// Compute temperature
	//   T_C_10 = (100*ADC_CAL_MV*ADC_VREF_MV)/(4092 * 34)
	t = t * adcVRefMv;
	t = t * 100;
	t = t / 139128;

	return (t);
}


// Return temp in units of C * 10
//
// External Temperature Sensor spec says slope is 10 mV/C and
// the nominal calibration 500 mV offset for 0C for this device
// and 12-bit ADC values (4092 * (500 mV/ADC_VREF_MV mV) = 1240 ideal).
int16_t _adc2ExtT10(uint16_t adcVal)
{
	int32_t t;

	// Offset for 0-degree
	t = 2046000 / adcVRefMv;
	t = (int32_t) adcVal - t;

	// Compute temperature
	//  T_C_10 = (ADC_CAL_MV*ADC_VREF_MV)/4092
	t = t * adcVRefMv;
	t = t / 4092;

	return (t);
}
