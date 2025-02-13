#include "stdafx.h"
#include <zAppO/PFF.h>
#include <zFormO/FormFile.h>
#include <zCapiO/CapiQuestionManager.h>
#include <zCaseO/Case.h>
#include <zFormatterO/QuestionnaireViewer.h>
#include <zFormatterO/TempFormFileSerializer.h>
#include <zDesignerF/UWM.h>


namespace
{
    constexpr const char* NoApplicationMessage                            = "No application exists in the current environment.";
    constexpr const TCHAR* NoApplicationComponentMessageWithNameFormatter = _T("No application component named '%s' exists.");
    constexpr const char* NoApplicationComponentMessage                   = "No applicable application component exists.";
    constexpr const char* NoPffMessage                                    = "No PFF exists in the current environment.";
}


const Application* ActionInvoker::Runtime::GetApplication(const bool throw_exception_if_does_not_exist)
{
    const PFF* pff = GetPff(false);
    const Application* application = ( pff != nullptr ) ? pff->GetApplication() :
                                                          nullptr;

    if( application != nullptr )
        return application;

    // while in the Designer, try to get the current application
    if( WindowsDesktopMessage::Send(UWM::Designer::GetApplication, &application) == 1 )
        return application;

    if( throw_exception_if_does_not_exist )
        throw CSProException(NoApplicationMessage);

    return nullptr;
}


const PFF* ActionInvoker::Runtime::GetPff(const bool throw_exception_if_does_not_exist)
{
    try
    {
        return &GetInterpreterAccessor().GetPff();

    }
    catch(...) { }

    if( throw_exception_if_does_not_exist )
        throw CSProException(NoPffMessage);

    return nullptr;
}


template<typename RequiredComponentType>
std::tuple<const Application*,
           std::shared_ptr<const CDEFormFile>,
           std::shared_ptr<const CDataDict>> ActionInvoker::Runtime::GetApplicationComponents(const std::optional<wstring_view>& name_sv)
{
    const Application* application = GetApplication(false);
    std::shared_ptr<const CDEFormFile> matched_form_file;
    std::shared_ptr<const CDataDict> matched_dictionary;
    bool matched_by_name = false;

    auto form_file_or_its_dictionary_name_matches = [&](const CDEFormFile& form_file)
    {
        ASSERT(name_sv.has_value());

        return ( ( SO::EqualsNoCase(*name_sv, form_file.GetName()) ) ||
                 ( form_file.GetDictionary() != nullptr && SO::EqualsNoCase(*name_sv, form_file.GetDictionary()->GetName()) ) );
    };

    // when an application exists...
    if( application != nullptr )
    {
        // ...and no name is specified, or the application name matches, use the primary form file
        if( !name_sv.has_value() || SO::EqualsNoCase(*name_sv, application->GetName()) )
        {
            matched_by_name = name_sv.has_value();

            if( !application->GetRuntimeFormFiles().empty() )
                matched_form_file = application->GetRuntimeFormFiles().front();            
        }

        // ...or search by form file or dictionary name
        else
        {
            // look at form files and their dictionaries
            for( const std::shared_ptr<const CDEFormFile>& form_file : application->GetRuntimeFormFiles() )
            {
                // the form file matches, or the form file's dictionary matches
                if( form_file_or_its_dictionary_name_matches(*form_file) )
                {
                    matched_by_name = true;
                    matched_form_file = form_file;
                    break;
                }
            }

            // look at external dictionaries
            if( matched_form_file == nullptr )
            {
                for( const std::shared_ptr<const CDataDict>& dictionary : application->GetRuntimeExternalDictionaries() )
                {
                    // an external dictionary matches
                    if( SO::EqualsNoCase(*name_sv, dictionary->GetName()) )
                    {
                        matched_by_name = true;
                        matched_dictionary = dictionary;
                        break;
                    }
                }
            }
        }
    }

    // when there is no application, but while in the Designer, try to get the current form file or dictionary
    else
    {
        std::variant<std::monostate, std::shared_ptr<const CDEFormFile>, std::shared_ptr<const CDataDict>> form_file_or_dictionary;

        if( WindowsDesktopMessage::Send(UWM::Designer::GetFormFileOrDictionary, &form_file_or_dictionary) == 1 )
        {
            if( std::holds_alternative<std::shared_ptr<const CDEFormFile>>(form_file_or_dictionary) )
            {
                matched_form_file = std::get<std::shared_ptr<const CDEFormFile>>(form_file_or_dictionary);
                ASSERT(matched_form_file != nullptr);

                if( name_sv.has_value() )
                    matched_by_name = form_file_or_its_dictionary_name_matches(*matched_form_file);
            }

            else
            {
                ASSERT(std::holds_alternative<std::shared_ptr<const CDataDict>>(form_file_or_dictionary));

                matched_dictionary = std::get<std::shared_ptr<const CDataDict>>(form_file_or_dictionary);
                ASSERT(matched_dictionary != nullptr);

                if( name_sv.has_value() )
                    matched_by_name = SO::EqualsNoCase(*name_sv, matched_dictionary->GetName());
            }
        }
    }

    // when the form file is matched, get the dictionary from it
    if( matched_dictionary == nullptr && matched_form_file != nullptr )
        matched_dictionary = matched_form_file->GetSharedDictionary();

    std::tuple<const Application*, std::shared_ptr<const CDEFormFile>, std::shared_ptr<const CDataDict>> application_components(application, std::move(matched_form_file), std::move(matched_dictionary));

    if( std::get<RequiredComponentType>(application_components) != nullptr && ( matched_by_name || !name_sv.has_value() ) )
    {
        return application_components;
    }

    else if( name_sv.has_value() )
    {
        throw CSProException(NoApplicationComponentMessageWithNameFormatter, std::wstring(*name_sv).c_str());
    }

    else
    {
        throw CSProException(NoApplicationComponentMessage);
    }
}

