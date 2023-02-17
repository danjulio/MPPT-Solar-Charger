/*
 * param.c
 *
 * Adjustable system parameter access.  Access mechanisms for setting and
 * getting configurable system parameters.
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
#include "param.h"
#include "config.h"
#include "smbus.h"


//-----------------------------------------------------------------------------
// External IO bits
//-----------------------------------------------------------------------------
SI_SBIT(IO_BATT_TYPE_I, SFR_P1, 7);



//-----------------------------------------------------------------------------
// Variables
//-----------------------------------------------------------------------------
bit PARAM_battIsLeadAcid;
uint16_t PARAM_bulkMv;
uint16_t PARAM_floatMv;
uint16_t PARAM_pwrOffMv;
uint16_t PARAM_pwrOnMv;



//-----------------------------------------------------------------------------
// Internal routine forward declarations
//-----------------------------------------------------------------------------
uint16_t _PARAM_ValidateNewValue(uint16_t val, uint16_t min, uint16_t max);



//-----------------------------------------------------------------------------
// API Routines
//-----------------------------------------------------------------------------
void PARAM_Init(){
	// Read the battery type first
	PARAM_battIsLeadAcid = IO_BATT_TYPE_I;

	// Set parameters
	if (PARAM_battIsLeadAcid == 1) {
		PARAM_bulkMv = V_BULK_DEFAULT_1;
		PARAM_pwrOnMv = V_LOAD_ON_1;
	} else {
		PARAM_bulkMv = V_BULK_DEFAULT_2;
		PARAM_pwrOnMv = V_LOAD_ON_2;
	}
	PARAM_floatMv = V_FLOAT_DEFAULT;
	PARAM_pwrOffMv = V_LOAD_OFF;
}


//
// API routines for interrupt-driven SMBUS logic
//
void PARAM_SetIndexedValue(uint8_t index, uint16_t val)
{
	switch (index) {
	case PARAM_INDEX_BULK:
		PARAM_bulkMv = _PARAM_ValidateNewValue(val, V_BULK_MIN, V_BULK_MAX);
		break;
	case PARAM_INDEX_FLOAT:
		PARAM_floatMv = _PARAM_ValidateNewValue(val, V_FLOAT_MIN, V_FLOAT_MAX);
		break;
	case PARAM_INDEX_PWROFF:
		PARAM_pwrOffMv = _PARAM_ValidateNewValue(val, V_LOAD_OFF_MIN, PARAM_pwrOnMv);
		break;
	case PARAM_INDEX_PWRON:
		PARAM_pwrOnMv = _PARAM_ValidateNewValue(val, V_LOAD_ON_MIN, V_LOAD_ON_MAX);
		break;
	}
}


uint16_t PARAM_GetIndexedValue(uint8_t index)
{
	uint16_t v = 0;

	switch (index) {
	case PARAM_INDEX_BULK:
		v = PARAM_bulkMv;
		break;
	case PARAM_INDEX_FLOAT:
		v = PARAM_floatMv;
		break;
	case PARAM_INDEX_PWROFF:
		v = PARAM_pwrOffMv;
		break;
	case PARAM_INDEX_PWRON:
		v = PARAM_pwrOnMv;
		break;
	}

	return v;
}


//
// API routines for main code thread
//
uint16_t PARAM_GetBulkMv()
{
	uint16_t v;

	// Atomically access variable
	SMBUS_DIS_INT();
	v = PARAM_bulkMv;
	SMBUS_EN_INT();

	return v;
}


uint16_t PARAM_GetFloatMv()
{
	uint16_t v;

	// Atomically access variable
	SMBUS_DIS_INT();
	v = PARAM_floatMv;
	SMBUS_EN_INT();

	return v;
}


uint16_t PARAM_GetPwrOffMv()
{
	uint16_t v;

	// Atomically access variable
	SMBUS_DIS_INT();
	v = PARAM_pwrOffMv;
	SMBUS_EN_INT();

	return v;
}


uint16_t PARAM_GetPwrOnMv()
{
	uint16_t v;

	// Atomically access variable
	SMBUS_DIS_INT();
	v = PARAM_pwrOnMv;
	SMBUS_EN_INT();

	return v;
}



//-----------------------------------------------------------------------------
// Internal routines
//-----------------------------------------------------------------------------
uint16_t _PARAM_ValidateNewValue(uint16_t val, uint16_t min, uint16_t max)
{
	if (val < min) {
		return min;
	} else if (val > max) {
		return max;
	} else {
		return val;
	}
}

