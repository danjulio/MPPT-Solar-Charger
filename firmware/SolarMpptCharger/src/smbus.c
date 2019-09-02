/*
 * smbus.c
 *
 * SMBus access module
 *  Implements an I2C-compatible interface at 7-bit address 0x12.  Main logic
 *  is interrupt driven by the SMBus peripheral with action taken based on
 *  current I2C access state.
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
#include "buck.h"
#include "charge.h"
#include "config.h"
#include "param.h"
#include "power.h"
#include "smbus.h"
#include "temp.h"
#include "watchdog.h"


//-----------------------------------------------------------------------------
// Variables
//-----------------------------------------------------------------------------
volatile uint16_t smbRoArray[SMB_NUM_RO];



//-----------------------------------------------------------------------------
// Internal Routine forward declarations
//-----------------------------------------------------------------------------
uint16_t _SMB_GetInitStatus();
uint8_t _SMB_ReadRegister(uint8_t reg);
void _SMB_WriteRegister(uint8_t reg, uint16_t d);



//-----------------------------------------------------------------------------
// API routines
//-----------------------------------------------------------------------------
void SMB_Init()
{
	uint16_t* roPtr = &smbRoArray[0];

	// Initialize our RO array (optimized for code space - this has to change if the index list changes)
	*roPtr++ = (FW_ID << 12) | (FW_VER_MAJOR << 4) | FW_VER_MINOR;
	*roPtr++ = _SMB_GetInitStatus();
	*roPtr++ = 0;                       // PWM starts off
	*roPtr++ = CHARGE_GetVsMv();
	*roPtr++ = CHARGE_GetIsMa();
	*roPtr++ = CHARGE_GetVbMv();
	*roPtr++ = CHARGE_GetIbMa();
	*roPtr++ = (uint16_t) CHARGE_GetIcMa();
	*roPtr++ = TEMP_GetIntTempC10();
	*roPtr++ = TEMP_GetExtTempC10();
	*roPtr++ = CHARGE_GetMpptMv();
	*roPtr   = CHARGE_GetCompMv();
}


void SMB_SetIndexedValue(uint8_t index, uint16_t val)
{
	SMBUS_DIS_INT();
	smbRoArray[index] = val;
	SMBUS_EN_INT();
}


// For atomic use by Buck code running in an ISR
void SMB_SetBuckStatus(uint16_t val)
{
	SMBUS_DIS_INT();
	smbRoArray[SMB_INDEX_BUCK] = val;
	SMBUS_EN_INT();
}


void SMB_SetStatusBit(uint16_t mask, bool val)
{
	SMBUS_DIS_INT();
	smbRoArray[SMB_INDEX_STATUS] = smbRoArray[SMB_INDEX_STATUS] & ~mask;
	if (val) {
		smbRoArray[SMB_INDEX_STATUS] |= mask;
	}
	SMBUS_EN_INT();
}


void SMB_SetStatusChargeState(uint8_t state)
{
	SMBUS_DIS_INT();
	smbRoArray[SMB_INDEX_STATUS] = (smbRoArray[SMB_INDEX_STATUS] & ~SMB_ST_CHG_ST_MASK) |
	                               (state & SMB_ST_CHG_ST_MASK);
	SMBUS_EN_INT();
}



//-----------------------------------------------------------------------------
// SMBUS0_ISR
//-----------------------------------------------------------------------------
//  SMBUS0_IRQ requires ~1-5 uSec
// IMPORTANT NOTE: This must be the highest priority interrupt to avoid the SMBUS hardware
// from stretching the clock if another interrupt is running.  The Raspberry Pi SBC I2C
// hardware has a bug that causes it to malfunction if the clock is stretched.  Even then
// (because there is some delay in handling the ISR) the I2C clock should be set to 50 kHz.
//   https://www.raspberrypi.org/forums/viewtopic.php?t=13771
//
SI_INTERRUPT (SMBUS0_ISR, SMBUS0_IRQn)
{
	static bool first_byte;
	static uint8_t smb_reg;
	static uint16_t smb_data;

	if (SMB0CN0_ARBLOST == 0)
	{
	  switch (SMB0CN0 & 0xF0)           // Decode the SMBus status vector
	  {
		 // Slave Receiver: Start+Address received
		 case  SMB_SRADD:
			SMB0CN0_STA = 0;                   // Clear SMB0CN0_STA bit
			first_byte = true;

			if((SMB0DAT&0x01) == SMB_READ) // If the transfer is a master READ,
			{
				// Prepare outgoing byte
				SMB0DAT = _SMB_ReadRegister(smb_reg);
				smb_reg++;
			}
			break;

		 // Slave Receiver: Data received
		 case  SMB_SRDB:

			 if (first_byte) {
				 // Set the register address
				 first_byte = false;
				 smb_reg = SMB0DAT;
				 smb_data = 0;
			 } else {
				 // Writing data
				 smb_data = (smb_data << 8) | SMB0DAT;
				 if (smb_reg & 0x01) {
					 // Low half - time to write
					 _SMB_WriteRegister(smb_reg, smb_data);
					 smb_data = 0;
				 }
				 smb_reg++;
			 }
			 SMB0CN0_ACK = 1;                // SMB0CN0_ACK received data
			break;

		 // Slave Receiver: Stop received while either a Slave Receiver or
		 // Slave Transmitter
		 case  SMB_SRSTO:
			 SMB0CN0_STO = 0;                // SMB0CN0_STO must be cleared by software when
									         // a STOP is detected as a slave
			 break;

		 // Slave Transmitter: Data byte transmitted
		 case  SMB_STDB:
			if (SMB0CN0_ACK == 1)            // If Master SMB0CN0_ACK's, send the next byte
			{
				SMB0DAT = _SMB_ReadRegister(smb_reg);
				smb_reg++;
			}                                // Otherwise, do nothing
			break;

		 // Slave Transmitter: Arbitration lost, Stop detected
		 //
		 // This state will only be entered on a bus error condition.
		 // In normal operation, the slave is no longer sending data or has
		 // data pending when a STOP is received from the master, so the SMB0CN0_TXMODE
		 // bit is cleared and the slave goes to the SRSTO state.
		 case  SMB_STSTO:
			SMB0CN0_STO = 0;                 // SMB0CN0_STO must be cleared by software when
									         // a STOP is detected as a slave
			break;

		 // Default: all other cases undefined
		 default:
			SMB0CF &= ~0x80;           // Reset communication
			SMB0CF |= 0x80;
			SMB0CN0_STA = 0;
			SMB0CN0_STO = 0;
			SMB0CN0_ACK = 1;
			break;
	  }
	}
	// SMB0CN0_ARBLOST = 1, Abort failed transfer
	else
	{
	  SMB0CN0_STA = 0;
	  SMB0CN0_STO = 0;
	  SMB0CN0_ACK = 1;
	}

	SMB0CN0_SI = 0;                             // Clear SMBus interrupt flag
}


//-----------------------------------------------------------------------------
// Internal Routines
//-----------------------------------------------------------------------------
uint16_t _SMB_GetInitStatus()
{
	uint16_t s = 0;

	// OR in possibly set bits
	if (WD_WatchdogWasTriggered()) s |= SMB_ST_SWD_DET_MASK;
	if (POWER_WatchdogWasTriggered()) s |= SMB_ST_PWD_TRIG_MASK;
	if (POWER_badBattery()) s |= SMB_ST_BAD_BATT_MASK;
	if (TEMP_ExtSensorMissing()) s |= SMB_ST_ET_MISS_MASK;
	if (POWER_WatchdogRunning()) s |= SMB_ST_WD_RUN_MASK;
	if (POWER_IsEnabled()) s |= SMB_ST_PWR_EN_MASK;
	if (POWER_IsAlert()) s |= SMB_ST_ALERT_MASK;
	if (POWER_PctrlSet()) s |= SMB_ST_PCTRL_MASK;
	if (CHARGE_IsTempLimited()) s |= SMB_ST_T_LIM_MASK;
	if (POWER_IsNight()) s |= SMB_ST_NIGHT_MASK;

	// OR in charge status
	s |= (CHARGE_GetState() & SMB_ST_CHG_ST_MASK);

	return (s);
}


uint8_t _SMB_ReadRegister(uint8_t reg)
{
	bool high_half = (reg & 0x01) == 0;
	uint8_t index = reg >> 1;     // Index is for 16-bit accesses
	uint16_t d16;

	if (reg < SMB_PARAM_START) {
		// RO register
		d16 = smbRoArray[index];

		// Special case for either watchdog detected bits in the STATUS register.  They
		// must be cleared after the register was read.  We test their boolean detected
		// flags before doing an address compare since it's faster.
		if (POWER_WatchdogWasTriggered() || WD_WatchdogWasTriggered()) {
			if (high_half && (index == SMB_INDEX_STATUS)) {
				POWER_ClearWatchdogTriggered();
				WD_ClearWatchdogTriggered();
				smbRoArray[SMB_INDEX_STATUS] &= ~(SMB_ST_PWD_TRIG_MASK | SMB_ST_SWD_DET_MASK);
			}
		}
	} else if (reg < SMB_WD_START) {
		// PARAM
		d16 = PARAM_GetIndexedValue(index - SMB_PARAM_START/2);
	} else if (reg == SMB_ADDR_WD_EN) {
		// Watchdog enable register (appears in a low 16-bit location)
		d16 = (POWER_WatchdogGlobalEnable()) ? 0x0001 : 0x0000;
	} else if (reg == SMB_ADDR_WD_TO) {
		// Watchdog timeout register (appears in a low 16-bit location)
		d16 = POWER_GetWatchdogTimeout();
	} else if (index == SMB_ADDR_WD_PWROFF/2) {
		// Watchdog power off timeout register is a 16-bit register
		d16 = POWER_GetWatchdogPwrOffTO();
	} else {
		d16 = 0;
	}

	if (high_half) {
		// Move high half down if necessary
		d16 = d16 >> 8;
	}

	return (d16 & 0xFF);
}


void _SMB_WriteRegister(uint8_t reg, uint16_t d)
{
	uint8_t index = reg >> 1;

	if ((reg >= SMB_PARAM_START) && (reg < SMB_WD_START)) {
		// PARAM
		PARAM_SetIndexedValue(index - SMB_PARAM_START/2, d);
	} else if (reg == SMB_ADDR_WD_EN) {
		// Watchdog enable register
		POWER_EnableWatchdog(d == SMB_WD_EN_MAGIC_BYTE);
	} else if (reg == SMB_ADDR_WD_TO) {
		// Watchdog timeout register
		POWER_SetWatchdogTimeout(d & 0xFF);
	} else if (index == SMB_ADDR_WD_PWROFF/2) {
		// Watchdog power off timeout register is a 16-bit register so we key off of it's 16-bit address
		POWER_SetWatchdogPwrOffTO(d);
	}
}
