#include "codec.h"
#include "comm.h"
#include "defines.h"

#include <errno.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "core_json.h"


bool NEI_ArkeoAPI_RequestIsValid(const NEI_ArkeoAPI_Request *request) {
    if (request && request->command) {

        if ((request->indices == NULL) && (request->nIndices != 0)) {
            return false;
        }
        // TODO
        return true;
    }
    return false;
}


static int NEI_ArkeoAPI_compilef(char **writePos, const char *endPos,
    const char *fmt, ...) {
    long remaining = endPos - *writePos;
    if (remaining <= 0) {
        return -1;
    }

    va_list args;
    va_start(args, fmt);
    int n = vsnprintf(*writePos, (size_t) remaining, fmt, args);
    va_end(args);

    if (n < 0 || n >= remaining) {
        return -1;
    }
    *writePos += n;
    return 0;
}

long NEI_ArkeoAPI_compileRequest(const NEI_ArkeoAPI_Request *request, char *buf,
    size_t size) {
    if (!NEI_ArkeoAPI_RequestIsValid(request)) {
        return -1;
    }

    // Switch to standard buf if NULL
    if (NEI_ArkeoAPI_resolveWriteBuf(&buf, &size) < 0) {
        return -1;
    }

    char *cur       = buf;
    char *const end = buf + min(size, LONG_MAX);

    if (NEI_ArkeoAPI_compilef(&cur, end, "{ \"command\": \"%s\"",
            request->command)
        != 0) {
        return -1;
    }

    if (request->nIndices) {
        if (NEI_ArkeoAPI_compilef(&cur, end, ", \"indices\": [") != 0) {
            return -1;
        }

        for (size_t i = 0; i < request->nIndices; i++) {
            const char *fmt = (i == request->nIndices - 1) ? "%d]" : "%d,";
            if (NEI_ArkeoAPI_compilef(&cur, end, fmt, request->indices[i])
                != 0) {
                return -1;
            }
        }
    }

    if (request->payload) {
        if (NEI_ArkeoAPI_compilef(&cur, end, ", \"parameter\": { %s}",
                request->payload)
            != 0) {
            return -1;
        }
    }

    if (NEI_ArkeoAPI_compilef(&cur, end, " }") != 0) {
        return -1;
    }

    const long len = cur - buf;
    return (JSON_Validate(buf, len) == JSONSuccess) ? len : -1;
}

int NEI_ArkeoAPI_parseResponse(NEI_ArkeoAPI_Response *response, char *buf,
    size_t msgLen) {
    if (!response) {
        return -1;
    }

    //
    NEI_ArkeoAPI_resolveReadBuf(&buf, &msgLen);

    //
    char charSave = '\0';
    char *value;
    size_t valueLen;

    JSONStatus_t result;
    result = JSON_Validate(buf, msgLen);

    if (result == JSONSuccess) {
        result = JSON_Search(buf, msgLen, "status", sizeof("status") - 1,
            &value, &valueLen);
    }
    if (result == JSONSuccess) {
        swap(value[valueLen], charSave);

        if (strcmp(value, "ok") == 0) {
            response->status = NEI_ARKEOAPI_OK;
        } else if (strcmp(value, "error") == 0) {
            response->status = NEI_ARKEOAPI_ERROR;
        } else {
            result = JSONIllegalDocument;
        }

        swap(value[valueLen], charSave);
    }
    if (result != JSONSuccess) {
        return -1;
    }

    if (response->status == NEI_ARKEOAPI_OK) {
        static const char *const payloadFields[] = {"channels", "sensors",
            "environments", "day_night"};
        static const size_t nPayloadFields =
            sizeof(payloadFields) / sizeof(*payloadFields);

        size_t i = 0;
        do {
            result = JSON_Search(buf, msgLen, payloadFields[i],
                strlen(payloadFields[i]), &value, &valueLen);
        } while ((result != JSONSuccess) && (++i < nPayloadFields));

        if (result == JSONSuccess) {
            response->body.payload = value;
            response->body.length  = valueLen;
            response->body.domain  = payloadFields[i];

            return 0;
        } else {
            return -1;
        }
    } else {
        uint32_t errorCode;
        result = JSON_Search(buf, msgLen, "error.code",
            sizeof("error.code") - 1, &value, &valueLen);

        if (result == JSONSuccess) {
            errno = 0;
            char *end;

            swap(value[valueLen], charSave);
            errorCode = strtoul(value, &end, 10);
            swap(value[valueLen], charSave);

            if (errno != 0 || end == value) {
                result = JSONIllegalDocument;
            } else {
                result = JSON_Search(buf, msgLen, "error.message",
                    sizeof("error.message") - 1, &value, &valueLen);
            }
        }
        if (result == JSONSuccess) {
            response->error.message = value;
            response->error.length  = valueLen;
            response->error.code    = errorCode;

            return 0;
        } else {
            return -1;
        }
    }
}

int NEI_ArkeoAPI_sendRequest(const NEI_ArkeoAPI_Request *request, char *buf,
    size_t size) {
    if (!request) {
        return -1;
    }

    if (NEI_ArkeoAPI_resolveWriteBuf(&buf, &size) < 0) {
        return -1;
    }

    const long msgLen = NEI_ArkeoAPI_compileRequest(request, buf, size);
    if (msgLen < 0) {
        return -1;
    }

    return NEI_ArkeoAPI_sendFrame(buf, msgLen);
}

int NEI_ArkeoAPI_receiveResponse(NEI_ArkeoAPI_Response *response, char *buf,
    size_t size) {
    if (!response) {
        return -1;
    }

    if (NEI_ArkeoAPI_resolveWriteBuf(&buf, &size) < 0) {
        return -1;
    }

    const long msgLen = NEI_ArkeoAPI_receiveFrame(buf, size);
    if (msgLen < 0) {
        return -1;
    }

    return NEI_ArkeoAPI_parseResponse(response, buf, msgLen);
}
