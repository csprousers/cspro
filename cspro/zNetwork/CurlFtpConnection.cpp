#include "stdafx.h"
#include "CurlFtpConnection.h"
#include <zSyncO/ISyncListener.h>
#include <zSyncO/SyncException.h>
#include <zSyncO/SyncUtil.h>
#include <easyloggingwrapper.h>
#include <curl/curl.h>
#include <sstream>
#include <fstream>

namespace {

    // Need to call curl_global_init at program startup and
    // curl_global_cleanup at program end. To do this from a DLL
    // create a global static object that calls init in the
    // constructor and cleanup in the destructor.
    // The way C++ deals with static object ensures that init
    // will get called before the first HttpConnection is
    // constructed and cleanup is called after the last one
    // is destructed.
    class CurlInitHelper
    {
    public:
        CurlInitHelper()
        {
            curl_global_init(CURL_GLOBAL_DEFAULT);
        }
        ~CurlInitHelper()
        {
            curl_global_cleanup();
        }
    };

    static CurlInitHelper curlInitializer;

    char errorBuffer[CURL_ERROR_SIZE];

    int ignoreCallback(void *, size_t size, size_t nmemb, void*)
    {
        return size * nmemb;
    }

    int writeCallback(char *data, size_t size, size_t nmemb, std::ostream *pOstream)
    {
        pOstream->write(data, size * nmemb);

        return size * nmemb;
    }

    struct ReadData {
        std::istream *pIStream;
        bool readError = false;
    };

    size_t readCallback(void *ptr, size_t size, size_t nmemb, ReadData* pData)
    {
        if (size*nmemb < 1)
            return 0;

        if (pData->pIStream->eof())
            return 0;

        if (pData->pIStream->fail() || pData->pIStream->bad()) {
            pData->readError = true;
            return CURL_READFUNC_ABORT;
        }

        pData->pIStream->read((char *)ptr, size * nmemb);

        return (size_t)pData->pIStream->gcount();
    }

    size_t headerCallback(char *data, size_t size, size_t nmemb, void*)
    {
        std::string sData(data, size * nmemb);

        CLOG(INFO, "sync") << sData;

        return size * nmemb;
    }

    int nextNumericToken(CString& s)
    {
        const int len = s.GetLength();

        int i = 0;
        while (i < len && !::_istdigit(s[i]))
            ++i;

        if (i == len)
            return -1;

        const int start = i;
        while (i < len && ::_istdigit(s[i]))
            ++i;

        const int token = _ttoi(s.Mid(start, i - start));

        s = i < len ? s.Mid(i) : CString();

        return token;
    }

    CString months[12] = { _T("jan"), _T("feb"), _T("mar"), _T("apr"), _T("may"), _T("jun"), _T("jul"), _T("aug"), _T("sep"), _T("oct"), _T("nov"), _T("dec") };

    bool parseUnixFileTime(CString sFileTime, time_t& fileTime)
    {
        // Unix filetimes come from Filezilla, Pure FTP and others.
        // It is either "MMM DD HH:MM" or "MMM DD YYYY" e.g.
        // "Apr 24 15:13" or "Sep 15 2016"
        if (sFileTime.GetLength() < 11)
            return false;

        tm timeStruct = { 0 };

        CString sMonth = sFileTime.Left(3);
        auto iMonth = std::find_if(std::begin(months), std::end(months), [sMonth](CString s) {return s.CompareNoCase(sMonth) == 0;});
        if (iMonth == std::end(months))
            return false;

        timeStruct.tm_mon = std::distance(std::begin(months), iMonth);

        sFileTime = sFileTime.Mid(3).TrimLeft();

        timeStruct.tm_mday = nextNumericToken(sFileTime);

        int colonPos = sFileTime.Trim().Find(_T(':'));
        if (colonPos == -1) {
            int year = nextNumericToken(sFileTime);
            if (year < 1970)
                return false;

            timeStruct.tm_year = year - 1900;

        } else if (colonPos == 2) {
            // Check for HH:MM
            timeStruct.tm_hour = nextNumericToken(sFileTime);
            if (timeStruct.tm_hour < 0 || timeStruct.tm_min > 23)
                return false;

            timeStruct.tm_min = nextNumericToken(sFileTime);
            if (timeStruct.tm_min < 0 || timeStruct.tm_min > 60)
                return false;

            // If no year is specified, use current year
            time_t currentTime = time(NULL);
            struct tm *pLocalTime = localtime(&currentTime);
            timeStruct.tm_year = pLocalTime->tm_year;

        } else {
            return false;
        }

        fileTime = mktime(&timeStruct);

        CLOG(INFO, "sync") << "Dirlist time: Ep=" << fileTime << "str=" << UTF8Convert::WideToUTF8(PortableFunctions::TimeToRFC3339String(fileTime));

        return fileTime > -1;
    }

