#include "AndroidFtpConnection.h"
#include "JNIHelpers.h"
#include <zToolsO/FileIO.h>
#include <zUtilO/TemporaryFile.h>
#include <zSyncO/SyncException.h>
#include <zSyncO/ISyncListener.h>
#include <android/log.h>


#define JNI_VERSION JNI_VERSION_1_6

AndroidFtpConnection::AndroidFtpConnection()
    : m_pEnv(GetJNIEnvForCurrentThread())
{
    jobject impl = m_pEnv->NewObject(JNIReferences::classAndroidFtpConnection, JNIReferences::methodAndroidFtpConnectionConstructor);
    m_javaImpl = m_pEnv->NewGlobalRef(impl);
}

AndroidFtpConnection::~AndroidFtpConnection()
{
    // Release ref to java implementation
    m_pEnv->DeleteGlobalRef(m_javaImpl);
}

void AndroidFtpConnection::connect(CString serverUrl, CString username, CString password)
{
    // Convert args to java
    jstring jUrl = WideToJava(m_pEnv, serverUrl);
    jstring jUsername = WideToJava(m_pEnv, username);
    jstring jPassword = WideToJava(m_pEnv, password);

    m_pEnv->CallVoidMethod(m_javaImpl,
        JNIReferences::methodAndroidFtpConnectionConnect,
        jUrl, jUsername, jPassword);

    // Java could throw an IOException
    jthrowable exception = m_pEnv->ExceptionOccurred();
    if (exception) {
        m_pEnv->ExceptionClear();
    }

    m_pEnv->DeleteLocalRef(jUrl);
    m_pEnv->DeleteLocalRef(jUsername);
    m_pEnv->DeleteLocalRef(jPassword);

    if (exception) {

        if (m_pEnv->IsInstanceOf(exception, m_pEnv->FindClass("gov/census/cspro/smartsync/SyncCancelException"))) {
            __android_log_print(ANDROID_LOG_DEBUG, "AndroidFtpClient", "Cancel exception");
            m_pEnv->DeleteLocalRef(exception);
            throw SyncCancelException();
        }

        if (m_pEnv->IsInstanceOf(exception, m_pEnv->FindClass("gov/census/cspro/smartsync/SyncLoginDeniedError"))) {
            m_pEnv->DeleteLocalRef(exception);
            throw SyncLoginDeniedError(100126);
        }

        logException(m_pEnv, ANDROID_LOG_DEBUG, "AndroidFtpClient", exception);

        std::wstring exceptionMessage = exceptionToString(m_pEnv, exception);
        throw SyncError(100101, exceptionMessage);
    }
}

void AndroidFtpConnection::disconnect()
{
    m_pEnv->CallVoidMethod(m_javaImpl,
        JNIReferences::methodAndroidFtpConnectionDisconnect);

    // Java could throw an exception
    jthrowable exception = m_pEnv->ExceptionOccurred();
    if (exception) {
        m_pEnv->ExceptionClear();
    }

    if (exception) {

        if (m_pEnv->IsInstanceOf(exception, m_pEnv->FindClass("gov/census/cspro/smartsync/SyncCancelException"))) {
            __android_log_print(ANDROID_LOG_DEBUG, "AndroidFtpClient", "Cancel exception");
            m_pEnv->DeleteLocalRef(exception);
            throw SyncCancelException();
        }

        logException(m_pEnv, ANDROID_LOG_DEBUG, "AndroidFtpClient", exception);
        std::wstring exceptionMessage = exceptionToString(m_pEnv, exception);
        throw SyncError(100119, exceptionMessage);
    }
}

void AndroidFtpConnection::download(CString remoteFilePath, CString localFilePath)
{
    // Convert args to java
    jstring jRemoteFilePath = WideToJava(m_pEnv, remoteFilePath);
    jstring jLocalFilePath = WideToJava(m_pEnv, localFilePath);

    m_pEnv->CallVoidMethod(m_javaImpl,
        JNIReferences::methodAndroidFtpConnectionDownload,
        jRemoteFilePath, jLocalFilePath);

    // Java could throw an IOException
    jthrowable exception = m_pEnv->ExceptionOccurred();
    if (exception) {
        m_pEnv->ExceptionClear();
    }

    m_pEnv->DeleteLocalRef(jRemoteFilePath);
    m_pEnv->DeleteLocalRef(jLocalFilePath);

    if (exception) {

        if (m_pEnv->IsInstanceOf(exception, m_pEnv->FindClass("gov/census/cspro/smartsync/SyncCancelException"))) {
            __android_log_print(ANDROID_LOG_DEBUG, "AndroidFtpClient", "Cancel exception");
            m_pEnv->DeleteLocalRef(exception);
            throw SyncCancelException();
        }

        logException(m_pEnv, ANDROID_LOG_DEBUG, "AndroidFtpClient", exception);
        std::wstring exceptionMessage = exceptionToString(m_pEnv, exception);
        throw SyncError(100119, exceptionMessage);
    }
}

