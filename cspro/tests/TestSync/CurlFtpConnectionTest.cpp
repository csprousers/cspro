#include "stdafx.h"
#include "CppUnitTest.h"
#include <zNetwork/CurlFtpConnection.h>
#include <zSyncO/SyncException.h>
#include <fstream>

namespace Microsoft {
    namespace VisualStudio {
        namespace CppUnitTestFramework
        {
            // ToString specialization is required for all types used in the AssertTrue macro
            template<> inline std::wstring ToString<CString>(const CString& t) { return std::wstring(t); }
        }
    }
}

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace SyncUnitTest
{
    CString m_serverUrl = L"ftp://localhost/";
    CString m_username = L"test";
    CString m_password = L"";

/*  CString m_serverUrl = L"ftp://127.0.0.1/";
    CString m_username = L"anonymous";
    CString m_password = L"thing@chose.com"; */

    CString makeGuid()
    {
        UUID uuid;
        UuidCreate(&uuid);
        WCHAR *str;
        UuidToStringW(&uuid, (RPC_WSTR*) &str);
        CString guid(str);
        RpcStringFreeW((RPC_WSTR*) &str);
        return guid;
    }

    CString createTempFile(CString name, CString data)
    {
        wchar_t pszTempPath[MAX_PATH];
        GetTempPath(MAX_PATH, pszTempPath);

        CString srcFilePath;
        srcFilePath.Format(L"%s%s", (LPCTSTR)pszTempPath, (LPCTSTR)name);
        DeleteFile(srcFilePath);

        std::wofstream srcWriteStream(srcFilePath);
        srcWriteStream << (LPCWSTR) data;
        srcWriteStream.close();

        return srcFilePath;
    }

    TEST_CLASS(CurlFtpConnectionTest)
    {
    public:
        TEST_METHOD(TestConnect)
        {
            CurlFtpConnection connection;

            connection.connect(m_serverUrl, m_username, m_password);

            connection.disconnect();
        }

        TEST_METHOD(TestUploadFile)
        {
            CurlFtpConnection connection;

            connection.connect(m_serverUrl, m_username, m_password);

            wchar_t pszTempPath[MAX_PATH];
            GetTempPath(MAX_PATH, pszTempPath);

            CString data = makeGuid();
            CString srcFilePath = createTempFile(L"zsynco-ftp-upload-test.txt", data);

            CString serverPath(L"/foo/bar/test-file.txt");

            connection.upload(srcFilePath, serverPath);

            DeleteFile(srcFilePath);

            connection.disconnect();
        }

        TEST_METHOD(TestDownloadFile)
        {
            CurlFtpConnection connection;

            connection.connect(m_serverUrl, m_username, m_password);

            wchar_t pszTempPath[MAX_PATH];
            GetTempPath(MAX_PATH, pszTempPath);

            CString data = makeGuid();
            CString srcFilePath = createTempFile(L"zsynco-ftp-upload-test.txt", data);

            CString serverPath(L"/foo/bar/test-file.txt");

            connection.upload(srcFilePath, serverPath);

            DeleteFile(srcFilePath);

            connection.download(L"/foo/bar/test-file.txt", srcFilePath);

            Assert::IsTrue(GetFileAttributes(srcFilePath) != -1);

            std::wifstream srcReadStream(srcFilePath);
            std::wstring actualContents(std::istreambuf_iterator<wchar_t>(srcReadStream), {});
            srcReadStream.close();

            Assert::AreEqual((LPCWSTR) data, actualContents.c_str());

            DeleteFile(srcFilePath);

            connection.disconnect();
        }

        TEST_METHOD(TestDirectoryListing)
        {
            CurlFtpConnection connection;

            connection.connect(m_serverUrl, m_username, m_password);

            // Create a temp directory on server
            CString serverPath = CString(L"/test/") + makeGuid() + L"/";

            // Upload some files
            for (int i = 0; i < 5; ++i) {

                CString fileName;
                fileName.Format(_T("file%d"), i);
                CString data = makeGuid();
                CString srcFilePath = createTempFile(fileName, data);
                CString remoteFilePath;
                remoteFilePath.Format(_T("%sfile%d"), (LPCTSTR)serverPath, i);

                connection.upload(srcFilePath, remoteFilePath);

                DeleteFile(srcFilePath);
            }

            // Create a subdirectory by uploading a file
            CString data = makeGuid();
            CString srcFilePath = createTempFile(L"subdirfile.txt", data);
            connection.upload(srcFilePath, serverPath + L"subdir/subdirfile.txt");

            std::unique_ptr<std::vector<FileInfo>> pFiles(connection.getDirectoryListing(serverPath));
            for (int i = 0; i < 5; ++i) {
                CString expectedName;
                expectedName.Format(_T("file%d"), i);
                auto match = std::find_if(pFiles->begin(), pFiles->end(), [&expectedName](const FileInfo& i) {
                    return i.getName() == expectedName;
                });
                Assert::IsFalse(match == pFiles->end(), expectedName + L" missing from directory listing");
                Assert::IsTrue(match->getType() == FileInfo::FileType::File, expectedName + L" not file in directory listing");
                Assert::AreEqual(data.GetLength(), (int) match->getSize());
                Assert::AreEqual(serverPath, match->getDirectory());
            }

            auto match = std::find_if(pFiles->begin(), pFiles->end(), [](const FileInfo& i) {
                return i.getName() == L"subdir";
            });
            Assert::IsFalse(match == pFiles->end(), L"subdir missing from directory listing");
            Assert::IsTrue(match->getType() == FileInfo::FileType::Directory, L"subdir not directory in directory listing");

            connection.disconnect();

        }

        TEST_METHOD(TestFtps)
        {
            CurlFtpConnection connection;

            connection.connect(L"ftps://test.rebex.net", L"demo", L"password");

            std::unique_ptr<std::vector<FileInfo>> pFiles(connection.getDirectoryListing(L"/"));

            std::ostringstream oss;
            connection.download(L"/readme.txt", oss);
            auto s = oss.str();

            connection.disconnect();
        }

        TEST_METHOD(TestFtpes)
        {
            CurlFtpConnection connection;

            connection.connect(L"ftpes://test.rebex.net", L"demo", L"password");

            std::unique_ptr<std::vector<FileInfo>> pFiles(connection.getDirectoryListing(L"/"));

            std::ostringstream oss;
            connection.download(L"/readme.txt", oss);
            auto s = oss.str();

            connection.disconnect();
        }

    };
}