    bool parseDosFileTime(CString sFileTime, time_t& fileTime)
    {
        // IIS Ftp server uses DOS dir format by default (although it can be
        // set to use Unix times too).
        // Format is "MM-DD-YY  HH:MM[AM|PM]" e.g. 04-24-17  03:18PM
        if (sFileTime.GetLength() < 17)
            return false;

        tm timeStruct = { 0 };

        timeStruct.tm_mon = nextNumericToken(sFileTime) - 1;
        if (timeStruct.tm_mon < 0 || timeStruct.tm_mon > 11)
            return false;
        timeStruct.tm_mday = nextNumericToken(sFileTime);
        if (timeStruct.tm_mday < 1 || timeStruct.tm_mday > 31)
            return false;

        int year = nextNumericToken(sFileTime);
        if (year < 0)
            return false;

        // Convert 2 digit year to 4 digit year
        // TODO: fix this before Jan 1, 2070
        if (year >= 70 && year <= 99)
            year += 1900;
        else
            year += 2000;

        timeStruct.tm_year = year - 1900;

        timeStruct.tm_hour = nextNumericToken(sFileTime);
        if (timeStruct.tm_hour < 0 || timeStruct.tm_min > 23)
            return false;

        timeStruct.tm_min = nextNumericToken(sFileTime);
        if (timeStruct.tm_min < 0 || timeStruct.tm_min > 60)
            return false;

        if (sFileTime.Trim().CompareNoCase(_T("PM")) == 0) {
            timeStruct.tm_hour += 12;
        }

        fileTime = mktime(&timeStruct);

        return fileTime > -1;
    }

    time_t parseFtpFileTime(const char* fileTime)
    {
        CString sFileTime = UTF8Convert::UTF8ToWide<CString>(fileTime).Trim();
        time_t result = 0;

        if (parseUnixFileTime(sFileTime, result)) {
            return result;
        } else if (parseDosFileTime(sFileTime, result)) {
            return result;
        }

        return time_t(0);
    }

    struct DirListCallbackData
    {
        std::vector<FileInfo>* pFiles;
        CString directoryPath;
    };

    long dirListBeginChunkCallback(struct curl_fileinfo *finfo,
        DirListCallbackData* pData,
        int)
    {
        CLOG(INFO, "sync") << "Dirlist: " << finfo->filename << ", " << finfo->strings.time << ", " << finfo->time;

        time_t lastModified = parseFtpFileTime(finfo->strings.time);

        pData->pFiles->emplace_back(finfo->filetype == CURLFILETYPE_DIRECTORY ? FileInfo::FileType::Directory : FileInfo::FileType::File,
            UTF8Convert::UTF8ToWide<CString>(finfo->filename), pData->directoryPath, finfo->size, lastModified, L"");

        // Don't download the file (we want info only)
        return CURL_CHUNK_BGN_FUNC_SKIP;
    }

    struct ProgressData
    {
        ISyncListener* pListener;
        int64_t uploadSizeBytes;
    };

    int progressCallback(void *clientp, curl_off_t dltotal, curl_off_t dlnow,
        curl_off_t ultotal, curl_off_t ulnow)
    {
        ProgressData* pProgressData = (ProgressData*) clientp;
        ISyncListener* pListener = pProgressData->pListener;
        if (pListener->getProgressTotal() <= 0) {
            if (ultotal <= 0 && pProgressData->uploadSizeBytes >= 0)
                ultotal = pProgressData->uploadSizeBytes;
            int64_t total = ultotal + dltotal;
            if (total > 0)
                pListener->setProgressTotal(total);
        }
        int64_t so_far = ulnow + dlnow;
        pListener->onProgress(so_far);
        if (pListener->isCancelled()) {
            // returning non-zero signals libcurl to abort
            return 1;
        }

        return 0;
    }

