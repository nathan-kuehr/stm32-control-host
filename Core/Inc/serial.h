#ifndef  __SERIAL_H__
#define  __SERIAL_H__

#include <stdlib.h>
#include <stdint.h>

#include "cmsis_os.h"

extern osMutexId serialWriteMutexHandle;

void serialWriteBlockingInit(void);

void serialWriteBlocking(const char * msg, uint16_t len);

#endif // __SERIAL_H__