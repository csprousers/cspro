#include "stdafx.h"
#include <zDictO/DDClass.h>
#include <zCaseO/Case.h>
#include <zCaseO/CaseBinaryDataVirtualFileMappingHandler.h>
#include <zFormatterO/QuestionnaireContentCreator.h>


ActionInvoker::Result ActionInvoker::Runtime::GetQuestionnaireContentWithCaseData(QuestionnaireContentCreator& questionnaire_content_creator, std::unique_ptr<Case> data_case,
                                                                                  const JsonNode<wchar_t>& json_node, const bool write_all_content, const bool case_content_is_from_current_case)
{
    ASSERT(data_case != nullptr);

    questionnaire_content_creator.SetCase(std::move(data_case));

    if( json_node.Contains(JK::serializationOptions) )
        questionnaire_content_creator.SetSerializationOptions(json_node.Get(JK::serializationOptions));

    if( case_content_is_from_current_case )
    {
        try
        {
            questionnaire_content_creator.SetFieldStatusRetriever(GetInterpreterAccessor().CreateFieldStatusRetriever());
        }
        catch(...) { }
    }

    std::wstring content = write_all_content ? questionnaire_content_creator.GetContent() :
                                               questionnaire_content_creator.GetCaseContent();

    // QuestionnaireContentCreator may write out the binary data in a case using a virtual file mapping handler;
    // if so add it to the Action Invoker's handlers
    std::shared_ptr<CaseBinaryDataVirtualFileMappingHandler> case_binary_data_virtual_file_mapping_handler = questionnaire_content_creator.GetCaseBinaryDataVirtualFileMappingHandler();

    if( case_binary_data_virtual_file_mapping_handler != nullptr )
        m_localHostKeyBasedVirtualFileMappingHandlers.emplace_back(std::move(case_binary_data_virtual_file_mapping_handler));

    return Result::JsonText(std::move(content));
}


ActionInvoker::Result ActionInvoker::Runtime::Data_getCase(const JsonNode<wchar_t>& json_node, Caller& /*caller*/)
{
    std::shared_ptr<const CDataDict> dictionary = std::get<2>(GetApplicationComponents<std::shared_ptr<const CDataDict>>(json_node.GetOptional<wstring_view>(JK::name)));
    ASSERT(dictionary != nullptr);

    std::unique_ptr<Case> data_case;
    bool case_content_is_from_current_case;

    // get content for a specific case...
    if( json_node.Contains(JK::key) || json_node.Contains(JK::uuid) )
    {
        data_case = GetInterpreterAccessor().GetCase(CS2WS(dictionary->GetName()),
                                                     json_node.GetOptional<std::wstring>(JK::uuid),
                                                     json_node.GetOptional<std::wstring>(JK::key));
        case_content_is_from_current_case = false;
    }

    // ...or the current case
    else
    {
        data_case = GetInterpreterAccessor().GetCurrentCase(CS2WS(dictionary->GetName()));
        case_content_is_from_current_case = true;
    }

    ASSERT(data_case != nullptr);

    QuestionnaireContentCreator questionnaire_content_creator;
    questionnaire_content_creator.SetDictionary(std::move(dictionary));

    return GetQuestionnaireContentWithCaseData(questionnaire_content_creator, std::move(data_case), json_node, false, case_content_is_from_current_case);
}
