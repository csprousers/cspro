#include "StdAfx.h"
#include "DictLevel.h"


DictLevel::DictLevel() noexcept
    :   m_levelNumber(0),
        m_idItemsDictRecord(true),
        m_iNumRecords(0)
{
}

DictLevel::DictLevel(const DictLevel& rhs) noexcept // DD_STD_REFACTOR_TODO is this needed?
    :   DictNamedBase(rhs),
        m_levelNumber(rhs.m_levelNumber),
        m_idItemsDictRecord(rhs.m_idItemsDictRecord),
        m_iNumRecords(0)
{
    for( size_t i = 0 ;i < rhs.m_aDictRecord.size(); ++i )
        AddRecord(rhs.m_aDictRecord[i]);
}

DictLevel::DictLevel(DictLevel&& rhs) noexcept // DD_STD_REFACTOR_TODO is this needed?
    :   DictNamedBase(rhs),
        m_levelNumber(rhs.m_levelNumber),
        m_idItemsDictRecord(std::move(rhs.m_idItemsDictRecord)),
        m_iNumRecords(rhs.m_iNumRecords)
{
    for( size_t i = 0; i < rhs.m_aDictRecord.size(); ++i )
        m_aDictRecord.emplace_back(rhs.m_aDictRecord[i]);

    rhs.m_aDictRecord.clear();
    rhs.m_iNumRecords = 0;
}


DictLevel::~DictLevel()
{
    for( size_t i = 0; i < m_aDictRecord.size(); ++i )
        delete m_aDictRecord[i];
}


DictLevel& DictLevel::operator=(const DictLevel& rhs) noexcept // DD_STD_REFACTOR_TODO is this needed?
{
    DictNamedBase::operator=(rhs);

    m_levelNumber = rhs.m_levelNumber;
    m_idItemsDictRecord = rhs.m_idItemsDictRecord;

    RemoveAllRecords();

    for( size_t i = 0 ; i < rhs.m_aDictRecord.size(); ++i )
        AddRecord(rhs.m_aDictRecord[i]);

    return *this;
}

/////////////////////////////////////////////////////////////////////////////
//
//                           DictLevel::AddRecord
//
/////////////////////////////////////////////////////////////////////////////
void DictLevel::AddRecord(const CDictRecord* pRecord)
{
    ASSERT(!pRecord->IsIdRecord());
    CDictRecord* pNewRecord = new CDictRecord(*pRecord);
    m_iNumRecords++;
    m_aDictRecord.emplace_back(pNewRecord);
}

/////////////////////////////////////////////////////////////////////////////
//
//                           DictLevel::InsertRecordAt
//
/////////////////////////////////////////////////////////////////////////////

void DictLevel::InsertRecordAt(int iIndex, const CDictRecord* pRecord)
{
    if (iIndex == COMMON) {
        ASSERT(pRecord->IsIdRecord());
        m_idItemsDictRecord = *pRecord;
    }
    else  {
        CDictRecord* pNewRecord = new CDictRecord(*pRecord);
        m_aDictRecord.insert(m_aDictRecord.begin() + iIndex, pNewRecord);
        m_iNumRecords++;
    }
}

/////////////////////////////////////////////////////////////////////////////
//
//                           DictLevel::RemoveAllRecords
//
/////////////////////////////////////////////////////////////////////////////

void DictLevel::RemoveAllRecords()
{
    while( !m_aDictRecord.empty() )
    {
        ASSERT(m_aDictRecord[0]);
        RemoveRecordAt(0);
    }
    m_iNumRecords = 0;
}

/////////////////////////////////////////////////////////////////////////////
//
//                           DictLevel::RemoveRecordAt
//
/////////////////////////////////////////////////////////////////////////////

void DictLevel::RemoveRecordAt (int iIndex) {

    if (iIndex == COMMON) {
        m_idItemsDictRecord.RemoveAllItems();
    }
    else  {
        CDictRecord* pRec = m_aDictRecord[iIndex];
        m_aDictRecord.erase(m_aDictRecord.begin() + iIndex);
        ASSERT(pRec);
        m_iNumRecords--;
        ASSERT(m_iNumRecords >= 0);
        delete pRec;
    }
}


DictLevel DictLevel::CreateFromJson(const JsonNode<wchar_t>& json_node)
{
    DictLevel dict_level;

    auto dict_serializer_helper = json_node.GetSerializerHelper().Get<DictionarySerializerHelper>();
    if( dict_serializer_helper != nullptr )
        dict_serializer_helper->SetCurrentLevel(dict_level);

    dict_level.DictNamedBase::ParseJsonInput(json_node);

    dict_level.m_idItemsDictRecord = CDictRecord::CreateFromJson(json_node.Get(JK::ids), true);

    std::vector<CDictRecord> dict_records = json_node.GetArrayOrEmpty(JK::records).GetVector<CDictRecord>(
        [&](const JsonParseException& exception)
        {
            json_node.LogWarning(_T("A record was not added to '%s' due to errors: %s"), (LPCTSTR)dict_level.GetName(), exception.GetErrorMessage().c_str());
        });

    for( const CDictRecord& dict_record : dict_records )
        dict_level.AddRecord(&dict_record);

    return dict_level;
}


void DictLevel::WriteJson(JsonWriter& json_writer) const
{
    auto dict_serializer_helper = json_writer.GetSerializerHelper().Get<DictionarySerializerHelper>();
    if( dict_serializer_helper != nullptr )
        dict_serializer_helper->SetCurrentLevel(*this);

    json_writer.BeginObject();

    DictNamedBase::WriteJson(json_writer);

    json_writer.Write(JK::ids, m_idItemsDictRecord);

    json_writer.BeginArray(JK::records);

    for( size_t i = 0; i < m_aDictRecord.size(); ++i )
        json_writer.Write(*m_aDictRecord[i]);

    json_writer.EndArray();

    json_writer.EndObject();
}


void DictLevel::serialize(Serializer& ar)
{
    DictNamedBase::serialize(ar);

    if( ar.PredatesVersionIteration(Serializer::Iteration_8_0_000_1) )
        SetNote(ar.Read<CString>());

    ar.IgnoreUnusedVariable<CString>(Serializer::Iteration_8_0_000_1); // m_csError

    if( ar.MeetsVersionIteration(Serializer::Iteration_8_0_002_1) )
        ar & m_levelNumber;

    ar & m_iNumRecords;

    ar.IgnoreUnusedVariable<bool>(Serializer::Iteration_8_0_000_1); // m_bUsed

    ar & m_idItemsDictRecord;

    ar & m_aDictRecord;
}
