#include "StandardSystemIncludes.h"
#include "INTERPRE.H"
#include <zEngineO/Versioning.h>
#include <zEngineO/Nodes/UserInterface.h>
#include <zToolsO/Encoders.h>
#include <zJson/Json.h>
#include <zUtilF/HtmlDialogFunctionRunner.h>
#include <ZBRIDGEO/npff.h>


double CIntDriver::exhtmldialog(int iExpr)
{
    const auto& html_dialog_node = GetNode<Nodes::HtmlDialog>(iExpr);

    std::wstring provided_html_filename = EvalAlphaExpr(html_dialog_node.filename_expression);
    std::wstring full_path_html_filename = provided_html_filename;
    MakeFullPathFileName(full_path_html_filename);

    // if the file does not exist, see if it exists in CSPro's html/dialogs directory or in an overriden HTML dialog directory
    if( !PortableFunctions::FileIsRegular(full_path_html_filename) )
    {
        auto locate_file = [&](wstring_view directory)
        {
            std::wstring test_html_filename = MakeFullPath(directory, provided_html_filename);

            if( PortableFunctions::FileIsRegular(test_html_filename) )
            {
                full_path_html_filename = test_html_filename;
                return true;
            }

            return false;
        };

        if( !locate_file(m_pEngineDriver->m_pPifFile->GetHtmlDialogsDirectory()) &&
            !locate_file(Html::GetDirectory(Html::Subdirectory::Dialogs)) )
        {
            issaerror(MessageType::Error, 2031, Logic::FunctionTable::GetFunctionName(html_dialog_node.function_code),
                      full_path_html_filename.c_str());

            return AssignBlankAlphaValue();
        }
    }

    std::optional<std::wstring> input_data;
    std::optional<std::wstring> display_options_json;

    // if the input was passed in using a single string, it might be a JSON object with nodes for inputData and/or displayOptions
    if( Versioning::MeetsCompiledLogicVersion(Serializer::Iteration_8_0_000_1) && html_dialog_node.single_input_version == 1 )
    {
        std::wstring single_input_text = EvalAlphaExpr(html_dialog_node.input_data_expression);

        HtmlDialogFunctionRunner::ParseSingleInputText(single_input_text, input_data, display_options_json);
        ASSERT(input_data.has_value());
    }

    else
    {
        input_data = EvaluateOptionalStringExpression(html_dialog_node.input_data_expression);
        display_options_json = EvaluateOptionalStringExpression(html_dialog_node.display_options_json_expression);
    }

    if( input_data.has_value() )
    {
        // prior to CSPro 8.0, the input data did not need to be JSON, so if the input data is not valid JSON, return it as a JSON string
        try
        {
            Json::Parse(*input_data);
        }

        catch(...)
        {
            static_assert(Serializer::GetCurrentVersion() / 10000 == 80, "Start adding runtime warnings when the input data is not JSON");
            *input_data = Encoders::ToJsonString(*input_data);
        }
    }

    HtmlDialogFunctionRunner html_dialog_function_runner(NavigationAddress::CreateHtmlFilenameReference(full_path_html_filename),
                                                         ValueOrDefault(std::move(input_data)),
                                                         std::move(display_options_json));

    html_dialog_function_runner.DoModalOnUIThread();

    // handle any program control exceptions that may have resulted from JavaScript calls into CSPro logic
    RethrowProgramControlExceptions();

    return AssignAlphaValue(html_dialog_function_runner.GetResultsText());
}
