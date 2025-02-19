#include "stdafx.h"
#include "ApplicationsTester.h"
#include "CaseTestHelpers.h"
#include <zToolsO/DirectoryLister.h>
#include <zSyncO/SyncMessage.h>
#include <zSyncO/SyncServiceFactory.h>

namespace SyncUnitTest { class FileBasedSyncServiceTest; }


TEST_CLASS(SyncUnitTest::FileBasedSyncServiceTest)
{
public:
    FileBasedSyncServiceTest();
    ~FileBasedSyncServiceTest();

    TEST_METHOD(TestDictionaries);

    TEST_METHOD(TestApplications);

    TEST_METHOD(TestMessages);

private:
    SyncClient CreateSyncClient();

    size_t CountFilesInDirectory(const std::string& remote_path, bool recursive);

private:
    std::string m_directory;
};



SyncUnitTest::FileBasedSyncServiceTest::FileBasedSyncServiceTest()
    :   m_directory(Path::Combine(GetTempDirectory(), "FileBasedSyncServiceTest-" + IntToString(GetTimestamp<int64_t>())))
{
    PortableFunctions::PathMakeDirectory(m_directory);
}


SyncUnitTest::FileBasedSyncServiceTest::~FileBasedSyncServiceTest()
{
    PortableFunctions::DirectoryDelete(m_directory, true);
}


SyncClient SyncUnitTest::FileBasedSyncServiceTest::CreateSyncClient()
{
    const DeviceId device_id = "FileBasedSyncServiceTest-" + IntToString(GetTimestamp<int64_t>());

    SyncClient sync_client(device_id, std::make_unique<SyncServiceFactory>(std::make_unique<SyncLoginAccessor>()));
    sync_client.SetSyncListener(std::make_unique<SyncLogSyncListener>());

    const SyncConnectionString sync_connection_string = SyncConnectionString::CreateLocalFilesSyncConnectionString(m_directory);

    const SyncClient::SyncResult result = sync_client.ConnectLocalFiles(sync_connection_string);
    Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);

    return sync_client;
}


size_t SyncUnitTest::FileBasedSyncServiceTest::CountFilesInDirectory(const std::string& remote_path, const bool recursive)
{
    const std::string evaluated_path = Path::Combine(m_directory, remote_path);
    return DirectoryLister(recursive).GetPaths(evaluated_path).size();
}


void SyncUnitTest::FileBasedSyncServiceTest::TestDictionaries()
{
    SyncClient sync_client = CreateSyncClient();

    const std::unique_ptr<CDataDict> dictionary = CreateTestDictionaryWithUniqueName();

    SyncClient::SyncResult result;

    // test 1: ensure that the dictionary does not already exist
    std::string retrieved_dictionary_text;
    result = sync_client.DownloadDictionary(dictionary->GetSyncableName(), retrieved_dictionary_text);
    Assert::AreEqual(SyncClient::SyncResult::SYNC_ERROR, result);


    // test 2: upload the dictionary
    TemporaryFile dictionary_temporary_file;
    dictionary->Save(dictionary_temporary_file.GetPath());

    result = sync_client.UploadDictionary(dictionary_temporary_file.GetPath());
    Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);

    const std::string remote_dictionary_directory = "/CSPro/DataSync/" + dictionary->GetSyncableName() + "/dict/";
    Assert::AreEqual(size_t(2), CountFilesInDirectory(remote_dictionary_directory, false));


    // test 3: get the dictionary
    result = sync_client.DownloadDictionary(dictionary->GetSyncableName(), retrieved_dictionary_text);
    Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);
    Assert::AreEqual(Json::Parse(FileIO::ReadText(dictionary_temporary_file.GetPath())).GetNodeAsString(),
                     Json::Parse(retrieved_dictionary_text).GetNodeAsString());


    // test 4: delete the dictionary
    result = sync_client.DeleteDictionary(dictionary->GetSyncableName());
    Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);
    Assert::AreEqual(size_t(0), CountFilesInDirectory(remote_dictionary_directory, false));


    // test 5: ensure that the dictionary does not already exist
    result = sync_client.DownloadDictionary(dictionary->GetSyncableName(), retrieved_dictionary_text);
    Assert::AreEqual(SyncClient::SyncResult::SYNC_ERROR, result);
}


void SyncUnitTest::FileBasedSyncServiceTest::TestApplications()
{
    const std::string remote_apps_directory = "/CSPro/apps/";

    SyncClient sync_client = CreateSyncClient();

    Assert::AreEqual(size_t(0), CountFilesInDirectory(remote_apps_directory, false));

    ApplicationsTester applications_tester(ApplicationPackage::DeploymentType::None, std::string());

    applications_tester.RunTest(sync_client, 0,
        [&]()
        {
            Assert::AreEqual(size_t(4), CountFilesInDirectory(remote_apps_directory, false));
        });

    Assert::AreEqual(size_t(0), CountFilesInDirectory(remote_apps_directory, false));
}


void SyncUnitTest::FileBasedSyncServiceTest::TestMessages()
{
    const std::string remote_messages_directory = "/CSPro/messages/";

    SyncClient sync_client = CreateSyncClient();

    // test 1: sync a message
    const SyncMessage sync_message1("Message Key", "Message Value 1");
    std::optional<JsonNode> sync_message_response = sync_client.SendSyncMessage(sync_message1);
    Assert::IsFalse(sync_message_response.has_value());
    Assert::AreEqual(size_t(1), CountFilesInDirectory(remote_messages_directory, false));


    // test 2: sync another message with the same timestamp
    const SyncMessage sync_message2(sync_message1.GetTimestamp(), "Message Key", "Message Value 2");
    sync_message_response = sync_client.SendSyncMessage(sync_message2);
    Assert::IsFalse(sync_message_response.has_value());
    Assert::AreEqual(size_t(2), CountFilesInDirectory(remote_messages_directory, false));


    // test 3: sync another message with a different timestamp
    const SyncMessage sync_message3(sync_message1.GetTimestamp() + 10, "Message Key", Json::Parse("{ \"test\": 3, \"text\": \"Message Value 3\" }"));
    sync_message_response = sync_client.SendSyncMessage(sync_message3);
    Assert::IsFalse(sync_message_response.has_value());
    Assert::AreEqual(size_t(3), CountFilesInDirectory(remote_messages_directory, false));
}
