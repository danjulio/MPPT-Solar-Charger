/*
 * param.h
 *
 * Adjustable system parameter access.  Access mechanisms for setting and
 * getting configurable system parameters.
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
