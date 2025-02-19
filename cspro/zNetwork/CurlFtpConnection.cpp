#include "stdafx.h"
#include "CurlFtpConnection.h"
#include "CurlWrapper.h"
#include "ParsedUri.h"


CurlFtpConnection::CurlFtpConnection()
    :   m_supportsMLSD(false),
        m_curlWrapper(std::make_unique<CurlWrapper>(CurlWrapper::Type::Ftp, m_curl))
{
    SyncLog::EnableLogging();
}


CurlFtpConnection::~CurlFtpConnection()
{
}


void CurlFtpConnection::SetSyncListener(std::shared_ptr<SyncListener> sync_listener)
{
    FtpConnection::SetSyncListener(sync_listener);

    m_curlWrapper->SetSyncListener(std::move(sync_listener));
}


std::string CurlFtpConnection::DoConnect(const std::string& username, const std::string& password)
{
    constexpr std::string_view FTPES_sv = "ftpes://";
    constexpr std::string_view FTPS_sv  = "ftps://";
    constexpr std::string_view FTP_sv   = "ftp://";

    std::string evaluated_url = m_providedUrl;

    // Extract the TLS type (if any from the URL)
    if( SO::StartsWith(m_providedUrl, FTPES_sv) )
    {
        // cURL wants a regular ftp:// url instead of an ftpes:// url
        // but with CURLUSESSL_ALL it will force explicit SSL
        evaluated_url.replace(0, FTPES_sv.length(), FTP_sv.data(), FTP_sv.length());
        curl_easy_setopt(m_curl, CURLOPT_USE_SSL, CURLUSESSL_ALL);
    }

    else if( SO::StartsWith(m_providedUrl, FTPS_sv) )
    {
        curl_easy_setopt(m_curl, CURLOPT_USE_SSL, CURLUSESSL_ALL);
    }

    else
    {
        // Must be a plain FTP url, no SSL
        ASSERT(SO::StartsWith(m_providedUrl, FTP_sv));
        curl_easy_setopt(m_curl, CURLOPT_USE_SSL, CURLUSESSL_NONE);
    }

    curl_easy_setopt(m_curl, CURLOPT_URL, evaluated_url.c_str());
    curl_easy_setopt(m_curl, CURLOPT_USERNAME, username.c_str());
    curl_easy_setopt(m_curl, CURLOPT_PASSWORD, password.c_str());

    curl_easy_setopt(m_curl, CURLOPT_XFERINFOFUNCTION, CurlWrapper::ProgressCallback);

    // Accept all encodings include gzip/deflate, CURL will automatically decompress
    curl_easy_setopt(m_curl, CURLOPT_ACCEPT_ENCODING, "");

    // Connection timeout after 10 seconds
    curl_easy_setopt(m_curl, CURLOPT_CONNECTTIMEOUT, 10);

    // Timeout if less than 1 byte is transferred over a period of 60 seconds
    curl_easy_setopt(m_curl, CURLOPT_LOW_SPEED_LIMIT, 1);
    curl_easy_setopt(m_curl, CURLOPT_LOW_SPEED_TIME, 60);

    // For connect send "FEAT" command to get supported features
    QueryFeatures();

    return evaluated_url;
}


void CurlFtpConnection::DoDisconnect()
{
}


std::string CurlFtpConnection::CreateRemoteUrl(const std::string& path) const
{
    return PortableFunctions::PathAppendForwardSlashToPath(m_url, Encoders::ToUri(path, false));
}


void CurlFtpConnection::QueryFeatures()
{
    curl_slist* const headers = curl_slist_append(nullptr, "FEAT");
    curl_easy_setopt(m_curl, CURLOPT_QUOTE, headers);
    curl_easy_setopt(m_curl, CURLOPT_NOBODY, 1);

    // Response to FEAT command will be sent on command channel which will
    // get written to header callback (unlike file downloads which are written
    // to body via write callback).
    std::ostringstream feature_list_stream;
    curl_easy_setopt(m_curl, CURLOPT_HEADERFUNCTION, CurlWrapper::WriteToOutputStreamCallback);
    curl_easy_setopt(m_curl, CURLOPT_HEADERDATA, &feature_list_stream);

    m_curlWrapper->SetOptionsForProgressHandling();

    const CURLcode result = curl_easy_perform(m_curl);
    curl_slist_free_all(headers);

    curl_easy_setopt(m_curl, CURLOPT_QUOTE, nullptr);
    curl_easy_setopt(m_curl, CURLOPT_HEADERFUNCTION, nullptr);
    curl_easy_setopt(m_curl, CURLOPT_HEADERDATA, nullptr);

    if( result != CURLE_OK )
        m_curlWrapper->ThrowSyncException(result, 100101);

    // Extract the features from the response
    bool in_feature_list = false;

    SO::ForeachLine<std::string>(feature_list_stream.str(), false,
        [&](const std::string& line)
        {
            SYNCLOG_INFO << line;

            if( SO::StartsWith(line, "220") )
            {
                // Welcome message, including server software name.
            }

            else if( SO::StartsWith(line, "211") )
            {
                in_feature_list = !in_feature_list;
            }

            else if( in_feature_list )
            {
                if( SO::EqualsNoCase(SO::Trim(line), "MLSD") )
                    m_supportsMLSD = true;
            }
        });
}


