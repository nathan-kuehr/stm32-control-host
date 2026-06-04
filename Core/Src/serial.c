#include "serial.h"

#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h> // for std stream file descr

#include "portmacro.h"
#include "stm32f7xx_hal.h"
#include "usart.h"
#include "stream_buffer.h"

#define NEI_BUFSIZE_256B 0x100
#define NEI_BUFSIZE_512B 0x200
#define NEI_BUFSIZE_1KB  0x400

/// @brief DMA Buffer for UART sending
static uint8_t NEI_SerialTxBuffer[NEI_BUFSIZE_256B]
    __attribute__((section(".UART_DMA_Section")));
/// @brief DMA Buffer for UART receiving
static uint8_t NEI_SerialRxBuffer[NEI_BUFSIZE_512B]
    __attribute__((section(".UART_DMA_Section")));

/// @brief Binary semaphore to sync DMA buffer access
extern SemaphoreHandle_t NEI_SerialTxBufferSemHandle;

/// @brief Stream buffer between _write and the IO task
static StreamBufferHandle_t NEI_SerialTxStreamBuffer;
/// @brief Stream buffer between _read and the IO task
static StreamBufferHandle_t NEI_SerialRxStreamBuffer;

/// @brief Read position in the RxBuffer
static uint16_t NEI_SerialRxPos = 0;

// Mutexes protecting the stream buffers
extern SemaphoreHandle_t NEI_SerialTxStreamBufferMtxHandle;
extern SemaphoreHandle_t NEI_SerialRxStreamBufferMtxHandle;

/// @brief Initialization state of the serial functionality
static bool NEI_SerialIsInit = false;


void NEI_SerialInit(void) {
    // Stream buffers between _write/_read and the IO task
    NEI_SerialTxStreamBuffer = xStreamBufferCreate(NEI_BUFSIZE_1KB, 1);
    NEI_SerialRxStreamBuffer = xStreamBufferCreate(NEI_BUFSIZE_1KB, 1);
    if (NEI_SerialTxStreamBuffer == NULL || NEI_SerialRxStreamBuffer == NULL) {
        return;
    }

    // Start circular RX DMA with idle line detect
    if (HAL_UARTEx_ReceiveToIdle_DMA(&huart3, NEI_SerialRxBuffer,
            sizeof NEI_SerialRxBuffer)
        != HAL_OK) {
        return;
    }

    // Forbid buffering as stream buffers with IO task hold 1 KB
    if (setvbuf(stdin, NULL, _IONBF, 0) != 0) {
        return;
    }
    if (setvbuf(stdout, NULL, _IONBF, 0) != 0) {
        return;
    }
    if (setvbuf(stderr, NULL, _IONBF, 0) != 0) {
        return;
    }

    NEI_SerialIsInit = true;
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart) {
    if (huart == &huart3) {
        BaseType_t woken = false;
        xSemaphoreGiveFromISR(NEI_SerialTxBufferSemHandle, &woken);
        portYIELD_FROM_ISR(woken);
    }
}

void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t writePos) {
    if (huart == &huart3) {
        // Event is either HT, TC, or IDLE -> in all cases push data into pipe
        BaseType_t woken1 = false, woken2 = false;

        int32_t rxHeadDiff = (int32_t) writePos - NEI_SerialRxPos;
        if (rxHeadDiff == 0) {
            return;
        } else if (rxHeadDiff > 0) { // write head lays right of read head
            xStreamBufferSendFromISR(NEI_SerialRxStreamBuffer,
                &NEI_SerialRxBuffer[NEI_SerialRxPos], rxHeadDiff, &woken1);
        } else {
            xStreamBufferSendFromISR(NEI_SerialRxStreamBuffer,
                &NEI_SerialRxBuffer[NEI_SerialRxPos],
                sizeof(NEI_SerialRxBuffer) - NEI_SerialRxPos, &woken1);
            xStreamBufferSendFromISR(NEI_SerialRxStreamBuffer,
                NEI_SerialRxBuffer, writePos, &woken2);
        }
        NEI_SerialRxPos = writePos;

        portYIELD_FROM_ISR(woken1 || woken2);
    }
}

void NEI_SerialTaskMain(const void *argument) {
    NEI_SerialInit();

    size_t lenTxBytes;
    while (NEI_SerialIsInit) {
        xSemaphoreTake(NEI_SerialTxBufferSemHandle, portMAX_DELAY);

        lenTxBytes = xStreamBufferReceive(NEI_SerialTxStreamBuffer,
            NEI_SerialTxBuffer, sizeof NEI_SerialTxBuffer, portMAX_DELAY);

        if (lenTxBytes
            && (HAL_UART_Transmit_DMA(&huart3, NEI_SerialTxBuffer, lenTxBytes)
                != HAL_OK)) {
            xSemaphoreGive(NEI_SerialTxBufferSemHandle);
        }
    }
    vTaskDelete(NULL);
}

int _write(int fd, const char *buf, int len) {
    TickType_t maxBlockingDelay;

    switch (fd) {
    case STDOUT_FILENO:
        maxBlockingDelay = portMAX_DELAY;
        break;
    case STDERR_FILENO:
        maxBlockingDelay = 0;
        break;
    default:
        errno = EBADF;
        return -1;
    }

    int sentBytes = -1;
    if (NEI_SerialIsInit) {
        xSemaphoreTake(NEI_SerialTxStreamBufferMtxHandle, portMAX_DELAY);
        sentBytes = xStreamBufferSend(NEI_SerialTxStreamBuffer, buf, len,
            maxBlockingDelay);
        xSemaphoreGive(NEI_SerialTxStreamBufferMtxHandle);
    }
    return sentBytes;
}

int _read(int file, char *ptr, int len) {
    if (file != STDIN_FILENO) {
        errno = EBADF;
        return -1;
    }

    int receivedBytes = -1;
    if (NEI_SerialIsInit) {
        xSemaphoreTake(NEI_SerialRxStreamBufferMtxHandle, portMAX_DELAY);
        receivedBytes = xStreamBufferReceive(NEI_SerialRxStreamBuffer, ptr, len,
            portMAX_DELAY);
        xSemaphoreGive(NEI_SerialRxStreamBufferMtxHandle);
    }
    return receivedBytes;
}
