#include <engine/StandardSystemIncludes.h>
#include "AndroidFtpConnection.h"
#include "JNIHelpers.h"
#include <zToolsO/FileIO.h>
#include <zToolsO/PortableFunctions.h>
#include <zUtilO/TemporaryFile.h>
#include <zNetwork/FileInfo.h>
#include <zNetwork/SyncException.h>
#include <zNetwork/SyncListener.h>
#include <android/log.h>


AndroidFtpConnection::AndroidFtpConnection()
    :   m_env(GetJNIEnvForCurrentThread())
{
    jobject impl = m_env->NewObject(JNIReferences::classAndroidFtpConnection, JNIReferences::methodAndroidFtpConnectionConstructor);
    m_javaImpl = m_env->NewGlobalRef(impl);
}


AndroidFtpConnection::~AndroidFtpConnection()
{
    // Release ref to java implementation
    m_env->DeleteGlobalRef(m_javaImpl);
}


void AndroidFtpConnection::SetSyncListener(std::shared_ptr<SyncListener> sync_listener)
{
    FtpConnection::SetSyncListener(std::move(sync_listener));

    jlong jNativeListener = (intptr_t)m_syncListener.get();
    jobject jListener = m_env->NewObject(JNIReferences::classSyncListenerWrapper, JNIReferences::methodSyncListenerWrapperConstructor, jNativeListener);
    m_env->CallVoidMethod(m_javaImpl, JNIReferences::methodAndroidFtpConnectionSetListener, jListener);
}


std::string AndroidFtpConnection::DoConnect(const std::string& username, const std::string& password)
{
    // Convert args to java
    jstring jUrl = JavaString::ToJava(*m_env, m_providedUrl);
    jstring jUsername = JavaString::ToJava(*m_env, username);
    jstring jPassword = JavaString::ToJava(*m_env, password);

    m_env->CallVoidMethod(m_javaImpl,
        JNIReferences::methodAndroidFtpConnectionConnect,
        jUrl, jUsername, jPassword);

    // Java could throw an IOException
    jthrowable exception = m_env->ExceptionOccurred();
    if (exception) {
        m_env->ExceptionClear();
    }

    m_env->DeleteLocalRef(jUrl);
    m_env->DeleteLocalRef(jUsername);
    m_env->DeleteLocalRef(jPassword);

    if (exception) {
        if (m_env->IsInstanceOf(exception, m_env->FindClass("gov/census/cspro/smartsync/SyncCancelException"))) {
            __android_log_print(ANDROID_LOG_DEBUG, "AndroidFtpClient", "Cancel exception");
            m_env->DeleteLocalRef(exception);
            throw SyncCancelException();
        }

        if (m_env->IsInstanceOf(exception, m_env->FindClass("gov/census/cspro/smartsync/SyncLoginDeniedError"))) {
            m_env->DeleteLocalRef(exception);
            throw SyncLoginDeniedError(100126);
        }

        logException(m_env, ANDROID_LOG_DEBUG, "AndroidFtpClient", exception);

        throw SyncConnectionError(exceptionToString(m_env, exception));
    }

    return m_providedUrl;
}


void AndroidFtpConnection::DoDisconnect()
{
    m_env->CallVoidMethod(m_javaImpl,
        JNIReferences::methodAndroidFtpConnectionDisconnect);

    // Java could throw an exception
    jthrowable exception = m_env->ExceptionOccurred();
    if (exception) {
        m_env->ExceptionClear();
    }

    if (exception) {
        if (m_env->IsInstanceOf(exception, m_env->FindClass("gov/census/cspro/smartsync/SyncCancelException"))) {
            __android_log_print(ANDROID_LOG_DEBUG, "AndroidFtpClient", "Cancel exception");
            m_env->DeleteLocalRef(exception);
            throw SyncCancelException();
        }

        logException(m_env, ANDROID_LOG_DEBUG, "AndroidFtpClient", exception);
        throw SyncError(100101, exceptionToString(m_env, exception));
    }
}


void AndroidFtpConnection::Download(const std::string& remote_file_path, const std::string& local_file_path)
{
    // Convert args to java
    jstring jRemoteFilePath = JavaString::ToJava(*m_env, remote_file_path);
    jstring jLocalFilePath = JavaString::ToJava(*m_env, local_file_path);

    m_env->CallVoidMethod(m_javaImpl,
        JNIReferences::methodAndroidFtpConnectionDownload,
        jRemoteFilePath, jLocalFilePath);

    // Java could throw an IOException
    jthrowable exception = m_env->ExceptionOccurred();
    if (exception) {
        m_env->ExceptionClear();
    }

    m_env->DeleteLocalRef(jRemoteFilePath);
    m_env->DeleteLocalRef(jLocalFilePath);

    if (exception) {
        if (m_env->IsInstanceOf(exception, m_env->FindClass("gov/census/cspro/smartsync/SyncCancelException"))) {
            __android_log_print(ANDROID_LOG_DEBUG, "AndroidFtpClient", "Cancel exception");
            m_env->DeleteLocalRef(exception);
            throw SyncCancelException();
        }

        logException(m_env, ANDROID_LOG_DEBUG, "AndroidFtpClient", exception);
        throw SyncError(100101, exceptionToString(m_env, exception));
    }
}