void CurlFtpConnection::Download(const std::string& remote_file_path, std::ostream& output_stream)
{
    ASSERT(remote_file_path == PortableFunctions::PathToForwardSlash(remote_file_path));

    const std::string remote_url = CreateRemoteUrl(remote_file_path);
    curl_easy_setopt(m_curl, CURLOPT_URL, remote_url.c_str());

    curl_easy_setopt(m_curl, CURLOPT_WRITEFUNCTION, CurlWrapper::WriteToOutputStreamCallback);
    curl_easy_setopt(m_curl, CURLOPT_WRITEDATA, &output_stream);

    curl_easy_setopt(m_curl, CURLOPT_UPLOAD, 0);
    curl_easy_setopt(m_curl, CURLOPT_READFUNCTION, nullptr);
    curl_easy_setopt(m_curl, CURLOPT_READDATA, nullptr);
    curl_easy_setopt(m_curl, CURLOPT_WILDCARDMATCH, 0);
    curl_easy_setopt(m_curl, CURLOPT_CHUNK_BGN_FUNCTION, nullptr);
    curl_easy_setopt(m_curl, CURLOPT_CHUNK_DATA, nullptr);
    curl_easy_setopt(m_curl, CURLOPT_FTP_CREATE_MISSING_DIRS, CURLFTP_CREATE_DIR_NONE);
    curl_easy_setopt(m_curl, CURLOPT_NOBODY, 0);
    curl_easy_setopt(m_curl, CURLOPT_FILETIME, 0);

    m_curlWrapper->SetOptionsForProgressHandling();

    const CURLcode result = curl_easy_perform(m_curl);

    if( result != CURLE_OK )
        m_curlWrapper->ThrowSyncException(result, 100101);
}


void CurlFtpConnection::Upload(std::istream& input_stream, const int64_t input_size_bytes, const std::string& remote_file_path)
{
    ASSERT(remote_file_path == PortableFunctions::PathToForwardSlash(remote_file_path));

    // Upload to temporary file on server so that you don't get half a file
    // if the upload is interrupted
    const std::string temporary_remote_file_path = remote_file_path + "__uploading.tmp";

    const std::string remote_url = CreateRemoteUrl(temporary_remote_file_path);
    curl_easy_setopt(m_curl, CURLOPT_URL, remote_url.c_str());

    CurlWrapper::InputStreamData input_stream_data { input_stream };
    curl_easy_setopt(m_curl, CURLOPT_READFUNCTION, CurlWrapper::ReadFromInputStreamDataCallback);
    curl_easy_setopt(m_curl, CURLOPT_READDATA, &input_stream_data);
    curl_easy_setopt(m_curl, CURLOPT_UPLOAD, 1);
    curl_easy_setopt(m_curl, CURLOPT_FTP_CREATE_MISSING_DIRS, CURLFTP_CREATE_DIR);

    curl_easy_setopt(m_curl, CURLOPT_INFILESIZE_LARGE, static_cast<curl_off_t>(input_size_bytes));

    curl_easy_setopt(m_curl, CURLOPT_WRITEFUNCTION, nullptr);
    curl_easy_setopt(m_curl, CURLOPT_WRITEDATA, nullptr);
    curl_easy_setopt(m_curl, CURLOPT_WILDCARDMATCH, 0);
    curl_easy_setopt(m_curl, CURLOPT_CHUNK_BGN_FUNCTION, nullptr);
    curl_easy_setopt(m_curl, CURLOPT_CHUNK_DATA, nullptr);
    curl_easy_setopt(m_curl, CURLOPT_NOBODY, 0);
    curl_easy_setopt(m_curl, CURLOPT_FILETIME, 0);

    m_curlWrapper->SetOptionsForProgressHandling();

    const CURLcode result = curl_easy_perform(m_curl);

    if( result != CURLE_OK )
    {
        if( result == CURLE_ABORTED_BY_CALLBACK && input_stream_data.read_error )
            throw SyncError(100134);

        m_curlWrapper->ThrowSyncException(result, 100101);
    }

    // Some servers (e.g., FileZilla) won't let you rename if the destination file already
    // exists. So need to delete file first before renaming.
    if( FileIsRegular(remote_file_path) )
        FileDelete(remote_file_path);

    FileRename(temporary_remote_file_path, remote_file_path);
}