    bool isRetryableError(CURLcode res)
    {
        switch (res) {
        case  CURLE_COULDNT_RESOLVE_PROXY:
        case  CURLE_COULDNT_RESOLVE_HOST:
        case  CURLE_COULDNT_CONNECT:
        case  CURLE_FTP_WEIRD_SERVER_REPLY:
        case  CURLE_REMOTE_ACCESS_DENIED:
        case  CURLE_FTP_ACCEPT_FAILED:
        case  CURLE_FTP_WEIRD_PASS_REPLY:
        case  CURLE_FTP_ACCEPT_TIMEOUT:
        case  CURLE_FTP_WEIRD_PASV_REPLY:
        case  CURLE_FTP_WEIRD_227_FORMAT:
        case  CURLE_FTP_CANT_GET_HOST:
        case  CURLE_HTTP2:
        case  CURLE_FTP_COULDNT_SET_TYPE:
        case  CURLE_PARTIAL_FILE:
        case  CURLE_FTP_COULDNT_RETR_FILE:
        case  CURLE_QUOTE_ERROR:
        case  CURLE_UPLOAD_FAILED:
        case  CURLE_OPERATION_TIMEDOUT:
        case  CURLE_FTP_PORT_FAILED:
        case  CURLE_FTP_COULDNT_USE_REST:
        case  CURLE_RANGE_ERROR:
        case  CURLE_HTTP_POST_ERROR:
        case  CURLE_SSL_CONNECT_ERROR:
        case  CURLE_BAD_DOWNLOAD_RESUME:
        case  CURLE_LDAP_CANNOT_BIND:
        case  CURLE_LDAP_SEARCH_FAILED:
        case  CURLE_GOT_NOTHING:
        case  CURLE_SEND_ERROR:
        case  CURLE_RECV_ERROR:
        case  CURLE_SSL_CIPHER:
        case  CURLE_SSL_CACERT:
        case  CURLE_LDAP_INVALID_URL:
        case  CURLE_USE_SSL_FAILED:
        case  CURLE_SEND_FAIL_REWIND:
        case  CURLE_TFTP_NOTFOUND:
        case  CURLE_TFTP_PERM:
        case  CURLE_REMOTE_DISK_FULL:
        case  CURLE_TFTP_ILLEGAL:
        case  CURLE_TFTP_UNKNOWNID:
        case  CURLE_REMOTE_FILE_EXISTS:
        case  CURLE_REMOTE_FILE_NOT_FOUND:
        case  CURLE_SSH:
        case  CURLE_SSL_SHUTDOWN_FAILED:
        case  CURLE_AGAIN:
        case  CURLE_SSL_ISSUER_ERROR:
        case  CURLE_FTP_PRET_FAILED:
        case  CURLE_RTSP_CSEQ_ERROR:
        case  CURLE_RTSP_SESSION_ERROR:
        case  CURLE_FTP_BAD_FILE_LIST:
            return true;
        default:
            return false;
        }
    }

    int debug_callback(CURL *,
        curl_infotype ,
        char *data,
        size_t size,
        void *)
    {
        std::string smsg(data, size);
        OutputDebugStringA(smsg.c_str());
        return 0;
    }

    const CString FTPES_SCHEME = _T("ftpes://");
    const CString FTPS_SCHEME = _T("ftps://");
    const CString FTP_SCHEME = _T("ftp://");
}

CurlFtpConnection::CurlFtpConnection() :
    m_pListener(NULL),
    m_bSupportsMLSD(false)
{
    m_pCurl = curl_easy_init();

    // Common settings for all operations
    curl_easy_setopt(m_pCurl, CURLOPT_ERRORBUFFER, errorBuffer);

    //curl_easy_setopt(m_pCurl, CURLOPT_VERBOSE, 1L);
    //curl_easy_setopt(m_pCurl, CURLOPT_DEBUGFUNCTION, debug_callback);
}

