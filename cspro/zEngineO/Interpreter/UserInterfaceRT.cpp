#include "stdafx.h"
#include "IncludesRT.h"
#include "Array.h"
#include "List.h"
#include "Nodes/UserInterface.h"
#include <zToolsO/NewlineSubstitutor.h>
#include <zToolsO/Screen.h>
#include <zUtilO/CustomFont.h>
#include <zHtml/UseHtmlDialogs.h>
#include <zParadataO/Logger.h>
#include <zUtilF/ChoiceDlg.h>
#include <zUtilF/HtmlDialogFunctionRunner.h>
#include <zUtilF/TextInputDlg.h>
#include <CSEntry/UWM.h>


std::optional<CSize> LogicInterpreter::EvaluateSize(const int width_program_index, const int height_program_index)
{
    ASSERT(( width_program_index != -1 ) == ( height_program_index != -1 ));

    if( width_program_index != -1 )
    {
        auto evaluate = [&](auto& result, const int program_index, const int max_value, const char* type)
        {
            result = Evaluate<int>(program_index);

            if( result < 1 || result > max_value )
            {
                IssueMessage(MessageType::Error, 2034, type, static_cast<int>(result), max_value);
                return false;
            }

            return true;
        };

        CSize size;

        if( evaluate(size.cx, width_program_index, Screen::GetMaxDisplayWidth(), "width") &&
            evaluate(size.cy, height_program_index, Screen::GetMaxDisplayHeight(), "height") )
        {
            return size;
        }
    }

    return std::nullopt;
}


std::unique_ptr<ViewerOptions> LogicInterpreter::EvaluateViewerOptions(const int viewer_options_node_program_index)
{
    if( viewer_options_node_program_index == -1 )
        return nullptr;

    const auto& viewer_options_node = GetNode<Nodes::ViewerOptions>(viewer_options_node_program_index);

    auto viewer_options = std::make_unique<ViewerOptions>();

    viewer_options->requested_size = EvaluateSize(viewer_options_node.width_expression, viewer_options_node.height_expression);

    if( viewer_options_node.title_expression != -1 )
        viewer_options->title = EvaluateSharableString(viewer_options_node.title_expression);

    if( viewer_options_node.show_close_button_expression != -1 )
        viewer_options->show_close_button = EvaluateConditional(viewer_options_node.show_close_button_expression);

    return viewer_options;
}


double LogicInterpreter::ex_prompt(const int program_index)
{
    if( !UseHtmlDialogs() )
        return RunSoonToBeRemoveFeature("prompt_pre77", program_index, nullptr);

    const auto& prompt_node = GetNode<Nodes::Prompt>(program_index);

    TextInputDlg text_input_dlg;

    text_input_dlg.SetTitle(ConvertV0Escapes(EvaluateSharableString(prompt_node.title_expression),
                                             V0_EscapeType::NewlinesToSlashN_Backslashes));

    auto evaluate_flag = [&](const int flag) { return ( ( prompt_node.flags & flag ) != 0 ); };

    text_input_dlg.SetNumeric(evaluate_flag(Nodes::Prompt::NumericFlag));
    text_input_dlg.SetPassword(evaluate_flag(Nodes::Prompt::PasswordFlag));
    text_input_dlg.SetUppercase(evaluate_flag(Nodes::Prompt::UppercaseFlag));

    const bool multiline = evaluate_flag(Nodes::Prompt::MultilineFlag);
    text_input_dlg.SetMultiline(multiline);

    if( prompt_node.initial_value_expression != -1 )
    {
        SharableString initial_value = ConvertV0Escapes(EvaluateSharableString(prompt_node.initial_value_expression), V0_EscapeType::NewlinesToSlashN_Backslashes);

        if( !multiline )
            NewlineSubstitutor::MakeNewlineToSpace(initial_value);

        text_input_dlg.SetInitialValue(std::move(initial_value));
    }

    std::unique_ptr<Paradata::OperatorSelectionEvent> operator_selection_event;

    if( Paradata::Logger::IsOpen() )
        operator_selection_event = std::make_unique<Paradata::OperatorSelectionEvent>(Paradata::OperatorSelectionEvent::Source::Prompt);

    SharableString return_value;

    if( text_input_dlg.DoModalOnUIThread() == IDOK )
        return_value = ApplyV0Escapes(text_input_dlg.GetTextInput(), V0_EscapeType::NewlinesToSlashN_Backslashes);

    if( operator_selection_event != nullptr )
    {
        operator_selection_event->SetPostSelectionValues(std::nullopt, return_value, true);
        RegisterAndLogEvent_INTERPRETER_DLL_TODO(std::move(operator_selection_event));
    }

    return AssignString(std::move(return_value));
}


