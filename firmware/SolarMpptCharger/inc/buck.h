/*
 * buck.h
 *
 * Header for buck converter control module
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

#ifndef BUCK_H_
#define BUCK_H_

#include "SI_EFM8SB1_Register_Enums.h"


//-----------------------------------------------------------------------------
// Constants
//-----------------------------------------------------------------------------
#define BUCK_PWM_MIN 0
#define BUCK_PWM_MAX 1023



//-----------------------------------------------------------------------------
// Externs
//-----------------------------------------------------------------------------
extern volatile uint16_t SI_SEG_IDATA buckCurVal;
extern volatile uint16_t SI_SEG_IDATA buckBattRegMv;
extern volatile uint16_t SI_SEG_IDATA buckSolarRegMv;
extern volatile bool buckEnable;
extern volatile bool buckLimit1;  // Over-current or over-voltage limit
extern volatile bool buckLimit2;  // Regulate voltage above min



//-----------------------------------------------------------------------------
// API Routines
//-----------------------------------------------------------------------------
void BUCK_Init();
void BUCK_SetSolarVoltage(uint16_t v);
void BUCK_SetBattVoltage(uint16_t v);
void BUCK_EnableRegulate(bool en);
void BUCK_EnableBatteryLimit(bool en);
void BUCK_Update();
uint8_t BUCK_GetEfficiency(uint16_t ps);

#define BUCK_GetPwm()		    buckCurVal
#define BUCK_GetCurBattRegMv()  buckBattRegMv
#define BUCK_GetCurSolarRegMv() buckSolarRegMv
#define BUCK_IsEnabled()        buckEnable
#define BUCK_IsLimiting()       (buckLimit1 || buckLimit2)
#define BUCK_IsLimit1()         buckLimit1
#define BUCK_IsLimit2()         buckLimit2

#endif /* BUCK_H_ */
