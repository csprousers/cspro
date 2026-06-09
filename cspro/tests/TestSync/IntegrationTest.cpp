#include "stdafx.h"
#include "ApplicationsTester.h"
#include "CaseTestHelpers.h"
#include "FailableHttpConnection.h"
#include "SyncTestCredentials.h"
#include "TestRepoBuilder.h"
#include <zDictO/DDClass.h>
#include <zCaseO/CaseItemReference.h>
#include <zNetwork/SyncCredentialStore.h>
#include <zSyncO/CSWebSyncService.h>
#include <zSyncO/NetworkDataChunk.h>
#include <zSyncO/SyncLoginAccessor.h>
#include <zSyncO/SyncMessage.h>
#include <zSyncO/SyncServiceFactory.h>
#include <fstream>

using namespace fakeit;


namespace SyncUnitTest
{
    TEST_CLASS(IntegrationTest)
    {
    private:
        const SyncTestCredentials::Credentials credentials = SyncTestCredentials().GetCredentialsCSWeb();

    private:
        std::unique_ptr<const CDataDict> dictionary = CreateTestDictionary();
        std::shared_ptr<CaseAccess> case_access = CaseAccess::CreateAndInitializeFullCaseAccess(*dictionary);

        void ResetServer()
        {
            DeviceId clientDeviceId = "mydevice";
            SyncClient client(clientDeviceId, std::make_unique<SyncServiceFactory>());
            client.SetSyncListener(std::make_unique<SyncLogSyncListener>());

            SyncClient::SyncResult result = client.ConnectCSWeb(credentials.sync_connection_string, std::make_unique<LoginCredentials>(credentials.username_password));
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);

            result = client.DeleteDictionary("TESTDICT_DICT");

            result = client.UploadDictionary(Path::Combine(GetTestFilesDirectory(), "TestDict.dcf"));
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);

