#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include "mpptChg.h"

mpptChg chg;


void printStatus(uint16_t s)
{
  switch (s & MPPT_CHG_STATUS_CHG_ST_MASK) {
    case MPPT_CHG_ST_NIGHT:
      printf("NIGHT  ");
      break;
    case MPPT_CHG_ST_IDLE:
      printf("IDLE   ");
      break;
    case MPPT_CHG_ST_VSRCV:
      printf("VSRCV  ");
      break;
    case MPPT_CHG_ST_SCAN:
      printf("SCAN   ");
      break;
    case MPPT_CHG_ST_BULK:
      printf("BULK   ");
      break;
    case MPPT_CHG_ST_ABSORB:
      printf("ABSORB ");
      break;
    case MPPT_CHG_ST_FLOAT:
      printf("FLOAT  ");
      break;
    default:
      printf("???    ");
  }
}


int main()
{
	uint16_t s;
	int16_t vs, is, vb, ib, ic;

	if (!chg.begin()) {
		printf("begin failed\n");
		return(-1);
	}

	if (chg.getStatusValue(SYS_ID, &s)) {
		printf("ID = 0x%4x\n", s);
	} else {
		printf("getStatusValue ID failed\n");
		return(-1);
	}

	while (1) {
		(void) chg.getStatusValue(SYS_STATUS, &s);
		(void) chg.getIndexedValue(VAL_VS, &vs);
		(void) chg.getIndexedValue(VAL_IS, &is);
		(void) chg.getIndexedValue(VAL_VB, &vb);
		(void) chg.getIndexedValue(VAL_IB, &ib);
		(void) chg.getIndexedValue(VAL_IC, &ic);

		printStatus(s);
		printf("%5d %5d %5d %5d %5d\n", vs, is, vb, ib, ic);

		sleep(1);
	}

	return(0);
}

