/*
 * param.h
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

#ifndef INC_PARAM_H_
#define INC_PARAM_H_

#include "SI_EFM8SB1_Register_Enums.h"

//-----------------------------------------------------------------------------
// Constants
//-----------------------------------------------------------------------------
#define PARAM_INDEX_BULK    0
#define PARAM_INDEX_FLOAT   1
#define PARAM_INDEX_PWROFF  2
#define PARAM_INDEX_PWRON   3



//-----------------------------------------------------------------------------
// API Routines
//-----------------------------------------------------------------------------
void PARAM_Init();

// API routines for interrupt-driven SMBUS logic
void PARAM_SetIndexedValue(uint8_t index, uint16_t val);
uint16_t PARAM_GetIndexedValue(uint8_t index);

// API routines for main code thread
uint16_t PARAM_GetBulkMv();
uint16_t PARAM_GetFloatMv();
uint16_t PARAM_GetPwrOffMv();
uint16_t PARAM_GetPwrOnMv();


#endif /* INC_PARAM_H_ */