double LogicInterpreter::ex_accept(const int program_index)
{
    if( !UseHtmlDialogs() )
        return RunSoonToBeRemoveFeature("exaccept_pre77", program_index, nullptr);

    const auto& va_with_size_node = GetNode<Nodes::VariableArgumentsWithSize>(program_index);

    ChoiceDlg choice_dlg(1);
    choice_dlg.SetTitle(ConvertV0Escapes(EvaluateSharableString(va_with_size_node.arguments[0])));

    // process the original style accept list
    if( va_with_size_node.arguments[1] >= 0 )
    {
        for( int i = 1; i < va_with_size_node.number_arguments; ++i )
            choice_dlg.AddChoice(ConvertV0Escapes(EvaluateSharableString(va_with_size_node.arguments[i])));
    }

    // process accept with a string List or string Array
    else
    {
        const Symbol& symbol = NPT_Ref(-1 * va_with_size_node.arguments[1]);

        if( symbol.IsA(SymbolType::List) )
        {
            const LogicList& accept_list = assert_cast<const LogicList&>(symbol);
            const size_t list_count = accept_list.GetCount();

            for( size_t i = 1; i <= list_count; ++i )
                choice_dlg.AddChoice(ConvertV0Escapes(accept_list.GetValue<SharableString>(i)));
        }

        else
        {
            const LogicArray& accept_array = assert_cast<const LogicArray&>(symbol);

            std::vector<SharableString> choices = accept_array.GetFilledCells<SharableString>();

            if( m_usingLogicSettingsV0 )
            {
                for( SharableString& choice : choices )
                    choice = ConvertV0Escapes(choice);
            }

            choice_dlg.SetChoices(std::move(choices));
        }
    }

    std::unique_ptr<Paradata::OperatorSelectionEvent> operator_selection_event;

    if( Paradata::Logger::IsOpen() )
        operator_selection_event = std::make_unique<Paradata::OperatorSelectionEvent>(Paradata::OperatorSelectionEvent::Source::Accept);

    int selection = 0;

    if( choice_dlg.DoModalOnUIThread() == IDOK )
        selection = choice_dlg.GetSelectedChoiceIndex();

    if( operator_selection_event != nullptr )
    {
        SharableString selected_text = ( selection != 0 ) ? choice_dlg.GetSelectedChoiceText() :
                                                            SharableString();
        operator_selection_event->SetPostSelectionValues(selection, std::move(selected_text), true);
        RegisterAndLogEvent_INTERPRETER_DLL_TODO(std::move(operator_selection_event));
    }

    return selection;
}


