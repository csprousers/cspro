#pragma once
#include <zSyncO/zSyncO.h>

class SYNC_API SyncCustomHeaders
{
public:
    static const CString DEVICE_ID_HEADER;
    static const CString UNIVERSE_HEADER;
    static const CString RANGE_COUNT_HEADER;
    static const CString START_AFTER_HEADER;
    static const CString IF_REVISION_EXISTS_HEADER;
    static const CString EXCLUDE_REVISIONS_HEADER;
    static const CString CHUNK_MAX_REVISION_HEADER;
    static const CString CURRENT_REVISION_HEADER;
    static const CString APP_PACKAGE_BUILD_TIME_HEADER;
    static const CString APP_PACKAGE_FILES_HEADER;
    static const CString MESSAGE_HEADER;
    static const CString PARADATA_LOG_UUID;
};
