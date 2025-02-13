#include "stdafx.h"
#include "QuestionnaireContentCreator.h"
#include "TempFormFileSerializer.h"
#include <zHtml/AccessUrlSerializer.h>
#include <zHtml/PortableLocalhost.h>
#include <zAppO/Properties/JsonProperties.h>
#include <zFormO/DragOptions.h>
#include <zCaseO/CaseBinaryDataVirtualFileMappingHandler.h>
#include <zCaseO/CaseJsonSerializer.h>


CREATE_JSON_KEY(writeFieldStatuses)


QuestionnaireContentCreator::QuestionnaireContentCreator()
    :   m_binaryDataUseLocalhostUrl(JsonProperties::DefaultBinaryDataFormat == JsonProperties::BinaryDataFormat::LocalhostUrl)
{
}


QuestionnaireContentCreator::~QuestionnaireContentCreator()
{
}


void QuestionnaireContentCreator::ResetInputs()
{
    m_dictionary.reset();
    m_formFile.reset();
    m_capiQuestionManager.reset();
    m_case.reset();
}


bool QuestionnaireContentCreator::DictionaryMatches(const CDataDict* const compare_dictionary) const
{
    ASSERT(m_dictionary != nullptr && !m_dictionary->GetFullFileName().IsEmpty());

    if( compare_dictionary != nullptr )
    {
        if( m_dictionary.get() == compare_dictionary ||
            SO::EqualsNoCase(m_dictionary->GetFullFileName(), compare_dictionary->GetFullFileName()) ||
            m_dictionary->GetStructureMd5() == compare_dictionary->GetStructureMd5() )
        {
            return true;
        }
    }

    return false;
}


std::wstring QuestionnaireContentCreator::GetContent()
{
    if( m_dictionary == nullptr )
        throw CSProException("A dictionary must be specified to generate content.");

    std::shared_ptr<const CDEFormFile> form_file = m_formFile;

    // make sure the form file matches the dictionary...
    if( form_file != nullptr )
    {
        if( !DictionaryMatches(form_file->GetDictionary()) )
        {
            throw CSProException(_T("The form file dictionary, '%s', does not match the dictionary '%s'."),
                                 ( form_file->GetDictionary() != nullptr ) ? form_file->GetDictionary()->GetName().GetString() : _T(""),
                                 m_dictionary->GetName().GetString());
        }
    }

    // ... or create a fake form file based on the dictionary
    else
    {
        form_file = CreateDummyFormFile();
    }

    // make sure the case matches the dictionary
    if( m_case != nullptr && !DictionaryMatches(&m_case->GetCaseMetadata().GetDictionary()) )
    {
        throw CSProException(_T("The case dictionary, '%s', does not match the dictionary '%s'."),
                             m_case->GetCaseMetadata().GetDictionary().GetName().GetString(),
                             m_dictionary->GetName().GetString());
    }


    // create the JSON content
    auto json_writer = Json::CreateStringWriter();
    json_writer->SetVerbose();

    // create access URLs for things like value set images
    auto access_url_serializer_helper_holder = json_writer->GetSerializerHelper().Register(std::make_unique<AccessUrl::SerializerHelper>());

    json_writer->BeginObject();

    // write the dictionary
    json_writer->Write(JK::dictionary, *m_dictionary);

    // write the forms
    json_writer->Write(JK::forms, *form_file);

    // optionally write the question text
    if( m_capiQuestionManager != nullptr )
        json_writer->Write(JK::questionText, *m_capiQuestionManager);

    // optionally write the contents of a case
    if( m_case != nullptr )
    {
        auto case_json_writer_serializer_holder = json_writer->GetSerializerHelper().Register(GetCaseJsonWriterSerializerHelper());

        json_writer->Key(JK::case_);
        m_case->WriteJson(*json_writer);
    }

    json_writer->EndObject();

    return json_writer->GetString();
}


