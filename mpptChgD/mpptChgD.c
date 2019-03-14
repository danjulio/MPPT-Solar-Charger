/*
 * mpptChgD - danjuliodesigns, LLC MPPT Solar Charger daemon for Raspberry Pi
 *
 * Connects to the MPPT Solar Charger I2C interface and provides the following
 * functions:
 *   1. Simple character-based access for applications to the charger through a
 *	  pseudo-tty called /dev/mpptChg 
 *   2. Optional functionality enabled by an external text configuration file
 *	  a. Automatic system shutdown on low-battery Alert
 *	  b. TCP port interface supporting same commands as pseudo-tty
 *	  c. Logging of charger values to an external file at a user-specified rate
 *	  d. Configuration of charger parameters for non-default operation
 *	  e. Watchdog management
 *
 * Requires Gordon Henderson's wiringPi library to compile and for the I2C interface
 * to be enabled on the Raspberry Pi.  Also uses Ben Hoyt's inih.c library for
 * parsing commands and the config file (included).
 *
 * Copyright (c) 2018-2019 Dan Julio (dan@danjuliodesigns.com)
 *
 * Author's note: Yeah, I know this has grown into a big ole' hack.  It should be 
 * refactored, etc, etc.  Were there more hours in the day...
 *
 * mpptChgD is free software: you can redistribute it and/or modify it
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
#include <stdio.h>
#define __USE_XOPEN_EXTENDED
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <syslog.h>
#include <signal.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <netdb.h>
#include <ctype.h>
#include "ini.h"
#include "wiringPi.h"
#include "wiringPiI2C.h"


//
// Constants
//

#define VERSION_MAJOR       0
#define VERSION_MINOR       5

#define MPPT_CHG_I2C_ADDR   0x12

// MAX_CONNECTIONS includes the pty and socket connections
#define MAX_CONNECTIONS     5

#define MAX_STRING_LEN      512
#define MAX_FIFO_LEN        2*MAX_STRING_LEN

#define WD_INIT_SECS        180
#define WD_UPDATE_SECS      60
#define WDEN_MAGIC_BYTE     0xEA

#define PARAM_CHECK_SECS    300

#define STATUS_ALERT_MASK   0x0040

#define MATCH(n) strcmp(name, n) == 0


//
// Commands
//
#define NUM_CMDS 18

typedef struct {
	const char* cName;
	bool isWritable;
	bool isWord;
	bool isSigned;
	int regAddr;
} cmd_t;

cmd_t cmdList[NUM_CMDS] = {
	{"ID", false, true, false, 0},
	{"STATUS", false, true, false, 2},
	{"BUCK", false, true, false, 4},
	{"VS", false, true, false, 6},
	{"IS", false, true, false, 8},
	{"VB", false, true, false, 10},
	{"IB", false, true, false, 12},
	{"IC", false, true, true, 14},
	{"IT", false, true, true, 16},
	{"ET", false, true, true, 18},
	{"VM", false, true, false, 20},
	{"TH", false, true, false, 22},
	{"BULKV", true, true, false, 24},
	{"FLOATV", true, true, false, 26},
	{"PWROFFV", true, true, false, 28},
	{"PWRONV", true, true, false, 30},
	{"WDEN", true, false, false, 33},
	{"WDCNT", true, false, false, 35}
};


//
// Configuration
//
#define NUM_PARAMS     4
#define PARAM_BULK_I   0
#define PARAM_FLOAT_I  1
#define PARAM_PWROFF_I 2
#define PARAM_PWRON_I  3

typedef struct {
	bool enAutoShutdown;
	bool enParamOverride;
	bool enLogging;
	bool enWatchdog;
	int tcpPort;
	int tcpMaxConnections;
	char logFileName[MAX_STRING_LEN];
	int logDelay;
	bool logMask[NUM_CMDS];
	int paramArray[NUM_PARAMS];
} config_t;

config_t config;


//
// Command Fifos
//
typedef struct {
	bool valid;
	int fd;
	int cmdFifoPushI;
	int cmdFifoPopI;
	int cmdBufI;
	char cmdFifo[MAX_FIFO_LEN];
	char cmdBuf[MAX_STRING_LEN];
} cmdBuf_t;


//
// Other global variables
//
int i2cFd;
int sockFd = -1;
int linkFd;
int logFd;
int *remotefd;
char *linkname = "/dev/mpptChg";
int curSockConnects = 0;
int curNumConnects = 0;
cmdBuf_t cmdBufs[MAX_CONNECTIONS];
int rspFifoPushI = 0;
char rspFifo[MAX_FIFO_LEN];
extern char* ptsname(int fd);



void SetupDefaultConfigValues()
{
	int i;

	config.enAutoShutdown = false;
	config.enParamOverride = false;
	config.enLogging = false;
	config.enWatchdog = false;
	config.tcpPort = 0;
	config.tcpMaxConnections = 1;
	config.logDelay = 60;

	strncpy(config.logFileName, "/home/pi/mpptChgConfig.txt", MAX_STRING_LEN);

	for (i=0; i<NUM_CMDS; i++) {
		config.logMask[i] = false;
	}
	for (i=0; i<NUM_PARAMS; i++) {
		config.paramArray[i] = 0;
	}
}


int FindCmdIndex(char* regS)
{
	int i = 0;

	while (i < NUM_CMDS) {
		if (strcmp(regS, cmdList[i].cName) == 0) {
			return i;
		}
		i++;
	}

	return -1;
}


int ParseKeyHandler(void* user, const char* section, const char* name, const char* value)
{
	config_t* pconfig = (config_t*) user;
	int t;

	if (MATCH("SHUTDOWN")) {
		pconfig->enAutoShutdown = (atoi(value) != 0);
		syslog(LOG_INFO,"Config SHUTDOWN = %d", pconfig->enAutoShutdown);
	} else if (MATCH("TCP_PORT")) {
		pconfig->tcpPort = atoi(value);
		syslog(LOG_INFO,"Config TCP_PORT = %d", pconfig->tcpPort);
	} else if (MATCH("TCP_MAX")) {
		pconfig->tcpMaxConnections = atoi(value);
		if (pconfig->tcpMaxConnections > (MAX_CONNECTIONS - 1))  {
			pconfig->tcpMaxConnections = MAX_CONNECTIONS - 1;
		}
		syslog(LOG_INFO,"Config TCP_MAX = %d", pconfig->tcpMaxConnections);
	} else if (MATCH("LOG")) {
		t = FindCmdIndex((char *) value);
		if (t != -1) {
			pconfig->logMask[t] = true;
			pconfig->enLogging = true;
			syslog(LOG_INFO, "Config LOG=%s", (char *) value);
		} else {
			syslog(LOG_INFO, "Config skipping unknown LOG=%s", (char *) value);
		}
	} else if (MATCH("LOG_DELAY")) {
		pconfig->logDelay = atoi(value);
		syslog(LOG_INFO,"Config LOG_DELAY = %d", pconfig->logDelay);
	} else if (MATCH("LOG_FILE")) {
		strncpy(pconfig->logFileName, value, MAX_STRING_LEN);
		syslog(LOG_INFO,"Config LOG_FILE = %s", pconfig->logFileName);
	} else if (MATCH("BULKV")) {
		t = atoi(value);
		if ((t >= 14000) && (t <= 15000)) {
			pconfig->paramArray[PARAM_BULK_I] = t;
			pconfig->enParamOverride = true;
			syslog(LOG_INFO,"Config BULKV = %d", t);
		} else {
			syslog(LOG_INFO,"Config BULKV = %d out of range", t);
		}
	} else if (MATCH("FLOATV")) {
		t = atoi(value);
		if ((t >= 13000) && (t <= 14000)) {
			pconfig->paramArray[PARAM_FLOAT_I] = t;
			pconfig->enParamOverride = true;
			syslog(LOG_INFO,"Config FLOATV = %d", t);
		} else {
			syslog(LOG_INFO,"Config FLOATV = %d out of range", t);
		}
	} else if (MATCH("PWROFFV")) {
		t = atoi(value);
		if ((t >= 11000) && (t <= 12000)) {
			pconfig->paramArray[PARAM_PWROFF_I] = t;
			pconfig->enParamOverride = true;
			syslog(LOG_INFO,"Config PWROFFV = %d", t);
		} else {
			syslog(LOG_INFO,"Config PWROFFV = %d out of range", t);
		}
	} else if (MATCH("PWRONV")) {
		t = atoi(value);
		if ((t >= 12000) && (t <= 13000)) {
			pconfig->paramArray[PARAM_PWRON_I] = t;
			pconfig->enParamOverride = true;
			syslog(LOG_INFO,"Config PWRONV = %d", t);
		} else {
			syslog(LOG_INFO,"Config PWRONV = %d out of range", t);
		}
	} else if (MATCH("WATCHDOG")) {
		pconfig->enWatchdog = (atoi(value) != 0);
		syslog(LOG_INFO,"Config WATCHDOG = %d", pconfig->enWatchdog);
	} else {
		syslog(LOG_INFO,"Config unknown %s", name);
		return 0;
	}

	return 1;
}


bool ReadCharger(char* regS, int* val)
{
	int cmdIndex;
	int retVal;

	// Figure out what we're reading
	if ((cmdIndex = FindCmdIndex(regS)) == -1) {
		syslog(LOG_ERR, "No I2C device %s for read", regS);
		return false;
	}

	// Do the read
	if (cmdList[cmdIndex].isWord) {
		retVal = wiringPiI2CReadReg16(i2cFd, cmdList[cmdIndex].regAddr);
		if (retVal != -1) {
			// Swap bytes for endian conversion
			retVal = ((retVal & 0xFF) << 8) | ((retVal >> 8) & 0xFF);
		}
	} else {
		retVal = wiringPiI2CReadReg8(i2cFd, cmdList[cmdIndex].regAddr);
	}

	if (retVal == -1) {
		syslog(LOG_ERR, "I2C read of %s (%d) failed", regS, cmdList[cmdIndex].regAddr);
		return false;
	} else {
		if (cmdList[cmdIndex].isSigned) {
			// 2's complement to create negative int
			if (cmdList[cmdIndex].isWord) {
				if (retVal & 0x8000) {
					retVal = -(0x8000 - (retVal & 0x7FFF));
				}
			} else {
				if (retVal & 0x80) {
					retVal = -(0x80 - (retVal & 0x7F));
				}
			}
		}
		*val = retVal;
		return true;
	}
}


// Assumes we never write a negative number
bool WriteCharger(char* regS, int val)
{
	int cmdIndex;
	int retVal;

	// Figure out what we're writing
	if ((cmdIndex = FindCmdIndex(regS)) == -1) {
		syslog(LOG_ERR, "No I2C device %s for write", regS);
		return false;
	}

	// Do the write
	if (cmdList[cmdIndex].isWord) {
		retVal = wiringPiI2CWriteReg16(i2cFd, cmdList[cmdIndex].regAddr,
									   ((val >> 8) & 0xFF) | ((val & 0xFF) << 8));
	} else {
		retVal = wiringPiI2CWriteReg8(i2cFd, cmdList[cmdIndex].regAddr, val & 0xFF);
	}

	if (retVal == -1) {
		syslog(LOG_ERR, "I2C write of %s (%d) failed", regS, cmdList[cmdIndex].regAddr);
		return false;
	} else {
		return true;
	}
}


bool ConnectCharger()
{
	int s;

	// Attempt to open the interface
	i2cFd = wiringPiI2CSetup(MPPT_CHG_I2C_ADDR);
	if (i2cFd == -1) {
		syslog(LOG_ERR, "Could not open I2C interface");
		return false;
	}

	// Attempt to communicate with the charger by validating the board ID
	if (ReadCharger("ID", &s)) {
		if ((s & 0xF000) == 0x1000) {
			syslog(LOG_NOTICE, "MPPT Solar Charger FW %d.%d",
				   (s & 0x00F0) >> 4,
				   (s & 0x000F));
			return true;
		} else {
			syslog(LOG_ERR, "Charger did not identify correctly");
			return false;
		}
	} else {
		syslog(LOG_ERR, "Could not communicate with charger");
		return false;
	}
}


bool UpdateParms()
{
	bool success = true;
	int i, n, t;

	// Find the first entry
	n = FindCmdIndex("BULKV");

	// Scan for any entries that are to be modified and need updating
	for (i=0; i<NUM_PARAMS; i++) {
		if (config.paramArray[i] != 0) {
			if (ReadCharger((char *) cmdList[i+n].cName, &t)) {
				if (config.paramArray[i] != t) {
					if (!WriteCharger((char *) cmdList[i+n].cName, config.paramArray[i])) {
						success = false;
						break;
					}
				}
			} else {
				success = false;
				break;
			}
		}
	}

	return success;
}


bool EnableWatchdog()
{
	if (!WriteCharger("WDEN", WDEN_MAGIC_BYTE)) {
		return false;
	}

	if (!WriteCharger("WDCNT", WD_INIT_SECS)) {
		return false;
	}

	return true;
}


bool DisableWatchdog()
{
	if (!WriteCharger("WDEN", 0)) {
		return false;
	}

	if (!WriteCharger("WDCNT", 0)) {
		return false;
	}

	return true;
}


bool CheckAlertStatus(bool* alert)
{
	int s;

	if (ReadCharger("STATUS", &s)) {
		*alert = ((s & STATUS_ALERT_MASK) == STATUS_ALERT_MASK);

		// Handle a special case where we have on very infrequent occasion
		// seen a response of 0xFFFF for the read.  This could cause us to shutdown
		// the host without the charger cycling power which results in a hung system
		// (host won't ever reboot).  So we check again just to be sure.
		if (*alert) {
			(void) ReadCharger("STATUS", &s);
			if ((s & STATUS_ALERT_MASK) == 0) {
				*alert = false;
			}
		}
		return true;
	} else {
		*alert = false;
		return false;
	}
}


void LogValueNames()
{
	int i;
	char logS[16];

	// Print a keyword to indicate this is a list of names
	sprintf(logS, "LOGGING: ");
	write(logFd, logS, strlen(logS));

	// Print enabled value names
	for (i=0; i<NUM_CMDS; i++) {
		if (config.logMask[i]) {
			sprintf(logS, "%s ", cmdList[i].cName);
			write(logFd, logS, strlen(logS));
		}
	}
	write(logFd, "\n", 1);
}


bool LogValues(struct timeval *t)
{
	bool success = true;
	char logS[16];
	int i, n;

	// Print time
	sprintf(logS, "%d: ", t->tv_sec);
	write(logFd, logS, strlen(logS));

	// Print enabled values
	for (i=0; i<NUM_CMDS; i++) {
		if (config.logMask[i]) {
			if (ReadCharger((char *) cmdList[i].cName, &n)) {
				sprintf(logS, "%d ", n);
				write(logFd, logS, strlen(logS));
			} else {
				success = false;
				break;
			}
		}
	}
	write(logFd, "\n", 1);

	return success;
}


bool SecTick(struct timeval *prevT)
{
	struct timeval curT;
	long dT;

	gettimeofday(&curT, NULL);
	dT = (curT.tv_sec * 1000000 + curT.tv_usec) - (prevT->tv_sec * 1000000 + prevT->tv_usec);
	if (dT >= 1000000) {
		prevT->tv_sec = curT.tv_sec;
		prevT->tv_usec = curT.tv_usec;
		return(true);
	} else {
		return(false);
	}
}


int CmdKeyHandler(void* user, const char* section, const char* name, const char* value)
{
	char rspBuf[64];
	int cmdIndex;
	int success = 0;
	int t;

	if (MATCH("READ")) {
		if ((cmdIndex = FindCmdIndex((char *) value)) != -1) {
			if (ReadCharger((char *) cmdList[cmdIndex].cName, &t)) {
				sprintf(rspBuf, "%s=%d\n\r", cmdList[cmdIndex].cName, t);
				success = 1;
			}
		}
	} else {
		// Validate write
		if ((cmdIndex = FindCmdIndex((char *) name)) != -1) {
			if (cmdList[cmdIndex].isWritable) {
				t = atoi(value);
				if (WriteCharger((char *) cmdList[cmdIndex].cName, t)) {
					sprintf(rspBuf, "%s=%d\n\r", cmdList[cmdIndex].cName, t);
					success = 1;
				}
			}
		}
	}

	if (success == 1) {
		// Append response
		t = 0;
		while (rspBuf[t] != 0) {
			rspFifo[rspFifoPushI++] = rspBuf[t++];
			if (rspFifoPushI >= MAX_FIFO_LEN) rspFifoPushI = 0;
		}
	}

	return success;
}


void InitCmdBuf()
{
	int i;

	for (i=0; i<MAX_CONNECTIONS; i++) {
		cmdBufs[i].valid = false;
		cmdBufs[i].fd = 0;
		cmdBufs[i].cmdFifoPushI = 0;
		cmdBufs[i].cmdFifoPopI = 0;
		cmdBufs[i].cmdBufI = 0;
	}
}


bool AddCmdBuf(int fd)
{
	int i;

	for (i=0; i<MAX_CONNECTIONS; i++) {
		if (cmdBufs[i].valid == false) {
			// Use this entry
			cmdBufs[i].valid = true;
			cmdBufs[i].fd = fd;
			cmdBufs[i].cmdFifoPushI = 0;
			cmdBufs[i].cmdFifoPopI = 0;
			cmdBufs[i].cmdBufI = 0;
			return true;
		}
	}

	return false;
}


bool DeleteCmdBuf(int fd)
{
	int i;

	for (i=0; i<MAX_CONNECTIONS; i++) {
		if ((cmdBufs[i].valid == true) && (cmdBufs[i].fd == fd)) {
			cmdBufs[i].valid = false;
			return true;
		}
	}

	return false;
}


cmdBuf_t* GetCmdBufPtr(int fd)
{
	int i;

	for (i=0; i<MAX_CONNECTIONS; i++) {
		if ((cmdBufs[i].valid == true) && (cmdBufs[i].fd == fd)) {
			return &cmdBufs[i];
		}
	}

	return NULL;
}  


bool ProcessCmdBytes(char* buf, int numBytes, cmdBuf_t* cmd)
{
	bool success = false;
	char c;
	int i;

	// Initialize response fifo in case we will process commands
	memset(rspFifo, '\0', sizeof(rspFifo));
	rspFifoPushI = 0;

	// Push bytes into command fifo
	for (i=0; i<numBytes; i++) {
		// Push received data
		c = *(buf + i);
		cmd->cmdFifo[cmd->cmdFifoPushI++] = c;
		if (cmd->cmdFifoPushI >= MAX_FIFO_LEN) cmd->cmdFifoPushI = 0;
	}

	// Pop bytes from command fifo looking for termination character to process command
	while (cmd->cmdFifoPopI != cmd->cmdFifoPushI) {
		// Pop into our local command buffer
		c = cmd->cmdFifo[cmd->cmdFifoPopI++];
		if (cmd->cmdFifoPopI >= MAX_FIFO_LEN) cmd->cmdFifoPopI = 0;

		// Look for complete packet
		if ((c == 0x0A) || (c == 0x0D)) {
			// Process command
			cmd->cmdBuf[cmd->cmdBufI] = 0;
			cmd->cmdBufI = 0;

			success |= (ini_parse_string(cmd->cmdBuf, CmdKeyHandler, &config) == 0);
		} else {
			// Continue building command
			cmd->cmdBuf[cmd->cmdBufI++] = c;
		}
	}

	return success;
}


void SigHandler(int sig)
{
	int i;

	if ( sockFd != -1 )
		close(sockFd);
	for (i=0 ; i<curSockConnects ; i++)
		close(remotefd[i]);
	if ( linkFd != -1 )
		close(linkFd);
	if (linkname)
		unlink(linkname);
	if (config.enLogging) 
		close(logFd);
	if (config.enWatchdog) 
		(void) DisableWatchdog();
	syslog(LOG_NOTICE, "Terminating on signal %d",sig);
	exit(0);
}


bool LinkSlave(int fd)
{
	char *slavename;
	int status = grantpt(linkFd);
	if (status != -1) {
		status = unlockpt(linkFd);
	}
	if (status != -1) {
		slavename = ptsname(linkFd);
		if (slavename) {
			// Safety first
			unlink(linkname);
			status = symlink(slavename, linkname);
			status = chmod(linkname, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH);
			if (status == -1) {
				syslog(LOG_ERR, "Cannot chmod link for pseudo-tty: %m");
				exit(1);
			}
		}
		else {
			status = -1;
		}
	}

	return (status != -1);
}


void Usage(char *progname) {
	printf("mpptChgD version %0d.%0d.  Usage:\n", VERSION_MAJOR, VERSION_MINOR);
	printf("mpptChgD [-d] [-f configfile] [-x debuglevel] [-h]\n\n");

	printf("-d			Run as a daemon program\n");
	printf("-f configfile		Read the specified configuration file\n");
	printf("-x debuglevel		Set debug level, 0 is default, 1-3 give more info in log\n");
	printf("-h					  Usage\n");
}


int main(int argc, char *argv[])
{
	extern char *optarg;
	extern int optind;
	bool isdaemon = false;
	bool hasConfig = false;
	int maxfd = -1;
	int debug=0;
	char devbuf[MAX_STRING_LEN];
	int devbytes;
	struct sockaddr_in addr,remoteaddr;
	int remoteaddrlen;
	fd_set fdsread,fdsreaduse;
	int c, i;
	struct timeval select_timeout, prev_time;
	int paramTimeout, logTimeout, watchdogTimeout;
	bool alertDetected;
	cmdBuf_t* curCmdBufP;

	// Setup default values
	FD_ZERO(&fdsread);
	InitCmdBuf();
	SetupDefaultConfigValues();

	// Parse command line options
	while ( (c=getopt(argc,argv,"df:x:h")) != EOF )
		switch (c) {
		case 'd':
			isdaemon = true;
			break;
		case 'f':
			strncpy(devbuf, optarg, MAX_STRING_LEN);
			hasConfig = true;
			break;
		case 'x':
			debug = atoi(optarg);
			break;
		case 'h':
		case '?':
			Usage(argv[0]);
			exit(1);
		}

	// Open our log file so we can note info and errors
	openlog("mpptChgD", LOG_PID, LOG_USER);

	// ID ourselves
	syslog(LOG_NOTICE, "MPPT Solar Charger daemon V%d.%d", VERSION_MAJOR, VERSION_MINOR);

	// Try to connect to the charger
	if (!ConnectCharger()) {
		exit(1);
	}

	// Parse the config file if specified
	if (hasConfig) {
		if (ini_parse(devbuf, ParseKeyHandler, &config) != 0) {
			syslog(LOG_ERR, "Can't process config file %s", devbuf);
			exit(1);
		}
	}

	// Open data logging file if necessary
	if (config.enLogging) {
		if ((logFd = open(config.logFileName, O_CREAT | O_APPEND | O_WRONLY, S_IWUSR | S_IRUSR | S_IRGRP | S_IROTH)) == -1) {
			syslog(LOG_ERR, "Open of log file %s failed: %m", config.logFileName);
			exit(1);
		}
	}

	// Array for remote TCP hosts
	remotefd = (int *) malloc (config.tcpMaxConnections * sizeof(int));

	// Setup device file link
	if ((linkFd = open("/dev/ptmx", O_RDWR)) == -1) {
		syslog(LOG_ERR, "Open of /dev/ptmx failed: %m");
		exit(1);
	}
	if (!LinkSlave(linkFd)) {
		syslog(LOG_ERR, "Cannot create link for psuedo-tty: %m");
		exit(1);
	}
	(void) AddCmdBuf(linkFd);

	signal(SIGINT,SigHandler);
	signal(SIGHUP,SigHandler);
	signal(SIGTERM,SigHandler);

	if (config.tcpPort != 0) {
		/* Open the socket for communications */
		sockFd = socket(AF_INET, SOCK_STREAM, 6);
		if ( sockFd == -1 ) {
			syslog(LOG_ERR, "Can't open socket: %m");
			exit(1);
		}

		addr.sin_family = AF_INET;
		addr.sin_addr.s_addr = 0;
		addr.sin_port = htons(config.tcpPort);
	 
		/* Set up to listen on the given port */
		if( bind( sockFd, (struct sockaddr*)(&addr),
			sizeof(struct sockaddr_in)) < 0 ) {
			syslog(LOG_ERR, "Couldn't bind port %d, aborting: %m", config.tcpPort );
			exit(1);
		}
		if ( debug>1 )
			syslog(LOG_NOTICE,"Bound port");

		/* Tell the system we want to listen on this socket */
		if ( listen(sockFd, 4) == -1 ) {
			syslog(LOG_ERR, "Socket listen failed: %m");
			exit(1);
		}

		if ( debug>1 )
			syslog(LOG_NOTICE,"Done listen");
	}

	if ( isdaemon ) {
		setsid();
		close(0);
		close(1);
		close(2);
	}

	// Set up the files/sockets for the select() call 
	FD_SET(linkFd,&fdsread);
	if ( linkFd >= maxfd ) {
		maxfd = linkFd + 1;
	}

	if ( sockFd != -1 ) {
		FD_SET(sockFd,&fdsread);
		if ( sockFd >= maxfd ) {
			maxfd = sockFd + 1;
		}
	}

	// Handle any requested charger configuration
	if (config.enParamOverride) {
		if (!UpdateParms()) {
			syslog(LOG_ERR, "Parameter update failed");
			exit(1);
		}
		paramTimeout = PARAM_CHECK_SECS;
	}

	if (config.enWatchdog) {
		if (!EnableWatchdog()) {
			// Make sure watchdog is completely disabled before we bail
			(void) DisableWatchdog();
			syslog(LOG_ERR, "Watchdog enabled failed");
			exit(1);
		}
		watchdogTimeout = WD_UPDATE_SECS;
	}

	if (config.enLogging) {
		logTimeout = config.logDelay;
		LogValueNames();
	}


	// Initial timestamp for timed activities
	gettimeofday(&prev_time, NULL);

	// Main loop
	while (1) {

		// Wait for data from the listening socket, the linked,
		// device, or the remote connection or evaluate any 
		// activities on 1 second intervals 
		fdsreaduse = fdsread;
		select_timeout.tv_sec = 1;
		select_timeout.tv_usec = 0;
		if ( (c = select(maxfd,&fdsreaduse,NULL,NULL,&select_timeout)) == -1 ) {
			break;
		}

		// Activity on the socket 
		if ( config.tcpPort && FD_ISSET(sockFd,&fdsreaduse) ) {
			int fd;

			// Accept the remote systems attachment 
			remoteaddrlen = sizeof(struct sockaddr_in);
			fd = accept(sockFd,(struct sockaddr*)(&remoteaddr),
				&remoteaddrlen);

			if ( fd == -1 )
				syslog(LOG_ERR,"accept failed: %m");
			else if (curSockConnects < config.tcpMaxConnections) {
				unsigned long ip;

				remotefd[curSockConnects++] = fd;
				if (!AddCmdBuf(fd)) {
					syslog(LOG_ERR, "Internal error, did not have cmdBuf for socket");
					goto err_exit;
				}
				// Tell select to watch this new socket 
				FD_SET(fd,&fdsread);
				if ( fd >= maxfd )
					maxfd = fd + 1;
				ip = ntohl(remoteaddr.sin_addr.s_addr);
				if (debug>0)
					syslog(LOG_NOTICE, "Connection from %d.%d.%d.%d",
						(int)(ip>>24)&0xff,
						(int)(ip>>16)&0xff,
						(int)(ip>>8)&0xff,
						(int)(ip>>0)&0xff);
			}
			else {
				// Too many connections, just close it to reject
				close(fd);
			}
		}

		// Data to read from the linked device file 
		if ( FD_ISSET(linkFd,&fdsreaduse) ) {
			devbytes = read(linkFd,devbuf,MAX_STRING_LEN);
			if (debug>1)
				syslog(LOG_INFO,"%s: %d bytes", linkname, devbytes);
			if ( devbytes <= 0 ) {
				if ( debug>0 ) {
					syslog(LOG_INFO,"%s closed",linkname);
				}
				close(linkFd);
				(void) DeleteCmdBuf(linkFd);
				FD_CLR(linkFd,&fdsread);
				while (1) {
					linkFd = open("/dev/ptmx", O_RDWR);
					if ( linkFd != -1 )
						break;
					syslog(LOG_ERR, "Open of /dev/ptmx failed: %m");
					if ( errno != EIO )
						goto err_exit;
					sleep(1);
				}
				if ( debug>0 ) {
					syslog(LOG_INFO,"/dev/ptmx re-opened");
				}
				if (!LinkSlave(linkFd)) {
					syslog(LOG_ERR, "Cannot recreate link for psuedo-tty: %m");
					goto err_exit;
				}
				(void) AddCmdBuf(linkFd);
				FD_SET(linkFd,&fdsread);
				if ( linkFd >= maxfd )
					maxfd = linkFd + 1;
			}
			else {
				// Process command
				devbuf[devbytes] = 0;
				if (debug > 2) syslog(LOG_INFO, "%s received %s", linkname, devbuf);
				if ((curCmdBufP = GetCmdBufPtr(linkFd)) == NULL) {
					syslog(LOG_ERR, "Internal error, could not find cmdBuf for linkFd");
					goto err_exit;
				}
				if (ProcessCmdBytes(devbuf, devbytes, curCmdBufP)) {
					if (debug > 2) syslog(LOG_INFO, "%s sent %s", linkname, rspFifo);
					write(linkFd, rspFifo, strlen(rspFifo));
				}
			}
		}

		// Data to read from the remote system 
		for (i=0 ; i<curSockConnects ; i++) {
			if (FD_ISSET(remotefd[i],&fdsreaduse) ) {

				devbytes = read(remotefd[i],devbuf,MAX_STRING_LEN);

				if (debug>1)
					syslog(LOG_INFO,"Remote: %d bytes",devbytes);

				if ( devbytes == 0 ) {
					register int j;

					if (debug>0) {
						syslog(LOG_NOTICE,"Connection closed");
					}
					(void) DeleteCmdBuf(remotefd[i]);
					close(remotefd[i]);
					FD_CLR(remotefd[i],&fdsread);
					curSockConnects--;
					for (j=i ; j<curSockConnects ; j++) {
						remotefd[j] = remotefd[j+1];
					}
				}
				else {
					// Process command
					devbuf[devbytes] = 0;
					if (debug > 2) syslog(LOG_INFO, "Remote received %s", devbuf);
					if ((curCmdBufP = GetCmdBufPtr(remotefd[i])) == NULL) {
						syslog(LOG_ERR, "Internal error, could not find cmdBuf for socket fd %d", remotefd[i]);
						goto err_exit;
					}
					if (ProcessCmdBytes(devbuf, devbytes, curCmdBufP)) {
						if (debug > 2) syslog(LOG_INFO, "Remote sent %s", rspFifo);
						write(remotefd[i], rspFifo, strlen(rspFifo));
					}
				}
			}
		}

		// Check for activities to do on second boundaries
		//   prev_time 
		if (SecTick(&prev_time)) {
			if (config.enAutoShutdown) {
				if (!CheckAlertStatus(&alertDetected)) {
					goto err_exit;
				}
				if (alertDetected) {
					syslog(LOG_CRIT, "Low Battery shutdown");
					system("sudo shutdown now");
				}
			}
			if (config.enParamOverride) {
				if (--paramTimeout == 0) {
					paramTimeout = PARAM_CHECK_SECS;
					if (!UpdateParms()) {
						goto err_exit;
					}
				}
			}
			if (config.enLogging) {
				if (--logTimeout == 0) {
					logTimeout = config.logDelay;
					if (!LogValues(&prev_time)) {
						goto err_exit;
					}
				}
			}
			if (config.enWatchdog) {
				if (--watchdogTimeout == 0) {
					watchdogTimeout = WD_UPDATE_SECS;
					if (!EnableWatchdog()) {
						goto err_exit;
					}
				}
			}
		}
	}

err_exit:
	// We normally only exit from a signal (via the signal handler) so this
	// is used for an  error exit
	close(sockFd);
	for (i=0 ; i<curSockConnects ; i++)
		close(remotefd[i]);
	close(linkFd);
	if (config.enLogging) 
		close(logFd);
	if (config.enWatchdog) 
		(void) DisableWatchdog();
	exit(1);
}