void CurlFtpConnection::connect(CString serverUrl, CString username, CString password)
{
    // Extract the TLS type (if any from the URL)
    if (serverUrl.Find(FTPES_SCHEME) == 0) {
        // curl wants a regular ftp:// url instead of an ftpes:// url
        // but with CURLUSESSL_ALL it will force explicit SSL
        serverUrl = FTP_SCHEME + serverUrl.Mid(FTPES_SCHEME.GetLength());
        curl_easy_setopt(m_pCurl, CURLOPT_USE_SSL, CURLUSESSL_ALL);
    } if (serverUrl.Find(FTPS_SCHEME) == 0) {
        curl_easy_setopt(m_pCurl, CURLOPT_USE_SSL, CURLUSESSL_ALL);
    } else {
        // Must be a plain FTP url, no SSL
        curl_easy_setopt(m_pCurl, CURLOPT_USE_SSL, CURLUSESSL_NONE);
    }

    //curl_easy_setopt(m_pCurl, CURLOPT_SSL_VERIFYPEER, 0L);
    //curl_easy_setopt(m_pCurl, CURLOPT_SSL_VERIFYHOST, 0L);

    m_serverUrl = UTF8Convert::WideToUTF8(serverUrl);

    curl_easy_setopt(m_pCurl, CURLOPT_URL, m_serverUrl.c_str());
    std::string usernameUtf8 = UTF8Convert::WideToUTF8(username);
    curl_easy_setopt(m_pCurl, CURLOPT_USERNAME, usernameUtf8.c_str());
    std::string passwordUtf8 = UTF8Convert::WideToUTF8(password);
    curl_easy_setopt(m_pCurl, CURLOPT_PASSWORD, passwordUtf8.c_str());

    curl_easy_setopt(m_pCurl, CURLOPT_XFERINFOFUNCTION, progressCallback);
    curl_easy_setopt(m_pCurl, CURLOPT_ACCEPT_ENCODING, ""); // Accept all encodings include gzip/deflate, CURL will automatically decompress

    // Connection timeout after 10 seconds
    curl_easy_setopt(m_pCurl, CURLOPT_CONNECTTIMEOUT, 10);

    // Timeout if less than 1 byte is transferred over a period of 60 seconds
    curl_easy_setopt(m_pCurl, CURLOPT_LOW_SPEED_LIMIT, 1);
    curl_easy_setopt(m_pCurl, CURLOPT_LOW_SPEED_TIME, 60);

    // For connect send "FEAT" command to get supported features
    struct curl_slist* headers = NULL;
    headers = curl_slist_append(headers, "FEAT");
    curl_easy_setopt(m_pCurl, CURLOPT_QUOTE, headers);
    curl_easy_setopt(m_pCurl, CURLOPT_NOBODY, 1L);

    // Response to FEAT command will be sent on command channel which will
    // get written to header callback (unlike file downloads which are written
    // to body via write callback).
    std::ostringstream featureListStream;
    curl_easy_setopt(m_pCurl, CURLOPT_HEADERFUNCTION, writeCallback);
    curl_easy_setopt(m_pCurl, CURLOPT_HEADERDATA, &featureListStream);

    ProgressData progressData;
    if (m_pListener) {
        progressData.pListener = m_pListener;
        progressData.uploadSizeBytes = 0;
        curl_easy_setopt(m_pCurl, CURLOPT_PROGRESSDATA, &progressData);
        curl_easy_setopt(m_pCurl, CURLOPT_NOPROGRESS, 0);
    }

    CURLcode res = curl_easy_perform(m_pCurl);

    curl_slist_free_all(headers);
    curl_easy_setopt(m_pCurl, CURLOPT_QUOTE, NULL);
    curl_easy_setopt(m_pCurl, CURLOPT_HEADERFUNCTION, NULL);
    curl_easy_setopt(m_pCurl, CURLOPT_HEADERDATA, NULL);

    if (res != CURLE_OK) {
        if (res == CURLE_ABORTED_BY_CALLBACK)
            throw SyncCancelException();
        else if (res == CURLE_LOGIN_DENIED)
            throw SyncLoginDeniedError(100126);
        else if (isRetryableError(res))
            throw SyncRetryableNetworkError(100101, UTF8Convert::UTF8ToWide<CString>(errorBuffer));
        else
            throw SyncError(100101, UTF8Convert::UTF8ToWide<CString>(errorBuffer));
    }

    // Extract the features from the response
    std::stringstream ss(featureListStream.str());
    std::string line;
    bool bInFeatureList = false;
    while (std::getline(ss, line)) {
        if (line.compare(0, 3, "220") == 0)
        {
            // Welcome message. Includes server software name
            // so write to log file for debugging
            CLOG(INFO, "sync") << line;
        } else if (line.compare(0, 3, "211") == 0) {
            bInFeatureList = !bInFeatureList;
            CLOG(INFO, "sync") << line;
        } else if (bInFeatureList) {
            CLOG(INFO, "sync") << line;
            CString featureName = UTF8Convert::UTF8ToWide<CString>(line).Trim().MakeUpper();
            if (featureName == _T("MLSD"))
                m_bSupportsMLSD = true;
        }
    }
}

void CurlFtpConnection::disconnect()
{}

void CurlFtpConnection::download(CString remoteFilePath, CString localFilePath)
{
    std::ofstream localFileStream(localFilePath, std::ios::binary);
    return download(remoteFilePath, localFileStream);
}

void CurlFtpConnection::download(CString remoteFilePath, std::ostream& localFileStream)
{
    std::string downloadUrl = m_serverUrl + UTF8Convert::WideToUTF8(remoteFilePath);
    curl_easy_setopt(m_pCurl, CURLOPT_URL, downloadUrl.c_str());

    curl_easy_setopt(m_pCurl, CURLOPT_WRITEFUNCTION, writeCallback);
    curl_easy_setopt(m_pCurl, CURLOPT_WRITEDATA, &localFileStream);

    curl_easy_setopt(m_pCurl, CURLOPT_UPLOAD, 0L);
    curl_easy_setopt(m_pCurl, CURLOPT_READFUNCTION, NULL);
    curl_easy_setopt(m_pCurl, CURLOPT_READDATA, NULL);
    curl_easy_setopt(m_pCurl, CURLOPT_WILDCARDMATCH, 0L);
    curl_easy_setopt(m_pCurl, CURLOPT_CHUNK_BGN_FUNCTION, NULL);
    curl_easy_setopt(m_pCurl, CURLOPT_CHUNK_DATA, NULL);
    curl_easy_setopt(m_pCurl, CURLOPT_FTP_CREATE_MISSING_DIRS, CURLFTP_CREATE_DIR_NONE);
    curl_easy_setopt(m_pCurl, CURLOPT_NOBODY, 0L);
    curl_easy_setopt(m_pCurl, CURLOPT_FILETIME, 0L);

    ProgressData progressData;
    if (m_pListener) {
        progressData.pListener = m_pListener;
        progressData.uploadSizeBytes = 0;
        curl_easy_setopt(m_pCurl, CURLOPT_PROGRESSDATA, &progressData);
        curl_easy_setopt(m_pCurl, CURLOPT_NOPROGRESS, 0);
    }

    CURLcode res = curl_easy_perform(m_pCurl);

    if (res != CURLE_OK) {
        if (res == CURLE_ABORTED_BY_CALLBACK)
            throw SyncCancelException();
        else if (isRetryableError(res))
            throw SyncRetryableNetworkError(100119, UTF8Convert::UTF8ToWide<CString>(errorBuffer));
        else
            throw SyncError(100119, UTF8Convert::UTF8ToWide<CString>(errorBuffer));
    }
}