bool CurlFtpConnection::FileExists(const std::string& remote_path)
{
    bool exists = QueryPath<bool>(remote_path);

    // if the path does not exist, but does not contain a trailing slash, try again as a directory
    if( !exists && !remote_path.empty() && remote_path.back() != '/' )
        exists = FileIsDirectory(remote_path);

    return exists;
}


bool CurlFtpConnection::FileIsRegular(const std::string& remote_path)
{
    if( !remote_path.empty() && remote_path.back() == '/' )
        return false;

    return QueryPath<bool>(remote_path);
}


bool CurlFtpConnection::FileIsDirectory(const std::string& remote_path)
{
    const std::string remote_path_with_trailing_slash = PortableFunctions::PathEnsureTrailingForwardSlash(remote_path);

    if( QueryPath<bool>(remote_path_with_trailing_slash) )
        return true;

    // querying for the path doesn't always work on directories (at least not on all servers)
    // so we need to do directory listing of parent here
    const std::string_view remote_path_without_trailing_slash_sv(remote_path_with_trailing_slash.data(), remote_path_with_trailing_slash.length() - 1);
    const std::string parent_remote_path = PortableFunctions::PathGetDirectory(remote_path_without_trailing_slash_sv);
    const std::string directory_name = PortableFunctions::PathGetFilename(remote_path_without_trailing_slash_sv);

    try
    {
        const std::vector<FileInfo> directory_listing = GetDirectoryListing(parent_remote_path, false);
        const auto& lookup = std::find_if(directory_listing.cbegin(), directory_listing.cend(),
                                          [&](const FileInfo& file_info) { return ( file_info.GetType() == FileInfo::FileType::Directory && file_info.GetName() == directory_name ); });

        return ( lookup != directory_listing.cend() );
    }

    catch(...)
    {
        return false;
    }
}


int64_t CurlFtpConnection::FileModifiedTime(const std::string& remote_path)
{
    ASSERT(remote_path.empty() || remote_path.back() != '/');

    return QueryPath<int64_t>(remote_path);
}


template<typename T>
T CurlFtpConnection::QueryPath(const std::string& remote_path)
{
    ASSERT(remote_path == PortableFunctions::PathToForwardSlash(remote_path));

    const std::string remote_url = CreateRemoteUrl(remote_path);
    curl_easy_setopt(m_curl, CURLOPT_URL, remote_url.c_str());

    curl_easy_setopt(m_curl, CURLOPT_NOBODY, 1);
    curl_easy_setopt(m_curl, CURLOPT_FILETIME, 1);
    curl_easy_setopt(m_curl, CURLOPT_READFUNCTION, nullptr);
    curl_easy_setopt(m_curl, CURLOPT_READDATA, nullptr);
    curl_easy_setopt(m_curl, CURLOPT_WRITEFUNCTION, CurlWrapper::WriteToNothingCallback);
    curl_easy_setopt(m_curl, CURLOPT_WRITEDATA, nullptr);
    curl_easy_setopt(m_curl, CURLOPT_WILDCARDMATCH, 0);
    curl_easy_setopt(m_curl, CURLOPT_CHUNK_BGN_FUNCTION, nullptr);
    curl_easy_setopt(m_curl, CURLOPT_CHUNK_DATA, nullptr);
    curl_easy_setopt(m_curl, CURLOPT_UPLOAD, 0);
    curl_easy_setopt(m_curl, CURLOPT_FTP_CREATE_MISSING_DIRS, CURLFTP_CREATE_DIR_NONE);

    m_curlWrapper->SetOptionsForProgressHandling();

    CURLcode result = curl_easy_perform(m_curl);

    if constexpr(std::is_same_v<T, bool>)
    {
        return ( result == CURLE_OK );
    }

    else
    {
        if( result != CURLE_OK )
            m_curlWrapper->ThrowSyncException(result, 100101);

        long file_time = -1;
        result = curl_easy_getinfo(m_curl, CURLINFO_FILETIME, &file_time);

        if( result == CURLE_OK && file_time >= 0 )
            return static_cast<int64_t>(file_time);

        return 0;
    }
}


