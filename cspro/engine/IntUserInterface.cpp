#include "StandardSystemIncludes.h"
#include "INTERPRE.H"
#include "Engine.h"
#include <zEngineO/Array.h>
#include <zEngineO/List.h>
#include <zEngineO/Versioning.h>
#include <zEngineO/Nodes/UserInterface.h>
#include <zEngineF/EngineUI.h>
#include <zToolsO/NewlineSubstitutor.h>
#include <zToolsO/Screen.h>
#include <zUtilO/ArrUtil.h>
#include <zUtilO/CustomFont.h>
#include <zUtilF/MsgOpt.h>
#include <zUtilF/ChoiceDlg.h>
#include <zUtilF/TextInputDlg.h>
#include <zParadataO/Logger.h>
#include <CSEntry/UWM.h>


std::optional<CSize> CIntDriver::EvaluateSize(int width_expression, int height_expression)
{
    ASSERT(( width_expression != -1 ) == ( height_expression != -1 ));

    if( width_expression != -1 )
    {
        auto evaluate = [&](auto& result, int expression, int max_value, const TCHAR* type) -> bool
        {
            result = evalexpr<int>(expression);

            if( result < 1 || result > max_value )
            {
                issaerror(MessageType::Error, 2034, type, (int)result, max_value);
                return false;
            }

            return true;
        };

        CSize size;

        if( evaluate(size.cx, width_expression, Screen::GetMaxDisplayWidth(), _T("width")) &&
            evaluate(size.cy, height_expression, Screen::GetMaxDisplayHeight(), _T("height")) )
        {
            return size;
        }

    }

    return std::nullopt;
}


std::unique_ptr<ViewerOptions> CIntDriver::EvaluateViewerOptions(const int viewer_options_node_index)
{
    if( viewer_options_node_index == -1 )
        return nullptr;

    const auto& viewer_options_node = GetNode<Nodes::ViewerOptions>(viewer_options_node_index);

    auto viewer_options = std::make_unique<ViewerOptions>();

    viewer_options->requested_size = EvaluateSize(viewer_options_node.width_expression, viewer_options_node.height_expression);

    if( viewer_options_node.title_expression != -1 )
        viewer_options->title = EvalAlphaExpr(viewer_options_node.title_expression);

    if( viewer_options_node.show_close_button_expression != -1 )
        viewer_options->show_close_button = ConditionalValueIsTrue(evalexpr(viewer_options_node.show_close_button_expression));

    return viewer_options;
}


double CIntDriver::exprompt(int iExpr)
{
    if( !UseHtmlDialogs() )
        return exprompt_pre77(iExpr);

    const auto& prompt_node = GetNode<Nodes::Prompt>(iExpr);

    TextInputDlg text_input_dlg;

    text_input_dlg.SetTitle(EvaluateStringExpressionConvertingV0Escapes(prompt_node.title_expression, V0_EscapeType::NewlinesToSlashN_Backslashes));

    auto evaluate_flag = [&](int flag) { return ( ( prompt_node.flags & flag ) != 0 ); };

    if( Versioning::PredatesCompiledLogicVersion(Serializer::Iteration_7_7_000_1) )
    {
        // the combination of password/multiline was disallowed in CSPro 7.7
        if( evaluate_flag(Nodes::Prompt::PasswordFlag) && evaluate_flag(Nodes::Prompt::MultilineFlag) )
            const_cast<Nodes::Prompt&>(prompt_node).flags &= ~Nodes::Prompt::MultilineFlag;
    }

    text_input_dlg.SetNumeric(evaluate_flag(Nodes::Prompt::NumericFlag));
    text_input_dlg.SetPassword(evaluate_flag(Nodes::Prompt::PasswordFlag));
    text_input_dlg.SetUppercase(evaluate_flag(Nodes::Prompt::UppercaseFlag));

    const bool multiline = evaluate_flag(Nodes::Prompt::MultilineFlag);
    text_input_dlg.SetMultiline(multiline);

    if( prompt_node.initial_value_expression != -1 )
    {
        std::wstring initial_value = EvaluateStringExpressionConvertingV0Escapes(prompt_node.initial_value_expression, V0_EscapeType::NewlinesToSlashN_Backslashes);

        if( !multiline )
            NewlineSubstitutor::MakeNewlineToSpace(initial_value);

        text_input_dlg.SetInitialValue(std::move(initial_value));
    }

    std::unique_ptr<Paradata::OperatorSelectionEvent> operator_selection_event;

    if( Paradata::Logger::IsOpen() )
        operator_selection_event = std::make_unique<Paradata::OperatorSelectionEvent>(Paradata::OperatorSelectionEvent::Source::Prompt);

    std::wstring return_value;

    if( text_input_dlg.DoModalOnUIThread() == IDOK )
        return_value = ApplyV0Escapes(text_input_dlg.GetTextInput(), V0_EscapeType::NewlinesToSlashN_Backslashes);

    if( operator_selection_event != nullptr )
    {
        operator_selection_event->SetPostSelectionValues(std::nullopt, return_value, true);
        m_pParadataDriver->RegisterAndLogEvent(std::move(operator_selection_event));
    }

    return AssignAlphaValue(std::move(return_value));
}