std::wstring QuestionnaireContentCreator::GetCaseContent()
{
    if( m_dictionary == nullptr || m_case == nullptr )
        throw CSProException("A dictionary and case must be specified to generate content.");

    ASSERT(DictionaryMatches(&m_case->GetCaseMetadata().GetDictionary()));

    // create the JSON content
    auto json_writer = Json::CreateStringWriter();
    json_writer->SetVerbose();

    auto case_json_writer_serializer_holder = json_writer->GetSerializerHelper().Register(GetCaseJsonWriterSerializerHelper());

    m_case->WriteJson(*json_writer);

    return json_writer->GetString();
}


std::unique_ptr<CDEFormFile> QuestionnaireContentCreator::CreateDummyFormFile() const
{
    ASSERT(m_dictionary != nullptr);

    auto form_file = std::make_unique<CDEFormFile>();

    form_file->SetDictionary(m_dictionary);

    // create the form file, using the default drag options
    const DragOptions drag_options;
    constexpr int DropSpacing = 500;
    const CSize single_character_text_extent(8, 16);

    // VQ_TODO: use this method, or create a new version that allows items and subitems on a roster?
    form_file->CreateFormFile(m_dictionary.get(), single_character_text_extent, drag_options, DropSpacing, true);
    form_file->SetName(m_dictionary->GetName());
    form_file->SetLabel(m_dictionary->GetLabel());
    form_file->UpdatePointers();

    return form_file;
}


void QuestionnaireContentCreator::SetSerializationOptions(const JsonNode<wchar_t>& serialization_options_node)
{
    m_caseJsonWriterSerializerHelper.reset();

    m_writeLabels = serialization_options_node.GetOptional<bool>(JK::writeLabels);
    m_writeFieldStatuses = serialization_options_node.GetOptional<bool>(JK::writeFieldStatuses);

    const JsonProperties json_properties = JsonProperties::CreateFromJson(serialization_options_node);
    m_binaryDataUseLocalhostUrl = ( json_properties.GetBinaryDataFormat() == JsonProperties::BinaryDataFormat::LocalhostUrl );
}


void QuestionnaireContentCreator::SetFieldStatusRetriever(std::shared_ptr<FieldStatusRetriever> function)
{
    m_caseJsonWriterSerializerHelper.reset();

    m_fieldStatusRetriever = std::move(function);
}


std::shared_ptr<CaseJsonWriterSerializerHelper> QuestionnaireContentCreator::GetCaseJsonWriterSerializerHelper()
{
    ASSERT(m_case != nullptr);

    if( m_caseJsonWriterSerializerHelper == nullptr )
    {
        m_caseJsonWriterSerializerHelper = std::make_shared<CaseJsonWriterSerializerHelper>();

        if( m_writeLabels.has_value() )
            m_caseJsonWriterSerializerHelper->SetWriteLabels(*m_writeLabels);

        // when possible, serialize the field status with the case contents
        if( m_writeFieldStatuses != false && m_fieldStatusRetriever != nullptr )
            m_caseJsonWriterSerializerHelper->SetFieldStatusRetriever(m_fieldStatusRetriever);

        // CaseJsonWriter::WriteBinaryCaseItem writes binary data using data URLs if not given a custom binary data writer;
        // by default this class (QuestionnaireContentCreator) will write binary data using the local web server
        if( m_binaryDataUseLocalhostUrl && m_case->GetCaseMetadata().UsesBinaryData() )
        {
            if( m_caseBinaryDataVirtualFileMappingHandler == nullptr )
            {
                m_caseBinaryDataVirtualFileMappingHandler = std::make_unique<CaseBinaryDataVirtualFileMappingHandler>();
                PortableLocalhost::CreateVirtualDirectory(*m_caseBinaryDataVirtualFileMappingHandler);
            }

            m_caseBinaryDataVirtualFileMappingHandler->SetCase(m_case);

            m_caseJsonWriterSerializerHelper->SetBinaryDataWriter(
                [&](JsonWriter& json_writer, const BinaryCaseItem& binary_case_item, const CaseItemIndex& index)
                {
                    const std::wstring access_key = index.GetSerializableText(binary_case_item);
                    json_writer.Write(JK::url, m_caseBinaryDataVirtualFileMappingHandler->CreateUrl(access_key));
                });
        }
    }

    return m_caseJsonWriterSerializerHelper;
}
