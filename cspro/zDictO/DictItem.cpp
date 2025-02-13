#include "StdAfx.h"
#include "DictItem.h"
#include "ValueSetFixer.h"


CDictItem::CDictItem()
    :   m_uStart(999),
        m_uLen(DictionaryDefaults::ItemLen),
        m_contentType(DictionaryDefaults::ContentType),
        m_itemType(DictionaryDefaults::ItemType),
        m_uOccurs(DictionaryDefaults::Occurs),
        m_uDecimal(DictionaryDefaults::Decimal),
        m_bDecChar(DictionaryDefaults::DecChar),
        m_bZeroFill(DictionaryDefaults::ZeroFill),
        m_iSymbol(-1),
        m_pParentItem(nullptr),
        m_pLevel(nullptr),
        m_pRecord(nullptr),
        m_iSonNumber(-1)
{
}

// copy constructor
CDictItem::CDictItem(const CDictItem& rhs)
    :   DictNamedBase(rhs),
        m_uStart(rhs.m_uStart),
        m_uLen(rhs.m_uLen),
        m_contentType(rhs.m_contentType),
        m_itemType(rhs.m_itemType),
        m_uOccurs(rhs.m_uOccurs),
        m_uDecimal(rhs.m_uDecimal),
        m_bDecChar(rhs.m_bDecChar),
        m_bZeroFill(rhs.m_bZeroFill),
        m_captureInfo(rhs.m_captureInfo),
        m_iSymbol(rhs.m_iSymbol),
        m_pParentItem(rhs.m_pParentItem),
        m_pLevel(rhs.m_pLevel),
        m_pRecord(rhs.m_pRecord),
        m_iSonNumber(rhs.m_iSonNumber),
        m_occurrenceLabels(rhs.m_occurrenceLabels),
        m_dictValueSets(rhs.m_dictValueSets)
{
}


DataType CDictItem::GetDataType() const
{
    return IsNumeric(m_contentType) ? DataType::Numeric :
           IsString(m_contentType)  ? DataType::String :
           IsBinary(m_contentType)  ? DataType::Binary :
                                      ReturnProgrammingError(DataType::Numeric);
}


CString CDictItem::GetQualifiedName() const
{
    return GetRecord()->GetDataDict()->MakeQualifiedName(GetName());
}


bool CDictItem::HasSubitems() const
{
    if( GetItemType() == ItemType::Item )
    {
        if( ( GetSonNumber() + 1 ) < GetRecord()->GetNumItems() )
            return ( GetRecord()->GetItem(GetSonNumber() + 1)->GetItemType() == ItemType::Subitem );
    }

    return false;
}


void CDictItem::AddValueSet(DictValueSet dict_value_set)
{
    m_dictValueSets.emplace_back(std::move(dict_value_set));
}

void CDictItem::InsertValueSet(size_t index, DictValueSet dict_value_set)
{
    ASSERT(index <= m_dictValueSets.size());
    m_dictValueSets.insert(m_dictValueSets.begin() + index, std::move(dict_value_set));
}

void CDictItem::RemoveValueSet(size_t index)
{
    ASSERT(index < m_dictValueSets.size());
    m_dictValueSets.erase(m_dictValueSets.begin() + index);
}

void CDictItem::RemoveAllValueSets()
{
    m_dictValueSets.clear();
}


/////////////////////////////////////////////////////////////////////////////
//
//                             CDictItem::operator=
//
/////////////////////////////////////////////////////////////////////////////

void CDictItem::operator=(const CDictItem& item)
{
    DictNamedBase::operator=(item);

    m_uStart = item.m_uStart;
    m_uLen = item.m_uLen;
    m_contentType = item.m_contentType;
    m_itemType = item.m_itemType;

    m_uOccurs = item.m_uOccurs;
    m_occurrenceLabels = item.m_occurrenceLabels;

    m_uDecimal = item.m_uDecimal;
    m_bDecChar = item.m_bDecChar;
    m_bZeroFill = item.m_bZeroFill;
    m_captureInfo = item.m_captureInfo;

    m_dictValueSets = item.m_dictValueSets;
}

