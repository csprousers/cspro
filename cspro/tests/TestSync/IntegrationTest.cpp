#include "stdafx.h"

#include "CppUnitTest.h"
#include <zUtilO/ICredentialStore.h>
#include <zSyncO/SyncClient.h>
#include <zSyncO/ConnectResponse.h>
#include <zSyncO/SyncServerConnectionFactory.h>
#include <zSyncO/ILoginDialog.h>
#include <fstream>
#include <zDictO/DDClass.h>
#include "TestRepoBuilder.h"
#include "FailableHttpConnection.h"
#include <zSyncO/CSWebSyncServerConnection.h>
#include <zSyncO/DefaultChunk.h>
#include <thread>
#include <mutex>
#include "CaseTestHelpers.h"

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
            template<> inline std::wstring ToString<PartialSaveMode>(const PartialSaveMode& m)
            {
                switch (m) {
                case PartialSaveMode::Add:
                    return L"PartialSaveMode::Add";
                case PartialSaveMode::Modify:
                    return L"PartialSaveMode::Modify";
                case PartialSaveMode::None:
                    return L"PartialSaveMode::None";
                case PartialSaveMode::Verify:
                    return L"PartialSaveMode::Verify";
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
    TEST_CLASS(IntegrationTest)
    {
    private:
        CString url = L"http://localhost/api/";
        CString username = L"user";
        CString password = L"password";

    private:

        // Make a new device id that is unique so we can test sync
        // on an existing server as if we started with a new device.
        DeviceId makeUniqueDeviceId(CString prefix)
        {
            // Currently device id is limited to 16 chars on server
            // so use prefix and fill rest with guid which should
            // unique enough.
            return prefix + makeUuid().Left(16 - prefix.GetLength());
        }

        std::unique_ptr<const CDataDict> dictionary = createTestDict();
        std::shared_ptr<CaseAccess> case_access = CaseAccess::CreateAndInitializeFullCaseAccess(*dictionary);

        void ResetServer()
        {
            DeviceId clientDeviceId = L"mydevice";
            SyncServerConnectionFactory serverFactory(NULL);
            SyncClient client(clientDeviceId, &serverFactory);

            auto result = client.connectWeb(url, username, password);
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);

            result = client.deleteDictionary(L"TESTDICT_DICT");

            result = client.uploadDictionary(L"TestDict.dcf");
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);

            result = client.disconnect();
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);
        }

        void CompareNotes(const std::vector<Note> actual,
            const std::vector<Note> expected)
        {
            Assert::AreEqual(expected.size(), actual.size(), L"Note vectors different sizes");
            for (auto iExpected = expected.begin(), iActual = actual.begin(); iExpected != expected.end(); ++iExpected, ++iActual) {
                Assert::AreEqual(iExpected->GetContent(), iActual->GetContent());
                const auto& expectedField = iExpected->GetNamedReference();
                const auto& actualField = iActual->GetNamedReference();
                Assert::AreEqual<CString>(expectedField.GetName(), actualField.GetName());
                Assert::AreEqual<CString>(expectedField.GetLevelKey(), actualField.GetLevelKey());
                Assert::AreEqual(expectedField.GetOneBasedOccurrences()[0], actualField.GetOneBasedOccurrences()[0]);
                Assert::AreEqual(expectedField.GetOneBasedOccurrences()[1], actualField.GetOneBasedOccurrences()[1]);
                Assert::AreEqual(expectedField.GetOneBasedOccurrences()[2], actualField.GetOneBasedOccurrences()[2]);
                Assert::AreEqual(iExpected->GetOperatorId(), iActual->GetOperatorId());
                Assert::AreEqual((int64_t) iExpected->GetModifiedDateTime(), (int64_t) iActual->GetModifiedDateTime());
            }
        }

    public:

        TEST_METHOD(TestConnectDisconnect)
        {
            DeviceId clientDeviceId = L"mydevice";
            SyncServerConnectionFactory serverFactory(NULL);
            SyncClient client(clientDeviceId, &serverFactory);

            auto result = client.connectWeb(url, username, password);
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);

            result = client.disconnect();
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);
        }

        TEST_METHOD(TestSyncData)
        {
            ResetServer();

            DeviceId client1DeviceId = makeUniqueDeviceId(L"it1-");
            SyncServerConnectionFactory serverFactory(NULL);

            TestRepoBuilder repoBuilder(client1DeviceId, dictionary.get());
            ISyncableDataRepository* pRepo = repoBuilder.GetRepo();

            SyncClient client1(client1DeviceId, &serverFactory);

            auto result = client1.connectWeb(url, username, password);
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);

            // First sync - empty server, shouldn't get any cases
            result = client1.syncData(SyncDirection::Both, *pRepo, L"");
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);
            Assert::AreEqual(0, (int) pRepo->GetNumberCases());

            // Add a couple of new cases
            VectorClock newClock;
            newClock.increment(client1DeviceId);
            CString newCase1Guid = makeUuid();
            repoBuilder.addRepoCase(newCase1Guid, 1, { L"NEWCASE1DATA" });
            CString newCase2Guid = makeUuid();
            repoBuilder.addRepoCase(newCase2Guid, 2, { L"NEWCASE2DATA" });
            result = client1.syncData(SyncDirection::Both, *pRepo, L"");
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);
            Assert::AreEqual(2, (int) pRepo->GetNumberCases());

            // Update one of the new cases
            const CString updatedCaseData(L"THISDATAWASUPDATED");
            repoBuilder.updateRepoCase(newCase1Guid, 1, updatedCaseData);

            // Still shouldn't get any new cases back
            result = client1.syncData(SyncDirection::Both, *pRepo, L"");
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);
            Assert::AreEqual(2, (int) pRepo->GetNumberCases());

            result = client1.disconnect();
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);

            // Sync with a second client to check if our changes were saved to the server
            DeviceId client2DeviceId = makeUniqueDeviceId(L"it2-");
            repoBuilder.ResetRepo(client2DeviceId, dictionary.get());
            pRepo = repoBuilder.GetRepo();

            SyncClient client2(client2DeviceId, &serverFactory);

            result = client2.connectWeb(url, username, password);
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);
            result = client2.syncData(SyncDirection::Both, *pRepo, L"");
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);
            Assert::AreEqual(2, (int) pRepo->GetNumberCases());

            // Make sure case got updateded
            repoBuilder.verifyCaseInRepo(newCase1Guid, updatedCaseData);

            // Make sure other new case got added
            repoBuilder.verifyCaseInRepo(newCase2Guid, L"NEWCASE2DATA");

            result = client2.disconnect();
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);
        }

        TEST_METHOD(TestSyncPartialSave)
        {
            DeviceId client1DeviceId = makeUniqueDeviceId(L"it1-");
            SyncServerConnectionFactory serverFactory(NULL);

            TestRepoBuilder repoBuilder(client1DeviceId, dictionary.get());
            ISyncableDataRepository* pRepo = repoBuilder.GetRepo();

            SyncClient client1(client1DeviceId, &serverFactory);

            // Test cases
            VectorClock newClock;
            newClock.increment(client1DeviceId);
            CString caseGuid = makeUuid();
            auto initCase = makeCase(*case_access, caseGuid, 1, { L"NEWCASE1DATA" },false);
            initCase->SetVectorClock(newClock);
            setPartialSave(*case_access, *initCase, PartialSaveMode::Add, "TEST_ITEM", 1, 1, 0);
            repoBuilder.setInitialRepoCases({ initCase }, client1DeviceId);

            auto result = client1.connectWeb(url, username, password);
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);

            // Upload test case to server
            result = client1.syncData(SyncDirection::Put, *pRepo, L"");
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);

            result = client1.disconnect();
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);

            // Sync with a second client to check if the partial save status
            // was saved correctly
            DeviceId client2DeviceId = makeUniqueDeviceId(L"it2-");
            repoBuilder.ResetRepo(client2DeviceId, dictionary.get());
            pRepo = repoBuilder.GetRepo();

            SyncClient client2(client2DeviceId, &serverFactory);

            result = client2.connectWeb(url, username, password);
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);
            result = client2.syncData(SyncDirection::Get, *pRepo, L"");
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);

            // Make sure partial save status came through
            std::shared_ptr<Case> repo_case = repoBuilder.findCaseByUuid(caseGuid);
            Assert::IsTrue(repo_case != nullptr, L"Case not found");
            Assert::AreEqual(initCase->GetPartialSaveMode(), repo_case->GetPartialSaveMode());
            auto actual_field_ref = repo_case->GetPartialSaveCaseItemReference();
            auto expected_field_ref = initCase->GetPartialSaveCaseItemReference();
            Assert::AreEqual<CString>(expected_field_ref->GetCaseItem().GetDictionaryItem().GetName(), actual_field_ref->GetCaseItem().GetDictionaryItem().GetName());
            Assert::AreEqual<CString>(expected_field_ref->GetLevelKey(), actual_field_ref->GetLevelKey());
            Assert::AreEqual(expected_field_ref->GetRecordOccurrence(), actual_field_ref->GetRecordOccurrence());
            Assert::AreEqual(expected_field_ref->GetItemOccurrence(), actual_field_ref->GetItemOccurrence());
            Assert::AreEqual(expected_field_ref->GetSubitemOccurrence(), actual_field_ref->GetSubitemOccurrence());

            result = client2.disconnect();
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);
        }

        TEST_METHOD(TestSyncNotes)
        {
            ResetServer();

            DeviceId client1DeviceId = makeUniqueDeviceId(L"it1-");
            SyncServerConnectionFactory serverFactory(NULL);

            TestRepoBuilder repoBuilder(client1DeviceId, dictionary.get());
            ISyncableDataRepository* pRepo = repoBuilder.GetRepo();

            SyncClient client1(client1DeviceId, &serverFactory);

            // Test cases
            VectorClock newClock;
            newClock.increment(client1DeviceId);
            CString caseGuid = makeUuid();
            std::vector<Note> notes;
            notes.push_back(createNote(*case_access, L"note1content", L"TEST_ITEM", 1, 1, 1, L"op1", time(NULL)));
            notes.push_back(createNote(*case_access, L"note2content", L"TEST_ITEM",  1, 2, 3, L"op2", time(NULL)));

            auto init_case = makeCase(*case_access, caseGuid, 1, { L"DATA" },false);
            init_case->SetNotes(notes);
            init_case->SetVectorClock(newClock);
            std::vector<std::shared_ptr<Case>> initCases = { init_case };
            repoBuilder.setInitialRepoCases(initCases, client1DeviceId);

            auto result = client1.connectWeb(url, username, password);
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);

            // Upload test case to server
            result = client1.syncData(SyncDirection::Put, *pRepo, L"");
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);

            // Sync with a second client to check if the notes were saved correctly
            DeviceId client2DeviceId = makeUniqueDeviceId(L"it2-");
            repoBuilder.ResetRepo(client2DeviceId, dictionary.get());
            pRepo = repoBuilder.GetRepo();

            SyncClient client2(client2DeviceId, &serverFactory);

            result = client2.connectWeb(url, username, password);
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);
            result = client2.syncData(SyncDirection::Get, *pRepo, L"");
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);

            // Make sure notes came through
            auto repo_case = repoBuilder.findCaseByUuid(init_case->GetUuid());
            CompareNotes(notes, repo_case->GetNotes());

            // Update note on client1 and resync
            notes[0] = createNote(*case_access, L"note1contentmodified", L"TEST_ITEM", 1, 1, 1, L"op1", time(NULL));
            repoBuilder.updateRepoCaseNotes(caseGuid, notes);
            result = client1.syncData(SyncDirection::Put, *pRepo, L"");
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);

            // Download to client2
            result = client2.syncData(SyncDirection::Get, *pRepo, L"");
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);

            // Make sure update came through
            repo_case = repoBuilder.findCaseByUuid(init_case->GetUuid());
            CompareNotes(notes, repo_case->GetNotes());

            result = client1.disconnect();
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);

            result = client2.disconnect();
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);
        }

        TEST_METHOD(TestSyncUnicode)
        {
            const wchar_t* unicodeData = L"🐼🐼🐼 有人做过一。Кто-то странное.";
            DeviceId client1DeviceId = makeUniqueDeviceId(L"it1-");
            SyncServerConnectionFactory serverFactory(NULL);
            TestRepoBuilder repoBuilder(client1DeviceId, dictionary.get());
            ISyncableDataRepository* pRepo = repoBuilder.GetRepo();

            SyncClient client(client1DeviceId, &serverFactory);

            auto result = client.connectWeb(url, username, password);
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);

            // Add a case with unicode characters
            VectorClock newClock;
            CString newCaseGuid = makeUuid();
            repoBuilder.addRepoCase(newCaseGuid, 1, unicodeData);

            // Sync the new case
            result = client.syncData(SyncDirection::Both, *pRepo, L"");
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);

            result = client.disconnect();
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);

            // Sync with a second client to check if our case was saved correctly
            DeviceId client2DeviceId = makeUniqueDeviceId(L"it2-");
            repoBuilder.ResetRepo(client2DeviceId, dictionary.get());
            pRepo = repoBuilder.GetRepo();

            SyncClient client2(client2DeviceId, &serverFactory);

            result = client2.connectWeb(url, username, password);
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);
            result = client2.syncData(SyncDirection::Both, *pRepo, L"");
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);

            // Make sure case got synced correctly
            repoBuilder.verifyCaseInRepo(newCaseGuid, unicodeData);

            result = client2.disconnect();
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);
        }

        TEST_METHOD(TestSyncVectorClock)
        {
            ResetServer();

            DeviceId client1DeviceId = makeUniqueDeviceId(L"it1-");
            SyncServerConnectionFactory serverFactory(NULL);
            TestRepoBuilder client1RepoBuilder(client1DeviceId, dictionary.get());
            ISyncableDataRepository* pClient1Repo = client1RepoBuilder.GetRepo();

            SyncClient client(client1DeviceId, &serverFactory);

            auto result = client.connectWeb(url, username, password);
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);

            // First sync
            result = client.syncData(SyncDirection::Both, *pClient1Repo, L"");
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);
            Assert::AreEqual(0, (int) pClient1Repo->GetNumberCases());

            // Add a few new cases
            VectorClock newClock;
            newClock.increment(client1DeviceId);
            CString client2NewerCaseGuid = makeUuid();
            auto client2NewerCase = makeCase(*case_access, client2NewerCaseGuid, 1, { L"NEWCASE1DATA" },false);
            client2NewerCase->SetVectorClock(newClock);
            CString client1NewerCaseGuid = makeUuid();
            auto client1NewerCase = makeCase(*case_access, client1NewerCaseGuid, 2, { L"NEWCASE2DATA" }, false);
            client1NewerCase->SetVectorClock(newClock);
            CString conflictCaseGuid = makeUuid();
            auto conflictCase = makeCase(*case_access, conflictCaseGuid, 3, { L"13NEWCASE3DATA" }, false);
            conflictCase->SetVectorClock(newClock);

            std::vector<std::shared_ptr<Case>> initCases = { client2NewerCase, client1NewerCase, conflictCase };
            client1RepoBuilder.setInitialRepoCases(initCases, client1DeviceId);

            // Sync cases to server
            result = client.syncData(SyncDirection::Both, *pClient1Repo, L"");
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);
            Assert::AreEqual(3, (int) pClient1Repo->GetNumberCases());

            // Create a second client
            DeviceId client2DeviceId = makeUniqueDeviceId(L"it2-");
            TestRepoBuilder client2RepoBuilder(client2DeviceId, dictionary.get());
            ISyncableDataRepository* pClient2Repo = client2RepoBuilder.GetRepo();
            SyncClient client2(client2DeviceId, &serverFactory);

            // Copy all cases to client2 (simulating p2p sync)
            client2RepoBuilder.setInitialRepoCases(initCases, client1DeviceId);

            // Update cases on client1
            client1RepoBuilder.updateRepoCase(client1NewerCaseGuid, 2, L"UPDATEDBYCLIENT1");
            client1RepoBuilder.updateRepoCase(conflictCaseGuid, 3, L"UPDATEDBYCLIENT1");

            // Update cases on client2
            client2RepoBuilder.updateRepoCase(client2NewerCaseGuid, 1, L"UPDATEDBYCLIENT2");
            client2RepoBuilder.updateRepoCase(conflictCaseGuid, 3, L"UPDATEDBYCLIENT2");

            // Sync both clients
            result = client2.connectWeb(url, username, password);
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);
            result = client2.syncData(SyncDirection::Both, *pClient2Repo, L"");
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);
            result = client2.disconnect();
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);

            result = client.syncData(SyncDirection::Both, *pClient1Repo, L"");
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);
            result = client.disconnect();
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);

            // client2Newer was updated on client 2 but not client 1
            client1RepoBuilder.verifyCaseInRepo(client2NewerCaseGuid, L"UPDATEDBYCLIENT2");

            // client1Newer was updated on client 1 but not client 2
            client1RepoBuilder.verifyCaseInRepo(client1NewerCaseGuid, L"UPDATEDBYCLIENT1");

            // Conflict should go to client 1 since it synced last
            client1RepoBuilder.verifyCaseInRepo(conflictCaseGuid, L"UPDATEDBYCLIENT1");

        }

        // Reproduce problem from Vietnam where downloading data failed
        // to parse vector clock because CSWeb is sending deviceid in vector
        // clock as a number instead of as a string.
        TEST_METHOD(TestSyncVectorClockWithNumericDeviceId)
        {
            ResetServer();

            DeviceId client1DeviceId = L"9675914571713748";
            SyncServerConnectionFactory serverFactory(NULL);
            TestRepoBuilder client1RepoBuilder(client1DeviceId, dictionary.get());
            ISyncableDataRepository* pClient1Repo = client1RepoBuilder.GetRepo();

            SyncClient client(client1DeviceId, &serverFactory);

            auto result = client.connectWeb(url, username, password);
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);

            // Add a few cases
            VectorClock newClock;
            newClock.increment(client1DeviceId);
            CString client2NewerCaseGuid = makeUuid();
            auto client2NewerCase = makeCase(*case_access, client2NewerCaseGuid, 1, { L"NEWCASE1DATA" }, false);
            client2NewerCase->SetVectorClock(newClock);
            CString client1NewerCaseGuid = makeUuid();
            auto client1NewerCase = makeCase(*case_access, client1NewerCaseGuid, 2, { L"NEWCASE2DATA" }, false);
            client1NewerCase->SetVectorClock(newClock);
            CString conflictCaseGuid = makeUuid();
            auto conflictCase = makeCase(*case_access, conflictCaseGuid, 3, { L"NEWCASE3DATA" }, false);
            conflictCase->SetVectorClock(newClock);
            std::vector<std::shared_ptr<Case>> initCases = { client2NewerCase, client1NewerCase, conflictCase };
            client1RepoBuilder.setInitialRepoCases(initCases, client1DeviceId);

            // Sync cases to server
            result = client.syncData(SyncDirection::Put, *pClient1Repo, L"");
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);
            Assert::AreEqual(3, (int) pClient1Repo->GetNumberCases());

            // Create a second client
            DeviceId client2DeviceId = L"123456789";
            TestRepoBuilder client2RepoBuilder(client2DeviceId, dictionary.get());
            ISyncableDataRepository* pClient2Repo = client2RepoBuilder.GetRepo();
            SyncClient client2(client2DeviceId, &serverFactory);

            // Copy all cases to client2 (simulating p2p sync)
            client2RepoBuilder.setInitialRepoCases(initCases, client1DeviceId);

            // Update cases on client1
            client1RepoBuilder.updateRepoCase(client1NewerCaseGuid, 2, L"UPDATEDBYCLIENT1");
            client1RepoBuilder.updateRepoCase(conflictCaseGuid, 2, L"UPDATEDBYCLIENT1");

            // Update cases on client2
            client2RepoBuilder.updateRepoCase(client2NewerCaseGuid, 1, L"UPDATEDBYCLIENT2");
            client2RepoBuilder.updateRepoCase(conflictCaseGuid, 3, L"UPDATEDBYCLIENT2");

            // Sync put for both clients to generate conflict
            result = client2.connectWeb(url, username, password);
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);
            result = client2.syncData(SyncDirection::Put, *pClient2Repo, L"");
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);
            result = client2.disconnect();
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);

            result = client.syncData(SyncDirection::Put, *pClient1Repo, L"");
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);
            result = client.disconnect();
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);

            // Do a get on a clean client to verify that clocks are correct
            DeviceId client3DeviceId = L"987654321";
            TestRepoBuilder client3RepoBuilder(client3DeviceId, dictionary.get());
            ISyncableDataRepository* pClient3Repo = client3RepoBuilder.GetRepo();
            SyncClient client3(client2DeviceId, &serverFactory);

            result = client3.connectWeb(url, username, password);
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);
            result = client3.syncData(SyncDirection::Get, *pClient3Repo, L"");
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);

            result = client3.disconnect();
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);
        }

        TEST_METHOD(TestSyncDataDirection)
        {
            DeviceId client1DeviceId = makeUniqueDeviceId(L"it1-");
            SyncServerConnectionFactory serverFactory(NULL);

            TestRepoBuilder client1RepoBuilder(client1DeviceId, dictionary.get());
            ISyncableDataRepository* pClient1Repo = client1RepoBuilder.GetRepo();

            SyncClient client(client1DeviceId, &serverFactory);

            auto result = client.connectWeb(url, username, password);
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);

            // Initial sync with server using get to get baseline num cases
            result = client.syncData(SyncDirection::Get, *pClient1Repo, L"");
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);
            const int numInitialServerCases = pClient1Repo->GetNumberCases();

            // Add a couple of new cases
            CString newCase1Guid = makeUuid();
            client1RepoBuilder.addRepoCase(newCase1Guid, 1, L"NEWCASE1DATA");
            CString newCase2Guid = makeUuid();
            client1RepoBuilder.addRepoCase(newCase2Guid, 2, L"NEWCASE2DATA");

            // Put local cases to server - results in 2 new cases on server, same number in client1
            result = client.syncData(SyncDirection::Put, *pClient1Repo, L"");
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);
            Assert::AreEqual(numInitialServerCases + 2, (int) pClient1Repo->GetNumberCases());

            // Sync with a second client to check if our changes were saved to the server
            DeviceId client2DeviceId = makeUniqueDeviceId(L"it2-");
            TestRepoBuilder client2RepoBuilder(client2DeviceId, dictionary.get());
            ISyncableDataRepository* pClient2Repo = client2RepoBuilder.GetRepo();

            // Add a case to client2
            CString newCase3Guid = makeUuid();
            client2RepoBuilder.addRepoCase(newCase3Guid, 3, L"NEWCASE3DATA");

            SyncClient client2(client2DeviceId, &serverFactory);

            result = client2.connectWeb(url, username, password);
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);

            // Do a get with client2 - make sure we get both cases from client 1, server cases remain unchanged, client2
            // has one more case than server
            result = client2.syncData(SyncDirection::Get, *pClient2Repo, L"");
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);
            Assert::AreEqual(numInitialServerCases + 3, (int) pClient2Repo->GetNumberCases());

            // Do a get with client1 to make sure the previous get didn't upload anything
            result = client.syncData(SyncDirection::Get, *pClient1Repo, L"");
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);
            Assert::AreEqual(numInitialServerCases + 2, (int) pClient1Repo->GetNumberCases());

            // Add a new case and do a put with client1 - results is that server now has 3 cases beyond what it started with,
            // client1 matches server
            CString newCase4Guid = makeUuid();
            client1RepoBuilder.addRepoCase(newCase4Guid, 4, L"NEWCASE4DATA");
            result = client.syncData(SyncDirection::Put, *pClient1Repo, L"");
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);

            // Do a put with client2 - make sure we don't get the new case (case4), this  uploads case3 resulting in server having
            // all 4 new cases but client2 doesn't have case4
            result = client2.syncData(SyncDirection::Put, *pClient2Repo, L"");
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);
            Assert::AreEqual(numInitialServerCases + 3, (int) pClient2Repo->GetNumberCases());

            // Do a get with client2 - make sure we do get case4 (server should send everything since direction changed)
            // Server and client2 now both have all four new cases
            result = client2.syncData(SyncDirection::Get, *pClient2Repo, L"");
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);
            Assert::AreEqual(numInitialServerCases + 4, (int) pClient2Repo->GetNumberCases());
        }

        TEST_METHOD(TestPutGetFile)
        {
            DeviceId clientDeviceId = L"mydevice";
            SyncServerConnectionFactory serverFactory(NULL);

            SyncClient client(clientDeviceId, &serverFactory);

            auto result = client.connectWeb(url, username, password);
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);

            wchar_t pszTempPath[MAX_PATH];
            GetTempPath(MAX_PATH, pszTempPath);

            CString srcFilePath;
            srcFilePath.Format(L"%szsynco-get-file-test.txt", pszTempPath);
            DeleteFile(srcFilePath);

            std::wofstream srcWriteStream(srcFilePath);
            CString data = makeUuid();
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


        TEST_METHOD(TestGetWildcard)
        {
            DeviceId clientDeviceId = L"mydevice";
            SyncServerConnectionFactory serverFactory(NULL);

            SyncClient client(clientDeviceId, &serverFactory);

            auto result = client.connectWeb(url, username, password);
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);

            wchar_t pszTempPath[MAX_PATH];
            GetTempPath(MAX_PATH, pszTempPath);

            CString srcFilePath;
            srcFilePath.Format(L"%szsynco-get-file-test.txt", pszTempPath);
            DeleteFile(srcFilePath);

            std::wofstream srcWriteStream(srcFilePath);
            CString data = makeUuid();
            srcWriteStream << (LPCWSTR)data;
            srcWriteStream.close();

            CString serverPath(L"test.txt");

            auto syncResult = client.syncFile(SyncDirection::Put, srcFilePath, serverPath, CString());
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, syncResult);

            DeleteFile(srcFilePath);

            syncResult = client.syncFile(SyncDirection::Get, "test.*", srcFilePath, CString());
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, syncResult);

            result = client.disconnect();
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);

            // File exists
            Assert::IsTrue(GetFileAttributes(srcFilePath) != -1);

            std::wifstream srcReadStream(srcFilePath);
            std::wstring actualContents(std::istreambuf_iterator<wchar_t>(srcReadStream), {});
            srcReadStream.close();

            Assert::AreEqual((LPCWSTR)data, actualContents.c_str());

            DeleteFile(srcFilePath);
        }

        TEST_METHOD(TestDeleteCase)
        {
            ResetServer();

            DeviceId client1DeviceId = makeUniqueDeviceId(L"it1-");
            SyncServerConnectionFactory serverFactory(NULL);

            TestRepoBuilder repoBuilder(client1DeviceId, dictionary.get());
            ISyncableDataRepository* pRepo = repoBuilder.GetRepo();

            SyncClient client1(client1DeviceId, &serverFactory);

            auto result = client1.connectWeb(url, username, password);
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);

            // Add a new case and sync
            VectorClock newClock;
            newClock.increment(client1DeviceId);
            CString newCase1Guid = makeUuid();
            repoBuilder.addRepoCase(newCase1Guid, 9, L"NEWCASE1DATA");
            result = client1.syncData(SyncDirection::Both, *pRepo, L"");
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);
            Assert::AreEqual(1, (int) pRepo->GetNumberCases());

            // Delete the new case and sync
            repoBuilder.GetRepo()->DeleteCase(L"9");
            result = client1.syncData(SyncDirection::Both, *pRepo, L"");
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);

            result = client1.disconnect();
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);

            // Sync with a second client to check if our changes were saved to the server
            DeviceId client2DeviceId = makeUniqueDeviceId(L"it2-");
            repoBuilder.ResetRepo(client2DeviceId, dictionary.get());
            pRepo = repoBuilder.GetRepo();

            SyncClient client2(client2DeviceId, &serverFactory);

            result = client2.connectWeb(url, username, password);
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);
            result = client2.syncData(SyncDirection::Both, *pRepo, L"");
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);

            // Make sure case got deleted
            repoBuilder.verifyCaseDeleted(newCase1Guid);

            result = client2.disconnect();
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);
        }

        TEST_METHOD(TestSyncUniverse)
        {
            ResetServer();

            DeviceId client1DeviceId = makeUniqueDeviceId(L"it1-");
            SyncServerConnectionFactory serverFactory(NULL);

            TestRepoBuilder repoBuilder(client1DeviceId, dictionary.get());
            ISyncableDataRepository* pRepo = repoBuilder.GetRepo();

            SyncClient client1(client1DeviceId, &serverFactory);

            auto result = client1.connectWeb(url, username, password);
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);

            const CString universe(L"0");

            // Add two cases, one in universe and one out and sync
            VectorClock newClock;
            newClock.increment(client1DeviceId);
            CString newCase1Guid = makeUuid();
            repoBuilder.addRepoCase(newCase1Guid, 0, L"NEWCASE0DATA");
            CString newCase2Guid = makeUuid();
            repoBuilder.addRepoCase(newCase2Guid, 1, L"NEWCASE1DATA");
            result = client1.syncData(SyncDirection::Both, *pRepo, universe);
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);
            Assert::AreEqual(2, (int) pRepo->GetNumberCases());

            result = client1.disconnect();
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);

            // Sync with a second client to check if our changes were saved to the server
            DeviceId client2DeviceId = makeUniqueDeviceId(L"it2-");
            repoBuilder.ResetRepo(client2DeviceId, dictionary.get());
            pRepo = repoBuilder.GetRepo();

            SyncClient client2(client2DeviceId, &serverFactory);

            result = client2.connectWeb(url, username, password);
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);
            result = client2.syncData(SyncDirection::Both, *pRepo, universe);
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);

            // Should only get one new case - the one in universe
            Assert::AreEqual(1, (int) pRepo->GetNumberCases());

            result = client2.disconnect();
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);
        }

        TEST_METHOD(TestRefreshToken)
        {
            DeviceId clientDeviceId = L"mydevice";
            SyncServerConnectionFactory serverFactory(NULL);
            Mock<ILoginDialog> mockLoginDialog;
            When(Method(mockLoginDialog, Show)).AlwaysDo([this](const CString&, bool) -> std::optional<std::tuple<CString, CString>> {return std::make_tuple(username, password);});

            CString refreshToken;

            Mock<ICredentialStore> mockCredentialStore1;
            When(Method(mockCredentialStore1, Store)).AlwaysDo([&refreshToken](const std::wstring& h, const std::wstring& s) -> void {if (h.find(L"refresh") != std::wstring::npos) refreshToken = WS2CS(s);});
            When(Method(mockCredentialStore1, Retrieve)).AlwaysReturn(L"");

            SyncClient client1(clientDeviceId, &serverFactory);

            // This will use username and password from mockLoginDialog to obtain auth and refresh tokens
            auto result = client1.connectWeb(url, &mockLoginDialog.get(), &mockCredentialStore1.get());
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);

            Mock<ICredentialStore> mockCredentialStore2;
            When(Method(mockCredentialStore2, Retrieve)).AlwaysDo([refreshToken](const std::wstring& h) -> std::wstring {if (h.find(L"refresh") != std::wstring::npos) return CS2WS(refreshToken); else return L"expired";});
            When(Method(mockCredentialStore2, Store)).AlwaysReturn();

            SyncClient client2(clientDeviceId, &serverFactory);

            // This will get auth token "expired" which will fail so it will get new token using refresh token
            result = client2.connectWeb(url, &mockLoginDialog.get(), &mockCredentialStore2.get());
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);

        }

        TEST_METHOD(TestSyncChunkedGet)
        {
            ResetServer();

            DeviceId client1DeviceId = makeUniqueDeviceId(L"it1-");

            std::shared_ptr<FailableHttpConnection> httpConnection = std::make_shared<FailableHttpConnection>();
            CSWebSyncServerConnection webConnection(httpConnection, url, username, password);
            CSWebSyncServerConnection webConnection2(httpConnection, url, username, password);
            const int chunkSize = 100;
            const int numChunks = 3;
            const int totalCases = chunkSize * ((1 << numChunks) - 1);
            Mock<ISyncServerConnectionFactory> serverFactory;
            When(OverloadedMethod(serverFactory, createCSWebConnection, ISyncServerConnection*(CString, CString, CString)))
                .Return(&(webConnection)).Return(&(webConnection2)); // use 2 different csweb connections so that chunk sizes are not shared 
            When(Method(serverFactory, destroy)).AlwaysReturn();

            TestRepoBuilder repo1Builder(client1DeviceId, dictionary.get());
            ISyncableDataRepository* pRepo1 = repo1Builder.GetRepo();

            std::vector<std::shared_ptr<Case>> initialCases;
            VectorClock clock;
            clock.increment(client1DeviceId);
            for (int i = 1; i <= totalCases; ++i) {
                CString uuid;
                uuid.Format(L"00000000-0000-0000-0000-00000000%04d", i);
                auto init_case = makeCase(*case_access, uuid, 1, { "intialdata" }, false);
                init_case->SetVectorClock(clock);
                initialCases.emplace_back(init_case);
            }
            repo1Builder.setInitialRepoCases(initialCases, client1DeviceId);

            SyncClient client1(client1DeviceId, &serverFactory.get());

            auto result = client1.connectWeb(url, username, password);
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);

            // Upload initial cases to server
            result = client1.syncData(SyncDirection::Put, *pRepo1, L"");
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);

            // Sync with a second client
            DeviceId client2DeviceId = makeUniqueDeviceId(L"it2-");
            TestRepoBuilder repo2Builder(client2DeviceId, dictionary.get());
            ISyncableDataRepository* pRepo2 = repo2Builder.GetRepo();

            SyncClient client2(client2DeviceId, &serverFactory.get());

            result = client2.connectWeb(url, username, password);
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);

            // Get first chunk of results then fail on unrecoverable error
            httpConnection->setErrorInGetAfterCalls(404, 1);
            result = client2.syncData(SyncDirection::Both, *pRepo2, L"");
            Assert::AreEqual(SyncClient::SyncResult::SYNC_ERROR, result);
            Assert::AreEqual(chunkSize, (int) repo2Builder.GetRepo()->GetNumberCases());

            // Update and add cases on server
            repo1Builder.updateRepoCase(initialCases.front()->GetUuid(), 1, L"Updated\r\n");
            repo1Builder.updateRepoCase(initialCases.back()->GetUuid(), 1, L"Updated\r\n");
            repo1Builder.addRepoCase(L"00000000-0000-0000-0000-000000000000", 1, L"added\r\n");
            repo1Builder.addRepoCase(L"10000000-0000-0000-0000-000000000000", 1, L"added\r\n");

            result = client1.syncData(SyncDirection::Put, *pRepo1, L"");

            // Get the remaining cases
            result = client2.syncData(SyncDirection::Both, *pRepo2, L"");
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);
            Assert::AreEqual(initialCases.size() + 2, repo2Builder.GetRepo()->GetNumberCases());

            repo2Builder.verifyCaseInRepo(initialCases.front()->GetUuid(), L"Updated");
            repo2Builder.verifyCaseInRepo(initialCases.back()->GetUuid(), L"Updated");
        }

        TEST_METHOD(TestSyncChunkedPut)
        {
            ResetServer();

            DeviceId client1DeviceId = makeUniqueDeviceId(L"it1-");

            std::shared_ptr<FailableHttpConnection> httpConnection = std::make_shared<FailableHttpConnection>();
            CSWebSyncServerConnection webConnection(httpConnection, url, username, password);
            DefaultDataChunk defaultChunk;
            const int chunkSize = defaultChunk.getSize();
            const int numChunks = 3;
            Mock<ISyncServerConnectionFactory> serverFactory;
            When(OverloadedMethod(serverFactory, createCSWebConnection, ISyncServerConnection*(CString, CString, CString)))
                .AlwaysReturn(&(webConnection));
            When(Method(serverFactory, destroy)).AlwaysReturn();

            TestRepoBuilder repo1Builder(client1DeviceId, dictionary.get());
            ISyncableDataRepository* pRepo1 = repo1Builder.GetRepo();

            std::vector<std::shared_ptr<Case>> casesToUpload;
            VectorClock clock;
            clock.increment(client1DeviceId);
            for (int i = 1; i <= chunkSize * numChunks; ++i) {
                CString uuid;
                uuid.Format(L"00000000-0000-0000-0000-000000000%03d", i);
                casesToUpload.push_back(makeCase(*case_access, uuid, i, {L"data"}, false));
                casesToUpload.back()->SetVectorClock(clock);
            }
            repo1Builder.setInitialRepoCases(casesToUpload, client1DeviceId);

            SyncClient client1(client1DeviceId, &serverFactory.get());

            auto result = client1.connectWeb(url, username, password);
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);

            // Try to upload first chunk of cases to server and fail with error
            // before uploading any cases
            httpConnection->setErrorInPostAfterCalls(404, 0);
            result = client1.syncData(SyncDirection::Put, *pRepo1, L"");
            Assert::AreEqual(SyncClient::SyncResult::SYNC_ERROR, result);

            // Upload first chunk of cases to server and fail with error
            // before second
            httpConnection->setErrorInPostAfterCalls(404, 1);
            result = client1.syncData(SyncDirection::Put, *pRepo1, L"");
            Assert::AreEqual(SyncClient::SyncResult::SYNC_ERROR, result);

            // Sync with a second client to see if first chunk is uploaded
            DeviceId client2DeviceId = makeUniqueDeviceId(L"it2-");
            TestRepoBuilder repo2Builder(client2DeviceId, dictionary.get());
            ISyncableDataRepository* pRepo2 = repo2Builder.GetRepo();
            SyncClient client2(client2DeviceId, &serverFactory.get());
            result = client2.connectWeb(url, username, password);
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);
            result = client2.syncData(SyncDirection::Get, *pRepo2, L"");
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);
            Assert::AreEqual(chunkSize, repo2Builder.GetRepo()->GetLastSyncStats().numReceived);

            // Update cases and add new cases on client
            casesToUpload.front() = makeCase(*case_access, casesToUpload.front()->GetUuid(), 1, { L"Updated" });
            repo1Builder.updateRepoCase(casesToUpload.front()->GetUuid(), 1, getCaseData(*casesToUpload.front()));
            casesToUpload.back() = makeCase(*case_access, casesToUpload.front()->GetUuid(), 1, { L"Updated" });
            repo1Builder.updateRepoCase(casesToUpload.back()->GetUuid(), 1, getCaseData(*casesToUpload.back()));
            CString uuid;
            uuid.Format(L"00000000-0000-0000-0000-000000000%03d", casesToUpload.size() + 1);
            casesToUpload.push_back(makeCase(*case_access, uuid, 1, { L"11added" }));
            repo1Builder.addRepoCase(casesToUpload.back()->GetUuid(), 1, getCaseData(*casesToUpload.back()));
            casesToUpload.push_back(makeCase(*case_access, L"00000000-0000-0000-0000-000000000000", 1, { L"11added" }));
            repo1Builder.addRepoCase(casesToUpload.back()->GetUuid(), 1, getCaseData(*casesToUpload.back()));

            // Upload rest of cases
            httpConnection->setErrorInPostAfterCalls(404, -1);
            result = client1.syncData(SyncDirection::Put, *pRepo1, L"");
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);

            // Sync with second client again to make sure rest of cases were downloaded
            result = client2.syncData(SyncDirection::Get, *pRepo2, L"");
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);
            Assert::AreEqual((int) casesToUpload.size() - chunkSize + 1, repo2Builder.GetRepo()->GetLastSyncStats().numReceived);
            Assert::AreEqual(casesToUpload.size(), repo2Builder.GetRepo()->GetNumberCases());

            for (auto c : casesToUpload)
                repo2Builder.verifyCaseInRepo(*c);
        }

        TEST_METHOD(TestSyncGetExcludesPreviousPuts)
        {
            ResetServer();

            DeviceId client1DeviceId = makeUniqueDeviceId(L"it1-");
            SyncServerConnectionFactory serverFactory(NULL);

            DefaultDataChunk defaultChunk;
            const int chunkSize = defaultChunk.getSize();
            const int numChunks = 3;

            TestRepoBuilder repo1Builder(client1DeviceId, dictionary.get());
            ISyncableDataRepository* pRepo1 = repo1Builder.GetRepo();

            std::vector<std::shared_ptr<Case>> casesToUpload;
            VectorClock clock;
            clock.increment(client1DeviceId);
            for (int i = 1; i <= chunkSize * numChunks; ++i) {
                CString uuid;
                uuid.Format(L"00000000-0000-0000-0000-000000000%03d", i);
                casesToUpload.push_back(makeCase(*case_access, uuid, 1, { L"data" },false));
                casesToUpload.back()->SetVectorClock(clock);
            }
            repo1Builder.setInitialRepoCases(casesToUpload, client1DeviceId);

            SyncClient client1(client1DeviceId, &serverFactory);

            auto result = client1.connectWeb(url, username, password);
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);

            // Sync with multiple chunks to have mutliple sync revisions
            result = client1.syncData(SyncDirection::Both, *pRepo1, L"");

            // Do another sync, make sure that no new cases are downloaded
            result = client1.syncData(SyncDirection::Get, *pRepo1, L"");
            Assert::AreEqual(0, repo1Builder.GetRepo()->GetLastSyncStats().numReceived);
        }

    };
}
