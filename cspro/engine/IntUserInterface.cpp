#include "StandardSystemIncludes.h"
#include "Interpreter.h"
#include "Engine.h"
#include <zEngineO/Array.h>
#include <zEngineO/List.h>
#include <zEngineO/Nodes/UserInterface.h>
#include <zEngineF/EngineUI.h>
#include <zToolsO/NewlineSubstitutor.h>
#include <zParadataO/Logger.h>


double CIntDriver::exprompt_pre77(int iExpr)
{
    const FNVARIOUS_NODE* various_node = (FNVARIOUS_NODE*)PPT(iExpr);
    EngineUI::PromptNode prompt_node;

    prompt_node.title = UTF8_TODO::GetCString(ConvertV0Escapes(EvaluateString(various_node->fn_expr[0]), V0_EscapeType::NewlinesToSlashN_Backslashes));

    if( various_node->fn_expr[1] >= 0 )
        prompt_node.initial_value = UTF8_TODO::GetCString(ConvertV0Escapes(EvaluateString(various_node->fn_expr[1]), V0_EscapeType::NewlinesToSlashN_Backslashes));

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

    prompt_node.return_value = UTF8_TODO::GetCString(ApplyV0Escapes(UTF8_TODO::GetUtf8(prompt_node.return_value), V0_EscapeType::NewlinesToSlashN_Backslashes));

    if( operator_selection_event != nullptr )
    {
        operator_selection_event->SetPostSelectionValues(std::nullopt, UTF8_TODO::GetUtf8(prompt_node.return_value), true);
        m_paradataDriver->RegisterAndLogEvent(std::move(operator_selection_event));
    }

    return AssignAlphaValue(prompt_node.return_value);
}


double CIntDriver::exaccept_pre77(int iExpr)
{
    const auto& function_node = GetNode<FNN_NODE>(iExpr);

    CString heading = EvalAlphaExprCS(function_node.fn_expr[0]);
    std::vector<CString> choices;

    // process the original style accept list
    if( function_node.fn_expr[1] >= 0 )
    {
        for( int i = 1; i < function_node.fn_nargs; ++i )
            choices.emplace_back(EvalAlphaExprCS(function_node.fn_expr[i]));
    }

    // process accept with a string list or string array
    else
    {
        const Symbol* symbol = NPT(-1 * function_node.fn_expr[1]);

        if( symbol->IsA(SymbolType::List) )
        {
            const LogicList* const accept_list = assert_cast<const LogicList*>(symbol);

            for( size_t i = 1; i <= accept_list->GetCount(); ++i )
                choices.emplace_back(UTF8_TODO::GetCString(accept_list->GetValue<SharableString>(i).GetString()));
        }

        else
        {
            ASSERT(symbol->IsA(SymbolType::Array));
            const LogicArray* const accept_array = assert_cast<const LogicArray*>(symbol);

            for( const SharableString& array_cell : accept_array->GetFilledCells<SharableString>() )
                choices.emplace_back(UTF8_TODO::GetCString(*array_cell));
        }
    }

    // prepare the data for the SelectDlgHelper
    std::vector<std::vector<CString>*> select_dlg_data;

    for( const CString& choice : choices )
        select_dlg_data.emplace_back(new std::vector<CString> { choice });

    int selection = SelectDlgHelper_pre77(function_node.fn_code, heading, &select_dlg_data, nullptr, nullptr, nullptr);

    for( const auto& data : select_dlg_data )
        delete data;

    return selection;
}