/////////////////////////////////////////////////////////////////////////////
//
//                             CDictItem::GetCompleteLen
//
/////////////////////////////////////////////////////////////////////////////
// rcl, Apr 2005
UINT CDictItem::GetCompleteLen() const
{
    int iLength = this->GetLen();
    if(!this->GetDecChar() && this->GetDecimal() != 0)
    {
        // The length does not account for the decimal character in this case
        // so we fix that here
        iLength++;
    }

    return iLength;
}

UINT CDictItem::GetIntegerLen() const
{
    ASSERT(GetContentType() == ContentType::Numeric);
    UINT integer_length = GetLen() - GetDecimal();

    if( GetDecChar() )
        --integer_length;

    return integer_length;
}

UINT CDictItem::GetItemSubitemOccurs() const
{
    UINT occurrences = GetOccurs();

    if( GetParentItem() != nullptr )
        occurrences *= GetParentItem()->GetOccurs();

    return occurrences;
}


CDictItem CDictItem::CreateFromJson(const JsonNode<wchar_t>& json_node)
{
    CDictItem dict_item;

    auto dict_serializer_helper = json_node.GetSerializerHelper().Get<DictionarySerializerHelper>();
    if( dict_serializer_helper != nullptr )
        dict_serializer_helper->SetCurrentItem(dict_item);

    dict_item.DictNamedBase::ParseJsonInput(json_node);

    if( json_node.Contains(JK::occurrences) )
    {
        const auto& occurrences_node = json_node.Get(JK::occurrences);

        dict_item.m_uOccurs = occurrences_node.Get<unsigned>(JK::maximum);

        if( dict_item.m_uOccurs > 1 && occurrences_node.Contains(JK::labels) )
            dict_item.m_occurrenceLabels = OccurrenceLabels::CreateFromJson(occurrences_node.Get(JK::labels), dict_item.m_uOccurs);
    }

    dict_item.m_contentType = json_node.Get<ContentType>(JK::contentType);

    if( dict_item.m_contentType == ContentType::Numeric || dict_item.m_contentType == ContentType::Alpha )
    {
        if( json_node.GetOrDefault(JK::subitem, false) )
            dict_item.m_itemType = ItemType::Subitem;

        dict_item.m_uLen = json_node.Get<unsigned>(JK::length);
    }

    else
    {
        ASSERT(IsBinary(dict_item));

        if( dict_serializer_helper != nullptr )
            dict_serializer_helper->SetUsesBinaryItems();

        dict_item.m_uLen = 1;
    }

    // by default the start position is not written when using relative positioning for non-ID items...
    if( json_node.Contains(JK::start) || dict_serializer_helper == nullptr || !dict_serializer_helper->GetDictionary().IsPosRelative() )
    {
        dict_item.m_uStart = json_node.Get<unsigned>(JK::start);
    }

    // ...so we have to calculate it ourselves
    else
    {
        if( dict_item.IsSubitem() )
        {
            dict_item.m_uStart = dict_serializer_helper->GetLastParentItemStart() + json_node.Get<unsigned>(JK::subitemOffset);
        }

        else
        {
            dict_item.m_uStart = dict_serializer_helper->GetItemStart();
        }
    }
            
    if( dict_serializer_helper != nullptr && !dict_item.IsSubitem() )
        dict_serializer_helper->IncrementItemStart(dict_item);

    if( dict_item.m_contentType == ContentType::Numeric )
    {
        if( json_node.Contains(JK::decimals) )
        {
            dict_item.m_uDecimal = json_node.Get<unsigned>(JK::decimals);

            if( dict_item.m_uDecimal > 0 )
                dict_item.m_bDecChar = json_node.Get<bool>(JK::decimalMark);
        }

        if( json_node.Contains(JK::zeroFill) )
        {
            dict_item.m_bZeroFill = json_node.Get<bool>(JK::zeroFill);
        }

        else
        {
            dict_item.m_bZeroFill = ( dict_serializer_helper != nullptr ) ? dict_serializer_helper->GetDictionary().IsZeroFill() :
                                                                            DictionaryDefaults::ZeroFill;
        }
    }

    if( json_node.Contains(JK::capture) )
        dict_item.m_captureInfo = json_node.Get<CaptureInfo>(JK::capture);

    dict_item.m_dictValueSets = json_node.GetArrayOrEmpty(JK::valueSets).GetVector<DictValueSet>(
        [&](const JsonParseException& exception)
        {
            json_node.LogWarning(_T("A value set was not added to '%s' due to errors: %s"), (LPCTSTR)dict_item.GetName(), exception.GetErrorMessage().c_str());
        });


    // fix numeric value sets
    ValueSetFixer(dict_item).Fix(dict_item.m_dictValueSets);


    return dict_item;
}


