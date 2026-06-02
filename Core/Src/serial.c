#include "serial.h"

#include "usart.h"

#include <assert.h>
#include <stdio.h>
#include <stdbool.h>


static bool __SerialWriteBlockingIsInit = false;

void serialWriteBlockingInit(void) {
    if (serialWriteMutexHandle != NULL && !__SerialWriteBlockingIsInit) {
        // Unbuffered output
        setvbuf(stdout, NULL, _IONBF, 0);
        setvbuf(stderr, NULL, _IONBF, 0);

        __SerialWriteBlockingIsInit = true;
    }
}

void serialWriteBlocking(const char * msg, uint16_t len) {
    if (__SerialWriteBlockingIsInit) {
        xSemaphoreTake(serialWriteMutexHandle, portMAX_DELAY);
        HAL_UART_Transmit(&huart3, (uint8_t*) msg, len * sizeof(char), HAL_MAX_DELAY);
        xSemaphoreGive(serialWriteMutexHandle);
    } else {
        // DEBUG HERE
    }
}

int _write(int fd, char *buf, int len) {
    serialWriteBlocking(buf, len);
    return len;
}