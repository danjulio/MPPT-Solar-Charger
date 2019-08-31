/*
 * temp.h
 *
 * Temperature sensor module header
 *
 */

#ifndef INC_TEMP_H_
#define INC_TEMP_H_

#include "SI_EFM8SB1_Register_Enums.h"

//-----------------------------------------------------------------------------
// Constants
//-----------------------------------------------------------------------------

//
// The missing temp sensor threshold is used to detect a disconnected ext sensor.
// According to the spec, the lowest value the sensor can read is about -40C
// (Vout=100 mV).  The external temp sensor has a 44.2 k-ohm pull-down resistor
// that will theoretically cause the ADC value to read near 0 if the sensor is
// missing.  In reality, leakage current and noise from the pull-down cause the
// ADC to read higher (measured ADC values were around 40-60).  For this reason
// we set the threshold to be slightly higher than the equivalent temperature.
//
// We detect a missing sensor if the external temp reading is below this
// threshold (C*10) and the internal temperature sensor is more than the
// difference (C*10) away.
//
#define TEMP_MISSING_THRESH_C10  -425
#define TEMP_INT_DIFF_THRESH_C10  200



//-----------------------------------------------------------------------------
// Externs
//-----------------------------------------------------------------------------
extern bool TEMP_extIsMissing;
extern int16_t TEMP_IntTempC10;
extern int16_t TEMP_ExtTempC10;
extern int16_t TEMP_CurTempC10;
extern uint16_t TEMP_compBulkMv;
extern uint16_t TEMP_compFloatMv;


//-----------------------------------------------------------------------------
// API Routines
//-----------------------------------------------------------------------------
void TEMP_Init();
void TEMP_Update();

#define TEMP_ExtSensorMissing() TEMP_extIsMissing
#define TEMP_GetCurTempC10()    TEMP_CurTempC10
#define TEMP_GetIntTempC10()    TEMP_IntTempC10
#define TEMP_GetExtTempC10()    TEMP_ExtTempC10
#define TEMP_GetCompBulkMv()    TEMP_compBulkMv
#define TEMP_GetCompFloatMv()   TEMP_compFloatMv

#endif /* INC_TEMP_H_ */
