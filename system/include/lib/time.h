#ifndef _SYSTIME_H_
#define _SYSTIME_H_

#include <cmsis_device.h>

typedef struct {
	void	 (*Delay)(int ms);
	uint64_t (*GetTick)(void);
} SysTime_Op;

void SysTime_Delay(int ms);
uint64_t SysTime_GetTick(void);
void SysTime_Init(SysTime_Op *op);

#endif