struct CurlFtpConnection::DirectoryListingInfo
{
    const std::string& remote_path_with_trailing_slash;
    const std::string& remote_url;
    std::vector<FileInfo> directory_listing;
};


std::vector<FileInfo> CurlFtpConnection::GetDirectoryListing(const std::string& remote_directory_path, const bool /*request_file_md5s*/)
{
    const std::string remote_path_with_trailing_slash = PortableFunctions::PathEnsureTrailingForwardSlash(remote_directory_path);
    const std::string remote_url = CreateRemoteUrl(remote_path_with_trailing_slash + "/*");

    DirectoryListingInfo directory_listing_info
    {
        remote_path_with_trailing_slash,
        remote_url
    };

    // When possible, retrieve machine readable directory listing using MLSD command.
    // This is the better method as it gives consistent and accurate
    // modified date/times but it is not supported on all servers (e.g. IIS)
    if( m_supportsMLSD )
    {
        GetDirectoryListingMLSD(directory_listing_info);
    }

    // Otherwise get the directory listing using CURL default method (LIST command) which
    // is supported on all servers but doesn't give consistent format
    // and modified times are not as accurate.
    else
    {
        GetDirectoryListingLIST(directory_listing_info);
    }

    return directory_listing_info.directory_listing;
}


bool CurlFtpConnection::IncludeInDirectoryListing(const std::string_view filename_sv)
{
    if( filename_sv.empty() )
        return false;

    // don't treat "." or ".." as files
    if( filename_sv.front() == '.' && ( filename_sv.length() == 1 || SO::Equals(filename_sv, "..") ) )
        return false;

    return true;
}


void CurlFtpConnection::GetDirectoryListingMLSD(DirectoryListingInfo& directory_listing_info)
{
    curl_easy_setopt(m_curl, CURLOPT_CUSTOMREQUEST, "MLSD");

    // Strangely to get the dirlist to work with MLSD you have to
    // tell CURL to ask for directory listing with files only.
    curl_easy_setopt(m_curl, CURLOPT_DIRLISTONLY, 1);

    curl_easy_setopt(m_curl, CURLOPT_URL, directory_listing_info.remote_url.c_str());

    std::ostringstream dir_list_stream;
    curl_easy_setopt(m_curl, CURLOPT_WRITEFUNCTION, CurlWrapper::WriteToOutputStreamCallback);
    curl_easy_setopt(m_curl, CURLOPT_WRITEDATA, &dir_list_stream);

    curl_easy_setopt(m_curl, CURLOPT_UPLOAD, 0);
    curl_easy_setopt(m_curl, CURLOPT_READFUNCTION, nullptr);
    curl_easy_setopt(m_curl, CURLOPT_READDATA, nullptr);
    curl_easy_setopt(m_curl, CURLOPT_FTP_CREATE_MISSING_DIRS, CURLFTP_CREATE_DIR_NONE);
    curl_easy_setopt(m_curl, CURLOPT_NOBODY, 0);
    curl_easy_setopt(m_curl, CURLOPT_FILETIME, 0);

    m_curlWrapper->SetOptionsForProgressHandling();

    const CURLcode result = curl_easy_perform(m_curl);

    curl_easy_setopt(m_curl, CURLOPT_DIRLISTONLY, 0);

    if( result != CURLE_OK )
        m_curlWrapper->ThrowSyncException(result, 100101);

    // MLSD response format is described in https://tools.ietf.org/html/rfc3659#page-23
    SO::ForeachLine<std::string>(dir_list_stream.str(), false,
        [&](const std::string& line)
        {
            const auto [facts_sv, filename_sv] = SO::GetTextOnEitherSideOfCharacter(line, ' ');

            if( filename_sv.empty() )
            {
                SYNCLOG_ERROR << "Missing ' ' in MLSD directory listing:" << line;
                throw SyncError(100121);
            }

            if( !IncludeInDirectoryListing(filename_sv) )
                return;

            FileInfo::FileType type = FileInfo::FileType::File;
            int64_t size;
            int64_t last_modified;

            SO::ForeachSection(facts_sv, ';',
                [&](const std::string_view text_sv)
                {
                    const auto [fact_type_sv, fact_value_sv] = SO::GetTextOnEitherSideOfCharacter(text_sv, '=');

                    if( fact_value_sv.empty() )
                    {
                        SYNCLOG_ERROR << "Fact missing '=' in MLSD directory listing:" << line;
                        throw SyncError(100121);
                    }

                    if( SO::EqualsNoCase(fact_type_sv, "type") )
                    {
                        if( SO::EqualsOneOfNoCase(fact_value_sv, "dir", "cdir", "pdir") )
                            type = FileInfo::FileType::Directory;
                    }

                    else if( SO::EqualsNoCase(fact_type_sv, "modify") )
                    {
                        // Remove the fractional seconds (after digit 14) since they won't fit in int64_t
                        last_modified = PortableFunctions::ParseYYYYMMDDhhmmssDateTime(std::string(fact_value_sv.substr(0, 14)));
                    }

                    else if( SO::EqualsNoCase(fact_type_sv, "size") )
                    {
                        char* endptr;
                        size = strtoll(std::string(fact_value_sv).c_str(), &endptr, 10);
                    }
                });

            directory_listing_info.directory_listing.emplace_back(type, std::string(filename_sv), directory_listing_info.remote_path_with_trailing_slash, size, last_modified);
        });
}


