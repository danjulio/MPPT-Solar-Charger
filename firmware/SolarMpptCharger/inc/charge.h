/*
 * charge.h
 *
 * Header for charge control logic module
 *
 * Copyright (c) 2018-2023 danjuliodesigns, LLC.  All rights reserved.
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

#ifndef INC_CHARGE_H_
#define INC_CHARGE_H_

#include "SI_EFM8SB1_Register_Enums.h"


//-----------------------------------------------------------------------------
// Constants
//-----------------------------------------------------------------------------

// Charge State
#define CHG_ST_NIGHT  0
#define CHG_ST_IDLE   1
#define CHG_ST_VSRCV  2
#define CHG_ST_SCAN   3
#define CHG_ST_BULK   4
#define CHG_ST_ABS    5
#define CHG_ST_FLT    6

//
// MPPT Scan parameters
//
// Value added to current battery voltage to establish ending solar panel voltage
#define CHG_SCAN_END_DELTA  1500


//-----------------------------------------------------------------------------
// Externs
//-----------------------------------------------------------------------------
extern bool chargeTempLimited;
extern uint8_t chargeState;
extern uint16_t v_s_mv;
extern uint16_t i_s_ma;
extern uint16_t v_b_mv;
extern uint16_t i_b_ma;
extern int16_t i_c_ma;
extern uint16_t chargeSolarRegMv;
extern uint16_t chargeCompThreshMv;


//-----------------------------------------------------------------------------
// API Macros
//-----------------------------------------------------------------------------
#define CHARGE_GetVsMv() v_s_mv
#define CHARGE_GetIsMa() i_s_ma
#define CHARGE_GetVbMv() v_b_mv
#define CHARGE_GetIbMa() i_b_ma
#define CHARGE_GetIcMa() i_c_ma
#define CHARGE_GetState() chargeState
#define CHARGE_IsTempLimited() chargeTempLimited
#define CHARGE_GetMpptMv() chargeSolarRegMv
#define CHARGE_GetCompMv() chargeCompThreshMv


//-----------------------------------------------------------------------------
// API Routines
//-----------------------------------------------------------------------------
void CHARGE_Init();
void CHARGE_MpptUpdate();
void CHARGE_StateUpdate();

#endif /* INC_CHARGE_H_ */