void AndroidFtpConnection::Download(const std::string& remote_file_path, std::ostream& output_stream)
{
    // Ftp4j doesn't have a public method to download to an output stream so we need to use a temp file
    TemporaryFile tempFile;
    Download(remote_file_path, tempFile.GetPath());

    try
    {
        const std::string content = FileIO::ReadText(tempFile.GetPath());
        output_stream << content;
    }

    catch( const std::exception& exception )
    {
        throw SyncError(100101, exception);
    }
}


void AndroidFtpConnection::Upload(const std::string& local_file_path, const std::string& remote_file_path)
{
    // Convert args to java
    jstring jRemoteFilePath = JavaString::ToJava(*m_env, remote_file_path);
    jstring jLocalFilePath = JavaString::ToJava(*m_env, local_file_path);

    m_env->CallVoidMethod(m_javaImpl,
        JNIReferences::methodAndroidFtpConnectionUpload,
        jLocalFilePath, jRemoteFilePath);

    // Java could throw an IOException
    jthrowable exception = m_env->ExceptionOccurred();
    if (exception) {
        m_env->ExceptionClear();
    }

    m_env->DeleteLocalRef(jRemoteFilePath);
    m_env->DeleteLocalRef(jLocalFilePath);

    if (exception) {
        if (m_env->IsInstanceOf(exception, m_env->FindClass("gov/census/cspro/smartsync/SyncCancelException"))) {
            __android_log_print(ANDROID_LOG_DEBUG, "AndroidFtpClient", "Cancel exception");
            m_env->DeleteLocalRef(exception);
            throw SyncCancelException();
        }

        logException(m_env, ANDROID_LOG_DEBUG, "AndroidFtpClient", exception);
        throw SyncError(100101, exceptionToString(m_env, exception));
    }
}


void AndroidFtpConnection::Upload(std::istream& input_stream, const int64_t input_size_bytes, const std::string& remote_file_path)
{
    // Convert args to java
    jstring jRemoteFilePath = JavaString::ToJava(*m_env, remote_file_path);

    // This cast is important or you end up storing the full 64 bit value which messes us up later
    // when we extract the ptr from the jlong.
    jlong jnativeLocalFileData = (intptr_t)&input_stream;
    jobject jlocalFileData = m_env->NewObject(JNIReferences::classIStreamWrapper, JNIReferences::methodIStreamWrapperConstructor, jnativeLocalFileData);

    m_env->CallVoidMethod(m_javaImpl,
        JNIReferences::methodAndroidFtpConnectionUploadStream,
        jlocalFileData, (jlong)input_size_bytes, jRemoteFilePath);

    // Java could throw an IOException
    jthrowable exception = m_env->ExceptionOccurred();
    if (exception) {
        m_env->ExceptionClear();
    }

    m_env->DeleteLocalRef(jRemoteFilePath);
    m_env->DeleteLocalRef(jlocalFileData);

    if (exception) {
        if (m_env->IsInstanceOf(exception, m_env->FindClass("gov/census/cspro/smartsync/SyncCancelException"))) {
            __android_log_print(ANDROID_LOG_DEBUG, "AndroidFtpClient", "Cancel exception");
            m_env->DeleteLocalRef(exception);
            throw SyncCancelException();
        }

        logException(m_env, ANDROID_LOG_DEBUG, "AndroidFtpClient", exception);
        throw SyncError(100101, exceptionToString(m_env, exception));
    }
}


bool AndroidFtpConnection::FileExists(const std::string& remote_path) // FTP_TODO properly implement
{
    return ( FileIsRegular(remote_path) || FileIsDirectory(remote_path) );
}


bool AndroidFtpConnection::FileIsRegular(const std::string& remote_path) // FTP_TODO properly implement
{
    try
    {
        FileModifiedTime(remote_path);
        return true;
    }
    catch(...) { }

    return false;
}