double LogicInterpreter::ex_htmldialog(const int program_index)
{
    const auto& html_dialog_node = GetNode<Nodes::HtmlDialog>(program_index);

    const SharableString provided_html_filename_or_path = EvaluateSharableString(html_dialog_node.file_path_expression);
    std::string full_path_html_file_path = GetAbsolutePath(provided_html_filename_or_path.GetString());

    // if the file does not exist, see if it exists in CSPro's html/dialogs directory or in an overridden HTML dialog directory
    if( !PortableFunctions::FileIsRegular(full_path_html_file_path) )
    {
        auto locate_file = [&](const std::string_view directory_sv)
        {
            std::string test_html_file_path = MakeFullPath(directory_sv, provided_html_filename_or_path.GetString());

            if( PortableFunctions::FileIsRegular(test_html_file_path) )
            {
                full_path_html_file_path = std::move(test_html_file_path);
                return true;
            }

            return false;
        };

        if( m_engineData->pff == nullptr || !locate_file(UTF8_TODO::GetUtf8(m_engineData->pff->GetHtmlDialogsDirectory())) )
        {
            if( !locate_file(Html::GetDirectory(Html::Subdirectory::Dialogs)) )
            {
                IssueMessage(MessageType::Error, 2031, Logic::FunctionTable::GetFunctionName(html_dialog_node.function_code),
                                                       full_path_html_file_path.c_str());
                return AssignStringNull();
            }
        }
    }

    SharableString input_data;
    SharableString display_options_json;

    // if the input was passed in using a single string, it might be a JSON object with nodes for inputData and/or displayOptions
    if( m_engineData->MeetsCompiledLogicVersion(Serializer::Iteration_8_0_000_1) && html_dialog_node.single_input_version == 1 )
    {
        const std::string single_input_text = EvaluateString(html_dialog_node.input_data_expression);

        HtmlDialogFunctionRunner::ParseSingleInputText(single_input_text, input_data, display_options_json);
        ASSERT(input_data.IsSet());
    }

    else
    {
        input_data = EvaluateNullableSharableString(html_dialog_node.input_data_expression);
        display_options_json = EvaluateNullableSharableString(html_dialog_node.display_options_json_expression);
    }

    if( input_data.IsSet() )
    {
        // prior to CSPro 8.0, the input data did not need to be JSON, so if the input data is not valid JSON, return it as a JSON string
        try
        {
            Json::Parse(*input_data);
        }

        catch(...)
        {
            static_assert(Versioning::Number == 8.0, "Start adding runtime warnings when the input data is not JSON");
            input_data = Encoders::ToJsonString(*input_data);
        }
    }

    HtmlDialogFunctionRunner html_dialog_function_runner(NavigationAddress::CreateHtmlFilePathReference(full_path_html_file_path),
                                                         std::move(input_data),
                                                         std::move(display_options_json));

    html_dialog_function_runner.DoModalOnUIThread();

    // handle any program control exceptions that may have resulted from JavaScript calls into CSPro logic
    RethrowProgramControlExceptions();

    return AssignString(html_dialog_function_runner.GetResultsText());
}


double LogicInterpreter::ex_setfont(const int program_index)
{
#ifdef WIN_DESKTOP
    UserDefinedFonts* user_defined_fonts;

    if( WindowsDesktopMessage::Send(WM_IMSA_GET_USER_FONTS, &user_defined_fonts) != 1 )
    {
        // if here, we are not running CSEntry
        return 0;
    }

    const auto& setfont_node = GetNode<Nodes::SetFont>(program_index);

    auto is_attribute = [&](const int flag) { return ( ( setfont_node.font_attributes & flag ) != 0 ); };

    const UserDefinedFonts::FontType font_type = static_cast<UserDefinedFonts::FontType>(setfont_node.font_attributes & Nodes::SetFont::TypeMask);

    // to restore the system default
    if( is_attribute(Nodes::SetFont::DefaultMask) )
    {
        user_defined_fonts->ResetFont(font_type);
        return 1;
    }

    const SharableString font_name = EvaluateSharableString(setfont_node.font_name_expression);
    const int font_size = Evaluate<int>(setfont_node.font_size_expression);

    if( !user_defined_fonts->SetFont(font_type, TC::ToWide(*font_name), font_size, is_attribute(Nodes::SetFont::BoldMask), is_attribute(Nodes::SetFont::ItalicsMask)) )
        return 0;

    if( font_type == UserDefinedFonts::FontType::ValueSets ||
        font_type == UserDefinedFonts::FontType::NumberPad ||
        font_type == UserDefinedFonts::FontType::All )
    {
        // refresh the response window
        AfxGetApp()->GetMainWnd()->PostMessage(UWM::CSEntry::ShowCapi);
    }

    return 1;

#else
    // not applicable on portable platforms
    return DEFAULT;

#endif
}
