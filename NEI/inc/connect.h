#ifndef __NEI_CONNECT_H__
#define __NEI_CONNECT_H__

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define NEI_CONN_HOST_MAX_LEN 20

/// @brief Represets a connection via TCP to the API provider
typedef struct NEI_ArkeoAPI_Client {
    char host[NEI_CONN_HOST_MAX_LEN + 1];
    uint16_t port;
    int tcpSocket;
} NEI_ArkeoAPI_Client;

/// @brief Initialises an API client structure
/// @param client Client handle to be initialised
/// @param host Hostname to set
/// @param port Port to set
/// @return 0 on success, -1 on error
int NEI_ArkeoAPI_ClientInit(NEI_ArkeoAPI_Client *client, const char *host,
    uint16_t port);
/// @brief Initialises an API client structure statically
/// @param host Hostname to set
/// @param port Port to set
/// @return 0 on success, -1 on error
#define NEI_ArkeoAPI_ClientInitStatic(host, port) {host, port, -1}


/// @brief Initialises an API client structure from a JSON
/// @param client Client handle to be initialised
/// @param json The buffer containing the JSON. Must contain "connection.host"
/// and "connection.port" as keys.
/// @param len The length of @p json
/// @return 0 on success, -1 on error
int NEI_ArkeoAPI_ClientInitFromJSON(NEI_ArkeoAPI_Client *client, char *json,
    size_t len);

/// @brief Establishes the connection with details given in @p client
/// @return 0 on success, otherwise -1
int NEI_ArkeoAPI_connect(NEI_ArkeoAPI_Client *client);
/// @brief Closes an open connection attributed to @p client
/// @return 0 on success, otherwise -1
int NEI_ArkeoAPI_disconnect(NEI_ArkeoAPI_Client *client);
/// @brief Checks if @p client is successfully connected
inline static bool NEI_ArkeoAPI_isConnected(const NEI_ArkeoAPI_Client *client) {
    return client && !(client->tcpSocket < 0);
}

#endif //__NEI_CONNECT_H__