bool AndroidFtpConnection::FileIsDirectory(const std::string& remote_path) // FTP_TODO properly implement
{
    try
    {
        // FileModifiedTime doesn't work on directories (at least not on all servers),
        // so we need to do a directory listing of the parent here
        const std::string remote_path_without_trailing_slash = Path::RemoveTrailingSlash(remote_path);
        const std::string parent_remote_path = PortableFunctions::PathGetDirectory(remote_path_without_trailing_slash);
        const std::string directory_name = Path::GetFilename(remote_path_without_trailing_slash);

        const std::vector<FileInfo> directory_listing = AndroidFtpConnection::GetDirectoryListing(parent_remote_path, false);

        const auto& lookup = std::find_if(directory_listing.cbegin(), directory_listing.cend(),
            [&](const FileInfo& fi)
            {
                return ( fi.GetType() == FileInfo::FileType::Directory &&
                         fi.GetName() == directory_name );
            });

        if( lookup != directory_listing.cend() )
            return true;
    }
    catch(...) { }

    return false;
}


int64_t AndroidFtpConnection::FileModifiedTime(const std::string& remote_path)
{
    // Convert args to java
    jstring jRemotePath = JavaString::ToJava(*m_env, remote_path);

    jlong lastModified = m_env->CallLongMethod(m_javaImpl,
        JNIReferences::methodAndroidFtpConnectionGetLastModifiedTime,
        jRemotePath);

    // Java could throw an IOException
    jthrowable exception = m_env->ExceptionOccurred();
    if (exception) {
        m_env->ExceptionClear();
    }

    m_env->DeleteLocalRef(jRemotePath);

    if (exception) {
        if (m_env->IsInstanceOf(exception, m_env->FindClass("gov/census/cspro/smartsync/SyncCancelException"))) {
            __android_log_print(ANDROID_LOG_DEBUG, "AndroidFtpClient", "Cancel exception");
            m_env->DeleteLocalRef(exception);
            throw SyncCancelException();
        }

        logException(m_env, ANDROID_LOG_DEBUG, "AndroidFtpClient", exception);
        throw SyncError(100101, exceptionToString(m_env, exception));
    }

    return lastModified;
}


std::vector<FileInfo> AndroidFtpConnection::GetDirectoryListing(const std::string& remote_directory_path, bool /*request_file_md5s*/)
{
    // Convert args to java
    jstring jRemotePath = JavaString::ToJava(*m_env, remote_directory_path);

    jobjectArray jFileInfoArray = (jobjectArray)m_env->CallObjectMethod(m_javaImpl,
        JNIReferences::methodAndroidFtpConnectionGetDirectoryListing,
        jRemotePath);

    // Java could throw an IOException
    jthrowable exception = m_env->ExceptionOccurred();
    if (exception) {
        m_env->ExceptionClear();
    }

    m_env->DeleteLocalRef(jRemotePath);

    jsize count = ( jFileInfoArray == nullptr ) ? 0 : m_env->GetArrayLength(jFileInfoArray);

    std::vector<FileInfo> directory_listing;
    for (int i = 0; i < count; ++i) {
        jobject jFileInfo = m_env->GetObjectArrayElement(jFileInfoArray, i);
        jstring jName = (jstring) m_env->CallObjectMethod(jFileInfo, JNIReferences::methodFileInfoGetName);
        bool isDir = m_env->CallBooleanMethod(jFileInfo, JNIReferences::methodFileInfoGetIsDirectory);
        int64_t size = m_env->CallLongMethod(jFileInfo, JNIReferences::methodFileInfoGetSize);
        jlong lastModified = m_env->CallLongMethod(jFileInfo, JNIReferences::methodFileInfoGetLastModifiedTimeSeconds);
        directory_listing.emplace_back(isDir ? FileInfo::FileType::Directory : FileInfo::FileType::File,
                                       JavaString::ToUtf8(*m_env, jName), remote_directory_path, size, lastModified);

        m_env->DeleteLocalRef(jName);
        m_env->DeleteLocalRef(jFileInfo);
    }

    m_env->DeleteLocalRef(jFileInfoArray);

    if (exception) {
        if (m_env->IsInstanceOf(exception, m_env->FindClass("gov/census/cspro/smartsync/SyncCancelException"))) {
            __android_log_print(ANDROID_LOG_DEBUG, "AndroidFtpClient", "Cancel exception");
            m_env->DeleteLocalRef(exception);
            throw SyncCancelException();
        }

        logException(m_env, ANDROID_LOG_DEBUG, "AndroidFtpClient", exception);
        throw SyncError(100101, exceptionToString(m_env, exception));
    }

    return directory_listing;
}


void AndroidFtpConnection::FileRename(const std::string& old_remote_file_path, const std::string& new_remote_file_path) // FTP_TODO implement
{
    old_remote_file_path; new_remote_file_path;
    ASSERT(false);
}


void AndroidFtpConnection::FileDelete(const std::string& remote_file_path) // FTP_TODO implement
{
    remote_file_path;
    ASSERT(false);
}


void AndroidFtpConnection::DirectoryDelete(const std::string& remote_directory_path) // FTP_TODO implement
{
    remote_directory_path;
    ASSERT(false);
}
