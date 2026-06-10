#include "connect.h"
#include "defines.h"

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <socket.h>

#include "core_json.h"
#include "netdb.h"
#include "FreeRTOS.h"
#include "projdefs.h"
#include "sockets.h"
#include "task.h"


#define NEI_CONFIG_CONN_HOST      "connection.host"
#define NEI_CONFIG_CONN_PORT      "connection.port"
#define NEI_CONN_MAX_CONN_RETRIES 5

int NEI_ArkeoAPI_ClientInit(NEI_ArkeoAPI_Client *client, const char *host,
    uint16_t port) {
    if (!client || !host) return -1;

    unsigned int len = strlen(host);
    if (len > NEI_CONN_HOST_MAX_LEN) {
        return -1;
    } else {
        memcpy(client->host, host, len + 1);
        client->port      = port;
        client->tcpSocket = -1;
        return 0;
    }
}

int NEI_ArkeoAPI_ClientInitFromJSON(NEI_ArkeoAPI_Client *client, char *json,
    size_t len) {
    if (!client) return -1;

    // Value from queried keys
    char *value;
    size_t valueLen;

    char charSave = '\0';
    unsigned long port;
    int result = -1;

    JSONStatus_t jsonResult = JSON_Validate(json, len);
    if (jsonResult == JSONSuccess) {
        jsonResult = JSON_Search(json, len, NEI_CONFIG_CONN_PORT,
            sizeof(NEI_CONFIG_CONN_PORT) - 1, &value, &valueLen);
    }
    if (jsonResult == JSONSuccess) {
        swap(charSave, value[valueLen]); // Swaps in a '\0'
        port = strtoul(value, NULL, 10);
        swap(charSave, value[valueLen]); // Restore orig. char

        if (port > UINT16_MAX) {
            jsonResult = JSONIllegalDocument;
        } else {
            jsonResult = JSON_Search(json, len, NEI_CONFIG_CONN_HOST,
                sizeof(NEI_CONFIG_CONN_HOST) - 1, &value, &valueLen);
        }
    }
    if (jsonResult == JSONSuccess) {
        swap(charSave, value[valueLen]);
        result = NEI_ArkeoAPI_ConnectionInit(client, value, port);
        swap(charSave, value[valueLen]);
    }
    return result;
}

int NEI_ArkeoAPI_connect(NEI_ArkeoAPI_Client *client) {
    if (!client) return -1;

    // We need the port as a string
    char alphaPort[6];
    sniprintf(alphaPort, sizeof(alphaPort), "%u", client->port);

    struct addrinfo *target,
        hints = {.ai_family = AF_INET, .ai_socktype = SOCK_STREAM};

    // Try resolving the host name & port
    if (getaddrinfo(client->host, alphaPort, &hints, &target) != 0) {
        return -1;
    }

    // target is a linked list -> walk it and try to connect
    int socketFd = -1;
    for (struct addrinfo *cur = target; cur != NULL; cur = cur->ai_next) {
        socketFd = socket(cur->ai_family, cur->ai_socktype, cur->ai_protocol);
        if (socketFd < 0) {
            continue;
        }
        if (connect(socketFd, cur->ai_addr, cur->ai_addrlen) == 0) {
            break; // Success
        }
        close(socketFd);
        socketFd = -1;
    }
    freeaddrinfo(target);

    if (socketFd >= 0) {
        if (NEI_ArkeoAPI_setSocketOptions(socketFd) == 0) {
            client->tcpSocket = socketFd;
            return 0;
        } else {
            close(socketFd);
        }
    }
    return -1;
}

int NEI_ArkeoAPI_disconnect(NEI_ArkeoAPI_Client *client) {
    if (!client) {
        return -1;
    } else {
        if (NEI_ArkeoAPI_isConnected(client)) {
            close(client->tcpSocket);
        }
        return 0;
    }
}
