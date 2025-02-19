#pragma once


namespace SyncCustomHeaders
{
    static constexpr std::string_view DEVICE_ID_HEADER                  = "x-csw-device";
    static constexpr std::string_view DICTIONARY_NAME                   = "x-csw-dictionary-name";
    static constexpr std::string_view UNIVERSE_HEADER                   = "x-csw-universe";
    static constexpr std::string_view RANGE_COUNT_HEADER                = "x-csw-case-range-count";
    static constexpr std::string_view START_AFTER_HEADER                = "x-csw-case-range-start-after";
    static constexpr std::string_view IF_REVISION_EXISTS_HEADER         = "x-csw-if-revision-exists";
    static constexpr std::string_view EXCLUDE_REVISIONS_HEADER          = "x-csw-exclude-revisions";
    static constexpr std::string_view CHUNK_MAX_REVISION_HEADER         = "x-csw-chunk-max-revision";
    static constexpr std::string_view CURRENT_REVISION_HEADER           = "x-csw-current-revision";
    static constexpr std::string_view APP_PACKAGE_BUILD_TIME_HEADER     = "x-csw-package-build-time";
    static constexpr std::string_view APP_PACKAGE_FILES_HEADER          = "x-csw-package-files";
    static constexpr std::string_view MESSAGE_HEADER                    = "x-csw-message";
    static constexpr std::string_view PARADATA_LOG_UUID                 = "x-csw-paradata-log-uuid";
    static constexpr std::string_view GET_FILE_MD5_HEADER               = "x-csw-get-file-md5";
    static constexpr std::string_view CASES_REPOSITORY_OPTIONS_HEADER   = "x-csw-cases-options";
};
