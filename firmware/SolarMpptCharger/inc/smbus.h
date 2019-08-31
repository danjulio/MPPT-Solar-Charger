/*
 * smbus.h
 *
 * Header for SMBus access module
 *
 */

#ifndef SMBUS_H_
#define SMBUS_H_


//-----------------------------------------------------------------------------
// Constants
//-----------------------------------------------------------------------------

// Register access indices
//  To simplify the interrupt-driven SMB logic, we segregate the registers by
//  type.  RO registers are first followed by RW registers.  RO registers are
//  held in a local array for fast access that is updated by main code as values
//  change.
//
// RO state 16-bit register indices (index into local array)
#define SMB_INDEX_ID       0
#define SMB_INDEX_STATUS   1
#define SMB_INDEX_BUCK     2
#define SMB_INDEX_VS       3
#define SMB_INDEX_IS       4
#define SMB_INDEX_VB       5
#define SMB_INDEX_IB       6
#define SMB_INDEX_IC       7
#define SMB_INDEX_I_TEMP   8
#define SMB_INDEX_E_TEMP   9
#define SMB_INDEX_VM       10
#define SMB_INDEX_TH       11


// RW PARAM 16-bit register SMBus address (must match PARAM order)
#define SMB_ADDR_BULK_V    24
#define SMB_ADDR_FLT_V     26
#define SMB_ADDR_POFF_V    28
#define SMB_ADDR_PON_V     30
// RW Watchdog 8-bit register SMBus address
#define SMB_ADDR_WD_EN     33
#define SMB_ADDR_WD_TO     35
// RW Watchdog 16-bit register SMBus address
#define SMB_ADDR_WD_PWROFF 36

#define SMB_NUM_RO         12
#define SMB_PARAM_START    SMB_ADDR_BULK_V
#define SMB_WD_START       32


// Status register masks
#define SMB_ST_SWD_DET_MASK  0x8000
#define SMB_ST_PWD_TRIG_MASK 0x4000
#define SMB_ST_BAD_BATT_MASK 0x2000
#define SMB_ST_ET_MISS_MASK  0x1000
#define SMB_ST_WD_RUN_MASK   0x0100
#define SMB_ST_PWR_EN_MASK   0x0080
#define SMB_ST_ALERT_MASK    0x0040
#define SMB_ST_PCTRL_MASK    0x0020
#define SMB_ST_T_LIM_MASK    0x0010
#define SMB_ST_NIGHT_MASK    0x0008
#define SMB_ST_CHG_ST_MASK   0x0007


// Buck Status register masks
#define SMB_BUCK_PWM_MASK    0xFF00
#define SMB_BUCK_LIM2_MASK   0x0002
#define SMB_BUCK_LIM1_MASK   0x0001


// Watchdog Enable Register magic value (to enable)
#define SMB_WD_EN_MAGIC_BYTE 0xEA


//
// Internal SMBus operation constants
//

// I2C Address LSB
#define  SMB_WRITE      0x00           // SMBus WRITE command
#define  SMB_READ       0x01           // SMBus READ command

// Status vector - top 4 bits only
#define  SMB_SRADD      0x20           // (SR) slave address received
                                       //    (also could be a lost
                                       //    arbitration)
#define  SMB_SRSTO      0x10           // (SR) STOP detected while SR or ST,
                                       //    or lost arbitration
#define  SMB_SRDB       0x00           // (SR) data byte received, or
                                       //    lost arbitration
#define  SMB_STDB       0x40           // (ST) data byte transmitted
#define  SMB_STSTO      0x50           // (ST) STOP detected during a
                                       //    transaction; bus error


//-----------------------------------------------------------------------------
// API Routines
//-----------------------------------------------------------------------------
void SMB_Init();

// Routines for use by main code to atomically update SMBus values
void SMB_SetIndexedValue(uint8_t index, uint16_t val);
void SMB_SetBuckStatus(uint16_t val);
void SMB_SetStatusBit(uint16_t mask, bool val);
void SMB_SetStatusChargeState(uint8_t state);

// Interrupt macros
#define SMBUS_DIS_INT() EIE1 &= ~EIE1_ESMB0__BMASK
#define SMBUS_EN_INT()  EIE1 |= EIE1_ESMB0__BMASK


#endif /* SMBUS_H_ */