void CDictItem::WriteJson(JsonWriter& json_writer) const
{
    auto dict_serializer_helper = json_writer.GetSerializerHelper().Get<DictionarySerializerHelper>();
    if( dict_serializer_helper != nullptr )
        dict_serializer_helper->SetCurrentItem(*this);

    json_writer.BeginObject();

    DictNamedBase::WriteJson(json_writer);

    if( json_writer.Verbose() || m_uOccurs > 1 )
    {
        json_writer.BeginObject(JK::occurrences);

        json_writer.Write(JK::maximum, m_uOccurs);

        if( m_uOccurs > 1 && ( json_writer.Verbose() || !m_occurrenceLabels.GetLabels().empty() ) )
        {
            json_writer.Key(JK::labels);
            m_occurrenceLabels.WriteJson(json_writer, m_uOccurs);
        }

        json_writer.EndObject();
    }

    json_writer.Write(JK::contentType, m_contentType);

    ASSERT(!IsSubitem() || m_contentType == ContentType::Numeric || m_contentType == ContentType::Alpha);

    if( json_writer.Verbose() || IsSubitem() )
        json_writer.Write(JK::subitem, IsSubitem());

    // when not in verbose mode, write out start positions for non-ID items only when using absolute positioning
    bool write_start_position = true;

    if( !json_writer.Verbose() && dict_serializer_helper != nullptr && dict_serializer_helper->GetDictionary().IsPosRelative() &&
        dict_serializer_helper->GetCurrentRecord() != nullptr && !dict_serializer_helper->GetCurrentRecord()->IsIdRecord() )
    {
        write_start_position = false;
    }

    if( write_start_position )
        json_writer.Write(JK::start, m_uStart);

    // for subitems, write the offset into the parent item
    if( IsSubitem() )
    {
        ASSERT(m_pParentItem != nullptr || dict_serializer_helper == nullptr);

        if( m_pParentItem != nullptr )
            json_writer.Write(JK::subitemOffset, m_uStart - m_pParentItem->m_uStart);            
    }

    json_writer.Write(JK::length, m_uLen);

    if( m_contentType == ContentType::Numeric )
    {
        if( json_writer.Verbose() || m_uDecimal > 0 )
            json_writer.Write(JK::decimals, m_uDecimal);

        if( m_uDecimal > 0 )
            json_writer.Write(JK::decimalMark, m_bDecChar);

        json_writer.Write(JK::zeroFill, m_bZeroFill);
    }

    if( json_writer.Verbose() || m_captureInfo.IsSpecified() )
        json_writer.Write(JK::capture, m_captureInfo);

    if( json_writer.Verbose() || !m_dictValueSets.empty() )
        json_writer.Write(JK::valueSets, m_dictValueSets);

    json_writer.EndObject();
}


void CDictItem::serialize(Serializer& ar)
{
    DictNamedBase::serialize(ar);

    if( ar.PredatesVersionIteration(Serializer::Iteration_8_0_000_1) )
        SetNote(ar.Read<CString>());

    ar.IgnoreUnusedVariable<CString>(Serializer::Iteration_8_0_000_1); // m_csError

    ar & m_uStart;
    ar & m_uLen;
    ar.SerializeEnum(m_contentType);
    ar.SerializeEnum(m_itemType);
    ar & m_uOccurs;
    ar & m_uDecimal;
    ar & m_bDecChar;
    ar & m_bZeroFill;

    if( ar.MeetsVersionIteration(Serializer::Iteration_7_7_000_1) )
        ar & m_captureInfo;

    ar & m_iSymbol;
    ar & m_iSonNumber;

    ar.IgnoreUnusedVariable<bool>(Serializer::Iteration_8_0_000_1); // m_bUsed
    ar.IgnoreUnusedVariable<int>(Serializer::Iteration_8_0_000_1); // m_iNumVSets

    ar & m_dictValueSets;

    if( ar.IsLoading() )
    {
        // fix numeric value sets
        ValueSetFixer(*this).Fix(m_dictValueSets);
    }

    ar & m_occurrenceLabels;
}
