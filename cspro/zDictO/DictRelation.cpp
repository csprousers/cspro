#include "StdAfx.h"
#include "DictRelation.h"


// --------------------------------------------------------------------------
// (old notes)
// --------------------------------------------------------------------------

    // Object = Item/Record/Subitem
    // Object will be represented by its long name as <Level Name>.<Record Name>[.<Item/Sub-Item name>]

    // GSF NEW RELATION

    // Notes:
        // m_csObject_1 is in DictRelation
        // m_csObject is required (cannot be null string)
        // each side must link either by variable (item) or by occurrence number
        // if m_bOcc is true, means linking by occurrence number of the Item/rec whichever it is

        // Cases
        //
        // 1.  item, item
        // RELATION Object_1 to Object_2 where m_csItem_1 = m_csItem_2
        //
        // 2.  item, occ
        // RELATION Object_1 to Object_2 linked by m_csItem_1
        //
        // 3.  occ, item
        // RELATION Object_1 to Object_2 linked by m_csItem_2
        //
        // 4.  occ, occ
        // RELATION Object_1 to Object_2 parallel



// --------------------------------------------------------------------------
// DictRelationPart
// --------------------------------------------------------------------------

namespace
{
    constexpr wstring_view OccurrenceText = _T("(occurrence)");
}


DictRelationPart DictRelationPart::CreateFromJson(const JsonNode<wchar_t>& json_node)
{
    DictRelationPart dict_relation_part;

    auto set_link = [&](std::wstring& link, const TCHAR* key)
    {
        link = json_node.GetOrDefault(key, SO::EmptyString);

        if( link == OccurrenceText )
            link.clear();
    };

    set_link(dict_relation_part.m_primaryLink, JK::primaryLink);
    dict_relation_part.m_secondaryName = json_node.Get<std::wstring>(JK::secondary);
    set_link(dict_relation_part.m_secondaryLink, JK::secondaryLink);

    return dict_relation_part;
}


void DictRelationPart::WriteJson(JsonWriter& json_writer) const
{
    json_writer.BeginObject()
               .Write(JK::primaryLink, m_primaryLink.empty() ? OccurrenceText : wstring_view(m_primaryLink))
               .Write(JK::secondary, m_secondaryName)
               .Write(JK::secondaryLink, m_secondaryLink.empty() ? OccurrenceText : wstring_view(m_secondaryLink))
               .EndObject();
}


void DictRelationPart::serialize(Serializer& ar)
{
    ar & m_primaryLink;
    ar & m_secondaryName;
    ar & m_secondaryLink;
}


// --------------------------------------------------------------------------
// DictRelation
// --------------------------------------------------------------------------

void DictRelation::AddRelationPart(DictRelationPart dict_relation_part)
{
    m_dictRelationParts.emplace_back(std::move(dict_relation_part));
}

void DictRelation::InsertRelationPart(size_t index, DictRelationPart dict_relation_part)
{
    ASSERT(index <= m_dictRelationParts.size());
    m_dictRelationParts.insert(m_dictRelationParts.begin() + index, std::move(dict_relation_part));
}

void DictRelation::RemoveRelationPart(size_t index)
{
    ASSERT(index < m_dictRelationParts.size());
    m_dictRelationParts.erase(m_dictRelationParts.begin() + index);
}


std::wstring DictRelation::GenerateCode(const std::wstring& dictionary_name) const
{
    // Relation RelName MultVar/Sec [  TO  MultVar/Sec  PARALLEL
    //                                                  LINKED BY exprlog
    //                                                  WHERE  exprlog [MULTIPLE]
    //                                    [
    //                                                    ......
    //                                                    ......
    //                                     TO  MultVar/Sec  PARALLEL
    //                                                      LINKED BY exprlog
    //                                                      WHERE  exprlog [MULTIPLE]
    //                                    ]
    //                                ]

    // 20140308 a relation wouldn't compile in a tabulation application because of the problem with the
    // infamous "ambiguous symbol" problem mixing dictionary records with _EDT groups; so we'll use
    // dictionary dot notation to make things extra clear
    std::wstring code = FormatTextCS2WS(_T("RELATION %s %s.%s"), GetName().GetString(), dictionary_name.c_str(), m_primaryName.c_str());

    for( const DictRelationPart& dict_relation_part : m_dictRelationParts )
    {
        SO::AppendFormat(code, _T(" TO %s.%s "), dictionary_name.c_str(), dict_relation_part.GetSecondaryName().c_str());

        // one link
        if( dict_relation_part.IsPrimaryLinkedByOccurrence() != dict_relation_part.IsSecondaryLinkedByOccurrence() )
        {
            SO::AppendFormat(code, _T("LINKED BY %s.%s"), dictionary_name.c_str(),
                                    dict_relation_part.IsPrimaryLinkedByOccurrence() ? dict_relation_part.GetSecondaryLink().c_str() :
                                                                                       dict_relation_part.GetPrimaryLink().c_str());
        }

        // both links
        else if( !dict_relation_part.IsPrimaryLinkedByOccurrence() )
        {
            SO::AppendFormat(code, _T("WHERE %s.%s = %s.%s"),
                                   dictionary_name.c_str(), dict_relation_part.GetPrimaryLink().c_str(),
                                   dictionary_name.c_str(), dict_relation_part.GetSecondaryLink().c_str());
        }

        // no links (both by occurrence)
        else
        {
            code.append(_T("PARALLEL"));
        }
    }

    code.push_back(';');

    return code;
}


DictRelation DictRelation::CreateFromJson(const JsonNode<wchar_t>& json_node)
{
    DictRelation dict_relation;

    dict_relation.DictNamedBase::ParseJsonInput(json_node, false);

    dict_relation.m_primaryName = json_node.Get<std::wstring>(JK::primary);

    dict_relation.m_dictRelationParts = json_node.GetArrayOrEmpty(JK::links).GetVector<DictRelationPart>(
        [&](const JsonParseException& exception)
        {
            json_node.LogWarning(_T("A relation link was not added to '%s' due to errors: %s"),
                                 dict_relation.GetName().GetString(), exception.GetErrorMessage().c_str());
        });

    return dict_relation;
}


void DictRelation::WriteJson(JsonWriter& json_writer) const
{
    json_writer.BeginObject();

    DictNamedBase::WriteJson(json_writer, false);

    json_writer.Write(JK::primary, m_primaryName);
    json_writer.Write(JK::links, m_dictRelationParts);

    json_writer.EndObject();
}


void DictRelation::serialize(Serializer& ar)
{
    if( ar.PredatesVersionIteration(Serializer::Iteration_8_0_000_1) )
    {
        SetName(ar.Read<CString>());
    }

    else
    {
        DictNamedBase::serialize(ar);
    }

    ar & m_primaryName;

    ar.IgnoreUnusedVariable<int>(Serializer::Iteration_8_0_000_1); // m_iNumRelationParts

    ar & m_dictRelationParts;
}