            result = client.Disconnect();
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);
        }

        void CompareNotes(const std::vector<Note>& actual, const std::vector<Note>& expected)
        {
            Assert::AreEqual(expected.size(), actual.size(), L"Note vectors different sizes");
            for (auto iExpected = expected.begin(), iActual = actual.begin(); iExpected != expected.end(); ++iExpected, ++iActual) {
                Assert::AreEqual(iExpected->GetContent(), iActual->GetContent());
                const NamedReference& expectedField = iExpected->GetNamedReference();
                const NamedReference& actualField = iActual->GetNamedReference();
                Assert::AreEqual(expectedField.GetName(), actualField.GetName());
                Assert::AreEqual(expectedField.GetLevelKey(), actualField.GetLevelKey());
                Assert::AreEqual(expectedField.GetOneBasedOccurrences()[0], actualField.GetOneBasedOccurrences()[0]);
                Assert::AreEqual(expectedField.GetOneBasedOccurrences()[1], actualField.GetOneBasedOccurrences()[1]);
                Assert::AreEqual(expectedField.GetOneBasedOccurrences()[2], actualField.GetOneBasedOccurrences()[2]);
                Assert::AreEqual(iExpected->GetOperatorId(), iActual->GetOperatorId());
                Assert::AreEqual<int64_t>(iExpected->GetModifiedDateTime(), iActual->GetModifiedDateTime());
            }
        }

    public:
        TEST_METHOD(TestConnectDisconnect)
        {
            DeviceId clientDeviceId = "mydevice";
            SyncClient client(clientDeviceId, std::make_unique<SyncServiceFactory>());
            client.SetSyncListener(std::make_unique<SyncLogSyncListener>());

            SyncClient::SyncResult result = client.ConnectCSWeb(credentials.sync_connection_string, std::make_unique<LoginCredentials>(credentials.username_password));
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);

            result = client.Disconnect();
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);
        }


        TEST_METHOD(TestSyncData)
        {
            ResetServer();

            DeviceId client1DeviceId = MakeUniqueDeviceId("it1-");

            TestRepoBuilder repoBuilder(client1DeviceId, dictionary.get());
            ISyncableDataRepository* pRepo = repoBuilder.GetRepo();

            SyncClient client1(client1DeviceId, std::make_unique<SyncServiceFactory>());
            client1.SetSyncListener(std::make_unique<SyncLogSyncListener>());

            SyncClient::SyncResult result = client1.ConnectCSWeb(credentials.sync_connection_string, std::make_unique<LoginCredentials>(credentials.username_password));
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);

            // First sync - empty server, shouldn't get any cases
            result = client1.SyncData(*pRepo, SyncDirection::Both, "");
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);
            Assert::AreEqual(size_t(0), pRepo->GetNumberCases());

            // Add a couple of new cases
            VectorClock newClock;
            newClock.increment(client1DeviceId);
            std::string newCase1Guid = CreateUuid();
            repoBuilder.addRepoCase(newCase1Guid, 1, { "NEWCASE1DATA" });
            std::string newCase2Guid = CreateUuid();
            repoBuilder.addRepoCase(newCase2Guid, 2, { "NEWCASE2DATA" });
            result = client1.SyncData(*pRepo, SyncDirection::Both, "");
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);
            Assert::AreEqual(size_t(2), pRepo->GetNumberCases());

            // Update one of the new cases
            const std::string updatedCaseData("THISDATAWASUPDATED");
            repoBuilder.updateRepoCase(newCase1Guid, 1, updatedCaseData);

            // Still shouldn't get any new cases back
            result = client1.SyncData(*pRepo, SyncDirection::Both, "");
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);
            Assert::AreEqual(size_t(2), pRepo->GetNumberCases());

            result = client1.Disconnect();
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);

            // Sync with a second client to check if our changes were saved to the server
            DeviceId client2DeviceId = MakeUniqueDeviceId("it2-");
            repoBuilder.ResetRepo(client2DeviceId, dictionary.get());
            pRepo = repoBuilder.GetRepo();

            SyncClient client2(client2DeviceId, std::make_unique<SyncServiceFactory>());
            client2.SetSyncListener(std::make_unique<SyncLogSyncListener>());

            result = client2.ConnectCSWeb(credentials.sync_connection_string, std::make_unique<LoginCredentials>(credentials.username_password));
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);
            result = client2.SyncData(*pRepo, SyncDirection::Both, "");
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);
            Assert::AreEqual(size_t(2), pRepo->GetNumberCases());

            // Make sure case got updateded
            repoBuilder.verifyCaseInRepo(newCase1Guid, updatedCaseData);

            // Make sure other new case got added
            repoBuilder.verifyCaseInRepo(newCase2Guid, "NEWCASE2DATA");

            result = client2.Disconnect();
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);
        }


        TEST_METHOD(TestSyncPartialSave)
        {
            ResetServer();

            DeviceId client1DeviceId = MakeUniqueDeviceId("it1-");

            TestRepoBuilder repoBuilder(client1DeviceId, dictionary.get());
            ISyncableDataRepository* pRepo = repoBuilder.GetRepo();

            SyncClient client1(client1DeviceId, std::make_unique<SyncServiceFactory>());
            client1.SetSyncListener(std::make_unique<SyncLogSyncListener>());

            // Test cases
            VectorClock newClock;
            newClock.increment(client1DeviceId);
            std::string caseGuid = CreateUuid();
            std::shared_ptr<Case> initCase = CreateCase(*case_access, caseGuid, 1, { "NEWCASE1DATA-1", "NEWCASE1DATA-2" }, false);
            initCase->SetVectorClock(newClock);
            SetPartialSave(*case_access, *initCase, PartialSaveMode::Add, "TEST_ITEM", 1, 0, 0);
            repoBuilder.setInitialRepoCases({ initCase }, client1DeviceId);

            SyncClient::SyncResult result = client1.ConnectCSWeb(credentials.sync_connection_string, std::make_unique<LoginCredentials>(credentials.username_password));
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);

            // Upload test case to server
            result = client1.SyncData(*pRepo, SyncDirection::Put, "");
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);

            result = client1.Disconnect();
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);

            // Sync with a second client to check if the partial save status
            // was saved correctly
            DeviceId client2DeviceId = MakeUniqueDeviceId("it2-");
            repoBuilder.ResetRepo(client2DeviceId, dictionary.get());
            pRepo = repoBuilder.GetRepo();

            SyncClient client2(client2DeviceId, std::make_unique<SyncServiceFactory>());
            client2.SetSyncListener(std::make_unique<SyncLogSyncListener>());

            result = client2.ConnectCSWeb(credentials.sync_connection_string, std::make_unique<LoginCredentials>(credentials.username_password));
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);
            result = client2.SyncData(*pRepo, SyncDirection::Get, "");
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);

            // Make sure partial save status came through
            std::shared_ptr<Case> repo_case = repoBuilder.findCaseByUuid(caseGuid);
            Assert::IsTrue(repo_case != nullptr, L"Case not found");
            Assert::AreEqual(initCase->GetPartialSaveMode(), repo_case->GetPartialSaveMode());
            const CaseItemReference* const actual_field_ref = repo_case->GetPartialSaveCaseItemReference();
            const CaseItemReference* const expected_field_ref = initCase->GetPartialSaveCaseItemReference();
            Assert::AreEqual(expected_field_ref->GetCaseItem().GetDictItem().GetName(), actual_field_ref->GetCaseItem().GetDictItem().GetName());
            Assert::AreEqual(expected_field_ref->GetLevelKey(), actual_field_ref->GetLevelKey());
            Assert::AreEqual(expected_field_ref->GetRecordOccurrence(), actual_field_ref->GetRecordOccurrence());
            Assert::AreEqual(expected_field_ref->GetItemOccurrence(), actual_field_ref->GetItemOccurrence());
            Assert::AreEqual(expected_field_ref->GetSubitemOccurrence(), actual_field_ref->GetSubitemOccurrence());

            result = client2.Disconnect();
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);
        }


        TEST_METHOD(TestSyncNotes)
        {
            ResetServer();

            DeviceId client1DeviceId = MakeUniqueDeviceId("it1-");

            TestRepoBuilder repoBuilder(client1DeviceId, dictionary.get());
            ISyncableDataRepository* pRepo = repoBuilder.GetRepo();

            SyncClient client1(client1DeviceId, std::make_unique<SyncServiceFactory>());
            client1.SetSyncListener(std::make_unique<SyncLogSyncListener>());

            // Test cases
            VectorClock newClock;
            newClock.increment(client1DeviceId);
            std::string caseGuid = CreateUuid();
            std::vector<Note> notes;
            notes.emplace_back(createNote(*case_access, "note1content", "TEST_ITEM", 0, 0, 0, "op1", time(nullptr)));
            notes.emplace_back(createNote(*case_access, "note2content", "TEST_ITEM",  1, 0, 0, "op2", time(nullptr)));

            std::shared_ptr<Case> init_case = CreateCase(*case_access, caseGuid, 1, { "DATA-1", "DATA-2" }, false);
            init_case->SetNotes(notes);
            init_case->SetVectorClock(newClock);
            std::vector<std::shared_ptr<Case>> initCases = { init_case };
            repoBuilder.setInitialRepoCases(initCases, client1DeviceId);

            SyncClient::SyncResult result = client1.ConnectCSWeb(credentials.sync_connection_string, std::make_unique<LoginCredentials>(credentials.username_password));
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);

            // Upload test case to server
            result = client1.SyncData(*pRepo, SyncDirection::Put, "");
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);

            // Sync with a second client to check if the notes were saved correctly
            DeviceId client2DeviceId = MakeUniqueDeviceId("it2-");
            repoBuilder.ResetRepo(client2DeviceId, dictionary.get());
            pRepo = repoBuilder.GetRepo();

            SyncClient client2(client2DeviceId, std::make_unique<SyncServiceFactory>());
            client2.SetSyncListener(std::make_unique<SyncLogSyncListener>());

            result = client2.ConnectCSWeb(credentials.sync_connection_string, std::make_unique<LoginCredentials>(credentials.username_password));
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);
            result = client2.SyncData(*pRepo, SyncDirection::Get, "");
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);

            // Make sure notes came through
            std::unique_ptr<Case> repo_case = repoBuilder.findCaseByUuid(init_case->GetUuid());
            CompareNotes(notes, repo_case->GetNotes());

            // Update note on client1 and resync
            notes[0] = createNote(*case_access, "note1contentmodified", "TEST_ITEM", 1, 0, 0, "op1", time(nullptr));
            repoBuilder.updateRepoCaseNotes(caseGuid, notes);
            result = client1.SyncData(*pRepo, SyncDirection::Put, "");
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);

            // Download to client2
            result = client2.SyncData(*pRepo, SyncDirection::Get, "");
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);

            // Make sure update came through
            repo_case = repoBuilder.findCaseByUuid(init_case->GetUuid());
            CompareNotes(notes, repo_case->GetNotes());

            result = client1.Disconnect();
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);

            result = client2.Disconnect();
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);
        }


        TEST_METHOD(TestSyncUnicode)
        {
            const char* unicodeData = u8"🐼🐼🐼 有人做过一。Кто-то странное.";
            DeviceId client1DeviceId = MakeUniqueDeviceId("it1-");
            TestRepoBuilder repoBuilder(client1DeviceId, dictionary.get());
            ISyncableDataRepository* pRepo = repoBuilder.GetRepo();

            SyncClient client(client1DeviceId, std::make_unique<SyncServiceFactory>());
            client.SetSyncListener(std::make_unique<SyncLogSyncListener>());

            SyncClient::SyncResult result = client.ConnectCSWeb(credentials.sync_connection_string, std::make_unique<LoginCredentials>(credentials.username_password));
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);

            // Add a case with unicode characters
            VectorClock newClock;
            std::string newCaseGuid = CreateUuid();
            repoBuilder.addRepoCase(newCaseGuid, 1, unicodeData);

            // Sync the new case
            result = client.SyncData(*pRepo, SyncDirection::Both, "");
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);

            result = client.Disconnect();
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);

            // Sync with a second client to check if our case was saved correctly
            DeviceId client2DeviceId = MakeUniqueDeviceId("it2-");
            repoBuilder.ResetRepo(client2DeviceId, dictionary.get());
            pRepo = repoBuilder.GetRepo();

            SyncClient client2(client2DeviceId, std::make_unique<SyncServiceFactory>());
            client2.SetSyncListener(std::make_unique<SyncLogSyncListener>());

            result = client2.ConnectCSWeb(credentials.sync_connection_string, std::make_unique<LoginCredentials>(credentials.username_password));
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);
            result = client2.SyncData(*pRepo, SyncDirection::Both, "");
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);

            // Make sure case got synced correctly
            repoBuilder.verifyCaseInRepo(newCaseGuid, unicodeData);

            result = client2.Disconnect();
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);
        }


        TEST_METHOD(TestSyncVectorClock)
        {
            ResetServer();

            DeviceId client1DeviceId = MakeUniqueDeviceId("it1-");
            TestRepoBuilder client1RepoBuilder(client1DeviceId, dictionary.get());
            ISyncableDataRepository* pClient1Repo = client1RepoBuilder.GetRepo();

            SyncClient client(client1DeviceId, std::make_unique<SyncServiceFactory>());
            client.SetSyncListener(std::make_unique<SyncLogSyncListener>());

            SyncClient::SyncResult result = client.ConnectCSWeb(credentials.sync_connection_string, std::make_unique<LoginCredentials>(credentials.username_password));
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);

            // First sync
            result = client.SyncData(*pClient1Repo, SyncDirection::Both, "");
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);
            Assert::AreEqual(size_t(0), pClient1Repo->GetNumberCases());

            // Add a few new cases
            VectorClock newClock;
            newClock.increment(client1DeviceId);
            std::string client2NewerCaseGuid = CreateUuid();
            std::shared_ptr<Case> client2NewerCase = CreateCase(*case_access, client2NewerCaseGuid, 1, { "NEWCASE1DATA" }, false);
            client2NewerCase->SetVectorClock(newClock);
            std::string client1NewerCaseGuid = CreateUuid();
            std::shared_ptr<Case> client1NewerCase = CreateCase(*case_access, client1NewerCaseGuid, 2, { "NEWCASE2DATA" }, false);
            client1NewerCase->SetVectorClock(newClock);
            std::string conflictCaseGuid = CreateUuid();
            std::shared_ptr<Case> conflictCase = CreateCase(*case_access, conflictCaseGuid, 3, { "13NEWCASE3DATA" }, false);
            conflictCase->SetVectorClock(newClock);

            std::vector<std::shared_ptr<Case>> initCases = { client2NewerCase, client1NewerCase, conflictCase };
            client1RepoBuilder.setInitialRepoCases(initCases, client1DeviceId);

            // Sync cases to server
            result = client.SyncData(*pClient1Repo, SyncDirection::Both, "");
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);
            Assert::AreEqual(size_t(3), pClient1Repo->GetNumberCases());

            // Create a second client
            DeviceId client2DeviceId = MakeUniqueDeviceId("it2-");
            TestRepoBuilder client2RepoBuilder(client2DeviceId, dictionary.get());
            ISyncableDataRepository* pClient2Repo = client2RepoBuilder.GetRepo();
            SyncClient client2(client2DeviceId, std::make_unique<SyncServiceFactory>());
            client2.SetSyncListener(std::make_unique<SyncLogSyncListener>());

            // Copy all cases to client2 (simulating P2P sync)
            client2RepoBuilder.setInitialRepoCases(initCases, client1DeviceId);

            // Update cases on client1
            client1RepoBuilder.updateRepoCase(client1NewerCaseGuid, 2, "UPDATEDBYCLIENT1");
            client1RepoBuilder.updateRepoCase(conflictCaseGuid, 3, "UPDATEDBYCLIENT1");

            // Update cases on client2
            client2RepoBuilder.updateRepoCase(client2NewerCaseGuid, 1, "UPDATEDBYCLIENT2");
            client2RepoBuilder.updateRepoCase(conflictCaseGuid, 3, "UPDATEDBYCLIENT2");

            // Sync both clients
            result = client2.ConnectCSWeb(credentials.sync_connection_string, std::make_unique<LoginCredentials>(credentials.username_password));
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);
            result = client2.SyncData(*pClient2Repo, SyncDirection::Both, "");
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);
            result = client2.Disconnect();
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);

            result = client.SyncData(*pClient1Repo, SyncDirection::Both, "");
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);
            result = client.Disconnect();
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);

            // client2Newer was updated on client 2 but not client 1
            client1RepoBuilder.verifyCaseInRepo(client2NewerCaseGuid, "UPDATEDBYCLIENT2");

            // client1Newer was updated on client 1 but not client 2
            client1RepoBuilder.verifyCaseInRepo(client1NewerCaseGuid, "UPDATEDBYCLIENT1");

            // Conflict should go to client 1 since it synced last
            client1RepoBuilder.verifyCaseInRepo(conflictCaseGuid, "UPDATEDBYCLIENT1");

        }


        // Reproduce problem from Vietnam where downloading data failed
        // to parse vector clock because CSWeb is sending deviceid in vector
        // clock as a number instead of as a string.
        TEST_METHOD(TestSyncVectorClockWithNumericDeviceId)
        {
            ResetServer();

            DeviceId client1DeviceId = "9675914571713748";
            TestRepoBuilder client1RepoBuilder(client1DeviceId, dictionary.get());
            ISyncableDataRepository* pClient1Repo = client1RepoBuilder.GetRepo();

            SyncClient client(client1DeviceId, std::make_unique<SyncServiceFactory>());
            client.SetSyncListener(std::make_unique<SyncLogSyncListener>());

            SyncClient::SyncResult result = client.ConnectCSWeb(credentials.sync_connection_string, std::make_unique<LoginCredentials>(credentials.username_password));
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);

            // Add a few cases
            VectorClock newClock;
            newClock.increment(client1DeviceId);
            std::string client2NewerCaseGuid = CreateUuid();
            std::shared_ptr<Case> client2NewerCase = CreateCase(*case_access, client2NewerCaseGuid, 1, { "NEWCASE1DATA" }, false);
            client2NewerCase->SetVectorClock(newClock);
            std::string client1NewerCaseGuid = CreateUuid();
            std::shared_ptr<Case> client1NewerCase = CreateCase(*case_access, client1NewerCaseGuid, 2, { "NEWCASE2DATA" }, false);
            client1NewerCase->SetVectorClock(newClock);
            std::string conflictCaseGuid = CreateUuid();
            std::shared_ptr<Case> conflictCase = CreateCase(*case_access, conflictCaseGuid, 3, { "NEWCASE3DATA" }, false);
            conflictCase->SetVectorClock(newClock);
            std::vector<std::shared_ptr<Case>> initCases = { client2NewerCase, client1NewerCase, conflictCase };
            client1RepoBuilder.setInitialRepoCases(initCases, client1DeviceId);

            // Sync cases to server
            result = client.SyncData(*pClient1Repo, SyncDirection::Put, "");
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);
            Assert::AreEqual(size_t(3), pClient1Repo->GetNumberCases());

            // Create a second client
            DeviceId client2DeviceId = "123456789";
            TestRepoBuilder client2RepoBuilder(client2DeviceId, dictionary.get());
            ISyncableDataRepository* pClient2Repo = client2RepoBuilder.GetRepo();
            SyncClient client2(client2DeviceId, std::make_unique<SyncServiceFactory>());
            client2.SetSyncListener(std::make_unique<SyncLogSyncListener>());

            // Copy all cases to client2 (simulating P2P sync)
            client2RepoBuilder.setInitialRepoCases(initCases, client1DeviceId);

            // Update cases on client1
            client1RepoBuilder.updateRepoCase(client1NewerCaseGuid, 2, "UPDATEDBYCLIENT1");
            client1RepoBuilder.updateRepoCase(conflictCaseGuid, 2, "UPDATEDBYCLIENT1");

            // Update cases on client2
            client2RepoBuilder.updateRepoCase(client2NewerCaseGuid, 1, "UPDATEDBYCLIENT2");
            client2RepoBuilder.updateRepoCase(conflictCaseGuid, 3, "UPDATEDBYCLIENT2");

            // Sync put for both clients to generate conflict
            result = client2.ConnectCSWeb(credentials.sync_connection_string, std::make_unique<LoginCredentials>(credentials.username_password));
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);
            result = client2.SyncData(*pClient2Repo, SyncDirection::Put, "");
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);
            result = client2.Disconnect();
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);

            result = client.SyncData(*pClient1Repo, SyncDirection::Put, "");
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);
            result = client.Disconnect();
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);

            // Do a get on a clean client to verify that clocks are correct
            DeviceId client3DeviceId = "987654321";
            TestRepoBuilder client3RepoBuilder(client3DeviceId, dictionary.get());
            ISyncableDataRepository* pClient3Repo = client3RepoBuilder.GetRepo();
            SyncClient client3(client2DeviceId, std::make_unique<SyncServiceFactory>());
            client3.SetSyncListener(std::make_unique<SyncLogSyncListener>());

            result = client3.ConnectCSWeb(credentials.sync_connection_string, std::make_unique<LoginCredentials>(credentials.username_password));
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);
            result = client3.SyncData(*pClient3Repo, SyncDirection::Get, "");
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);

            result = client3.Disconnect();
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);
        }


        TEST_METHOD(TestSyncDataDirection)
        {
            DeviceId client1DeviceId = MakeUniqueDeviceId("it1-");

            TestRepoBuilder client1RepoBuilder(client1DeviceId, dictionary.get());
            ISyncableDataRepository* pClient1Repo = client1RepoBuilder.GetRepo();

            SyncClient client(client1DeviceId, std::make_unique<SyncServiceFactory>());
            client.SetSyncListener(std::make_unique<SyncLogSyncListener>());

            SyncClient::SyncResult result = client.ConnectCSWeb(credentials.sync_connection_string, std::make_unique<LoginCredentials>(credentials.username_password));
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);

            // Initial sync with server using get to get baseline num cases
            result = client.SyncData(*pClient1Repo, SyncDirection::Get, "");
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);
            const size_t numInitialServerCases = pClient1Repo->GetNumberCases();

            // Add a couple of new cases
            std::string newCase1Guid = CreateUuid();
            client1RepoBuilder.addRepoCase(newCase1Guid, 1, "NEWCASE1DATA");
            std::string newCase2Guid = CreateUuid();
            client1RepoBuilder.addRepoCase(newCase2Guid, 2, "NEWCASE2DATA");

            // Put local cases to server - results in 2 new cases on server, same number in client1
            result = client.SyncData(*pClient1Repo, SyncDirection::Put, "");
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);
            Assert::AreEqual(numInitialServerCases + 2, pClient1Repo->GetNumberCases());

            // Sync with a second client to check if our changes were saved to the server
            DeviceId client2DeviceId = MakeUniqueDeviceId("it2-");
            TestRepoBuilder client2RepoBuilder(client2DeviceId, dictionary.get());
            ISyncableDataRepository* pClient2Repo = client2RepoBuilder.GetRepo();

            // Add a case to client2
            std::string newCase3Guid = CreateUuid();
            client2RepoBuilder.addRepoCase(newCase3Guid, 3, "NEWCASE3DATA");

            SyncClient client2(client2DeviceId, std::make_unique<SyncServiceFactory>());
            client2.SetSyncListener(std::make_unique<SyncLogSyncListener>());

            result = client2.ConnectCSWeb(credentials.sync_connection_string, std::make_unique<LoginCredentials>(credentials.username_password));
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);

            // Do a get with client2 - make sure we get both cases from client 1, server cases remain unchanged, client2
            // has one more case than server
            result = client2.SyncData(*pClient2Repo, SyncDirection::Get, "");
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);
            Assert::AreEqual(numInitialServerCases + 3, pClient2Repo->GetNumberCases());

            // Do a get with client1 to make sure the previous get didn't upload anything
            result = client.SyncData(*pClient1Repo, SyncDirection::Get, "");
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);
            Assert::AreEqual(numInitialServerCases + 2, pClient1Repo->GetNumberCases());

            // Add a new case and do a put with client1 - results is that server now has 3 cases beyond what it started with,
            // client1 matches server
            std::string newCase4Guid = CreateUuid();
            client1RepoBuilder.addRepoCase(newCase4Guid, 4, "NEWCASE4DATA");
            result = client.SyncData(*pClient1Repo, SyncDirection::Put, "");
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);

            // Do a put with client2 - make sure we don't get the new case (case4), this  uploads case3 resulting in server having
            // all 4 new cases but client2 doesn't have case4
            result = client2.SyncData(*pClient2Repo, SyncDirection::Put, "");
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);
            Assert::AreEqual(numInitialServerCases + 3, pClient2Repo->GetNumberCases());

            // Do a get with client2 - make sure we do get case4 (server should send everything since direction changed)
            // Server and client2 now both have all four new cases
            result = client2.SyncData(*pClient2Repo, SyncDirection::Get, "");
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);
            Assert::AreEqual(numInitialServerCases + 4, pClient2Repo->GetNumberCases());
        }


        TEST_METHOD(TestPutGetFile)
        {
            DeviceId clientDeviceId = "mydevice";
            SyncClient client(clientDeviceId, std::make_unique<SyncServiceFactory>());
            client.SetSyncListener(std::make_unique<SyncLogSyncListener>());

            SyncClient::SyncResult result = client.ConnectCSWeb(credentials.sync_connection_string, std::make_unique<LoginCredentials>(credentials.username_password));
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);

            const std::string srcFilePath = Path::Combine(GetTempDirectory(), "zsynco-get-file-test.txt");
            PortableFunctions::FileDeleteWithExceptions(srcFilePath);

            const std::string data = CreateUuid();
            FileIO::WriteText(srcFilePath, data, false);

            const std::string serverPath = "/foo/bar/test.txt";

            SyncClient::SyncResult syncResult = client.SyncFile(SyncDirection::Put, srcFilePath, serverPath);
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, syncResult);

            PortableFunctions::FileDeleteWithExceptions(srcFilePath);

            syncResult = client.SyncFile(SyncDirection::Get, serverPath, srcFilePath);
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, syncResult);

            // Get file again and make sure it is not re-downloaded by checking modified time
            const int64_t file_modified_time = PortableFunctions::FileModifiedTime(srcFilePath);
            syncResult = client.SyncFile(SyncDirection::Get, serverPath, srcFilePath);
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, syncResult);
            Assert::AreEqual(file_modified_time, PortableFunctions::FileModifiedTime(srcFilePath), L"File was modified by second download");

            result = client.Disconnect();
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);

            // File exists
            Assert::IsTrue(PortableFunctions::FileIsRegular(srcFilePath));
            Assert::AreEqual(data, FileIO::ReadText(srcFilePath));

            PortableFunctions::FileDeleteWithExceptions(srcFilePath);
        }


        TEST_METHOD(TestGetWildcard)
        {
            DeviceId clientDeviceId = "mydevice";
            SyncClient client(clientDeviceId, std::make_unique<SyncServiceFactory>());
            client.SetSyncListener(std::make_unique<SyncLogSyncListener>());

            SyncClient::SyncResult result = client.ConnectCSWeb(credentials.sync_connection_string, std::make_unique<LoginCredentials>(credentials.username_password));
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);

            TemporaryFile temporary_file;

            const std::string data = CreateUuid();
            FileIO::WriteText(temporary_file.GetPath(), data, false);

            SyncClient::SyncResult syncResult = client.SyncFile(SyncDirection::Put, temporary_file.GetPath(), PortableFunctions::PathGetFilename(temporary_file.GetPath()));
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, syncResult);

            PortableFunctions::FileDeleteWithExceptions(temporary_file.GetPath());

            syncResult = client.SyncFile(SyncDirection::Get, Path::GetFilenameWithoutExtension(temporary_file.GetPath()) + ".*",
                                                             PortableFunctions::PathGetDirectory(temporary_file.GetPath()));
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, syncResult);

            result = client.Disconnect();
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);

            // File exists
            Assert::IsTrue(PortableFunctions::FileIsRegular(temporary_file.GetPath()));
            Assert::AreEqual(data, FileIO::ReadText(temporary_file.GetPath()));
        }


        TEST_METHOD(TestDeleteCase)
        {
            ResetServer();

            DeviceId client1DeviceId = MakeUniqueDeviceId("it1-");

            TestRepoBuilder repoBuilder(client1DeviceId, dictionary.get());
            ISyncableDataRepository* pRepo = repoBuilder.GetRepo();

            SyncClient client1(client1DeviceId, std::make_unique<SyncServiceFactory>());
            client1.SetSyncListener(std::make_unique<SyncLogSyncListener>());

            SyncClient::SyncResult result = client1.ConnectCSWeb(credentials.sync_connection_string, std::make_unique<LoginCredentials>(credentials.username_password));
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);

            // Add a new case and sync
            VectorClock newClock;
            newClock.increment(client1DeviceId);
            std::string newCase1Guid = CreateUuid();
            repoBuilder.addRepoCase(newCase1Guid, 9, "NEWCASE1DATA");
            result = client1.SyncData(*pRepo, SyncDirection::Both, "");
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);
            Assert::AreEqual(size_t(1), pRepo->GetNumberCases());

            // Delete the new case and sync
            repoBuilder.GetRepo()->DeleteCase("  9");
            result = client1.SyncData(*pRepo, SyncDirection::Both, "");
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);

            result = client1.Disconnect();
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);

            // Sync with a second client to check if our changes were saved to the server
            DeviceId client2DeviceId = MakeUniqueDeviceId("it2-");
            repoBuilder.ResetRepo(client2DeviceId, dictionary.get());
            pRepo = repoBuilder.GetRepo();

            SyncClient client2(client2DeviceId, std::make_unique<SyncServiceFactory>());
            client2.SetSyncListener(std::make_unique<SyncLogSyncListener>());

            result = client2.ConnectCSWeb(credentials.sync_connection_string, std::make_unique<LoginCredentials>(credentials.username_password));
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);
            result = client2.SyncData(*pRepo, SyncDirection::Both, "");
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);

            // Make sure case got deleted
            repoBuilder.verifyCaseDeleted(newCase1Guid);

            result = client2.Disconnect();
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);
        }


        TEST_METHOD(TestSyncUniverse)
        {
            ResetServer();

            DeviceId client1DeviceId = MakeUniqueDeviceId("it1-");

            TestRepoBuilder repoBuilder(client1DeviceId, dictionary.get());
            ISyncableDataRepository* pRepo = repoBuilder.GetRepo();

            SyncClient client1(client1DeviceId, std::make_unique<SyncServiceFactory>());
            client1.SetSyncListener(std::make_unique<SyncLogSyncListener>());

            SyncClient::SyncResult result = client1.ConnectCSWeb(credentials.sync_connection_string, std::make_unique<LoginCredentials>(credentials.username_password));
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);

            const std::string universe = "  0";

            // Add two cases, one in universe and one out and sync
            VectorClock newClock;
            newClock.increment(client1DeviceId);
            std::string newCase1Guid = CreateUuid();
            repoBuilder.addRepoCase(newCase1Guid, 0, "NEWCASE0DATA");
            std::string newCase2Guid = CreateUuid();
            repoBuilder.addRepoCase(newCase2Guid, 1, "NEWCASE1DATA");
            result = client1.SyncData(*pRepo, SyncDirection::Both, universe);
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);
            Assert::AreEqual(size_t(2), pRepo->GetNumberCases());

            result = client1.Disconnect();
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);

            // Sync with a second client to check if our changes were saved to the server
            DeviceId client2DeviceId = MakeUniqueDeviceId("it2-");
            repoBuilder.ResetRepo(client2DeviceId, dictionary.get());
            pRepo = repoBuilder.GetRepo();

            SyncClient client2(client2DeviceId, std::make_unique<SyncServiceFactory>());
            client2.SetSyncListener(std::make_unique<SyncLogSyncListener>());

            result = client2.ConnectCSWeb(credentials.sync_connection_string, std::make_unique<LoginCredentials>(credentials.username_password));
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);
            result = client2.SyncData(*pRepo, SyncDirection::Both, universe);
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);

            // Should only get one new case - the one in universe
            Assert::AreEqual(size_t(1), pRepo->GetNumberCases());

            result = client2.Disconnect();
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);
        }


        TEST_METHOD(TestRefreshToken)
        {
            DeviceId clientDeviceId = "mydevice";

            std::string refreshToken;

            Mock<SyncCredentialStore> mockCredentialStore1;
            When(Method(mockCredentialStore1, Store)).AlwaysDo([&refreshToken](const std::string_view h_sv, const std::string& s) {
                if( h_sv.find("refresh") != std::string_view::npos ) {
                    refreshToken = s;
                }
            });
            When(Method(mockCredentialStore1, Retrieve)).AlwaysReturn("");

            Mock<SyncLoginAccessor> mockSyncLoginAccessor1;
            When(Method(mockSyncLoginAccessor1, GetSyncCredentialStore)).AlwaysReturn(std::shared_ptr<SyncCredentialStore>(&mockCredentialStore1.get()));
            When(Method(mockSyncLoginAccessor1, CreateHttpConnection)).AlwaysDo([]() { return std::make_unique<CurlHttpConnection>(); });
            When(Method(mockSyncLoginAccessor1, QueryUsernamePassword)).AlwaysReturn(std::make_optional(credentials.username_password));

            SyncClient client1(clientDeviceId, std::make_unique<SyncServiceFactory>(std::unique_ptr<SyncLoginAccessor>(&mockSyncLoginAccessor1.get())));
            client1.SetSyncListener(std::make_unique<SyncLogSyncListener>());

            // This will use username and password from mockLoginDialog to obtain auth and refresh tokens
            SyncClient::SyncResult result = client1.ConnectCSWeb(credentials.sync_connection_string, nullptr);
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);

            Mock<SyncCredentialStore> mockCredentialStore2;
            When(Method(mockCredentialStore2, Store)).AlwaysReturn();
            When(Method(mockCredentialStore2, Retrieve)).AlwaysDo([refreshToken](const std::string_view h_sv) {
                return ( h_sv.find("refresh") != std::string_view::npos ) ? refreshToken : "expired";
            });

            Mock<SyncLoginAccessor> mockSyncLoginAccessor2;
            When(Method(mockSyncLoginAccessor2, GetSyncCredentialStore)).AlwaysReturn(std::shared_ptr<SyncCredentialStore>(&mockCredentialStore2.get()));
            When(Method(mockSyncLoginAccessor2, CreateHttpConnection)).AlwaysDo([]() { return std::make_unique<CurlHttpConnection>(); });
            When(Method(mockSyncLoginAccessor2, QueryUsernamePassword)).AlwaysReturn(std::make_optional(credentials.username_password));

            SyncClient client2(clientDeviceId, std::make_unique<SyncServiceFactory>(std::unique_ptr<SyncLoginAccessor>(&mockSyncLoginAccessor2.get())));
            client2.SetSyncListener(std::make_unique<SyncLogSyncListener>());

            // This will get auth token "expired" which will fail so it will get new token using refresh token
            result = client2.ConnectCSWeb(credentials.sync_connection_string, nullptr);
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);
        }


        TEST_METHOD(TestSyncChunkedGet)
        {
            ResetServer();

            DeviceId client1DeviceId = MakeUniqueDeviceId("it1-");

            auto httpConnection1_ptr = std::make_unique<FailableHttpConnection>();
            auto httpConnection2_ptr = std::make_unique<FailableHttpConnection>();
            FailableHttpConnection* httpConnection2 = httpConnection2_ptr.get();

            const int chunkSize = 100;
            const int numChunks = 3;
            const int totalCases = chunkSize * ((1 << numChunks) - 1);

            // use 2 different csweb connections so that chunk sizes are not shared
            Mock<ISyncServiceFactory> serverFactory1;
            When(OverloadedMethod(serverFactory1, CreateCSWebSyncService, std::unique_ptr<ISyncService>(SyncConnectionString, std::unique_ptr<LoginCredentials>)))
                .Return(std::make_unique<CSWebSyncService>(std::move(httpConnection1_ptr), credentials.sync_connection_string, credentials.username_password));

            Mock<ISyncServiceFactory> serverFactory2;
            When(OverloadedMethod(serverFactory2, CreateCSWebSyncService, std::unique_ptr<ISyncService>(SyncConnectionString, std::unique_ptr<LoginCredentials>)))
                .Return(std::make_unique<CSWebSyncService>(std::move(httpConnection2_ptr), credentials.sync_connection_string, credentials.username_password));

            TestRepoBuilder repo1Builder(client1DeviceId, dictionary.get());
            ISyncableDataRepository* pRepo1 = repo1Builder.GetRepo();

            std::vector<std::shared_ptr<Case>> initialCases;
            VectorClock clock;
            clock.increment(client1DeviceId);
            for (int i = 1; i <= totalCases; ++i) {
                std::string uuid = FormatText("00000000-0000-0000-0000-00000000%04d", i);
                std::unique_ptr<Case> init_case = CreateCase(*case_access, uuid, 1, { "intialdata" }, false);
                init_case->SetVectorClock(clock);
                initialCases.emplace_back(std::move(init_case));
            }
            repo1Builder.setInitialRepoCases(initialCases, client1DeviceId);

            SyncClient client1(client1DeviceId, std::unique_ptr<ISyncServiceFactory>(&serverFactory1.get()));
            client1.SetSyncListener(std::make_unique<SyncLogSyncListener>());

            SyncClient::SyncResult result = client1.ConnectCSWeb(credentials.sync_connection_string, std::make_unique<LoginCredentials>(credentials.username_password));
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);

            // Upload initial cases to server
            result = client1.SyncData(*pRepo1, SyncDirection::Put, "");
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);

            // Sync with a second client
            DeviceId client2DeviceId = MakeUniqueDeviceId("it2-");
            TestRepoBuilder repo2Builder(client2DeviceId, dictionary.get());
            ISyncableDataRepository* pRepo2 = repo2Builder.GetRepo();

            SyncClient client2(client2DeviceId, std::unique_ptr<ISyncServiceFactory>(&serverFactory2.get()));
            client2.SetSyncListener(std::make_unique<SyncLogSyncListener>());

            result = client2.ConnectCSWeb(credentials.sync_connection_string, std::make_unique<LoginCredentials>(credentials.username_password));
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);

            // Get first chunk of results then fail on unrecoverable error
            httpConnection2->setErrorInGetAfterCalls(HttpResponse::Status_404_NotFound, 1);
            result = client2.SyncData(*pRepo2, SyncDirection::Both, "");
            Assert::AreEqual(SyncClient::SyncResult::SYNC_ERROR, result);
            Assert::AreEqual(size_t(chunkSize), repo2Builder.GetRepo()->GetNumberCases());

            // Update and add cases on server
            repo1Builder.updateRepoCase(initialCases.front()->GetUuid(), 1, "Updated\r\n");
            repo1Builder.updateRepoCase(initialCases.back()->GetUuid(), 1, "Updated\r\n");
            repo1Builder.addRepoCase("00000000-0000-0000-0000-000000000000", 1, "added\r\n");
            repo1Builder.addRepoCase("10000000-0000-0000-0000-000000000000", 1, "added\r\n");

            result = client1.SyncData(*pRepo1, SyncDirection::Put, "");

            // Get the remaining cases
            result = client2.SyncData(*pRepo2, SyncDirection::Both, "");
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);
            Assert::AreEqual(initialCases.size() + 2, repo2Builder.GetRepo()->GetNumberCases());

            repo2Builder.verifyCaseInRepo(initialCases.front()->GetUuid(), "Updated");
            repo2Builder.verifyCaseInRepo(initialCases.back()->GetUuid(), "Updated");
        }


        TEST_METHOD(TestSyncChunkedPut)
        {
            ResetServer();

            DeviceId client1DeviceId = MakeUniqueDeviceId("it1-");

            auto httpConnection1_ptr = std::make_unique<FailableHttpConnection>();
            FailableHttpConnection* httpConnection1 = httpConnection1_ptr.get();
            auto httpConnection2_ptr = std::make_unique<FailableHttpConnection>();

            NetworkDataChunk dataChunk;
            const size_t chunkSize = dataChunk.GetCaseSize();
            const size_t numChunks = 3;

            Mock<ISyncServiceFactory> serverFactory1;
            When(OverloadedMethod(serverFactory1, CreateCSWebSyncService, std::unique_ptr<ISyncService>(SyncConnectionString, std::unique_ptr<LoginCredentials>)))
                .Return(std::make_unique<CSWebSyncService>(std::move(httpConnection1_ptr), credentials.sync_connection_string, credentials.username_password));

            Mock<ISyncServiceFactory> serverFactory2;
            When(OverloadedMethod(serverFactory2, CreateCSWebSyncService, std::unique_ptr<ISyncService>(SyncConnectionString, std::unique_ptr<LoginCredentials>)))
                .Return(std::make_unique<CSWebSyncService>(std::move(httpConnection2_ptr), credentials.sync_connection_string, credentials.username_password));

            TestRepoBuilder repo1Builder(client1DeviceId, dictionary.get());
            ISyncableDataRepository* pRepo1 = repo1Builder.GetRepo();

            std::vector<std::shared_ptr<Case>> casesToUpload;
            VectorClock clock;
            clock.increment(client1DeviceId);
            for (int i = 1; i <= static_cast<int>(chunkSize * numChunks); ++i) {
                std::string uuid = FormatText("00000000-0000-0000-0000-000000000%03d", i);
                casesToUpload.emplace_back(CreateCase(*case_access, uuid, i, { "data" }, false));
                casesToUpload.back()->SetVectorClock(clock);
            }
            repo1Builder.setInitialRepoCases(casesToUpload, client1DeviceId);

            SyncClient client1(client1DeviceId, std::unique_ptr<ISyncServiceFactory>(&serverFactory1.get()));
            client1.SetSyncListener(std::make_unique<SyncLogSyncListener>());

            SyncClient::SyncResult result = client1.ConnectCSWeb(credentials.sync_connection_string, std::make_unique<LoginCredentials>(credentials.username_password));
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);

            // Try to upload first chunk of cases to server and fail with error
            // before uploading any cases
            httpConnection1->setErrorInPostAfterCalls(HttpResponse::Status_404_NotFound, 0);
            result = client1.SyncData(*pRepo1, SyncDirection::Put, "");
            Assert::AreEqual(SyncClient::SyncResult::SYNC_ERROR, result);

            // Upload first chunk of cases to server and fail with error
            // before second
            httpConnection1->setErrorInPostAfterCalls(HttpResponse::Status_404_NotFound, 1);
            result = client1.SyncData(*pRepo1, SyncDirection::Put, "");
            Assert::AreEqual(SyncClient::SyncResult::SYNC_ERROR, result);

            // Sync with a second client to see if first chunk is uploaded
            DeviceId client2DeviceId = MakeUniqueDeviceId("it2-");
            TestRepoBuilder repo2Builder(client2DeviceId, dictionary.get());
            ISyncableDataRepository* pRepo2 = repo2Builder.GetRepo();
            SyncClient client2(client2DeviceId, std::unique_ptr<ISyncServiceFactory>(&serverFactory2.get()));
            client2.SetSyncListener(std::make_unique<SyncLogSyncListener>());
            result = client2.ConnectCSWeb(credentials.sync_connection_string, std::make_unique<LoginCredentials>(credentials.username_password));
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);
            result = client2.SyncData(*pRepo2, SyncDirection::Get, "");
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);
            Assert::AreEqual(chunkSize, repo2Builder.GetRepo()->GetLastSyncStats().cases_received);

            // Update cases and add new cases on client
            casesToUpload.front() = CreateCase(*case_access, casesToUpload.front()->GetUuid(), 1, { "Updated" });
            repo1Builder.updateRepoCase(casesToUpload.front()->GetUuid(), 1, getCaseData(*casesToUpload.front()));
            casesToUpload.back() = CreateCase(*case_access, casesToUpload.front()->GetUuid(), 1, { "Updated" });
            repo1Builder.updateRepoCase(casesToUpload.back()->GetUuid(), 1, getCaseData(*casesToUpload.back()));
            std::string uuid = FormatText("00000000-0000-0000-0000-000000000%03d", casesToUpload.size() + 1);
            casesToUpload.emplace_back(CreateCase(*case_access, uuid, 1, { "11added" }));
            repo1Builder.addRepoCase(casesToUpload.back()->GetUuid(), 1, getCaseData(*casesToUpload.back()));
            casesToUpload.emplace_back(CreateCase(*case_access, "00000000-0000-0000-0000-000000000000", 1, { "11added" }));
            repo1Builder.addRepoCase(casesToUpload.back()->GetUuid(), 1, getCaseData(*casesToUpload.back()));

            // Upload rest of cases
            httpConnection1->setErrorInPostAfterCalls(HttpResponse::Status_404_NotFound, -1);
            result = client1.SyncData(*pRepo1, SyncDirection::Put, "");
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);

            // Sync with second client again to make sure rest of cases were downloaded
            result = client2.SyncData(*pRepo2, SyncDirection::Get, "");
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);
            Assert::AreEqual(casesToUpload.size() - chunkSize + 1, repo2Builder.GetRepo()->GetLastSyncStats().cases_received);
            Assert::AreEqual(casesToUpload.size(), repo2Builder.GetRepo()->GetNumberCases());

            for (auto c : casesToUpload)
                repo2Builder.verifyCaseInRepo(*c);
        }


        TEST_METHOD(TestSyncGetExcludesPreviousPuts)
        {
            ResetServer();

            DeviceId client1DeviceId = MakeUniqueDeviceId("it1-");

            NetworkDataChunk dataChunk;
            const size_t chunkSize = dataChunk.GetCaseSize();
            const size_t numChunks = 3;

            TestRepoBuilder repo1Builder(client1DeviceId, dictionary.get());
            ISyncableDataRepository* pRepo1 = repo1Builder.GetRepo();

            std::vector<std::shared_ptr<Case>> casesToUpload;
            VectorClock clock;
            clock.increment(client1DeviceId);
            for (int i = 1; i <= static_cast<int>(chunkSize * numChunks); ++i) {
                std::string uuid = FormatText("00000000-0000-0000-0000-000000000%03d", i);
                casesToUpload.emplace_back(CreateCase(*case_access, uuid, 1, { "data" },false));
                casesToUpload.back()->SetVectorClock(clock);
            }
            repo1Builder.setInitialRepoCases(casesToUpload, client1DeviceId);

            SyncClient client1(client1DeviceId, std::make_unique<SyncServiceFactory>());
            client1.SetSyncListener(std::make_unique<SyncLogSyncListener>());

            SyncClient::SyncResult result = client1.ConnectCSWeb(credentials.sync_connection_string, std::make_unique<LoginCredentials>(credentials.username_password));
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);

            // Sync with multiple chunks to have mutliple sync revisions
            result = client1.SyncData(*pRepo1, SyncDirection::Both, "");

            // Do another sync, make sure that no new cases are downloaded
            result = client1.SyncData(*pRepo1, SyncDirection::Get, "");
            Assert::AreEqual(size_t(0), repo1Builder.GetRepo()->GetLastSyncStats().cases_received);
        }


        TEST_METHOD(TestApplications)
        {
            ApplicationsTester applications_tester(ApplicationPackage::DeploymentType::CSWeb, credentials.sync_connection_string.GetUrl());
            applications_tester.RunTest([&](SyncClient& sync_client) { return sync_client.ConnectCSWeb(credentials.sync_connection_string, std::make_unique<LoginCredentials>(credentials.username_password)); });
        }


        TEST_METHOD(TestSyncMessages)
        {
            try
            {
                DeviceId device_id = MakeUniqueDeviceId("TestSyncMessages-");
                SyncClient sync_client(std::move(device_id), std::make_unique<SyncServiceFactory>());
                sync_client.SetSyncListener(std::make_unique<SyncLogSyncListener>());

                SyncClient::SyncResult result = sync_client.ConnectCSWeb(credentials.sync_connection_string, std::make_unique<LoginCredentials>(credentials.username_password));
                Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);

                const int64_t timestamp = GetTimestamp();
                const SyncMessage sync_message(timestamp, "CSPro-TestSync-Key-" + IntToString(timestamp), "CSPro-TestSync-Value");
                const std::optional<JsonNode> sync_message_response = sync_client.SendSyncMessage(sync_message);
                Assert::IsTrue(!sync_message_response.has_value());

                result = sync_client.Disconnect();
                Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);
            }

            catch( const CSProException& exception )
            {
                Assert::Fail(TC::ToWide(exception.what()).c_str());
            }
        }
    };
}
