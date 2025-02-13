#pragma once

#include <zJson/Json.h>
#include <zJson/JsonSerializer.h>
#include <zFormO/FormFile.h>
#include <zFormO/Roster.h>


template<>
struct JsonSerializer<CDEFormFile>
{
    static void WriteJson(JsonWriter& json_writer, const CDEFormFile& form_file);
};


struct RestructuredGroupItems
{
    const CDEItemBase& form_item_base;
    std::vector<RestructuredGroupItems> children;
};


class TempFormFileSerializer
{
public:
    TempFormFileSerializer(JsonWriter& json_writer, const CDEFormFile& form_file)
        :   m_jsonWriter(json_writer),
            m_formFile(form_file)
    {
        ASSERT(m_formFile.GetDictionary() != nullptr);
    }

    void WriteFormFile();

private:
    void WriteFormLevel(const CDELevel& form_level);
    void WriteFormGroup(const CDEGroup& form_group, bool wrap_in_object_with_name_label_type = true);
    void WriteFormRoster(const CDERoster& form_roster);
    void WriteFormBlock(const CDEBlock& form_block, const std::vector<RestructuredGroupItems>& items);
    void WriteFormField(const CDEField& form_field);

    void WriteRestructuredGroupItems(const std::vector<RestructuredGroupItems>& items);

    std::vector<RestructuredGroupItems> RestructureGroup(const CDEGroup& form_group);

private:
    JsonWriter& m_jsonWriter;
    const CDEFormFile& m_formFile;
};



inline void JsonSerializer<CDEFormFile>::WriteJson(JsonWriter& json_writer, const CDEFormFile& form_file)
{
    TempFormFileSerializer(json_writer, form_file).WriteFormFile();
}


inline void TempFormFileSerializer::WriteFormFile()
{
    m_jsonWriter.BeginObject();

    m_jsonWriter.Write(JK::name, m_formFile.GetName());
    m_jsonWriter.Write(JK::label, m_formFile.GetLabel());

    m_jsonWriter.BeginObject(JK::dictionary)
                .Write(JK::name, m_formFile.GetDictionary()->GetName())
                .WriteRelativePath(JK::path, CS2WS(m_formFile.GetDictionaryFilename()))
                .EndObject();


    m_jsonWriter.BeginArray(JK::levels);

    for( int level = 0; level < m_formFile.GetNumLevels(); ++level )
    {
        const CDELevel* form_level = m_formFile.GetLevel(level);
        WriteFormLevel(*form_level);
    }

    m_jsonWriter.EndArray();

    m_jsonWriter.EndObject();
}


inline void TempFormFileSerializer::WriteFormLevel(const CDELevel& form_level)
{
    m_jsonWriter.BeginObject();

    m_jsonWriter.Write(JK::name, form_level.GetName());
    m_jsonWriter.Write(JK::label, form_level.GetLabel());

    m_jsonWriter.BeginArray(_T("items"));

    for( int group = 0; group < form_level.GetNumGroups(); ++group )
    {
        const CDEGroup* form_group = form_level.GetGroup(group);
        WriteFormGroup(*form_group);
    }

    m_jsonWriter.EndArray();

    m_jsonWriter.EndObject();
}


inline std::vector<RestructuredGroupItems> TempFormFileSerializer::RestructureGroup(const CDEGroup& form_group)
{
    // embed a block's fields as children
    std::vector<RestructuredGroupItems> items;

    for( int i = 0; i < form_group.GetNumItems(); ++i )
    {
        const CDEItemBase& form_item_base = *form_group.GetItem(i);

        items.emplace_back(RestructuredGroupItems { form_item_base, { } });

        if( form_item_base.isA(CDEFormBase::eItemType::Block) )
        {
            const CDEBlock& form_block = assert_cast<const CDEBlock&>(form_item_base);

            for( int j = form_block.GetNumFields(); j > 0; --j )
            {
                ++i;
                items.back().children.emplace_back(RestructuredGroupItems { *form_group.GetItem(i), { } });
                ASSERT(dynamic_cast<const CDEField*>(&items.back().children.back().form_item_base) != nullptr);
            }
        }
    }

    return items;
}


