#include "comm.h"
#include "def.h"
#include "sockets.h"

#include <stdint.h>
#include <string.h>
#include <sys/_timeval.h>
#include <sys/select.h>

/// @brief Standard message buffer
static char NEI_ArkeoAPI_MessageBuffer[NEI_ARKEOAPI_MESSAGE_BUFSIZE] = {0};

/// @brief Main Arkeo API client to establish connections
static NEI_ArkeoAPI_Client NEI_ArkeoAPI_ClientHandle =
    NEI_ArkeoAPI_ClientInitStatic("192.168.100.1", 6340);

struct timeval NEI_ArkeoAPI_IOTimeout = {.tv_sec = 5};

char *NEI_ArkeoAPI_getMessageBuffer(void) {
    return NEI_ArkeoAPI_MessageBuffer;
}
NEI_ArkeoAPI_Client *NEI_ArkeoAPI_getClient(void) {
    return &NEI_ArkeoAPI_ClientHandle;
}

int NEI_ArkeoAPI_resolveWriteBuf(char **buf, size_t *size) {
    if (!*buf) {
        *buf  = NEI_ArkeoAPI_getMessageBuffer();
        *size = NEI_ARKEOAPI_MESSAGE_BUFSIZE;
    } else if (!*size) {
        return -1; // Non-standard buffer w/ 0 size given
    }
    return 0;
}
void NEI_ArkeoAPI_resolveReadBuf(const char **buf, size_t *len) {
    if (!*buf) {
        *buf = NEI_ArkeoAPI_getMessageBuffer();
        *len = strnlen(*buf, NEI_ARKEOAPI_MESSAGE_BUFSIZE);
    } else if (!*len) {
        *len = strlen(*buf);
    }
}

int NEI_ArkeoAPI_setSocketOptions(int tcpSocket) {
    if (setsockopt(tcpSocket, SOL_SOCKET, SO_RCVTIMEO, &NEI_ArkeoAPI_IOTimeout,
            sizeof(struct timeval))
        != 0) {
        return -1;
    }
    if (setsockopt(tcpSocket, SOL_SOCKET, SO_SNDTIMEO, &NEI_ArkeoAPI_IOTimeout,
            sizeof(struct timeval))
        != 0) {
        return -1;
    }
    return 0;
}

/// @brief Sends @p nBytes starting from position @p pos
/// @return 0 on success, -1 on failure
static int NEI_ArkeoAPI_send(uint8_t *pos, size_t nBytes) {
    if (!NEI_ArkeoAPI_ClientIsEstablished(&NEI_ArkeoAPI_ClientHandle)) {
        return -1;
    }
    uint8_t *cur       = pos;
    uint8_t *const end = pos + nBytes;

    ssize_t sentBytes;
    while (cur < end) {
        sentBytes =
            send(NEI_ArkeoAPI_ClientHandle.tcpSocket, cur, end - cur, 0);
        if (sentBytes <= 0) {
            return -1;
        }
        cur += sentBytes;
    }
    return 0;
}

/// @brief Receive @p nBytes and write them starting from @p pos
/// @return 0 on success, -1 if an error or timeout occurred
static int NEI_ArkeoAPI_receive(uint8_t *pos, size_t nBytes) {
    if (!NEI_ArkeoAPI_ClientIsEstablished(&NEI_ArkeoAPI_ClientHandle)) {
        return -1;
    }
    uint8_t *cur       = pos;
    uint8_t *const end = pos + nBytes;

    ssize_t readBytes;
    while (cur < end) {
        readBytes = recv(NEI_ArkeoAPI_ClientHandle.tcpSocket, cur, end - cur,
            MSG_WAITALL);

        if (readBytes <= 0) {
            if (readBytes == 0) {
                // Connection closed
                NEI_ArkeoAPI_disconnect(&NEI_ArkeoAPI_ClientHandle);
            }
            return -1;
        }
        cur += readBytes;
    }
    return 0;
}

/// @brief Sends a frame of data to Arkeo
/// @param buf The array of the data to send. If NULL, the standard message
/// buffer is used
/// @param len The amount of data to send. If NULL, strlen is used to find it.
/// @return 0 on success, -1 otherwise
int NEI_ArkeoAPI_sendFrame(const char *buf, size_t msgLen) {
    uint32_t lenEncoded;

    NEI_ArkeoAPI_resolveReadBuf(&buf, &msgLen);
    lenEncoded = htonl(msgLen);

    if (NEI_ArkeoAPI_send((uint8_t *) &lenEncoded, sizeof(lenEncoded)) < 0) {
        return -1;
    }
    if (NEI_ArkeoAPI_send(buf, msgLen) < 0) {
        return -1;
    }
    return 0;
}

/// @brief Receives a frame of data from Arkeo
/// @param buf The buffer to receive it in. If NULL, the standard message buffer
/// is used
/// @param bufSize The size of @p buf. If @p buf is NULL, it is ignored
/// @return The length of the received data on success, -1 otherwise
/// @note In case of error, @p buf contains an empty string
long NEI_ArkeoAPI_receiveFrame(char *buf, size_t size) {
    uint32_t lenEncoded;

    if (NEI_ArkeoAPI_resolveWriteBuf(&buf, &size) < 0) {
        return -1;
    }

    if (NEI_ArkeoAPI_receive((uint8_t *) &lenEncoded, sizeof(lenEncoded)) < 0) {
        return -1;
    }
    uint32_t len = ntohl(lenEncoded);

    // Need one byte left over for the terminating '\0' written below.
    if (len == 0 || len >= size || len > LONG_MAX) {
        return -1;
    }

    if (NEI_ArkeoAPI_receive(buf, len) < 0) {
        buf[0] = '\0';
        return -1;
    } else {
        buf[len] = '\0';
        return len;
    }
}