void CurlFtpConnection::GetDirectoryListingLIST(DirectoryListingInfo& directory_listing_info)
{
    curl_easy_setopt(m_curl, CURLOPT_WILDCARDMATCH, 1);
    curl_easy_setopt(m_curl, CURLOPT_CHUNK_BGN_FUNCTION, DirectoryListingWildcardMatchCallback);
    curl_easy_setopt(m_curl, CURLOPT_CHUNK_DATA, &directory_listing_info);

    curl_easy_setopt(m_curl, CURLOPT_URL, directory_listing_info.remote_url.c_str());

    curl_easy_setopt(m_curl, CURLOPT_WRITEFUNCTION, nullptr);
    curl_easy_setopt(m_curl, CURLOPT_WRITEDATA, nullptr);

    curl_easy_setopt(m_curl, CURLOPT_UPLOAD, 0);
    curl_easy_setopt(m_curl, CURLOPT_READFUNCTION, nullptr);
    curl_easy_setopt(m_curl, CURLOPT_READDATA, nullptr);
    curl_easy_setopt(m_curl, CURLOPT_FTP_CREATE_MISSING_DIRS, CURLFTP_CREATE_DIR_NONE);
    curl_easy_setopt(m_curl, CURLOPT_NOBODY, 0);
    curl_easy_setopt(m_curl, CURLOPT_FILETIME, 0);

    m_curlWrapper->SetOptionsForProgressHandling();

    const CURLcode result = curl_easy_perform(m_curl);

    if( result != CURLE_OK )
        m_curlWrapper->ThrowSyncException(result, 100101);
}


long CurlFtpConnection::DirectoryListingWildcardMatchCallback(const void* const transfer_info, DirectoryListingInfo* const directory_listing_info, const int /*remains*/)
{
    const curl_fileinfo* const file_info = reinterpret_cast<const curl_fileinfo*>(transfer_info);

    SYNCLOG_INFO << "Dirlist: " << file_info->filename << ", " << file_info->strings.time << ", " << file_info->time;

    std::string filename = file_info->filename;

    if( IncludeInDirectoryListing(filename) )
    {
        const std::string time_text = file_info->strings.time;
        std::optional<int64_t> last_modified = ParseUnixFileTime(time_text);

        if( !last_modified.has_value() )
            last_modified = ParseDosFileTime(time_text);

        directory_listing_info->directory_listing.emplace_back(( file_info->filetype == CURLFILETYPE_DIRECTORY ) ? FileInfo::FileType::Directory : FileInfo::FileType::File,
                                                               std::move(filename),
                                                               directory_listing_info->remote_path_with_trailing_slash,
                                                               file_info->size,
                                                               last_modified.value_or(0));
    }

    // Don't download the file (we want info only)
    return CURL_CHUNK_BGN_FUNC_SKIP;
}


