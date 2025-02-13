#pragma once
#include <zSyncO/zSyncO.h>

enum ObexResponseCode : unsigned char {
    OBEX_CONTINUE = 0x90,
    OBEX_OK = 0xA0,
    OBEX_CREATED = 0xA1,
    OBEX_ACCEPTED = 0xA2,
    OBEX_NON_AUTHORITATIVE_INFORMATION = 0xA3,
    OBEX_NO_CONTENT = 0xA4,
    OBEX_RESET_CONTENT = 0xA5,
    OBEX_PARTIAL_CONTENT = 0xA6,
    OBEX_MUTLIPLE_CHOICES = 0xB0,
    OBEX_MOVED_PERMANENTLY = 0xB1,
    OBEX_MOVED_TEMPORARILY = 0xB2,
    OBEX_SEE_OTHER = 0xB3,
    OBEX_NOT_MODIFIED = 0xB4,
    OBEX_USE_PROXY = 0xB5,
    OBEX_IS_LAST_FILE_CHUNK = 0xB6,
    OBEX_BAD_REQUEST = 0xC0,
    OBEX_UNAUTHORIZED = 0xC1,
    OBEX_PAYMENT_REQUIRED = 0xC2,
    OBEX_FORBIDDEN = 0xC3,
    OBEX_NOT_FOUND = 0xC4,
    OBEX_METHOD_NOT_ALLOWED = 0xC5,
    OBEX_NOT_ACCEPTABLE = 0xC6,
    OBEX_PROXY_AUTHENTCATION_REQUIRED = 0xC7,
    OBEX_REQUEST_TIMEOUT = 0xC8,
    OBEX_CONFLICT = 0xC9,
    OBEX_GONE = 0xCA,
    OBEX_LENGTH_REQUIRED = 0xCB,
    OBEX_PRECONDITION_FAILED = 0xCC,
    OBEX_REQUESTED_ENTITY_TOO_LARGE = 0xCD,
    OBEX_REQUESTED_URL_TOO_LARGE = 0XCE,
    OBEX_UNSUPPORTED_MEDIA_TYPE = 0xCF,
    OBEX_INTERNAL_SERVER_ERROR = 0xD0,
    OBEX_NOT_IMPLEMENTED = 0xD1,
    OBEX_BAD_GATEWAY = 0xD2,
    OBEX_SERVICE_UNAVAILABLE = 0xD3,
    OBEX_GATEWAY_TIMEOUT = 0xD4,
    OBEX_HTTP_VERSION_NOT_SUPPORTED = 0xD5,
    OBEX_DATABASE_FULL = 0xE0,
    OBEX_DATABASE_LOCKED = 0xE1,
    OBEX_CANCELED_BY_USER = 0xF0
};

SYNC_API bool IsObexError(ObexResponseCode code);

SYNC_API CString ObexResponseCodeToString(ObexResponseCode code);

// Bluetooth protocol versioning. Increment when changes are not backwards compatible.
// Mismatched versions will be detected during synchronization and a pop-up warning will be displayed.
extern SYNC_API const unsigned int BLUETOOTH_PROTOCOL_VERSION;

// Unique uuid for CSPro Sync service
extern SYNC_API const GUID OBEX_SYNC_SERVICE_UUID;

// Standard uuid for obex folder browsing target
extern SYNC_API const unsigned char OBEX_FOLDER_BROWSING_UUID[16];

// custom mime type for data file smart sync (to differ it from file transfer)
extern SYNC_API const wchar_t* OBEX_SYNC_DATA_MEDIA_TYPE;

// custom mime-type for json directory listing
extern SYNC_API const wchar_t* OBEX_DIRECTORY_LISTING_MEDIA_TYPE;

extern SYNC_API const wchar_t* OBEX_BINARY_FILE_MEDIA_TYPE;

// custom mime-type for sync app
extern SYNC_API const wchar_t* OBEX_SYNC_APP_MEDIA_TYPE;

// custom mime-type for syncing message
extern SYNC_API const wchar_t* OBEX_SYNC_MESSAGE_MEDIA_TYPE;

// custom mime-types for syncing paradata
extern SYNC_API const wchar_t* OBEX_SYNC_PARADATA_SYNC_HANDSHAKE;
extern SYNC_API const wchar_t* OBEX_SYNC_PARADATA_TYPE;

enum ObexOpCode : unsigned char
{
    OBEX_CONNECT = 0x80,
    OBEX_DISCONNECT = 0x81,
    OBEX_PUT = 0x02,
    OBEX_GET = 0x03,
    OBEX_SET_PATH = 0x85,
    OBEX_ABORT = 0xFF
};

enum ObexHeaderCode : unsigned char
{
    OBEX_HEADER_COUNT = 0xC0,
    OBEX_HEADER_NAME = 0x01,
    OBEX_HEADER_TYPE = 0x42,
    OBEX_HEADER_LENGTH = 0xC3,
    OBEX_HEADER_TIME_ISO_8601 = 0x44,
    OBEX_HEADER_TIME_4_BYTE = 0xC4,
    OBEX_HEADER_DESCRIPTION = 0x05,
    OBEX_HEADER_TARGET = 0x46,
    OBEX_HEADER_HTTP = 0x47,
    OBEX_HEADER_BODY = 0x48,
    OBEX_HEADER_END_OF_BODY = 0x49,
    OBEX_HEADER_WHO = 0x4A,
    OBEX_HEADER_CONNECTION_ID = 0xCB,
    OBEX_HEADER_APP_PARAMETER = 0x4C,
    OBEX_HEADER_AUTH_CHALLENGE = 0x4D,
    OBEX_HEADER_AUTH_RESPONSE = 0x4E,
    OBEX_HEADER_OBJECT_CLASS = 0x4F,
    OBEX_HEADER_BLUETOOTH_PROTOCOL_VERSION = 0x30,
    OBEX_HEADER_IS_LAST_FILE_CHUNK = 0x31
};

const unsigned char OBEX_FINAL = 0x80;

const unsigned char OBEX_VERSION = 0x10;

const size_t OBEX_MIN_PACKET_SIZE = 255;

const size_t OBEX_MAX_PACKET_SIZE = 0xFFFE;

const size_t OBEX_BASE_PACKET_SIZE = 3;
