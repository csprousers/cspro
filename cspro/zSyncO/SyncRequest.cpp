#include "stdafx.h"
#include "SyncRequest.h"
#include <zCaseO/Case.h>
#include <zCaseO/CaseAccess.h>
#include <zCaseO/BinaryCaseItem.h>


SyncRequest::SyncRequest(const CaseAccess& case_access, const DeviceId& device, CString universe,
    const std::vector<std::shared_ptr<Case>>& client_cases, CString lastServerRevision, bool isFirstChunk)
    : m_caseAccess(case_access),
      m_deviceId(device),
      m_universe(universe),
      m_clientCases(client_cases),
      m_lastServerRevision(lastServerRevision),
      m_isFirstChunk(isFirstChunk)
{
}

SyncRequest::SyncRequest(const CaseAccess& case_access, const DeviceId& device, CString universe,
    const std::vector<std::shared_ptr<Case>>& client_cases, std::vector<std::pair<const BinaryCaseItem*, CaseItemIndex>>& binary_case_items,
    CString lastServerRevision, bool isFirstChunk)
    : m_caseAccess(case_access),
    m_deviceId(device),
    m_universe(universe),
    m_clientCases(client_cases),
    m_binary_case_items(binary_case_items),
    m_lastServerRevision(lastServerRevision),
    m_isFirstChunk(isFirstChunk)
{
}

SyncRequest::SyncRequest(const CaseAccess& case_access, const DeviceId& device, CString universe,
    CString lastServerRevision, CString lastCaseUuid, const std::vector<CString>& excludedServerRevisions, bool isFirstChunk)
    : m_caseAccess(case_access),
      m_deviceId(device),
      m_universe(universe),
      m_lastServerRevision(lastServerRevision),
      m_lastCaseUuid(lastCaseUuid),
      m_excludedServerRevisions(excludedServerRevisions),
      m_isFirstChunk(isFirstChunk)
{
}

const CaseAccess& SyncRequest::getCaseAccess() const
{
    return m_caseAccess;
}

const CDataDict& SyncRequest::getDictionary() const
{
    return m_caseAccess.GetDataDict();
}

DeviceId SyncRequest::getDeviceId() const
{
    return m_deviceId;
}

CString SyncRequest::getUniverse() const
{
    return m_universe;
}

const std::vector<std::shared_ptr<Case>>& SyncRequest::getClientCases() const
{
    return m_clientCases;
}

const std::vector<std::pair<const BinaryCaseItem*, CaseItemIndex>>& SyncRequest::getBinaryCaseItems() const
{
    return m_binary_case_items;
}

CString SyncRequest::getLastServerRevision() const
{
    return m_lastServerRevision;
}

CString SyncRequest::getLastCaseUuid() const
{
    return m_lastCaseUuid;
}

const std::vector<CString>& SyncRequest::getExcludedServerRevisions() const
{
    return m_excludedServerRevisions;
}

bool SyncRequest::isFirstChunk() const
{
    return m_isFirstChunk;
}
