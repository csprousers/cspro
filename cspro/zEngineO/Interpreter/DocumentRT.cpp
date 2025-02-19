#include "stdafx.h"
#include "IncludesRT.h"
#include "Audio.h"
#include "Document.h"
#include "Geometry.h"
#include "Image.h"


double LogicInterpreter::ex_Document_compute(const int program_index)
{
    const auto& symbol_compute_with_subscript_node = GetOrConvertPre80SymbolComputeWithSubscriptNode(program_index);
    const SymbolReference<Symbol*> lhs_symbol_reference = EvaluateSymbolReference<Symbol*>(symbol_compute_with_subscript_node.lhs_symbol_index, symbol_compute_with_subscript_node.lhs_subscript_compilation);

    LogicDocument* lhs_logic_document;

    auto get_lhs_logic_document = [&]()
    {
        lhs_logic_document = GetFromSymbolOrEngineItem<LogicDocument*>(lhs_symbol_reference);
        return ( lhs_logic_document != nullptr );
    };

    // assigning a string
    if( symbol_compute_with_subscript_node.rhs_symbol_index == -1 )
    {
        SharableString document_text = EvaluateSharableString(symbol_compute_with_subscript_node.rhs_subscript_compilation);

        if( !get_lhs_logic_document() )
            return 0;

        *lhs_logic_document = document_text.Release();
    }

    // assigning another symbol
    else
    {
        Symbol* const rhs_symbol = GetFromSymbolOrEngineItem(symbol_compute_with_subscript_node.rhs_symbol_index, symbol_compute_with_subscript_node.rhs_subscript_compilation);

        if( rhs_symbol == nullptr || !get_lhs_logic_document() )
            return 0;

        try
        {
            if( rhs_symbol->IsA(SymbolType::Document) )
            {
                *lhs_logic_document = assert_cast<const LogicDocument&>(*rhs_symbol);
            }

            else if( rhs_symbol->IsA(SymbolType::Audio) )
            {
                *lhs_logic_document = assert_cast<const LogicAudio&>(*rhs_symbol);
            }

            else if( rhs_symbol->IsA(SymbolType::Geometry) )
            {
                *lhs_logic_document = assert_cast<const LogicGeometry&>(*rhs_symbol);
            }

            else if( rhs_symbol->IsA(SymbolType::Image) )
            {
                *lhs_logic_document = assert_cast<const LogicImage&>(*rhs_symbol);
            }

            else
            {
                ASSERT(false);
            }
        }

        catch( const CSProException& exception )
        {
            IssueMessage(MessageType::Error, MGF::Document_assignment_error_100343, rhs_symbol->GetName().c_str(), lhs_logic_document->GetName().c_str(), exception.what());
        }
    }

    return 0;
}


double LogicInterpreter::ex_Document_clear(const int program_index)
{
    const auto& symbol_va_with_subscript_node = GetOrConvertPre80SymbolVariableArgumentsWithSubscriptNode(program_index);
    LogicDocument* const logic_document = GetFromSymbolOrEngineItem<LogicDocument*>(symbol_va_with_subscript_node.symbol_index, symbol_va_with_subscript_node.subscript_compilation);

    if( logic_document == nullptr )
        return 0;

    logic_document->Reset();

    return 1;
}


double LogicInterpreter::ex_Document_load(const int program_index)
{
    const auto& symbol_va_with_subscript_node = GetOrConvertPre80SymbolVariableArgumentsWithSubscriptNode(program_index);
    const std::string file_path = EvaluatePath(symbol_va_with_subscript_node.arguments[0]);
    LogicDocument* const logic_document = GetFromSymbolOrEngineItem<LogicDocument*>(symbol_va_with_subscript_node.symbol_index, symbol_va_with_subscript_node.subscript_compilation);

    if( logic_document == nullptr )
        return 0;

    try
    {
        logic_document->Load(file_path);
    }

    catch( const CSProException& exception )
    {
        IssueMessage(MessageType::Error, MGF::Document_load_error_100341, file_path.c_str(), exception.what());
        return 0;
    }

    return 1;
}


double LogicInterpreter::ex_Document_save(const int program_index)
{
    const auto& symbol_va_with_subscript_node = GetOrConvertPre80SymbolVariableArgumentsWithSubscriptNode(program_index);
    const std::string file_path = EvaluatePath(symbol_va_with_subscript_node.arguments[0]);
    LogicDocument* const logic_document = GetFromSymbolOrEngineItem<LogicDocument*>(symbol_va_with_subscript_node.symbol_index, symbol_va_with_subscript_node.subscript_compilation);

    if( logic_document == nullptr )
        return DEFAULT;

    if( !logic_document->HasContent() )
    {
        IssueMessage(MessageType::Error, MGF::Document_no_document_for_action_100340, logic_document->GetName().c_str(), "save the document");
        return DEFAULT;
    }

    try
    {
        logic_document->Save(file_path);
    }

    catch( const CSProException& exception )
    {
        IssueMessage(MessageType::Error, MGF::Document_save_error_100342, file_path.c_str(), exception.what());
        return 0;
    }

    return 1;
}


double LogicInterpreter::ex_Document_view(const int program_index)
{
    const auto& symbol_va_with_subscript_node = GetOrConvertPre80SymbolVariableArgumentsWithSubscriptNode(program_index);
    LogicDocument* const logic_document = GetFromSymbolOrEngineItem<LogicDocument*>(symbol_va_with_subscript_node.symbol_index, symbol_va_with_subscript_node.subscript_compilation);

    if( logic_document == nullptr )
        return DEFAULT;

    const std::unique_ptr<const ViewerOptions> viewer_options = m_engineData->MeetsCompiledLogicVersion(Serializer::Iteration_8_0_000_3) ? EvaluateViewerOptions(symbol_va_with_subscript_node.arguments[0]) :
                                                                                                                                           nullptr;

    return ex_Document_view(*logic_document, viewer_options.get());
}


double LogicInterpreter::ex_Document_view(const LogicDocument& logic_document, const ViewerOptions* viewer_options)
{
    if( !logic_document.HasContent() )
    {
        IssueMessage(MessageType::Error, MGF::Document_no_document_for_action_100340, logic_document.GetName().c_str(), "view the document");
        return DEFAULT;
    }

    return logic_document.View(viewer_options);
}