void CurlFtpConnection::upload(CString localFilePath, CString remoteFilePath)
{
    std::ifstream localFileStream(localFilePath, std::ios::binary);
    int64_t fileSizeBytes = PortableFunctions::FileSize(localFilePath);
    return upload(localFileStream, fileSizeBytes, remoteFilePath);
}

void CurlFtpConnection::upload(std::istream& localFileStream, int64_t fileSizeBytes, CString remoteFilePath)
{
    // Upload to temporary file on server so that you don't get half a file
    // if the upload is interrupted
    CString remoteName = PortableFunctions::PathGetFilename(remoteFilePath);
    CString remoteDirectory = PortableFunctions::PathGetDirectory<CString>(remoteFilePath);
    CString tempName = remoteName + _T("__uploading.tmp");
    CString remoteTempPath = PortableFunctions::PathAppendForwardSlashToPath(remoteDirectory, tempName);

    std::string uploadUrl = m_serverUrl + UTF8Convert::WideToUTF8(remoteTempPath);
    curl_easy_setopt(m_pCurl, CURLOPT_URL, uploadUrl.c_str());

    ReadData readData;
    readData.pIStream = &localFileStream;
    readData.readError = false;

    curl_easy_setopt(m_pCurl, CURLOPT_READFUNCTION, readCallback);
    curl_easy_setopt(m_pCurl, CURLOPT_READDATA, &readData);
    curl_easy_setopt(m_pCurl, CURLOPT_UPLOAD, 1L);
    curl_easy_setopt(m_pCurl, CURLOPT_FTP_CREATE_MISSING_DIRS, CURLFTP_CREATE_DIR);

    curl_easy_setopt(m_pCurl, CURLOPT_INFILESIZE_LARGE, (curl_off_t) fileSizeBytes);

    curl_easy_setopt(m_pCurl, CURLOPT_WRITEFUNCTION, NULL);
    curl_easy_setopt(m_pCurl, CURLOPT_WRITEDATA, NULL);
    curl_easy_setopt(m_pCurl, CURLOPT_WILDCARDMATCH, 0L);
    curl_easy_setopt(m_pCurl, CURLOPT_CHUNK_BGN_FUNCTION, NULL);
    curl_easy_setopt(m_pCurl, CURLOPT_CHUNK_DATA, NULL);
    curl_easy_setopt(m_pCurl, CURLOPT_NOBODY, 0L);
    curl_easy_setopt(m_pCurl, CURLOPT_FILETIME, 0L);

    ProgressData progressData;
    if (m_pListener) {
        progressData.pListener = m_pListener;
        progressData.uploadSizeBytes = 0;
        curl_easy_setopt(m_pCurl, CURLOPT_PROGRESSDATA, &progressData);
        curl_easy_setopt(m_pCurl, CURLOPT_NOPROGRESS, 0);
    }

    CURLcode res = curl_easy_perform(m_pCurl);

    if (res != CURLE_OK) {
        if (res == CURLE_ABORTED_BY_CALLBACK) {
            if (readData.readError == false)
                throw SyncCancelException();
            else
                throw SyncError(100134);
        }
        else if (isRetryableError(res))
            throw SyncRetryableNetworkError(100119, UTF8Convert::UTF8ToWide<CString>(errorBuffer));
        else
            throw SyncError(100119, UTF8Convert::UTF8ToWide<CString>(errorBuffer));
    }

    // Some servers (Filezilla for example) won't let you rename if the destination file already
    // exists. So need to delete file first before renaming.
    if (fileExists(remoteFilePath)) {
        deleteFile(remoteFilePath);
    }
    renameFile(remoteDirectory, tempName, remoteName);

}

std::vector<FileInfo>* CurlFtpConnection::getDirectoryListing(CString remotePath)
{
    if (m_bSupportsMLSD) {
        return getDirectoryListingMLSD(remotePath);
    } else {
        return getDirectoryListingLIST(remotePath);
    }
}

