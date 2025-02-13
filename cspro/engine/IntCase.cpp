#include "StandardSystemIncludes.h"
#include "INTERPRE.H"
#include "Engine.h"
#include "EngineQuestionnaireViewer.h"
#include <zEngineO/EngineDictionary.h>


double CIntDriver::exdictcompute(int iExpr)
{
    // assigning a case to a case
    const auto& symbol_compute_node = GetNode<Nodes::SymbolCompute>(iExpr);
    ASSERT(symbol_compute_node.rhs_symbol_type == SymbolType::Dictionary);

    EngineDictionary& lhs_engine_dictionary = GetSymbolEngineDictionary(symbol_compute_node.lhs_symbol_index);
    const EngineDictionary& rhs_engine_dictionary = GetSymbolEngineDictionary(symbol_compute_node.rhs_symbol_index);

    lhs_engine_dictionary.GetEngineCase().GetCase() = rhs_engine_dictionary.GetEngineCase().GetCase();

    return 0;
}


double CIntDriver::exCase_view(const int program_index)
{
    const auto& symbol_va_with_subscript_node = GetNode<Nodes::SymbolVariableArgumentsWithSubscript>(program_index);
    DICT* pDicT = DPT(symbol_va_with_subscript_node.symbol_index);
    std::unique_ptr<const ViewerOptions> viewer_options = EvaluateViewerOptions(symbol_va_with_subscript_node.arguments[0]);

    return exCase_view(*pDicT, viewer_options.get());
}


double CIntDriver::exCase_view(const DICT& dictionary, const ViewerOptions* viewer_options)
{
    if( !IsDataAccessible(dictionary, true) )
    {
        ASSERT80(false); // nodes should be wrapped in a data access check, so the above check should not be necessary
        return 0;
    }

    EngineQuestionnaireViewer engine_questionnaire_viewer(m_pEngineDriver, dictionary.GetName());
    engine_questionnaire_viewer.View(viewer_options);

    return 1;
}