double CIntDriver::exprompt_pre77(int iExpr)
{
    const FNVARIOUS_NODE* various_node = (FNVARIOUS_NODE*)PPT(iExpr);
    EngineUI::PromptNode prompt_node;

    prompt_node.title = WS2CS(EvaluateStringExpressionConvertingV0Escapes(various_node->fn_expr[0], V0_EscapeType::NewlinesToSlashN_Backslashes));

    if( various_node->fn_expr[1] >= 0 )
        prompt_node.initial_value = WS2CS(EvaluateStringExpressionConvertingV0Escapes(various_node->fn_expr[1], V0_EscapeType::NewlinesToSlashN_Backslashes));

    const int& flags = various_node->fn_expr[2];

    prompt_node.numeric = ( ( flags & Nodes::Prompt::NumericFlag ) != 0 );
    prompt_node.password = ( ( flags & Nodes::Prompt::PasswordFlag ) != 0 );
    prompt_node.upper_case = ( ( flags & Nodes::Prompt::UppercaseFlag ) != 0 );
    prompt_node.multiline = ( ( flags & Nodes::Prompt::MultilineFlag ) != 0 );

    if( !prompt_node.multiline )
        NewlineSubstitutor::MakeNewlineToSpace(prompt_node.initial_value);

    std::unique_ptr<Paradata::OperatorSelectionEvent> operator_selection_event;

    if( Paradata::Logger::IsOpen() )
        operator_selection_event = std::make_unique<Paradata::OperatorSelectionEvent>(Paradata::OperatorSelectionEvent::Source::Prompt);

    SendEngineUIMessage(EngineUI::Type::Prompt, prompt_node);

    prompt_node.return_value = WS2CS(ApplyV0Escapes(CS2WS(prompt_node.return_value), V0_EscapeType::NewlinesToSlashN_Backslashes));

    if( operator_selection_event != nullptr )
    {
        operator_selection_event->SetPostSelectionValues(std::nullopt, CS2WS(prompt_node.return_value), true);
        m_pParadataDriver->RegisterAndLogEvent(std::move(operator_selection_event));
    }

    return AssignAlphaValue(prompt_node.return_value);
}