// Retrieve machine readable directory listing using MLSD command.
// This is the better method as it gives consistent and accurate
// modified date/times but it is not supported on all servers (e.g. IIS)
std::vector<FileInfo>* CurlFtpConnection::getDirectoryListingMLSD(CString remotePath)
{
    remotePath = forceEndWithSlash(remotePath);

    curl_easy_setopt(m_pCurl, CURLOPT_CUSTOMREQUEST, "MLSD");

    // Strangely to get the dirlist to work with MLSD you have to
    // tell CURL to ask for directory listing with files only.
    curl_easy_setopt(m_pCurl, CURLOPT_DIRLISTONLY, 1L);

    std::string downloadUrl = m_serverUrl + UTF8Convert::WideToUTF8(forceEndWithSlash(remotePath)) + "/*";
    curl_easy_setopt(m_pCurl, CURLOPT_URL, downloadUrl.c_str());

    std::ostringstream dirListStream;
    curl_easy_setopt(m_pCurl, CURLOPT_WRITEFUNCTION, writeCallback);
    curl_easy_setopt(m_pCurl, CURLOPT_WRITEDATA, &dirListStream);

    curl_easy_setopt(m_pCurl, CURLOPT_UPLOAD, 0L);
    curl_easy_setopt(m_pCurl, CURLOPT_READFUNCTION, NULL);
    curl_easy_setopt(m_pCurl, CURLOPT_READDATA, NULL);
    curl_easy_setopt(m_pCurl, CURLOPT_FTP_CREATE_MISSING_DIRS, CURLFTP_CREATE_DIR_NONE);
    curl_easy_setopt(m_pCurl, CURLOPT_NOBODY, 0L);
    curl_easy_setopt(m_pCurl, CURLOPT_FILETIME, 0L);

    ProgressData progressData;
    if (m_pListener) {
        progressData.pListener = m_pListener;
        progressData.uploadSizeBytes = 0;
        curl_easy_setopt(m_pCurl, CURLOPT_PROGRESSDATA, &progressData);
        curl_easy_setopt(m_pCurl, CURLOPT_NOPROGRESS, 0);
    }

    CURLcode res = curl_easy_perform(m_pCurl);

    curl_easy_setopt(m_pCurl, CURLOPT_DIRLISTONLY, 0L);

    if (res != CURLE_OK) {
        if (res == CURLE_ABORTED_BY_CALLBACK)
            throw SyncCancelException();
        else if (isRetryableError(res))
            throw SyncRetryableNetworkError(100119, UTF8Convert::UTF8ToWide<CString>(errorBuffer));
        else
            throw SyncError(100119, UTF8Convert::UTF8ToWide<CString>(errorBuffer));
    }

    std::unique_ptr<std::vector<FileInfo>> pFiles(new std::vector<FileInfo>());

    // MLSD response format is described in https://tools.ietf.org/html/rfc3659#page-23
    std::stringstream ss(dirListStream.str());
    std::string line;
    while (std::getline(ss, line)) {

        CString fileLine = UTF8Convert::UTF8ToWide<CString>(line);

        int spacePos = fileLine.Find(_T(' '));
        if (spacePos < 0) {
            CLOG(ERROR, "sync") << "Missing ' ' in MLSD directory listing:" << line;
            throw SyncError(100121);
        }
        CString filename = fileLine.Mid(spacePos + 1);
        if (filename[filename.GetLength() - 1] == _T('\r'))
            filename = filename.Left(filename.GetLength() - 1);

        FileInfo::FileType fileType = FileInfo::FileType::File;
        time_t lastModified = -1;
        int64_t fileSize = -1;

        CString facts = fileLine.Mid(0, spacePos);
        int iCurrentFact = 0;
        CString fact = facts.Tokenize(_T(";"), iCurrentFact);
        while (fact != _T("")) {
            int equalsPos = fact.Find(_T('='));
            if (equalsPos < 0) {
                CLOG(ERROR, "sync") << "Fact missing '=' in MLSD directory listing:" << line;
                throw SyncError(100121);
            }
            CString name = fact.Mid(0, equalsPos);
            CString val = fact.Mid(equalsPos + 1);
            if (name.CompareNoCase(_T("type")) == 0) {
                if (val.CompareNoCase(_T("dir")) == 0 || val.CompareNoCase(_T("cdir")) == 0 || val.CompareNoCase(_T("pdir")) == 0) {
                    fileType = FileInfo::FileType::Directory;
                }
            } else if (name.CompareNoCase(_T("modify")) == 0) {
                // Remove the fractional seconds (after digit 14) since they won't fit in time_t
                if (val.GetLength() > 14) {
                    val = val.Left(14);
                }
                lastModified = PortableFunctions::ParseYYYYMMDDhhmmssDateTime(CS2WS(val));
            } else if (name.CompareNoCase(_T("size")) == 0) {
                char *endptr;
                fileSize = strtoll(UTF8Convert::WideToUTF8(val).c_str(), &endptr, 10);
            }
            fact = facts.Tokenize(_T(";"), iCurrentFact);
        }

        pFiles->emplace_back(fileType, filename, remotePath, fileSize, lastModified, std::wstring());
    }

    return pFiles.release();
}

