#include "stdafx.h"

#include "CppUnitTest.h"
#include <zSyncO/SyncClient.h>
#include <zSyncO/SyncServerConnectionFactory.h>
#include <fstream>

namespace Microsoft {
    namespace VisualStudio {
        namespace CppUnitTestFramework
        {
            // ToString specialization is required for all types used in the AssertTrue macro
            template<> inline std::wstring ToString<CString>(const CString& t) { return std::wstring(t); }
            template<> inline std::wstring ToString<SyncClient::SyncResult>(const SyncClient::SyncResult& r)
            {
                switch (r) {
                case SyncClient::SyncResult::SYNC_OK:
                    return L"SyncResult::SYNC_OK";
                case SyncClient::SyncResult::SYNC_ERROR:
                    return L"SyncResult::SYNC_ERROR";
                case SyncClient::SyncResult::SYNC_CANCELED:
                    return L"SyncResult::SYNC_CANCELED";
                }
                return L"THIS SHOULD NEVER HAPPEN";
            };
        }
    }
}

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace fakeit;

namespace SyncUnitTest
{
    TEST_CLASS(DropboxIntegrationTest)
    {
    private:

        CString accessToken = L"2v_60xKfBaAAAAAAAAACvIYQ8KDfoNQkR2VgBJiTt5bGbLtQhnBiXKOl8xAuaLED";

    private:

        CString makeGuid()
        {
            UUID uuid;
            UuidCreate(&uuid);
            WCHAR *str;
            UuidToStringW(&uuid, (RPC_WSTR*)&str);
            CString guid(str);
            RpcStringFreeW((RPC_WSTR*)&str);
            return guid;
        }

        // Make a new device id that is unique so we can test sync
        // on an existing server as if we started with a new device.

        DeviceId makeUniqueDeviceId(CString prefix)
        {
            // Currently device id is limited to 16 chars on server
            // so use prefix and fill rest with guid which should
            // unique enough.
            return prefix + makeGuid().Left(16-prefix.GetLength());
        }

    public:

        TEST_METHOD(Dropbox_TestConnectDisconnect)
        {
            DeviceId clientDeviceId = L"mydevice";
            SyncServerConnectionFactory serverFactory(NULL);
            SyncClient client(clientDeviceId, &serverFactory);

            auto result = client.connectDropbox(accessToken);
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);

            result = client.disconnect();
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);
        }

        TEST_METHOD(Dropbox_TestPutGetFile)
        {
            DeviceId clientDeviceId = L"mydevice";
            SyncServerConnectionFactory serverFactory(NULL);

            SyncClient client(clientDeviceId, &serverFactory);

            auto result = client.connectDropbox(accessToken);
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);

            wchar_t pszTempPath[MAX_PATH];
            GetTempPath(MAX_PATH, pszTempPath);

            CString srcFilePath;
            srcFilePath.Format(L"%szsynco-get-file-test.txt", pszTempPath);
            DeleteFile(srcFilePath);

            std::wofstream srcWriteStream(srcFilePath);
            CString data = makeGuid();
            srcWriteStream << (LPCWSTR) data;
            srcWriteStream.close();

            CString serverPath(L"/foo/bar/test.txt");

            auto syncResult = client.syncFile(SyncDirection::Put, srcFilePath, serverPath, CString());
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, syncResult);

            DeleteFile(srcFilePath);

            syncResult = client.syncFile(SyncDirection::Get, serverPath, srcFilePath, CString());
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, syncResult);

            // Get file again and make sure it is not re-downloaded by checking modified time
            struct _stat attrib;
            _wstat(srcFilePath, &attrib);
            time_t lastMod = attrib.st_mtime;
            syncResult = client.syncFile(SyncDirection::Get, serverPath, srcFilePath, CString());
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, syncResult);
            _wstat(srcFilePath, &attrib);
            Assert::AreEqual(lastMod, attrib.st_mtime, _T("File was modified by second download"));

            result = client.disconnect();
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);

            // File exists
            Assert::IsTrue(GetFileAttributes(srcFilePath) != -1);

            std::wifstream srcReadStream(srcFilePath);
            std::wstring actualContents(std::istreambuf_iterator<wchar_t>(srcReadStream), {});
            srcReadStream.close();

            Assert::AreEqual((LPCWSTR) data, actualContents.c_str());

            DeleteFile(srcFilePath);
        }
    };
}
