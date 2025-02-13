#include "StandardSystemIncludes.h"
#include "INTERPRE.H"
#include <zEngineO/Audio.h>
#include <zEngineO/Document.h>
#include <zEngineO/Geometry.h>
#include <zEngineO/Image.h>
#include <zEngineO/Versioning.h>


double CIntDriver::exDocument_compute(const int program_index)
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
        const std::wstring document_text = EvalAlphaExpr(symbol_compute_with_subscript_node.rhs_subscript_compilation);

        if( !get_lhs_logic_document() )
            return 0;

        *lhs_logic_document = document_text;
    }

    // assigning another symbol
    else
    {
        Symbol* rhs_symbol = GetFromSymbolOrEngineItem(symbol_compute_with_subscript_node.rhs_symbol_index, symbol_compute_with_subscript_node.rhs_subscript_compilation);

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
            issaerror(MessageType::Error, 100343, rhs_symbol->GetName().c_str(), lhs_logic_document->GetName().c_str(), exception.GetErrorMessage().c_str());
        }
    }

    return 0;
}


double CIntDriver::exDocument_clear(const int program_index)
{
    const auto& symbol_va_with_subscript_node = GetOrConvertPre80SymbolVariableArgumentsWithSubscriptNode(program_index);
    LogicDocument* logic_document = GetFromSymbolOrEngineItem<LogicDocument*>(symbol_va_with_subscript_node.symbol_index, symbol_va_with_subscript_node.subscript_compilation);

    if( logic_document == nullptr )
        return 0;

    logic_document->Reset();

    return 1;
}


double CIntDriver::exDocument_load(const int program_index)
{
    const auto& symbol_va_with_subscript_node = GetOrConvertPre80SymbolVariableArgumentsWithSubscriptNode(program_index);
    const std::wstring filename = EvalFullPathFileName(symbol_va_with_subscript_node.arguments[0]);
    LogicDocument* logic_document = GetFromSymbolOrEngineItem<LogicDocument*>(symbol_va_with_subscript_node.symbol_index, symbol_va_with_subscript_node.subscript_compilation);

    if( logic_document == nullptr )
        return 0;

    try
    {
        logic_document->Load(filename);
    }

    catch( const CSProException& exception )
    {
        issaerror(MessageType::Error, 100341, filename.c_str(), exception.GetErrorMessage().c_str());
        return 0;
    }

    return 1;
}


double CIntDriver::exDocument_save(const int program_index)
{
    const auto& symbol_va_with_subscript_node = GetOrConvertPre80SymbolVariableArgumentsWithSubscriptNode(program_index);
    const std::wstring filename = EvalFullPathFileName(symbol_va_with_subscript_node.arguments[0]);
    LogicDocument* logic_document = GetFromSymbolOrEngineItem<LogicDocument*>(symbol_va_with_subscript_node.symbol_index, symbol_va_with_subscript_node.subscript_compilation);

    if( logic_document == nullptr )
        return DEFAULT;

    if( !logic_document->HasContent() )
    {
        issaerror(MessageType::Error, 100340, logic_document->GetName().c_str(), _T("save the document"));
        return DEFAULT;
    }

    try
    {
        logic_document->Save(filename);
    }

    catch( const CSProException& exception )
    {
        issaerror(MessageType::Error, 100342, filename.c_str(), exception.GetErrorMessage().c_str());
        return 0;
    }

    return 1;
}


double CIntDriver::exDocument_view(const int program_index)
{
    const auto& symbol_va_with_subscript_node = GetOrConvertPre80SymbolVariableArgumentsWithSubscriptNode(program_index);
    LogicDocument* logic_document = GetFromSymbolOrEngineItem<LogicDocument*>(symbol_va_with_subscript_node.symbol_index, symbol_va_with_subscript_node.subscript_compilation);

    if( logic_document == nullptr )
        return DEFAULT;

    std::unique_ptr<const ViewerOptions> viewer_options = Versioning::MeetsCompiledLogicVersion(Serializer::Iteration_8_0_000_3) ? EvaluateViewerOptions(symbol_va_with_subscript_node.arguments[0]) : 
                                                                                                                                   nullptr;

    return exDocument_view(*logic_document, viewer_options.get());
}


double CIntDriver::exDocument_view(const LogicDocument& logic_document, const ViewerOptions* viewer_options)
{
    if( !logic_document.HasContent() )
    {
        issaerror(MessageType::Error, 100340, logic_document.GetName().c_str(), _T("view the document"));
        return DEFAULT;
    }

    return logic_document.View(viewer_options) ? 1 : 0;
}