int CurlFtpConnection::GetNextNumericToken(std::string_view& text_sv)
{
    size_t start_pos = 0;

    while( start_pos < text_sv.length() && !std::isdigit(text_sv[start_pos]) )
        ++start_pos;

    if( start_pos == text_sv.length() )
        return -1;

    size_t end_pos = start_pos + 1;

    while( end_pos < text_sv.length() && std::isdigit(text_sv[end_pos]) )
        ++end_pos;

    int token = atoi(std::string(text_sv.substr(start_pos, end_pos - start_pos)).c_str());

    text_sv = text_sv.substr(end_pos);

    return token;
}


std::optional<int64_t> CurlFtpConnection::ValidateFileTime(const DateTime::Components& date_time_components)
{
    if( ( date_time_components.year >= 1970 ) &&
        ( date_time_components.month >= 1 && date_time_components.month <= 12 ) &&
        ( date_time_components.day >= 1 && date_time_components.day <= 31 ) &&
        ( date_time_components.hour >= 0 && date_time_components.hour <= 23 ) &&
        ( date_time_components.minute >= 0 && date_time_components.minute <= 60 ) )
    {
        int64_t time = DateTime::CreateTime(date_time_components);

        if( time >= 0 )
        {
            SYNCLOG_INFO << "Dirlist time: Ep =" << time << " str=" << DateTime::TimeToRFC3339(time);
            return time;
        }
    }

    SYNCLOG_INFO << "Dirlist time invalid";

    return std::nullopt;
}


std::optional<int64_t> CurlFtpConnection::ParseUnixFileTime(std::string_view time_text_sv)
{
    // Unix filetimes come from FileZilla, Pure FTP and others.
    // It is either "MMM DD HH:MM" or "MMM DD YYYY" e.g.
    // "Apr 24 15:13" or "Sep 15 2016"
    if( time_text_sv.length() < 11 )
        return std::nullopt;

    constexpr const char* MonthStrings[] = { "jan", "feb", "mar", "apr", "may", "jun", "jul", "aug", "sep", "oct", "nov", "dec" };
    const auto& month_lookup = std::find_if(std::cbegin(MonthStrings), std::cend(MonthStrings),
                                            [&](const char* const month) { return SO::StartsWithNoCase(time_text_sv, month); });

    if( month_lookup == std::cend(MonthStrings) )
        return std::nullopt;

    DateTime::Components date_time_components { 0 };

    date_time_components.month = 1 + std::distance(std::cbegin(MonthStrings), month_lookup);

    time_text_sv = time_text_sv.substr(3);
    date_time_components.day = GetNextNumericToken(time_text_sv);

    time_text_sv = SO::TrimLeft(time_text_sv);
    const size_t colon_pos = time_text_sv.find(':');

    if( colon_pos == std::string_view::npos )
    {
        date_time_components.year = GetNextNumericToken(time_text_sv);
        return ValidateFileTime(date_time_components);
    }

    else if( colon_pos == 2 )
    {
        // Check for HH:MM
        date_time_components.hour = GetNextNumericToken(time_text_sv);
        date_time_components.minute = GetNextNumericToken(time_text_sv);

        // When no year is specified, use current year
        date_time_components.year = DateTime::LocalYear();

        return ValidateFileTime(date_time_components);
    }

    else
    {
        return std::nullopt;
    }
}


std::optional<int64_t> CurlFtpConnection::ParseDosFileTime(std::string_view time_text_sv)
{
    // IIS Ftp server uses DOS dir format by default (although it can be set to use Unix times too).
    // Format is "MM-DD-YY  HH:MM[AM|PM]"
    // e.g. 04-24-17  03:18PM
    if( time_text_sv.length() < 17 )
        return std::nullopt;

    DateTime::Components date_time_components { 0 };

    date_time_components.month = GetNextNumericToken(time_text_sv);
    date_time_components.day = GetNextNumericToken(time_text_sv);
    date_time_components.year = GetNextNumericToken(time_text_sv);

    if( date_time_components.year < 0 )
        return std::nullopt;

    // Convert 2 digit year to 4 digit year
    // TODO: fix this before Jan 1, 2070
    if( date_time_components.year >= 70 && date_time_components.year <= 99 )
    {
        date_time_components.year += 1900;
    }

    else
    {
        date_time_components.year += 2000;
    }

    date_time_components.hour = GetNextNumericToken(time_text_sv);
    date_time_components.minute = GetNextNumericToken(time_text_sv);

    if( SO::EqualsNoCase(SO::Trim(time_text_sv), "PM") )
        date_time_components.hour += 12;

    return ValidateFileTime(date_time_components);
}


