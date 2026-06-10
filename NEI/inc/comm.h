#ifndef __NEI_COMM_H__
#define __NEI_COMM_H__

#include "connect.h"
#include "defines.h"

#include <stddef.h>

#define NEI_ARKEOAPI_MESSAGE_BUFSIZE NEI_BUFSIZE_2KB

/// @brief Gets the handle to the standard API message buffer
/// @note The buffer is of size NEI_ARKEOAPI_MESSAGE_BUFSIZE
char *NEI_ArkeoAPI_getMessageBuffer(void);

/// @brief Gets the client for establishing connections with the Arkeo API
NEI_ArkeoAPI_Client *NEI_ArkeoAPI_getClient(void);

/// @brief Helper function. Provides the standard message buffer & capacity if
/// NULL is passed to @p *buf. In that case, sets @p *size accordingly. If NULL
/// is not passed to @p buf, simply checks that @p *size > 0.
/// @return 0 on success, -1 otherwise
int NEI_ArkeoAPI_resolveWriteBuf(char **buf, size_t *size);
/// @brief Helper function. Provides the standard message buffer & message
/// length if NULL is passed to @p *buf (assuming NULL-terminated). If @p *buf
/// != NULL, updates @p *size if its zero using strlen (again, assuming
/// NULL-terminated string)
void NEI_ArkeoAPI_resolveReadBuf(const char **buf, size_t *len);

/// @brief Configures @p tcpSocket for the Arkeo API, mainly the timeout
/// @return 0 on success, otherwise -1
int NEI_ArkeoAPI_setSocketOptions(int tcpSocket);

/// @brief Sends a frame to the active Arkeo client
/// @param buf Buffer to send data from. If NULL, the standard message buffer is
/// used
/// @param msgLen Amount of data to send. If 0, a NULL-terminated string is
/// assumed and the length determimed with strlen()
/// @return 0 on success, otherwise -1
int NEI_ArkeoAPI_sendFrame(const char *buf, size_t msgLen);
/// @brief Receives a frame from the active Arkeo client
/// @param buf Buffer to store the data. If NULL, the standard message buffer is
/// used
/// @param size Capacity of the buffer and must be > 0. Overwritten if NULL is
/// passed to @p buf
/// @return The size of the received frame, otherwise -1
long NEI_ArkeoAPI_receiveFrame(char *buf, size_t size);

#endif //__NEI_COMM_H__
