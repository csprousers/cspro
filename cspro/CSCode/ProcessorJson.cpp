#include "StdAfx.h"
#include "ProcessorJson.h"
#include <zJavaScript/Executor.h>
#include <zJson/JsonSpecFile.h>
#include <zAppO/Application.h>
#include <zAppO/Properties/ApplicationProperties.h>
#include <zDiffO/DiffSpec.h>
#include <zPackO/PackSpec.h>
#include <zSortO/SortSpec.h>
#include <zDesignerF/BuildWndJsonReaderInterface.h>


void ProcessorJson::FormatJson(CodeView& code_view, bool compress_mode)
{
    CLogicCtrl* logic_ctrl = code_view.GetLogicCtrl();
    logic_ctrl->ClearErrorAndWarningMarkers();

    try
    {
        const JsonNode<wchar_t> json_node = Json::Parse(logic_ctrl->GetText());

        logic_ctrl->SetText(json_node.GetNodeAsString(compress_mode ? JsonFormattingOptions::Compact :
                                                                      DefaultJsonFileWriterFormattingOptions));

        code_view.GetCodeDoc().SetModifiedFlag(TRUE, &code_view);
    }

    catch(...)
    {
        ErrorMessage::Display(_T("The JSON is not valid and cannot be formatted. Validate the JSON and fix any errors before formatting it."));
    }
}


void ProcessorJson::ValidateJson(CodeView& code_view)
{
    CLogicCtrl* logic_ctrl = code_view.GetLogicCtrl();
    ASSERT(logic_ctrl->GetLexer() == SCLEX_JSON);

    CSCodeBuildWnd* build_wnd = assert_cast<CMainFrame*>(AfxGetMainWnd())->GetBuildWnd();

    if( build_wnd == nullptr )
        return;

    build_wnd->Initialize(code_view, _T("JSON validation"));

    try
    {
        Json::Parse(logic_ctrl->GetText());
    }

    catch( const CSProException& exception )
    {
        build_wnd->AddError(exception);
    }

    build_wnd->Finalize();
}


void ProcessorJson::ValidateSpecFile(CodeView& code_view) // JSON_TODO make sure that all spec files all handled here
{
    CLogicCtrl* logic_ctrl = code_view.GetLogicCtrl();
    ASSERT(logic_ctrl->GetLexer() == SCLEX_JSON &&
           code_view.GetLanguageSettings().GetLanguageType() == LanguageType::CSProSpecFileJson &&
           code_view.GetLanguageSettings().GetJsonSpecFileIndex().has_value());

    CSCodeBuildWnd* build_wnd = assert_cast<CMainFrame*>(AfxGetMainWnd())->GetBuildWnd();

    if( build_wnd == nullptr )
        return;

    const unsigned json_spec_file_index = *code_view.GetLanguageSettings().GetJsonSpecFileIndex();

    // the descriptions contain the file extensions in parentheses, so remove them
    const std::wstring description = SO::TrimRight(SO::RemoveTextFollowingCharacter(
                                                   LanguageJsonSpecFile::GetDescriptionFromIndex(json_spec_file_index), '(', true));

    build_wnd->Initialize(code_view, description + _T(" specification file validation"));

    try
    {
        BuildWndJsonReaderInterface json_reader_interface(code_view.GetCodeDoc(), *build_wnd);
        const JsonNode<wchar_t> json_node = Json::Parse(logic_ctrl->GetText(), &json_reader_interface);

        if( json_spec_file_index == ID_RUN_CSPRO_SPEC_FILE_APP )
        {
            Application::CreateFromJson(json_node);
        }

        else if( json_spec_file_index == ID_RUN_CSPRO_SPEC_FILE_CSPROPS )
        {
            ApplicationProperties::CreateFromJson(json_node);
        }

        else if( json_spec_file_index == ID_RUN_CSPRO_SPEC_FILE_CMP )
        {
            DiffSpec diff_spec;
            diff_spec.Load(json_node, false);
        }

        else if( json_spec_file_index == ID_RUN_CSPRO_SPEC_FILE_DCF )
        {
            CDataDict::CreateFromJson(json_node);
        }

        else if( json_spec_file_index == ID_RUN_CSPRO_SPEC_FILE_CSPACK )
        {
            PackSpec pack_spec;
            pack_spec.Load(json_node, false);
        }

        else if( json_spec_file_index == ID_RUN_CSPRO_SPEC_FILE_SSF )
        {
            SortSpec sort_spec;
            sort_spec.Load(json_node, false);
        }

        else if( json_spec_file_index == ID_RUN_CSPRO_SPEC_FILE_CSDS ||  // CODE_TODO implement if/when possible
                 json_spec_file_index == ID_RUN_CSPRO_SPEC_FILE_XL2CS || // CODE_TODO implement if/when possible
                 json_spec_file_index == ID_RUN_CSPRO_SPEC_FILE_EXF ||   // CODE_TODO implement when possible
                 json_spec_file_index == ID_RUN_CSPRO_SPEC_FILE_FQF )    // CODE_TODO implement when possible
        {
            build_wnd->AddWarning(FormatTextCS2WS(_T("CSCode %0.1f can only validate the JSON for files of type: %s"),
                                                  CSPRO_VERSION_NUMBER, description.c_str()));
        }

        else
        {
            ASSERT(false);
        }
    }

    catch( const CSProException& exception )
    {
        build_wnd->AddError(exception);
    }

    build_wnd->Finalize();
}


