#pragma once

#include <zSyncO/zSyncO.h>
#include <zAppO/SyncTypes.h>

class Case;
class CaseAccess;
class CDataDict;
class BinaryCaseItem;
class CaseItemIndex;

// Request to send to server to sync a data file
class SYNC_API SyncRequest
{
public:
    // Create SyncRequest
    SyncRequest(const CaseAccess& case_access, const DeviceId& device, CString universe,
        const std::vector<std::shared_ptr<Case>>& client_cases, CString lastServerRevision, bool isFirstChunk = true);

    SyncRequest(const CaseAccess& case_access, const DeviceId& device, CString universe,
        const std::vector<std::shared_ptr<Case>>& client_cases, std::vector<std::pair<const BinaryCaseItem*, CaseItemIndex>>& binary_case_items,
        CString lastServerRevision, bool isFirstChunk = true);

    SyncRequest(const CaseAccess& case_access, const DeviceId& device, CString universe,
        CString lastServerRevision, CString lastCaseUuid, const std::vector<CString>& excludedServerRevisions, bool isFirstChunk = true);

    const CaseAccess& getCaseAccess() const;

    const CDataDict& getDictionary() const;

    DeviceId getDeviceId() const;

    CString getUniverse() const;

    const std::vector<std::shared_ptr<Case>>& getClientCases() const;
    const std::vector<std::pair<const BinaryCaseItem*, CaseItemIndex>>& getBinaryCaseItems() const;

    CString getLastServerRevision() const;

    CString getLastCaseUuid() const;

    const std::vector<CString>& getExcludedServerRevisions() const;

    bool isFirstChunk() const;

private:
    const CaseAccess& m_caseAccess;
    const DeviceId m_deviceId;
    const CString m_universe;
    const std::vector<std::shared_ptr<Case>> m_clientCases;
    const std::vector<std::pair<const BinaryCaseItem*, CaseItemIndex>> m_binary_case_items;
    const CString m_lastServerRevision;
    const CString m_lastCaseUuid;
    const std::vector<CString> m_excludedServerRevisions;
    const bool m_isFirstChunk;
};
