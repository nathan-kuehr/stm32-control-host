#ifndef __NEI_CONNECT_H__
#define __NEI_CONNECT_H__

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define NEI_CONN_HOST_MAX_LEN 20

/// @brief Represets a connection via TCP to the API provider
typedef struct NEI_ArkeoAPI_Connection {
    char host[NEI_CONN_HOST_MAX_LEN + 1];
    uint16_t port;
    int tcpSocket;
} NEI_ArkeoAPI_Connection;

/// @brief Initialises an API connection structure
/// @param apiConn Connection handle to be initialised
/// @param host Hostname to set
/// @param port Port to set
/// @return 0 on success, -1 on error
int NEI_ArkeoAPI_ConnectionInit(NEI_ArkeoAPI_Connection *apiConn,
    const char *host, uint16_t port);
/// @brief Initialises an API connection structure statically
/// @param apiConn Connection handle to be initialised
/// @param host Hostname to set
/// @param port Port to set
/// @return 0 on success, -1 on error
#define NEI_ArkeoAPI_ConnectionInit_Static(host, port) {host, port, -1}


/// @brief Initialises an API connection structure from a JSON
/// @param apiConn Connection handle to be initialised
/// @param json The buffer containing the JSON. Must contain "connection.host"
/// and "connection.port" as keys.
/// @param len The length of @p json
/// @return 0 on success, -1 on error
int NEI_ArkeoAPI_ConnectionInitFromJSON(NEI_ArkeoAPI_Connection *apiConn,
    char *json, size_t len);

int NEI_ArkeoAPI_ConnectionEstablish(NEI_ArkeoAPI_Connection *apiConn);

inline bool NEI_ArkeoAPI_ConnectionIsEstablished(
    const NEI_ArkeoAPI_Connection *apiConn) {
    return apiConn && (apiConn->tcpSocket >= 0);
}

#endif //__NEI_CONNECT_H__
