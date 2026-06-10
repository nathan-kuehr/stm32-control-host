#ifndef __NEI_CODEC_H__
#define __NEI_CODEC_H__

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/// @brief A structured API request
typedef struct NEI_ArkeoAPI_Request {
    const char *command;
    const int *indices;
    size_t nIndices;
    const char *payload;
} NEI_ArkeoAPI_Request;

/// @brief Arkeo API response status
typedef enum NEI_ArkeoAPI_Status {
    NEI_ARKEOAPI_OK,
    NEI_ARKEOAPI_ERROR
} NEI_ArkeoAPI_Status;

/// @brief A structured API response
/// @note CAVEAT: The contained strings are NOT NULL-terminated (therefore the
/// length parameters), and are valid only during the lifetime of the data in
/// the receiving buffer.
typedef struct NEI_ArkeoAPI_Response {
    NEI_ArkeoAPI_Status status;
    union {
        /// @brief The structure of the response if status == NEI_ARKEOAPI_ERROR
        struct {
            const char *message;
            size_t length;
            uint32_t code;
        } error;
        /// @brief The structure of the response if status == NEI_ARKEOAPI_OK
        struct {
            const char *payload;
            size_t length;
            const char *domain; //< NULL-terminated
        } body;
    };
} NEI_ArkeoAPI_Response;

/// @brief Checks if @p request ist a valid API request
bool NEI_ArkeoAPI_RequestIsValid(const NEI_ArkeoAPI_Request *request);

/// @brief Compiles a structured API request.
/// @param request The request to compile
/// @param buf Where to store the compiles request. If NULL, uses the standard
/// message buffer
/// @param size Size of @p buf. If @p buf is NULL, will be overwritten
/// @return On success, the length of the compiled request, otherwise -1
long NEI_ArkeoAPI_compileRequest(const NEI_ArkeoAPI_Request *request, char *buf,
    size_t size);

/// @brief Parses a received API response
/// @param response The reponse structure to fill. CAVEAT: Only valid as long as
/// the data in @p buf is not touched
/// @param buf The buffer containing the received response. If NULL, uses the
/// standard message buffer
/// @param msgLen The size of the received response. If 0, assumes a
/// NULL-terminated string and calculates it (strlen())
/// @return 0 on success, -1 otherwise
int NEI_ArkeoAPI_parseResponse(NEI_ArkeoAPI_Response *response, char *buf,
    size_t msgLen);

/// @brief Sends a structured API request
/// @param request The request to send
/// @param buf Where to compile the request. If NULL, uses the standard message
/// buffer
/// @param size Size of @p buf. If @p buf is NULL, will be overwritten
/// @return 0 on success, otherwise -1
int NEI_ArkeoAPI_sendRequest(const NEI_ArkeoAPI_Request *request, char *buf,
    size_t size);

/// @brief Receives an API response
/// @param response The reponse structure to fill. CAVEAT: Only valid as long as
/// the data in @p buf is not touched
/// @param buf The buffer that will be filled with received data. If NULL, uses
/// the standard message buffer
/// @param size Size of @p buf. If @p buf is NULL, will be overwritten
/// @return 0 on success, -1 otherwise
int NEI_ArkeoAPI_receiveResponse(NEI_ArkeoAPI_Response *response, char *buf,
    size_t size);

// TODO:
int NEI_ArkeoAPI_transcieve();

#endif //__NEI_CODEC_H__
