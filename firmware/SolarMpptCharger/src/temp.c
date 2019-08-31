/*
 * temp.c
 *
 * Temperature sensor module
 *  1. Compute temperature compensated battery voltage thresholds for
 *     Bulk/Absorption and Float conditions using external temperature
 *     sensor (if it is present) or internal temperature sensor as a
 *     backup if the external sensor is not present.
 *  2. Detect missing external temperature sensor.
 *  3. Update SMBus registers with temperature sensor values and missing
 *     temperature sensor detection.
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
#include "temp.h"
#include "adc.h"
#include "config.h"
#include "math.h"
#include "param.h"
#include "smbus.h"


//-----------------------------------------------------------------------------
// Variables
//-----------------------------------------------------------------------------
bool TEMP_extIsMissing;
int16_t TEMP_IntTempC10;
int16_t TEMP_ExtTempC10;
int16_t TEMP_CurTempC10;
uint16_t TEMP_compBulkMv;
uint16_t TEMP_compFloatMv;



//-----------------------------------------------------------------------------
// API Routines
//-----------------------------------------------------------------------------
void TEMP_Init()
{
	TEMP_Update();
}


void TEMP_Update()
{
	int32_t t;

	// Update temperature values
	TEMP_IntTempC10 = ADC_GetValue(ADC_MEAS_TI_INDEX);
	TEMP_ExtTempC10 = ADC_GetValue(ADC_MEAS_TE_INDEX);

	// Look for missing external temperature sensor
	if ((TEMP_ExtTempC10 < TEMP_MISSING_THRESH_C10) &&
		(abs(TEMP_IntTempC10 - TEMP_ExtTempC10) > TEMP_INT_DIFF_THRESH_C10)) {

		TEMP_extIsMissing = true;
		TEMP_CurTempC10 = TEMP_IntTempC10;
	} else {
		TEMP_extIsMissing = false;
		TEMP_CurTempC10 = TEMP_ExtTempC10;
	}

	//
	// Compute temperature compensated voltage thresholds
	//
	//  Since both our temperature and compensation constant values are x 10, we have to divide
	//  by 100 to properly compute mV delta values.
	//
	// Evaluate temperature compensated charge voltage for FLOAT
	t = ((int32_t) TEMP_CurTempC10 - 250) * V_FLOAT_COMP_X10;
	if ((t % 100) >= 50) {
		// Round up
		t += 100;
	}
	TEMP_compFloatMv = (uint16_t) ((int32_t) PARAM_GetFloatMv() + (t / 100));

	// Evaluate temperature compensated charge voltage for BULK/ABS
	t = ((int32_t) TEMP_CurTempC10 - 250) * V_BULK_COMP_X10;
	if ((t % 100) >= 50) {
		// Round up
		t += 100;
	}
	TEMP_compBulkMv = (uint16_t) ((int32_t) PARAM_GetBulkMv() + (t / 100));

	// Update SMB registers
	SMB_SetStatusBit(SMB_ST_ET_MISS_MASK, TEMP_extIsMissing);
	SMB_SetIndexedValue(SMB_INDEX_I_TEMP, TEMP_IntTempC10);
	SMB_SetIndexedValue(SMB_INDEX_E_TEMP, TEMP_ExtTempC10);
}