inline void TempFormFileSerializer::WriteFormGroup(const CDEGroup& form_group, bool wrap_in_object_with_name_label_type/* = true*/)
{
    if( wrap_in_object_with_name_label_type )
    {
        m_jsonWriter.BeginObject();

        m_jsonWriter.Write(JK::name, form_group.GetName());
        m_jsonWriter.Write(JK::label, form_group.GetLabel());
        m_jsonWriter.Write(JK::type, _T("group"));
    }

    WriteRestructuredGroupItems(RestructureGroup(form_group));   

    if( wrap_in_object_with_name_label_type )
    {
        m_jsonWriter.EndObject();
    }
}


inline void TempFormFileSerializer::WriteRestructuredGroupItems(const std::vector<RestructuredGroupItems>& items)
{
    m_jsonWriter.BeginArray(JK::items);

    for( const RestructuredGroupItems& rgi : items )
    {
        if( rgi.form_item_base.isA(CDEFormBase::eItemType::Group) )
        {
            WriteFormGroup(assert_cast<const CDEGroup&>(rgi.form_item_base));
        }

        else if( rgi.form_item_base.isA(CDEFormBase::eItemType::Roster) )
        {
            WriteFormRoster(assert_cast<const CDERoster&>(rgi.form_item_base));
        }

        else if( rgi.form_item_base.isA(CDEFormBase::eItemType::Block) )
        {
            WriteFormBlock(assert_cast<const CDEBlock&>(rgi.form_item_base), rgi.children);
        }

        else if( rgi.form_item_base.isA(CDEFormBase::eItemType::Field) )
        {
            WriteFormField(assert_cast<const CDEField&>(rgi.form_item_base));
        }

        else
        {
            ASSERT(false);
        }
    }

    m_jsonWriter.EndArray();
}


inline void TempFormFileSerializer::WriteFormRoster(const CDERoster& form_roster)
{
    m_jsonWriter.BeginObject();

    m_jsonWriter.Write(JK::name, form_roster.GetName());
    m_jsonWriter.Write(JK::label, form_roster.GetLabel());
    m_jsonWriter.Write(JK::type, _T("roster"));
    m_jsonWriter.Write(_T("orientation"), ( form_roster.GetOrientation() == RosterOrientation::Horizontal ) ? _T("horizontal") : _T("vertical"));

    WriteFormGroup(form_roster, false);

    m_jsonWriter.EndObject();
}


inline void TempFormFileSerializer::WriteFormBlock(const CDEBlock& form_block, const std::vector<RestructuredGroupItems>& items)
{
    m_jsonWriter.BeginObject();

    m_jsonWriter.Write(JK::name, form_block.GetName());
    m_jsonWriter.Write(JK::label, form_block.GetLabel());
    m_jsonWriter.Write(JK::type, _T("block"));

    if( m_jsonWriter.Verbose() || !items.empty() )
        WriteRestructuredGroupItems(items);

    m_jsonWriter.EndObject();
}


inline void TempFormFileSerializer::WriteFormField(const CDEField& form_field)
{
    m_jsonWriter.BeginObject();

    m_jsonWriter.Write(JK::dictionary, m_formFile.GetDictionary()->GetName());
    m_jsonWriter.Write(JK::name, form_field.GetName());

    CString label =
        ( form_field.GetDictItem() == nullptr || form_field.GetFieldLabelType() == FieldLabelType::Custom ) ? form_field.GetLabel() :
        ( form_field.GetFieldLabelType() == FieldLabelType::DictionaryName )                                ? form_field.GetDictItem()->GetLabel() :
                                                                                                              form_field.GetDictItem()->GetName();

    if( label.IsEmpty() && form_field.GetDictItem() != nullptr ) // investigate why
        label = form_field.GetDictItem()->GetLabel();

    m_jsonWriter.Write(JK::type, _T("field"));

    m_jsonWriter.Write(_T("protected"), form_field.IsProtected());
    m_jsonWriter.Write(_T("hideInCaseTree"), form_field.IsHiddenInCaseTree());
    m_jsonWriter.Write(_T("mirror"), form_field.IsMirror());

    m_jsonWriter.Write(JK::capture, form_field.GetEvaluatedCaptureInfo());

    if( form_field.GetDictItem() == nullptr )
    {
        ASSERT(false);
    }

    else if( form_field.GetDictItem()->GetContentType() == ContentType::Alpha )
    {
        m_jsonWriter.Write(_T("tickmarks"), !form_field.UseUnicodeTextBox());
        m_jsonWriter.Write(_T("multiline"), form_field.AllowMultiLine());
    }

    m_jsonWriter.EndObject();
}