double CIntDriver::exaccept(int iExpr)
{
    if( !UseHtmlDialogs() )
        return exaccept_pre77(iExpr);

    const auto& va_with_size_node = GetNode<Nodes::VariableArgumentsWithSize>(iExpr);

    ChoiceDlg choice_dlg(1);
    choice_dlg.SetTitle(EvaluateStringExpressionConvertingV0Escapes(va_with_size_node.arguments[0]));

    // process the original style accept list
    if( va_with_size_node.arguments[1] >= 0 )
    {
        for( int i = 1; i < va_with_size_node.number_arguments; ++i )
            choice_dlg.AddChoice(EvaluateStringExpressionConvertingV0Escapes(va_with_size_node.arguments[i]));
    }

    // process accept with a string list or string array
    else
    {
        const Symbol& symbol = NPT_Ref(-1 * va_with_size_node.arguments[1]);

        if( symbol.IsA(SymbolType::List) )
        {
            const LogicList& accept_list = assert_cast<const LogicList&>(symbol);
            const size_t list_count = accept_list.GetCount();

            for( size_t i = 1; i <= list_count; ++i )
                choice_dlg.AddChoice(ConvertV0Escapes(accept_list.GetString(i)));
        }

        else
        {
            const LogicArray& accept_array = assert_cast<const LogicArray&>(symbol);
            choice_dlg.SetChoices(ConvertV0Escapes(accept_array.GetStringFilledCells()));
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
        std::optional<std::wstring> selected_text = ( selection != 0 ) ? std::make_optional(choice_dlg.GetSelectedChoiceText()) :
                                                                         std::nullopt;
        operator_selection_event->SetPostSelectionValues(selection, std::move(selected_text), true);
        m_pParadataDriver->RegisterAndLogEvent(std::move(operator_selection_event));
    }

    return selection;
}


double CIntDriver::exaccept_pre77(int iExpr)
{
    const auto& function_node = GetNode<FNN_NODE>(iExpr);

    CString heading = EvalAlphaExpr<CString>(function_node.fn_expr[0]);
    std::vector<CString> choices;

    // process the original style accept list
    if( function_node.fn_expr[1] >= 0 )
    {
        for( int i = 1; i < function_node.fn_nargs; ++i )
            choices.emplace_back(EvalAlphaExpr<CString>(function_node.fn_expr[i]));
    }

    // process accept with a string list or string array
    else
    {
        const Symbol* symbol = NPT(-1 * function_node.fn_expr[1]);

        if( symbol->IsA(SymbolType::List) )
        {
            const LogicList* accept_list = assert_cast<const LogicList*>(symbol);

            for( size_t i = 1; i <= accept_list->GetCount(); ++i )
                choices.emplace_back(WS2CS(accept_list->GetString(i)));
        }

        else
        {
            ASSERT(symbol->IsA(SymbolType::Array));
            const LogicArray* accept_array = assert_cast<const LogicArray*>(symbol);

            for( const std::wstring& array_cell : accept_array->GetStringFilledCells() )
                choices.emplace_back(WS2CS(array_cell));
        }
    }

    // prepare the data for the SelectDlgHelper
    std::vector<std::vector<CString>*> select_dlg_data;

    for( const CString& choice : choices )
        select_dlg_data.emplace_back(new std::vector<CString> { choice });

    int selection = SelectDlgHelper_pre77(function_node.fn_code, &heading, &select_dlg_data, nullptr, nullptr, nullptr);

    for( const auto& data : select_dlg_data )
        delete data;

    return selection;
}


double CIntDriver::exsetfont(int iExpr)
{
#ifdef WIN_DESKTOP
    UserDefinedFonts* user_defined_fonts;

    if( WindowsDesktopMessage::Send(WM_IMSA_GET_USER_FONTS, &user_defined_fonts) != 1 )
    {
        // if here, we are not running CSEntry
        return 0;
    }

    const auto& setfont_node = GetNode<Nodes::SetFont>(iExpr);

    auto is_attribute = [&](int flag) { return ( ( setfont_node.font_attributes & flag ) != 0 ); };

    UserDefinedFonts::FontType font_type = static_cast<UserDefinedFonts::FontType>(setfont_node.font_attributes & Nodes::SetFont::TypeMask);

    // to restore the system default
    if( is_attribute(Nodes::SetFont::DefaultMask) )
    {
        user_defined_fonts->ResetFont(font_type);
        return 1;
    }

    std::wstring font_name = EvalAlphaExpr(setfont_node.font_name_expression);
    int font_size = evalexpr<int>(setfont_node.font_size_expression);

    if( !user_defined_fonts->SetFont(font_type, font_name, font_size, is_attribute(Nodes::SetFont::BoldMask), is_attribute(Nodes::SetFont::ItalicsMask)) )
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
    return DEFAULT; // not applicable on portable platforms
#endif
}