template std::tuple<const Application*,
                    std::shared_ptr<const CDEFormFile>,
                    std::shared_ptr<const CDataDict>> ActionInvoker::Runtime::GetApplicationComponents<std::shared_ptr<const CDataDict>>(const std::optional<wstring_view>& name_sv);


ActionInvoker::Result ActionInvoker::Runtime::Application_getFormFile(const JsonNode<wchar_t>& json_node, Caller& caller)
{
    std::shared_ptr<const CDEFormFile> form_file;

    if( json_node.Contains(JK::path) )
    {
        const std::wstring path = caller.EvaluateAbsolutePath(json_node.Get<std::wstring>(JK::path));

        auto form_file_on_disk = std::make_unique<CDEFormFile>();

        if( !form_file_on_disk->Open(WS2CS(path), true) )
            throw CSProException(_T("The form file could not be opened: %s"), path.c_str());

        // connect the form file with its dictionary, first seeing if it is part of the application
        ASSERT(form_file_on_disk->GetDictionary() == nullptr);

        std::shared_ptr<const CDataDict> dictionary;
        const Application* application = GetApplication(false);

        if( application != nullptr )
        {
            for( const std::shared_ptr<const CDEFormFile>& application_form_file : application->GetRuntimeFormFiles() )
            {
                if( SO::EqualsNoCase(form_file_on_disk->GetDictionaryFilename(), application_form_file->GetDictionaryFilename()) )
                {
                    dictionary = application_form_file->GetSharedDictionary();
                    break;
                }
            }

            if( dictionary == nullptr )
            {
                for( const std::shared_ptr<const CDataDict>& application_dictionary : application->GetRuntimeExternalDictionaries() )
                {
                    if( SO::EqualsNoCase(form_file_on_disk->GetDictionaryFilename(), application_dictionary->GetFullFileName()) )
                    {
                        dictionary = application_dictionary;
                        break;
                    }
                }
            }
        }

        // if not part of the application, load it from the disk
        if( dictionary == nullptr )
            dictionary = CDataDict::InstantiateAndOpen(form_file_on_disk->GetDictionaryFilename(), true);

        ASSERT(dictionary != nullptr);
        form_file_on_disk->SetDictionary(std::move(dictionary));
        form_file_on_disk->UpdatePointers();

        form_file = std::move(form_file_on_disk);
    }

    else
    {
        form_file = std::get<1>(GetApplicationComponents<std::shared_ptr<const CDEFormFile>>(json_node.GetOptional<wstring_view>(JK::name)));
    }

    ASSERT(form_file != nullptr);

    auto json_writer = Json::CreateStringWriter();
    json_writer->SetVerbose();

    json_writer->Write(*form_file);

    return Result::JsonText(json_writer);
}


ActionInvoker::Result ActionInvoker::Runtime::Application_getQuestionText(const JsonNode<wchar_t>& json_node, Caller& caller)
{
    std::shared_ptr<const CapiQuestionManager> question_manager;

    if( json_node.Contains(JK::path) )
    {
        const std::wstring path = caller.EvaluateAbsolutePath(json_node.Get<std::wstring>(JK::path));

        auto question_manager_on_disk = std::make_unique<CapiQuestionManager>();
        question_manager_on_disk->Load(path);
        question_manager = std::move(question_manager_on_disk);
    }

    else
    {
        const Application* application = std::get<0>(GetApplicationComponents<const Application*>(json_node.GetOptional<wstring_view>(JK::name)));
        ASSERT(application != nullptr);

        if( !application->GetUseQuestionText() || ( question_manager = application->GetCapiQuestionManager() ) == nullptr )
            throw CSProException(NoApplicationComponentMessage);
    }

    ASSERT(question_manager != nullptr);

    auto json_writer = Json::CreateStringWriter();
    json_writer->SetVerbose();

    json_writer->Write(*question_manager);

    return Result::JsonText(json_writer);
}


ActionInvoker::Result ActionInvoker::Runtime::Application_getQuestionnaireContent(const JsonNode<wchar_t>& json_node, Caller& /*caller*/)
{
    const Application* application;
    std::shared_ptr<const CDEFormFile> form_file;
    std::shared_ptr<const CDataDict> dictionary;

    std::tie(application, form_file, dictionary) = GetApplicationComponents<std::shared_ptr<const CDataDict>>(json_node.GetOptional<wstring_view>(JK::name));
    ASSERT(dictionary != nullptr);

    QuestionnaireContentCreator questionnaire_content_creator;

    // add as many associated application components as exist
    questionnaire_content_creator.SetDictionary(dictionary);
    questionnaire_content_creator.SetFormFile(std::move(form_file));

    if( application != nullptr && application->GetUseQuestionText() )
        questionnaire_content_creator.SetCapiQuestionManager(application->GetCapiQuestionManager());

    std::unique_ptr<Case> data_case;
    bool case_content_is_from_current_case;

    // return content with a case, either directly specified...
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
        try
        {
            data_case = GetInterpreterAccessor().GetCurrentCase(CS2WS(dictionary->GetName()));
            case_content_is_from_current_case = true;
        }

        catch(...)
        {
            // if no case is available, return content without a case
            return Result::JsonText(questionnaire_content_creator.GetContent()); 
        }
    }

    ASSERT(data_case != nullptr);

    return GetQuestionnaireContentWithCaseData(questionnaire_content_creator, std::move(data_case), json_node, true, case_content_is_from_current_case);
}