// Get directory listing using CURL default method (LIST command) which
// is supported on all servers but doesn't give consistent format
// and modified times are not as accurate.
std::vector<FileInfo>* CurlFtpConnection::getDirectoryListingLIST(CString remotePath)
{
    curl_easy_setopt(m_pCurl, CURLOPT_WILDCARDMATCH, 1L);
    curl_easy_setopt(m_pCurl, CURLOPT_CHUNK_BGN_FUNCTION, dirListBeginChunkCallback);

    std::unique_ptr<std::vector<FileInfo>> pFiles(new std::vector<FileInfo>());
    DirListCallbackData callbackData;
    callbackData.pFiles = pFiles.get();
    callbackData.directoryPath = remotePath;
    curl_easy_setopt(m_pCurl, CURLOPT_CHUNK_DATA, &callbackData);

    std::string downloadUrl = m_serverUrl + UTF8Convert::WideToUTF8(forceEndWithSlash(remotePath)) + "/*";
    curl_easy_setopt(m_pCurl, CURLOPT_URL, downloadUrl.c_str());

    curl_easy_setopt(m_pCurl, CURLOPT_WRITEFUNCTION, NULL);
    curl_easy_setopt(m_pCurl, CURLOPT_WRITEDATA, NULL);

    curl_easy_setopt(m_pCurl, CURLOPT_UPLOAD, 0L);
    curl_easy_setopt(m_pCurl, CURLOPT_READFUNCTION, NULL);
    curl_easy_setopt(m_pCurl, CURLOPT_READDATA, NULL);
    curl_easy_setopt(m_pCurl, CURLOPT_FTP_CREATE_MISSING_DIRS, CURLFTP_CREATE_DIR_NONE);
    curl_easy_setopt(m_pCurl, CURLOPT_NOBODY, 0L);
    curl_easy_setopt(m_pCurl, CURLOPT_FILETIME, 0L);

    ProgressData progressData;
    if (m_pListener) {
        progressData.pListener = m_pListener;
        progressData.uploadSizeBytes = 0;
        curl_easy_setopt(m_pCurl, CURLOPT_PROGRESSDATA, &progressData);
        curl_easy_setopt(m_pCurl, CURLOPT_NOPROGRESS, 0);
    }

    CURLcode res = curl_easy_perform(m_pCurl);

    if (res != CURLE_OK) {
        if (res == CURLE_ABORTED_BY_CALLBACK)
            throw SyncCancelException();
        else if (isRetryableError(res))
            throw SyncRetryableNetworkError(100119, UTF8Convert::UTF8ToWide<CString>(errorBuffer));
        else
            throw SyncError(100119, UTF8Convert::UTF8ToWide<CString>(errorBuffer));
    }

    return pFiles.release();
}

time_t CurlFtpConnection::getLastModifiedTime(CString remotePath)
{
    std::string fileUrl = m_serverUrl + UTF8Convert::WideToUTF8(remotePath);
    curl_easy_setopt(m_pCurl, CURLOPT_URL, fileUrl.c_str());

    curl_easy_setopt(m_pCurl, CURLOPT_NOBODY, 1L);
    curl_easy_setopt(m_pCurl, CURLOPT_FILETIME, 1L);
    curl_easy_setopt(m_pCurl, CURLOPT_READFUNCTION, NULL);
    curl_easy_setopt(m_pCurl, CURLOPT_READDATA, NULL);
    curl_easy_setopt(m_pCurl, CURLOPT_WRITEFUNCTION, ignoreCallback);
    curl_easy_setopt(m_pCurl, CURLOPT_WRITEDATA, NULL);
    curl_easy_setopt(m_pCurl, CURLOPT_WILDCARDMATCH, 0L);
    curl_easy_setopt(m_pCurl, CURLOPT_CHUNK_BGN_FUNCTION, NULL);
    curl_easy_setopt(m_pCurl, CURLOPT_CHUNK_DATA, NULL);
    curl_easy_setopt(m_pCurl, CURLOPT_UPLOAD, 0L);
    curl_easy_setopt(m_pCurl, CURLOPT_FTP_CREATE_MISSING_DIRS, CURLFTP_CREATE_DIR_NONE);

    ProgressData progressData;
    if (m_pListener) {
        progressData.pListener = m_pListener;
        progressData.uploadSizeBytes = 0;
        curl_easy_setopt(m_pCurl, CURLOPT_PROGRESSDATA, &progressData);
        curl_easy_setopt(m_pCurl, CURLOPT_NOPROGRESS, 0);
    }

    CURLcode res = curl_easy_perform(m_pCurl);

    if (res != CURLE_OK) {
        if (res == CURLE_ABORTED_BY_CALLBACK)
            throw SyncCancelException();
        else if (isRetryableError(res))
            throw SyncRetryableNetworkError(100119, UTF8Convert::UTF8ToWide<CString>(errorBuffer));
        else
            throw SyncError(100119, UTF8Convert::UTF8ToWide<CString>(errorBuffer));
    }

    long filetime = -1;
    res = curl_easy_getinfo(m_pCurl, CURLINFO_FILETIME, &filetime);
    if ((CURLE_OK == res) && (filetime >= 0)) {
        return (time_t) filetime;
    }

    return time_t(0);
}

CurlFtpConnection::~CurlFtpConnection()
{
    curl_easy_cleanup(m_pCurl);
}

void CurlFtpConnection::setListener(ISyncListener* pListener)
{
    m_pListener = pListener;
}

