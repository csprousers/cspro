#pragma once

#include <zAppO/SyncTypes.h>
#include <zDataO/ISyncableDataRepository.h>
#include <zDataO/WriteCaseParameter.h>
#include "CaseTestHelpers.h"

class CDataDict;


///<summary>
/// Helper class for creating a SQLite repo and cleaning it up
///</summary>
class TestRepoBuilder
{
public:

    TestRepoBuilder(DeviceId deviceId, const CDataDict* pDict) :
        m_deviceId(deviceId)
    {
        CreateRepo(deviceId, pDict);
    }

    ~TestRepoBuilder()
    {
        DeleteRepo();
    }

    ISyncableDataRepository* GetRepo()
    {
        return m_pRepo;
    }

    void setInitialRepoCases(const std::vector<std::shared_ptr<Case>>& cases, CString deviceId)
    {
        // Use sync since this is the only way to add a new case and preserve
        // the uuid
        m_pRepo->StartSync(deviceId, CString(), CString(), SyncDirection::Put, L"", true);
        m_pRepo->SyncCasesFromRemote(cases, CString());
        m_pRepo->EndSync();
    }

    void addRepoCase(CString uuid, int caseid, const CString& data)
    {
        // Use sync since this is the only way to add a new case and preserve
        // the uuid
        VectorClock clock;
        clock.increment(m_deviceId);
        auto data_case = makeCase(*m_pRepo->GetCaseAccess(), uuid, caseid, { data });
        data_case->SetVectorClock(clock);
        m_pRepo->StartSync(m_deviceId, CString(), CString(), SyncDirection::Put, L"", true);
        m_pRepo->SyncCasesFromRemote({ data_case }, CString());
        m_pRepo->EndSync();
    }

    void updateRepoCase(CString uuid, int caseid, const CString& data)
    {
        auto data_case = makeCase(*m_pRepo->GetCaseAccess(), uuid, caseid, { data });
        WriteCaseParameter write_case_parameter = WriteCaseParameter::CreateModifyParameter(*data_case);
        m_pRepo->WriteCase(*data_case, &write_case_parameter);
    }

    void updateRepoCaseNotes(CString uuid, const std::vector<Note>& notes)
    {
        auto data_case = findCaseByUuid(uuid);
        data_case->SetNotes(notes);
        WriteCaseParameter write_case_parameter = WriteCaseParameter::CreateModifyParameter(*data_case);
        write_case_parameter.SetNotesModified();
        m_pRepo->WriteCase(*data_case, &write_case_parameter);
    }

    void verifyCaseInRepo(const Case& expected_case)
    {
        auto actual_case = findCaseByUuid(expected_case.GetUuid());
        CString actual_data = getCaseData(*actual_case);
        CString expected_data = getCaseData(expected_case);
        if (expected_data != actual_data)
            Assert::AreEqual<CString>(expected_data, actual_data, L"Case data does not match");
    }

    void verifyCaseInRepo(const CString& uuid, const CString& expected_data)
    {
        auto actual_case = findCaseByUuid(uuid);
        CString actual_data = getCaseData(*actual_case);
        if (expected_data != actual_data)
            Assert::AreEqual<CString>(expected_data, actual_data, L"Case data does not match");
    }

    void verifyCaseDeleted(CString uuid)
    {

        auto data_case = findCaseByUuid(uuid);
        Assert::IsTrue(data_case->GetDeleted(), L"Case not deleted");
    }

    std::shared_ptr<Case> findCaseByUuid(const CString& uuid)
    {
        CString key;
        double pos;
        CString u = uuid;
        m_pRepo->PopulateCaseIdentifiers(key, u, pos);
        auto data_case = std::make_shared<Case>(m_pRepo->GetCaseAccess()->GetCaseMetadata());
        m_pRepo->ReadCase(*data_case, pos);
        return data_case;
    }

    void ResetRepo(DeviceId deviceId, const CDataDict* pDict)
    {
        DeleteRepo();
        CreateRepo(deviceId, pDict);
    }

private:

    void CreateRepo(DeviceId deviceId, const CDataDict* pDict)
    {
        wchar_t pszTempPath[MAX_PATH];
        GetTempPath(MAX_PATH, pszTempPath);
        m_repoPath.Format(L"%stestrepo_%s.csdb", pszTempPath, (LPCTSTR) deviceId);
        DeleteFile(m_repoPath);
        m_pRepo = assert_cast<ISyncableDataRepository*>(DataRepository::CreateAndOpen(CaseAccess::CreateAndInitializeFullCaseAccess(*pDict),
                                                        ConnectionString(m_repoPath),
                                                        DataRepositoryAccess::EntryInput,
                                                        DataRepositoryOpenFlag::CreateNew).release());
    }

    void DeleteRepo()
    {

        m_pRepo->Close();
        delete m_pRepo;
        DeleteFile(m_repoPath);
    }

    CString m_repoPath;
    ISyncableDataRepository* m_pRepo;
    CString m_deviceId;
};
