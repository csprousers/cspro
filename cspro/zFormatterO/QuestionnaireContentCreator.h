#pragma once

#include <zFormatterO/zFormatterO.h>
#include <zAppO/FieldStatus.h>

class CapiQuestionManager;
class Case;
class CaseBinaryDataVirtualFileMappingHandler;
class CaseJsonWriterSerializerHelper;
class CDataDict;
class CDEFormFile;


class ZFORMATTERO_API QuestionnaireContentCreator
{
public:
    QuestionnaireContentCreator();
    ~QuestionnaireContentCreator();

    void SetDictionary(std::shared_ptr<const CDataDict> dictionary)                               { m_dictionary = std::move(dictionary); }
    void SetFormFile(std::shared_ptr<const CDEFormFile> form_file)                                { m_formFile = std::move(form_file); }
    void SetCapiQuestionManager(std::shared_ptr<const CapiQuestionManager> capi_question_manager) { m_capiQuestionManager = std::move(capi_question_manager); }
    void SetCase(std::shared_ptr<const Case> data_case)                                           { m_case = std::move(data_case); }

    // resets the inputs (in anticipation of setting them up again)
    void ResetInputs();

    // sets the serialization options (throws exceptions on errors)
    void SetSerializationOptions(const JsonNode<wchar_t>& serialization_options_node);

    // sets a field status retriever
    void SetFieldStatusRetriever(std::shared_ptr<FieldStatusRetriever> function);

    // returns all available content in JSON format
    std::wstring GetContent();

    // returns the case content in JSON format
    std::wstring GetCaseContent();

    // returns the virtual file mapping handler that serves case binary data
    std::shared_ptr<CaseBinaryDataVirtualFileMappingHandler> GetCaseBinaryDataVirtualFileMappingHandler() { return m_caseBinaryDataVirtualFileMappingHandler; }

private:
    bool DictionaryMatches(const CDataDict* compare_dictionary) const;

    std::unique_ptr<CDEFormFile> CreateDummyFormFile() const;

    std::shared_ptr<CaseJsonWriterSerializerHelper> GetCaseJsonWriterSerializerHelper();

private:
    std::shared_ptr<const CDataDict> m_dictionary;
    std::shared_ptr<const CDEFormFile> m_formFile;
    std::shared_ptr<const CapiQuestionManager> m_capiQuestionManager;
    std::shared_ptr<const Case> m_case;

    std::optional<bool> m_writeLabels;
    std::optional<bool> m_writeFieldStatuses;
    bool m_binaryDataUseLocalhostUrl;
    std::shared_ptr<FieldStatusRetriever> m_fieldStatusRetriever;

    std::shared_ptr<CaseJsonWriterSerializerHelper> m_caseJsonWriterSerializerHelper;
    std::shared_ptr<CaseBinaryDataVirtualFileMappingHandler> m_caseBinaryDataVirtualFileMappingHandler;
};
