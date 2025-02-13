#include "StdAfx.h"
#include "DictRecord.h"
#include "DictionaryValidator.h"


/////////////////////////////////////////////////////////////////////////////
//
//                             CDictRecord::CDictRecord
//
/////////////////////////////////////////////////////////////////////////////

CDictRecord::CDictRecord(bool id_record/* = false*/)
    :   m_idRecord(id_record)
{
    if( m_idRecord )
    {
        SetName(_T("_IDS"));
        SetLabel(_T("(Id Items)"));
    }

    m_csRecTypeVal.Empty();
    m_bRequired = DictionaryDefaults::Required;
    SetMaxRecs (DictionaryDefaults::MaxRecs);
    m_iRecLen = DictionaryDefaults::RecLen;
    RemoveAllItems();

    m_iSymbol = -1;
    m_pDataDict = NULL;
    m_pLevel = NULL;
    m_iSonNumber = -1;
}

// copy constructor
CDictRecord::CDictRecord(const CDictRecord& r)
    :   DictNamedBase(r),
        m_idRecord(r.m_idRecord)
{
    m_csRecTypeVal = r.m_csRecTypeVal;
    m_bRequired = r.m_bRequired;
    SetMaxRecs(r.m_uMaxRecs);
    m_iRecLen = r.m_iRecLen;
    RemoveAllItems();
    for (size_t i = 0 ; i < r.m_aDictItem.size() ; i++) {
        AddItem(r.m_aDictItem[i]);
    }

    m_occurrenceLabels = r.m_occurrenceLabels;

    m_iSymbol = r.m_iSymbol;
    m_pDataDict = NULL;
    m_pLevel = NULL;
    m_iSonNumber = r.m_iSonNumber;
}

/////////////////////////////////////////////////////////////////////////////
//
//                             CDictRecord::~CDictRecord
//
/////////////////////////////////////////////////////////////////////////////

CDictRecord::~CDictRecord()
{
    for ( size_t i = 0 ; i < m_aDictItem.size() ; i++)
    {
        ASSERT(m_aDictItem[i]);
        delete m_aDictItem[i];
    }
}



const CDictItem* CDictRecord::FindItem(wstring_view item_name) const
{
    for( int i = 0; i < GetNumItems(); ++i )
    {
        const CDictItem* pItem = GetItem(i);

        if( SO::EqualsNoCase(item_name, pItem->GetName()) )
            return pItem;
    }

    return nullptr;
}

CDictItem* CDictRecord::FindItem(wstring_view item_name)
{
    return const_cast<CDictItem*>(const_cast<const CDictRecord*>(this)->FindItem(item_name));
}



/////////////////////////////////////////////////////////////////////////////
//
//                             CDictRecord::AddItem
//
/////////////////////////////////////////////////////////////////////////////

void CDictRecord::AddItem(const CDictItem* pItem)
{
    CDictItem* pNewItem = new CDictItem(*pItem);
    m_iNumItems++;
    m_aDictItem.emplace_back(pNewItem);
}

/////////////////////////////////////////////////////////////////////////////
//
//                             CDictRecord::InsertItemAt
//
/////////////////////////////////////////////////////////////////////////////

void CDictRecord::InsertItemAt (int iIndex, const CDictItem* pItem) {

    CDictItem* pNewItem = new CDictItem(*pItem);
    m_iNumItems++;
    m_aDictItem.insert(m_aDictItem.begin() + iIndex, pNewItem);
}

bool CDictRecord::Is2DRecord() const
{
    if( m_uMaxRecs == 1 )
        return false;

    for( size_t i = 0; i < m_aDictItem.size(); ++i )
    {
        if( m_aDictItem[i]->GetOccurs() > 1 )
            return true;
    }

    return false;
}

/////////////////////////////////////////////////////////////////////////////
//
//                             CDictRecord::RemoveAllItems
//
/////////////////////////////////////////////////////////////////////////////

void CDictRecord::RemoveAllItems()
{
    while( !m_aDictItem.empty() )
    {
        ASSERT(m_aDictItem[0]);
        RemoveItemAt(0);
    }

    m_iNumItems = 0;
}

/////////////////////////////////////////////////////////////////////////////
//
//                             CDictRecord::RemoveItemAt
//
/////////////////////////////////////////////////////////////////////////////

void CDictRecord::RemoveItemAt(int iIndex)
{
    CDictItem* pItem = m_aDictItem[iIndex];
    m_aDictItem.erase(m_aDictItem.begin() + iIndex);
    ASSERT(pItem);
    m_iNumItems--;
    ASSERT(m_iNumItems >= 0);
    delete pItem;
}

/////////////////////////////////////////////////////////////////////////////
//
//                           CDictRecord::CalculateRecLen
//
/////////////////////////////////////////////////////////////////////////////

UINT CDictRecord::CalculateRecLen()
{
    const CDataDict* dictionary = GetDataDict();
    ASSERT(dictionary != nullptr);

    UINT uRecLen = DictionaryValidator::GetRecordLength(*dictionary, this);
    SetRecLen(uRecLen);

    return uRecLen;
}


