/*
 * mpptChg.h - Header file for danjuliodesigns, LLC MPPT Solar Charger.
 *
 * The mpptChg library provides the following functions for either an
 * Arduino-class micro-controller (using the Arduino development environment)
 * or a Raspberry Pi (using Gordon Henderson's wiringPi library):
 *
 *  1. Access to read-only operating values
 *  2. Access to read-write configuration parameters
 *  3. Access to the watchdog timeout mechanism
 *  4. Optional access to the NIGHT and ALERT_N GPIO signals
 *
 * It is designed to be connected to the MPPT Solar Charger via the micro-controller's
 * primary I2C bus and optionally one or two GPIO pins.  It uses the Wire I2C library
 * in the Arduino environment and the wiringPi library in the Raspberry Pi environment.
 *
 * Compilation of platform dependent components keys on the "ARDUINO" define.
 *
 * Copyright (c) 2018 Dan Julio (dan@danjuliodesigns.com)
 *
 * mpptChg is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * mpptChg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * See <http://www.gnu.org/licenses/>.
 *
 */
#ifndef MPPT_CHG_H_
#define MPPT_CHG_H_

#include <inttypes.h>


// ================================================================================
// Constants
// ================================================================================

//
// 7-bit I2C address of the charger
//
#define MPPT_CHG_I2C_ADDR 0x12

//
// Internal register addresses
//
// RO values (16-bits)
#define MPPT_CHG_REG_ID   0
#define MPPT_CHG_STATUS   2
#define MPPT_CHG_BUCK     4
#define MPPT_CHG_VS       6
#define MPPT_CHG_IS       8
#define MPPT_CHG_VB       10
#define MPPT_CHG_IB       12
#define MPPT_CHG_IC       14
#define MPPT_CHG_INT_T    16
#define MPPT_CHG_EXT_T    18
#define MPPT_CHG_VM       20
#define MPPT_CHG_TH       22
// RW Parameters (16-bits)
#define MPPT_CHG_BUCK_TH  24
#define MPPT_CHG_FLOAT_TH 26
#define MPPT_CHG_PWROFF   28
#define MPPT_CHG_PWRON    30
// Watchdog registers (8-bits)
#define MPPT_WD_EN        33
#define MPPT_WD_COUNT     35


//
// ID Register bit masks
//
#define MPPT_CHG_ID_BRD_ID_MASK  0xF000
#define MPPT_CHG_ID_MAJ_REV_MASK 0x00F0
#define MPPT_CHG_ID_MIN_REV_MASK 0x000F

//
// Status Register bit masks
//
#define MPPT_CHG_STATUS_HW_WD_MASK    0x8000
#define MPPT_CHG_STATUS_SW_WD_MASK    0x4000
#define MPPT_CHG_STATUS_BAD_BATT_MASK 0x2000
#define MPPT_CHG_STATUS_EXT_MISS_MASK 0x1000
#define MPPT_CHG_STATUS_WD_RUN_MASK   0x0100
#define MPPT_CHG_STATUS_PWR_EN_MASK   0x0080
#define MPPT_CHG_STATUS_ALERT_MASK    0x0040
#define MPPT_CHG_STATUS_PCTRL_MASK    0x0020
#define MPPT_CHG_STATUS_T_LIM_MASK    0x0010
#define MPPT_CHG_STATUS_NIGHT_MASK    0x0008
#define MPPT_CHG_STATUS_CHG_ST_MASK   0x0007

//
// Status Register Charge States
//
#define MPPT_CHG_ST_NIGHT  0
#define MPPT_CHG_ST_IDLE   1
#define MPPT_CHG_ST_VSRCV  2
#define MPPT_CHG_ST_SCAN   3
#define MPPT_CHG_ST_BULK   4
#define MPPT_CHG_ST_ABSORB 5
#define MPPT_CHG_ST_FLOAT  6

//
// Buck Status bit masks
//
#define MPPT_CHG_BUCK_PWM_MASK  0xFF00
#define MPPT_CHG_BUCK_LIM2_MASK 0x0002
#define MPPT_CHG_BUCK_LIM1_MASK 0x0001

//
// Watchdog enable register value
//
#define MPPT_CHG_WD_ENABLE 0xEA


// ================================================================================
// User-friendly enums for accessing values in the charger
// ================================================================================

//
// Unsigned 16-bit system status operating values
//
typedef enum
{
	SYS_ID = 0,
	SYS_STATUS,
	SYS_BUCK
} mpptChg_sys_t;

//
// Signed 16-bit operating values
//
typedef enum
{
	VAL_VS = 0,
	VAL_IS,
	VAL_VB,
	VAL_IB,
	VAL_IC,
	VAL_INT_TEMP,
	VAL_EXT_TEMP,
	VAL_V_MPPT,
	VAL_V_TH
} mpptChg_val_t;

//
// Signed 16-bit configuration parameters (although they are always positive)
//
typedef enum
{
	CFG_BUCK_V_TH = 0,
	CFG_FLOAT_V_TH,
	CFG_PWR_OFF_TH,
	CFG_PWR_ON_TH
} mpptChg_cfg_t;


// ================================================================================
// Class Header
// ================================================================================
class mpptChg
{
	public:
		mpptChg();
		mpptChg(int aPin);
		mpptChg(int aPin, int nPin);
		bool begin();
		bool getStatusValue(mpptChg_sys_t index, uint16_t* val);
		bool getIndexedValue(mpptChg_val_t index, int16_t* val);
		bool getConfigurationValue(mpptChg_cfg_t index, uint16_t* val);
		bool setConfigurationValue(mpptChg_cfg_t index, uint16_t val);
		bool getWatchdogEnable(bool* val);
		bool setWatchdogEnable(bool* val);
		bool setWatchdogTimeout(uint8_t val);
		bool getWatchdogTimeout(uint8_t* val);
		bool isAlert(bool* val);
		bool isNight(bool* val);

	private:
		bool _Read8(uint8_t reg, uint8_t* val);
		bool _Read16(uint8_t reg, uint16_t* val);
		bool _Write8(uint8_t reg, uint8_t val);
		bool _Write16(uint8_t reg, uint16_t val);

		int alertPin;
		int nightPin;
#ifndef ARDUINO
		int linuxI2cFd;
#endif
};

#endif // MPPT_CHG_H_
