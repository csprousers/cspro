#pragma once

#include <zDictO/zDictO.h>
#include <zDictO/DictNamedBase.h>


// --------------------------------------------------------------------------
// DictRelationPart
// --------------------------------------------------------------------------

class CLASS_DECL_ZDICTO DictRelationPart
{
public:
    const std::string& GetPrimaryLink() const     { return m_primaryLink; }
    bool IsPrimaryLinkedByOccurrence() const      { return m_primaryLink.empty(); }
    void SetPrimaryLink(std::string primary_link) { m_primaryLink = std::move(primary_link); }

    const std::string& GetSecondaryName() const       { return m_secondaryName; }
    void SetSecondaryName(std::string secondary_name) { m_secondaryName = std::move(secondary_name); }

    const std::string& GetSecondaryLink() const       { return m_secondaryLink; }
    bool IsSecondaryLinkedByOccurrence() const        { return m_secondaryLink.empty(); }
    void SetSecondaryLink(std::string secondary_link) { m_secondaryLink = std::move(secondary_link); }


    // serialization
    // ------------------------------
    static DictRelationPart CreateFromJson(const JsonNode& json_node);
    void WriteJson(JsonWriter& json_writer) const;

    void serialize(Serializer& ar);


private:
    std::string m_primaryLink;   // primary linking item name (empty if by occurrence)
    std::string m_secondaryName; // secondary record or item name
    std::string m_secondaryLink; // secondary linking item name (empty if by occurrence)
};



// --------------------------------------------------------------------------
// DictRelation
// --------------------------------------------------------------------------

class CLASS_DECL_ZDICTO DictRelation : public DictNamedBase
{
public:
    std::unique_ptr<DictNamedBase> Clone() const override { return std::make_unique<DictRelation>(*this); }

    DictElementType GetElementType() const override { return DictElementType::Relation; }

    const std::string& GetPrimaryName() const     { return m_primaryName; }
    void SetPrimaryName(std::string primary_name) { m_primaryName = std::move(primary_name); }

    const std::vector<DictRelationPart>& GetRelationParts() const { return m_dictRelationParts; }

    const DictRelationPart& GetRelationPart(size_t index) const { ASSERT(index < m_dictRelationParts.size()); return m_dictRelationParts[index]; }
    DictRelationPart& GetRelationPart(size_t index)             { ASSERT(index < m_dictRelationParts.size()); return m_dictRelationParts[index]; }

    void AddRelationPart(DictRelationPart dict_relation_part);
    void InsertRelationPart(size_t index, DictRelationPart dict_relation_part);
    void RemoveRelationPart(size_t index);

    // returns the logic to declare this relation in logic
    std::string GenerateCode(const std::string& dictionary_name) const;


    // serialization
    static DictRelation CreateFromJson(const JsonNode& json_node);
    void WriteJson(JsonWriter& json_writer) const;

    void serialize(Serializer& ar);


private:
    std::string m_primaryName; // primary record or item name
    std::vector<DictRelationPart> m_dictRelationParts;
};