void ProcessorJson::DowngradeSpecFile(CodeView& code_view)
{
    const CodeDoc& code_doc = code_view.GetCodeDoc();
    const unsigned json_spec_file_index = *code_view.GetLanguageSettings().GetJsonSpecFileIndex();

    const TCHAR* exception_prefix = nullptr;

    try
    {
        const std::tuple<const TCHAR*, const TCHAR*> filename_and_function = 
            ( json_spec_file_index == ID_RUN_CSPRO_SPEC_FILE_APP ) ? std::make_tuple(_T("application.mjs"), _T("convertApplication")) :
            ( json_spec_file_index == ID_RUN_CSPRO_SPEC_FILE_DCF ) ? std::make_tuple(_T("dictionary.mjs"),  _T("convertDictionary")) :
                                                                     throw ProgrammingErrorException();

        const std::wstring js_filename = PortableFunctions::PathAppendToPath(PortableFunctions::PathAppendToPath(
                                                                             Html::GetDirectory(Html::Subdirectory::Utilities),
                                                                             _T("spec-file-downgrader")),
                                                                             std::get<0>(filename_and_function));

        if( !PortableFunctions::FileIsRegular(js_filename) )
            throw CSProException(_T("The specification filename downgrader routine could not be found here: ") + js_filename);

        // parse the JSON
        exception_prefix = _T("The JSON is not valid. Validate the JSON and fix any errors before proceeding.\n\n");

        const std::wstring directory = PortableFunctions::PathGetDirectory(code_doc.GetPathName());
        JsonReaderInterface json_reader_interface(directory);

        const JsonNode<wchar_t> json_node = Json::Parse(code_view.GetLogicCtrl()->GetText(), &json_reader_interface);

        // execute the JavaScript conversion routine
        exception_prefix = _T("There was an error converting the specification file to CSPro 7.7 format.\n\n");

        JavaScript::Executor executor(directory);
        const std::string ini_result = executor.ExecuteFunction(js_filename, std::get<1>(filename_and_function),
                                                                json_node,
                                                                PortableFunctions::PathRemoveTrailingSlash(directory));

        // display the results in a new tab
        POSITION template_pos = AfxGetApp()->GetFirstDocTemplatePosition();
        ASSERT(template_pos != nullptr);
        CDocTemplate* doc_template = AfxGetApp()->GetNextDocTemplate(template_pos);
        ASSERT(doc_template != nullptr);

        CodeDoc* new_code_doc = assert_cast<CodeDoc*>(doc_template->OpenDocumentFile(nullptr));
        new_code_doc->GetLanguageSettings().SetLanguageType(LanguageType::CSProSpecFileIni, std::wstring());

        CodeView& new_code_view = new_code_doc->GetPrimaryCodeView();
        new_code_view.RefreshLogicControlLexer();
        new_code_view.GetLogicCtrl()->SetText(ini_result);
    }

    catch( const CSProException& exception )
    {
        if( exception_prefix == nullptr )
        {
            ErrorMessage::Display(exception);
        }

        else
        {
            ErrorMessage::Display(exception_prefix + exception.GetErrorMessage());
        }
    }
}
