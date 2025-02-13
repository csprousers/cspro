#pragma once

#include <zDictO/zDictO.h>
#include <zAppO/LabelSet.h>

class CSpecFile;


enum class DictElementType { Dictionary, Level, Record, Item, ValueSet, Value, Relation };


// a common parent class for dictionary elements that have labels

class CLASS_DECL_ZDICTO DictBase
{
public:
    DictBase() noexcept;
    DictBase(const DictBase& rhs) noexcept;
    DictBase(DictBase&& rhs) noexcept;

    virtual ~DictBase() { }

    virtual DictElementType GetElementType() const = 0;

    // label
    const CString& GetLabel() const     { return m_label.GetLabel(); }
    const LabelSet& GetLabelSet() const { return m_label; }
    LabelSet& GetLabelSet()             { return m_label; }
    void SetLabel(const CString& label) { m_label.SetLabel(label); }

    // note
    const CString& GetNote() const    { return m_note; }
    void SetNote(const CString& note) { m_note = note; }

protected:
    DictBase& operator=(const DictBase& rhs) noexcept;
    DictBase& operator=(DictBase&& rhs) noexcept;

    // serialization
    void ParseJsonInput(const JsonNode<wchar_t>& json_node);
    void WriteJson(JsonWriter& json_writer) const;

    void serialize(Serializer& ar);

protected:
    LabelSet m_label;
    CString m_note;
};
