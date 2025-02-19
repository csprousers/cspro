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
    TestRepoBuilder(DeviceId deviceId, const CDataDict* pDict)
        :   m_deviceId(std::move(deviceId))
    {
        CreateRepo(m_deviceId, pDict);
    }

    ~TestRepoBuilder()
    {
        DeleteRepo();
    }

    ISyncableDataRepository* GetRepo()
    {
        return m_pRepo.get();
    }

    void setInitialRepoCases(const std::vector<std::shared_ptr<Case>>& cases, DeviceId deviceId)
    {
        // Use sync since this is the only way to add a new case and preserve
        // the uuid
        m_pRepo->StartSync(std::move(deviceId), std::string(), std::string(), SyncDirection::Put, "", true);
        m_pRepo->SyncCasesFromRemote(cases, SO::Empty_string);
        m_pRepo->EndSync();
    }

    std::shared_ptr<Case> addRepoCase(std::string uuid, int id, const std::string& data, std::optional<VectorClock> clock = std::nullopt)
    {
        // Use sync since this is the only way to add a new case and preserve the UUID
        if( !clock.has_value() )
        {
            clock.emplace();
            clock->increment(m_deviceId);
        }

        std::shared_ptr<Case> data_case = CreateCase(m_pRepo->GetCaseAccess(), std::move(uuid), id, { data }, false);
        data_case->SetVectorClock(std::move(*clock));

        m_pRepo->StartSync(m_deviceId, std::string(), std::string(), SyncDirection::Put, "", true);
        m_pRepo->SyncCasesFromRemote({ data_case }, SO::Empty_string);
        m_pRepo->EndSync();

        return data_case;
    }

    std::shared_ptr<Case> updateRepoCase(std::string uuid, const int id, const std::string& data)
    {
        std::shared_ptr<Case> data_case = CreateCase(m_pRepo->GetCaseAccess(), std::move(uuid), id, { data }, false);
        WriteCaseParameter write_case_parameter = WriteCaseParameter::CreateModifyParameter(*data_case);
        m_pRepo->WriteCase(*data_case, &write_case_parameter);
        return data_case;
    }

    void updateRepoCaseNotes(const std::string& uuid, std::vector<Note> notes)
    {
        const std::unique_ptr<Case> data_case = findCaseByUuid(uuid);
        data_case->SetNotes(std::move(notes));
        WriteCaseParameter write_case_parameter = WriteCaseParameter::CreateModifyParameter(*data_case);
        write_case_parameter.SetNotesModified();
        m_pRepo->WriteCase(*data_case, &write_case_parameter);
    }

    void verifyCaseInRepo(const Case& expected_case)
    {
        const std::unique_ptr<Case> actual_case = findCaseByUuid(expected_case.GetUuid());
        Assert::AreEqual(getCaseData(expected_case), getCaseData(*actual_case), L"Case data does not match");
    }

    void verifyCaseInRepo(const std::string& uuid, const std::string& expected_data)
    {
        const std::unique_ptr<Case> actual_case = findCaseByUuid(uuid);
        Assert::AreEqual(expected_data, getCaseData(*actual_case), L"Case data does not match");
    }

    void verifyCaseDeleted(const std::string& uuid)
    {
        const std::shared_ptr<Case> data_case = findCaseByUuid(uuid);
        Assert::IsTrue(data_case->GetDeleted(), L"Case not deleted");
    }

    std::unique_ptr<Case> findCaseByUuid(const std::string& uuid)
    {
        std::unique_ptr<Case> data_case = m_pRepo->GetCaseAccess().CreateCase();
        m_pRepo->ReadCaseByUuid(*data_case, uuid);
        return data_case;
    }

    void ResetRepo(DeviceId deviceId, const CDataDict* pDict)
    {
        DeleteRepo();
        CreateRepo(std::move(deviceId), pDict);
    }

private:
    void CreateRepo(const DeviceId& deviceId, const CDataDict* pDict)
    {
        m_repoPath = Path::Combine(GetTempDirectory(), FormatText("testrepo_%s.csdb", deviceId.c_str()));
        PortableFunctions::FileDelete(m_repoPath);

        DataRepository* repo = DataRepository::CreateAndOpen(CaseAccess::CreateAndInitializeFullCaseAccess(*pDict),
                                                             ConnectionString(m_repoPath),
                                                             DataRepositoryAccess::EntryInput,
                                                             DataRepositoryOpenFlag::CreateNew).release();
        m_pRepo = std::unique_ptr<ISyncableDataRepository>(assert_cast<ISyncableDataRepository*>(repo));
    }

    void DeleteRepo()
    {
        m_pRepo->Close();
        m_pRepo.reset();
        PortableFunctions::FileDelete(m_repoPath);
    }

private:
    std::string m_repoPath;
    std::unique_ptr<ISyncableDataRepository> m_pRepo;
    const DeviceId m_deviceId;
};