void CurlFtpConnection::renameFile(CString directory, CString originalName, CString newName)
{
    std::string uploadUrl = m_serverUrl + UTF8Convert::WideToUTF8(directory);
    curl_easy_setopt(m_pCurl, CURLOPT_URL, uploadUrl.c_str());

    struct curl_slist* headerlist = NULL;
    CString renameFrom = _T("RNFR ") + originalName;
    CString renameTo = _T("RNTO ") + newName;
    headerlist = curl_slist_append(headerlist, UTF8Convert::WideToUTF8(renameFrom).c_str());
    headerlist = curl_slist_append(headerlist, UTF8Convert::WideToUTF8(renameTo).c_str());
    curl_easy_setopt(m_pCurl, CURLOPT_POSTQUOTE, headerlist);

    curl_easy_setopt(m_pCurl, CURLOPT_READFUNCTION, NULL);
    curl_easy_setopt(m_pCurl, CURLOPT_UPLOAD, 0L);
    curl_easy_setopt(m_pCurl, CURLOPT_WRITEFUNCTION, NULL);
    curl_easy_setopt(m_pCurl, CURLOPT_WRITEDATA, NULL);
    curl_easy_setopt(m_pCurl, CURLOPT_WILDCARDMATCH, 0L);
    curl_easy_setopt(m_pCurl, CURLOPT_CHUNK_BGN_FUNCTION, NULL);
    curl_easy_setopt(m_pCurl, CURLOPT_CHUNK_DATA, NULL);
    curl_easy_setopt(m_pCurl, CURLOPT_NOBODY, 1L);
    curl_easy_setopt(m_pCurl, CURLOPT_FILETIME, 0L);

    ProgressData progressData;
    if (m_pListener) {
        progressData.pListener = m_pListener;
        progressData.uploadSizeBytes = 0;
        curl_easy_setopt(m_pCurl, CURLOPT_PROGRESSDATA, &progressData);
        curl_easy_setopt(m_pCurl, CURLOPT_NOPROGRESS, 0);
    }

    CURLcode res = curl_easy_perform(m_pCurl);

    curl_slist_free_all(headerlist);
    curl_easy_setopt(m_pCurl, CURLOPT_POSTQUOTE, NULL);

    if (res != CURLE_OK) {
        if (res == CURLE_ABORTED_BY_CALLBACK) {
            throw SyncCancelException();
        }
        else if (isRetryableError(res))
            throw SyncRetryableNetworkError(100119, UTF8Convert::UTF8ToWide<CString>(errorBuffer));
        else
            throw SyncError(100119, UTF8Convert::UTF8ToWide<CString>(errorBuffer));
    }
}

void CurlFtpConnection::deleteFile(CString filepath)
{
    std::string uploadUrl = m_serverUrl + UTF8Convert::WideToUTF8(PortableFunctions::PathGetDirectory(filepath));
    curl_easy_setopt(m_pCurl, CURLOPT_URL, uploadUrl.c_str());

    struct curl_slist* headerlist = NULL;
    CString deleteCmd = _T("DELE ") + CString(PortableFunctions::PathGetFilename(filepath));
    headerlist = curl_slist_append(headerlist, UTF8Convert::WideToUTF8(deleteCmd).c_str());
    curl_easy_setopt(m_pCurl, CURLOPT_POSTQUOTE, headerlist);

    curl_easy_setopt(m_pCurl, CURLOPT_READFUNCTION, NULL);
    curl_easy_setopt(m_pCurl, CURLOPT_UPLOAD, 0L);
    curl_easy_setopt(m_pCurl, CURLOPT_WRITEFUNCTION, NULL);
    curl_easy_setopt(m_pCurl, CURLOPT_WRITEDATA, NULL);
    curl_easy_setopt(m_pCurl, CURLOPT_WILDCARDMATCH, 0L);
    curl_easy_setopt(m_pCurl, CURLOPT_CHUNK_BGN_FUNCTION, NULL);
    curl_easy_setopt(m_pCurl, CURLOPT_CHUNK_DATA, NULL);
    curl_easy_setopt(m_pCurl, CURLOPT_NOBODY, 1L);
    curl_easy_setopt(m_pCurl, CURLOPT_FILETIME, 0L);

    ProgressData progressData;
    if (m_pListener) {
        progressData.pListener = m_pListener;
        progressData.uploadSizeBytes = 0;
        curl_easy_setopt(m_pCurl, CURLOPT_PROGRESSDATA, &progressData);
        curl_easy_setopt(m_pCurl, CURLOPT_NOPROGRESS, 0);
    }

    CURLcode res = curl_easy_perform(m_pCurl);

    curl_slist_free_all(headerlist);
    curl_easy_setopt(m_pCurl, CURLOPT_POSTQUOTE, NULL);

    if (res != CURLE_OK) {
        if (res == CURLE_ABORTED_BY_CALLBACK) {
            throw SyncCancelException();
        }
        else if (isRetryableError(res))
            throw SyncRetryableNetworkError(100119, UTF8Convert::UTF8ToWide<CString>(errorBuffer));
        else
            throw SyncError(100119, UTF8Convert::UTF8ToWide<CString>(errorBuffer));
    }
}

bool CurlFtpConnection::fileExists(CString filePath)
{
    try
    {
        getLastModifiedTime(filePath);
        return true;
    }
    catch (const SyncError&)
    {
    }

    return false;
}