void AndroidFtpConnection::download(CString remoteFilePath, std::ostream& localFileStream)
{
    // Ftp4j doesn't have a public method to download to an output stream so we
    // need to use a temp file
    TemporaryFile tempFile;
    download(remoteFilePath, WS2CS(tempFile.GetPath()));
    try {
        std::string content = FileIO::ReadText<std::string>(tempFile.GetPath());
        localFileStream << content;
    } catch (const std::exception& e)
    {
        throw SyncError(100119, e.what());
    }
}

void AndroidFtpConnection::upload(CString localFilePath, CString remoteFilePath)
{
    // Convert args to java
    jstring jRemoteFilePath = WideToJava(m_pEnv, remoteFilePath);
    jstring jLocalFilePath = WideToJava(m_pEnv, localFilePath);

    m_pEnv->CallVoidMethod(m_javaImpl,
        JNIReferences::methodAndroidFtpConnectionUpload,
        jLocalFilePath, jRemoteFilePath);

    // Java could throw an IOException
    jthrowable exception = m_pEnv->ExceptionOccurred();
    if (exception) {
        m_pEnv->ExceptionClear();
    }

    m_pEnv->DeleteLocalRef(jRemoteFilePath);
    m_pEnv->DeleteLocalRef(jLocalFilePath);

    if (exception) {

        if (m_pEnv->IsInstanceOf(exception, m_pEnv->FindClass("gov/census/cspro/smartsync/SyncCancelException"))) {
            __android_log_print(ANDROID_LOG_DEBUG, "AndroidFtpClient", "Cancel exception");
            m_pEnv->DeleteLocalRef(exception);
            throw SyncCancelException();
        }

        logException(m_pEnv, ANDROID_LOG_DEBUG, "AndroidFtpClient", exception);
        std::wstring exceptionMessage = exceptionToString(m_pEnv, exception);
        throw SyncError(100119, exceptionMessage);
    }
}

void AndroidFtpConnection::upload(std::istream& localFileData, int64_t fileSizeBytes, CString remoteFilePath)
{
    // Convert args to java
    jstring jRemoteFilePath = WideToJava(m_pEnv, remoteFilePath);

    // This cast is important or you end up storing the full 64 bit value which messes us up later
    // when we extract the ptr from the jlong.
    jlong jnativeLocalFileData = (intptr_t) &localFileData;
    jobject jlocalFileData = m_pEnv->NewObject(JNIReferences::classIStreamWrapper, JNIReferences::methodIStreamWrapperConstructor, jnativeLocalFileData);

    m_pEnv->CallVoidMethod(m_javaImpl,
        JNIReferences::methodAndroidFtpConnectionUploadStream,
        jlocalFileData, (jlong) fileSizeBytes, jRemoteFilePath);

    // Java could throw an IOException
    jthrowable exception = m_pEnv->ExceptionOccurred();
    if (exception) {
        m_pEnv->ExceptionClear();
    }

    m_pEnv->DeleteLocalRef(jRemoteFilePath);
    m_pEnv->DeleteLocalRef(jlocalFileData);

    if (exception) {

        if (m_pEnv->IsInstanceOf(exception, m_pEnv->FindClass("gov/census/cspro/smartsync/SyncCancelException"))) {
            __android_log_print(ANDROID_LOG_DEBUG, "AndroidFtpClient", "Cancel exception");
            m_pEnv->DeleteLocalRef(exception);
            throw SyncCancelException();
        }

        logException(m_pEnv, ANDROID_LOG_DEBUG, "AndroidFtpClient", exception);
        std::wstring exceptionMessage = exceptionToString(m_pEnv, exception);
        throw SyncError(100119, exceptionMessage);
    }
}

