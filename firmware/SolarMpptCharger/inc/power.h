/*
 * power.h
 *
 * Header for output power control module
 *
 */

#ifndef INC_POWER_H_
#define INC_POWER_H_

#include "SI_EFM8SB1_Register_Enums.h"

//-----------------------------------------------------------------------------
// Constants
//-----------------------------------------------------------------------------
#define PWR_ST_OFF_LB    0
#define PWR_ST_OFF_DAY   1
#define PWR_ST_ALERT_LB  2
#define PWR_ST_ALERT_DAY 3
#define PWR_ST_ON        4
#define PWR_ST_WD_ALERT  5
#define PWR_ST_WD_OFF    6



//-----------------------------------------------------------------------------
// Externs
//-----------------------------------------------------------------------------
extern bool POWER_badBatt;
extern bool POWER_enableAtNight;
extern bool POWER_isNight;
extern bool POWER_powerEnabled;
extern bool POWER_watchdogGlobalEnable;
extern bool POWER_watchdogTriggered;
extern uint8_t POWER_state;
extern uint8_t POWER_watchdogCount;
extern uint16_t POWER_watchdogPwrOffTO;



//-----------------------------------------------------------------------------
// API Routines
//-----------------------------------------------------------------------------
void POWER_Init();
void POWER_Update();
void POWER_EnableWatchdog(bool en);
void POWER_SetWatchdogTimeout(uint8_t sec);
void POWER_SetWatchdogPwrOffTO(uint16_t sec);
bool POWER_WatchdogRunning();

#define POWER_badBattery()             POWER_badBatt
#define POWER_PctrlSet()               POWER_enableAtNight
#define POWER_IsEnabled()              POWER_powerEnabled
#define POWER_LowBattDisabled()        (POWER_state == PWR_ST_OFF_LB)
#define POWER_IsAlert()                (POWER_state != PWR_ST_ON)
#define POWER_IsNight()                POWER_isNight
#define POWER_WatchdogGlobalEnable()   POWER_watchdogGlobalEnable
#define POWER_GetWatchdogTimeout()     POWER_watchdogCount
#define POWER_GetWatchdogPwrOffTO()    POWER_watchdogPwrOffTO
#define POWER_WatchdogWasTriggered()   POWER_watchdogTriggered
#define POWER_ClearWatchdogTriggered() POWER_watchdogTriggered = false

#endif /* INC_POWER_H_ */