/////////////////////////////////////////////////////////////////////////////
//
//                             CDictRecord::operator=
//
/////////////////////////////////////////////////////////////////////////////

void CDictRecord::operator=(const CDictRecord& record)
{
    DictNamedBase::operator=(record);

    m_idRecord = record.m_idRecord;

    m_csRecTypeVal = record.m_csRecTypeVal;
    m_iRecLen = record.m_iRecLen;
    SetMaxRecs(record.m_uMaxRecs);
    m_bRequired = record.m_bRequired;

    RemoveAllItems();
    for (size_t i = 0 ; i < record.m_aDictItem.size() ; i++) {
        AddItem(record.m_aDictItem[i]);
    }
}


CDictRecord CDictRecord::CreateFromJson(const JsonNode<wchar_t>& json_node, bool id_record/* = false*/)
{
    CDictRecord dict_record(id_record);

    auto dict_serializer_helper = json_node.GetSerializerHelper().Get<DictionarySerializerHelper>();
    if( dict_serializer_helper != nullptr )
    {
        dict_serializer_helper->SetCurrentRecord(dict_record);
        dict_record.m_pDataDict = &dict_serializer_helper->GetDictionary();
    }

    if( !id_record )
    {
        dict_record.DictNamedBase::ParseJsonInput(json_node);

        dict_record.m_csRecTypeVal = json_node.GetOrDefault(JK::recordType, SO::EmptyCString);

        const auto& occurrences_node = json_node.Get(JK::occurrences);

        dict_record.m_bRequired = occurrences_node.Get<bool>(JK::required);
        dict_record.m_uMaxRecs = occurrences_node.Get<unsigned>(JK::maximum);

        if( dict_record.m_uMaxRecs > 1 && occurrences_node.Contains(JK::labels) )
            dict_record.m_occurrenceLabels = OccurrenceLabels::CreateFromJson(occurrences_node.Get(JK::labels), dict_record.m_uMaxRecs);

        // set the initial start position for items on this record
        if( dict_serializer_helper != nullptr && dict_serializer_helper->GetDictionary().IsPosRelative() &&
            dict_serializer_helper->GetCurrentLevel() != nullptr )
        {
            dict_serializer_helper->SetItemStart(dict_serializer_helper->GetCurrentLevel()->GetIdItemsRec()->GetRecLen() + 1);
        }
    }

    std::vector<CDictItem> dict_items = json_node.GetArrayOrEmpty(JK::items).GetVector<CDictItem>(
        [&](const JsonParseException& exception)
        {
            json_node.LogWarning(_T("An item was not added to '%s' due to errors: %s"), (LPCTSTR)dict_record.GetName(), exception.GetErrorMessage().c_str());
        });

    for( const CDictItem& dict_item : dict_items )
        dict_record.AddItem(&dict_item);

    // update the record length
    if( dict_record.m_pDataDict != nullptr )
        dict_record.CalculateRecLen();

    return dict_record;
}


void CDictRecord::WriteJson(JsonWriter& json_writer) const
{
    auto dict_serializer_helper = json_writer.GetSerializerHelper().Get<DictionarySerializerHelper>();
    if( dict_serializer_helper != nullptr )
        dict_serializer_helper->SetCurrentRecord(*this);

    json_writer.BeginObject();

    if( !IsIdRecord() )
    {
        DictNamedBase::WriteJson(json_writer);

        if( json_writer.Verbose() || !m_csRecTypeVal.IsEmpty() )
            json_writer.Write(JK::recordType, m_csRecTypeVal);

        json_writer.BeginObject(JK::occurrences);

        json_writer.Write(JK::required, m_bRequired);
        json_writer.Write(JK::maximum, m_uMaxRecs);

        if( m_uMaxRecs > 1 && ( json_writer.Verbose() || !m_occurrenceLabels.GetLabels().empty() ) )
        {
            json_writer.Key(JK::labels);
            m_occurrenceLabels.WriteJson(json_writer, m_uMaxRecs);
        }

        json_writer.EndObject();
    }

    json_writer.BeginArray(JK::items);

    for( size_t i = 0; i < m_aDictItem.size(); ++i )
        json_writer.Write(*m_aDictItem[i]);

    json_writer.EndArray();

    json_writer.EndObject();
}


void CDictRecord::serialize(Serializer& ar)
{
    DictNamedBase::serialize(ar);

    if( ar.PredatesVersionIteration(Serializer::Iteration_8_0_000_1) )
        SetNote(ar.Read<CString>());

    ar & m_csRecTypeVal;

    ar.IgnoreUnusedVariable<CString>(Serializer::Iteration_8_0_000_1); // m_csError

    ar & m_bRequired;
    ar & m_uMaxRecs;
    ar & m_iRecLen;
    ar & m_iNumItems;

    ar & m_iSymbol;
    ar & m_iSonNumber;

    ar.IgnoreUnusedVariable<bool>(Serializer::Iteration_8_0_000_1); // m_bUsed

    ar & m_aDictItem;

    ar & m_occurrenceLabels;
}