std::vector<FileInfo>* AndroidFtpConnection::getDirectoryListing(CString remotePath)
{
    // Convert args to java
    jstring jRemotePath = WideToJava(m_pEnv, remotePath);

    jobjectArray jFileInfoArray = (jobjectArray) m_pEnv->CallObjectMethod(m_javaImpl,
        JNIReferences::methodAndroidFtpConnectionGetDirectoryListing,
        jRemotePath);

    // Java could throw an IOException
    jthrowable exception = m_pEnv->ExceptionOccurred();
    if (exception) {
        m_pEnv->ExceptionClear();
    }

    m_pEnv->DeleteLocalRef(jRemotePath);

    jsize count = jFileInfoArray == NULL ? 0 : m_pEnv->GetArrayLength(jFileInfoArray);

    std::unique_ptr<std::vector<FileInfo>> pFileInfoVector(new std::vector<FileInfo>());
    for (int i = 0; i < count; ++i) {
        jobject jFileInfo = m_pEnv->GetObjectArrayElement(jFileInfoArray, i);
        jstring jName = (jstring) m_pEnv->CallObjectMethod(jFileInfo, JNIReferences::methodFileInfoGetName);
        bool isDir = m_pEnv->CallBooleanMethod(jFileInfo, JNIReferences::methodFileInfoGetIsDirectory);
        int64_t size = m_pEnv->CallLongMethod(jFileInfo, JNIReferences::methodFileInfoGetSize);
        jlong lastModified = m_pEnv->CallLongMethod(jFileInfo, JNIReferences::methodFileInfoGetLastModifiedTimeSeconds);
        pFileInfoVector->emplace_back(isDir ? FileInfo::FileType::Directory : FileInfo::FileType::File,
            JavaToWSZ(m_pEnv, jName), remotePath, size, lastModified, std::wstring());

        m_pEnv->DeleteLocalRef(jName);
        m_pEnv->DeleteLocalRef(jFileInfo);
    }

    m_pEnv->DeleteLocalRef(jFileInfoArray);

    if (exception) {

        if (m_pEnv->IsInstanceOf(exception, m_pEnv->FindClass("gov/census/cspro/smartsync/SyncCancelException"))) {
            __android_log_print(ANDROID_LOG_DEBUG, "AndroidFtpClient", "Cancel exception");
            m_pEnv->DeleteLocalRef(exception);
            throw SyncCancelException();
        }

        logException(m_pEnv, ANDROID_LOG_DEBUG, "AndroidFtpClient", exception);
        std::wstring exceptionMessage = exceptionToString(m_pEnv, exception);
        throw SyncError(100119, exceptionMessage);
    }
    return pFileInfoVector.release();
}

time_t AndroidFtpConnection::getLastModifiedTime(CString remotePath)
{
    // Convert args to java
    jstring jRemotePath = WideToJava(m_pEnv, remotePath);

    jlong lastModified = m_pEnv->CallLongMethod(m_javaImpl,
        JNIReferences::methodAndroidFtpConnectionGetLastModifiedTime,
        jRemotePath);

    // Java could throw an IOException
    jthrowable exception = m_pEnv->ExceptionOccurred();
    if (exception) {
        m_pEnv->ExceptionClear();
    }

    m_pEnv->DeleteLocalRef(jRemotePath);

    if (exception) {

        if (m_pEnv->IsInstanceOf(exception, m_pEnv->FindClass("gov/census/cspro/smartsync/SyncCancelException"))) {
            __android_log_print(ANDROID_LOG_DEBUG, "AndroidFtpClient", "Cancel exception");
            m_pEnv->DeleteLocalRef(exception);
            throw SyncCancelException();
        }

        logException(m_pEnv, ANDROID_LOG_DEBUG, "AndroidFtpClient", exception);

        std::wstring exceptionMessage = exceptionToString(m_pEnv, exception);
        throw SyncError(100119, exceptionMessage);
    }
    return time_t(lastModified);
}

void AndroidFtpConnection::setListener(ISyncListener* pListener)
{
    jlong jNativeListener = (intptr_t) pListener;
    jobject jListener = m_pEnv->NewObject(JNIReferences::classSyncListenerWrapper, JNIReferences::methodSyncListenerWrapperConstructor, jNativeListener);
    m_pEnv->CallVoidMethod(m_javaImpl, JNIReferences::methodAndroidFtpConnectionSetListener, jListener);
}
