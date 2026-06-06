#include "connect.h"

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
#include "task.h"


#define NEI_CONFIG_CONN_HOST      "connection.host"
#define NEI_CONFIG_CONN_PORT      "connection.port"
#define NEI_CONN_MAX_CONN_RETRIES 5

#define NEI_DELAY_1S pdMS_TO_TICKS(1000)

/// @brief Swaps a and b
/// @note Avoid to put rvalues and costly evaluations, as parameters could be
/// multiply evaluated
#define swap(a, b)                                                             \
    do {                                                                       \
        typeof(a) _swap_tmp = (a);                                             \
        (a)                 = (b);                                             \
        (b)                 = _swap_tmp;                                       \
    } while (0)


int NEI_ArkeoAPI_ConnectionInit(NEI_ArkeoAPI_Connection *apiConn,
    const char *host, uint16_t port) {
    if (!apiConn || !host) {
        return -1;
    }

    unsigned int len = strlen(host);
    if (len > NEI_CONN_HOST_MAX_LEN) {
        return -1;
    } else {
        memcpy(apiConn->host, host, len + 1);
        apiConn->port      = port;
        apiConn->tcpSocket = -1;
        return 0;
    }
}

int NEI_ArkeoAPI_ConnectionInitFromJSON(NEI_ArkeoAPI_Connection *apiConn,
    char *json, size_t len) {
    if (!apiConn) {
        return -1;
    }

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
        result = NEI_ArkeoAPI_ConnectionInit(apiConn, value, port);
        swap(charSave, value[valueLen]);
    }

    return result;
}

int NEI_ArkeoAPI_ConnectionEstablish(NEI_ArkeoAPI_Connection *apiConn) {
    if (!apiConn) {
        return -1;
    }

    // We need the port as a string
    char alphaPort[6];
    sniprintf(alphaPort, sizeof(alphaPort), "%u", apiConn->port);

    struct addrinfo *target,
        hints = {.ai_family = AF_INET, .ai_socktype = SOCK_STREAM};

    // Try resolving the host name & port
    if (getaddrinfo(apiConn->host, alphaPort, &hints, &target) != 0) {
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

    if (socketFd < 0) {
        return -1;
    } else {
        apiConn->tcpSocket = socketFd;
        return 0;
    }
}


void NEI_testConnection(void) {
    NEI_ArkeoAPI_Connection arkeo =
        NEI_ArkeoAPI_ConnectionInit_Static("192.168.100.1", 6340);

    if (NEI_ArkeoAPI_ConnectionEstablish(&arkeo) == 0) {
        close(arkeo.tcpSocket);
    } else {
        vTaskDelay(NEI_DELAY_1S);
    }
}