void CurlFtpConnection::FileRename(const std::string& old_remote_file_path, const std::string& new_remote_file_path)
{
    const std::string remote_url = CreateRemoteUrl(PortableFunctions::PathGetDirectory(old_remote_file_path));
    curl_easy_setopt(m_curl, CURLOPT_URL, remote_url.c_str());

    const std::string rename_from_command = "RNFR " + PortableFunctions::PathGetFilename(old_remote_file_path);

    // make sure that the renamed file is evaluated based on the connection path
    const std::string rename_to_command = "RNTO " + PortableFunctions::PathAppendForwardSlashToPath(m_urlPath, new_remote_file_path);

    curl_slist* headers = curl_slist_append(nullptr, rename_from_command.c_str());
    headers = curl_slist_append(headers, rename_to_command.c_str());

    curl_easy_setopt(m_curl, CURLOPT_POSTQUOTE, headers);

    curl_easy_setopt(m_curl, CURLOPT_READFUNCTION, nullptr);
    curl_easy_setopt(m_curl, CURLOPT_UPLOAD, 0);
    curl_easy_setopt(m_curl, CURLOPT_WRITEFUNCTION, nullptr);
    curl_easy_setopt(m_curl, CURLOPT_WRITEDATA, nullptr);
    curl_easy_setopt(m_curl, CURLOPT_WILDCARDMATCH, 0);
    curl_easy_setopt(m_curl, CURLOPT_CHUNK_BGN_FUNCTION, nullptr);
    curl_easy_setopt(m_curl, CURLOPT_CHUNK_DATA, nullptr);
    curl_easy_setopt(m_curl, CURLOPT_NOBODY, 1);
    curl_easy_setopt(m_curl, CURLOPT_FILETIME, 0);

    m_curlWrapper->SetOptionsForProgressHandling();

    const CURLcode result = curl_easy_perform(m_curl);

    curl_slist_free_all(headers);
    curl_easy_setopt(m_curl, CURLOPT_POSTQUOTE, nullptr);

    if( result != CURLE_OK )
        m_curlWrapper->ThrowSyncException(result, 100177);
}


void CurlFtpConnection::DeleteWorker(const char* const command, const std::string& remote_path)
{
    const std::string remote_url = CreateRemoteUrl(PortableFunctions::PathGetDirectory(remote_path));
    curl_easy_setopt(m_curl, CURLOPT_URL, remote_url.c_str());

    const std::string delete_command = command + PortableFunctions::PathGetFilename(remote_path);
    curl_slist* const headers = curl_slist_append(nullptr, delete_command.c_str());

    curl_easy_setopt(m_curl, CURLOPT_POSTQUOTE, headers);

    curl_easy_setopt(m_curl, CURLOPT_READFUNCTION, nullptr);
    curl_easy_setopt(m_curl, CURLOPT_UPLOAD, 0);
    curl_easy_setopt(m_curl, CURLOPT_WRITEFUNCTION, nullptr);
    curl_easy_setopt(m_curl, CURLOPT_WRITEDATA, nullptr);
    curl_easy_setopt(m_curl, CURLOPT_WILDCARDMATCH, 0);
    curl_easy_setopt(m_curl, CURLOPT_CHUNK_BGN_FUNCTION, nullptr);
    curl_easy_setopt(m_curl, CURLOPT_CHUNK_DATA, nullptr);
    curl_easy_setopt(m_curl, CURLOPT_NOBODY, 1);
    curl_easy_setopt(m_curl, CURLOPT_FILETIME, 0);

    m_curlWrapper->SetOptionsForProgressHandling();

    const CURLcode result = curl_easy_perform(m_curl);
    curl_slist_free_all(headers);

    curl_easy_setopt(m_curl, CURLOPT_POSTQUOTE, nullptr);

    if( result != CURLE_OK )
        m_curlWrapper->ThrowSyncException(result, 100101);
}


void CurlFtpConnection::FileDelete(const std::string& remote_file_path)
{
    DeleteWorker("DELE ", remote_file_path);
}


void CurlFtpConnection::DirectoryDelete(const std::string& remote_directory_path)
{
    DeleteWorker("RMD ", PortableFunctions::PathRemoveTrailingSlash(remote_directory_path));
}
