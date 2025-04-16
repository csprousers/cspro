//------------------------------------------------------------------------
//
//  INTCAPI.CPP        CSPRO CAPI INTERPRETER
//
//  History:    Date       Author   Comment
//              ---------------------------
//              07 Nov 02   RHF     Creation
//
//------------------------------------------------------------------------
#include "StandardSystemIncludes.h"
#include "INTERPRE.H"
#include "Engine.h"
#include "Entdrv.h"
#include "ProgramControl.h"
#include <zCapiO/CapiName.h>
#include <zCapiO/CapiQuestionManager.h>


SharableString CIntDriver::EvaluateCapiText(const std::string& language_name, const bool is_question, const int symbol_index)
{
    const Symbol& symbol = NPT_Ref(symbol_index);

    const CapiQuestion* const question = assert_cast<const CEntryDriver*>(m_pEngineDriver)->GetQuestMgr()->GetQuestion(CapiName::Create(symbol));

    return ( question != nullptr ) ? EvaluateCapiText(*question, symbol, language_name, is_question) :
                                     SharableString();
}


SharableString CIntDriver::EvaluateCapiText(const CapiQuestion& question, const Symbol& symbol, const std::string& language_name, const bool is_question)
{
    const CapiCondition* best_condition = nullptr;

    for( const CapiCondition& condition : question.GetConditions() )
    {
        if( condition.GetProgramIndex() == -1 ||
            EvaluateQuestionTextCondition(symbol, condition.GetProgramIndex()) )
        {
            best_condition = &condition;
            break;
        }
    }

    if( best_condition == nullptr )
        return SharableString();

    const CapiText::Type text_type = is_question ? CapiText::Type::Question : CapiText::Type::Help;
    const CapiText* capi_text = best_condition->GetText(language_name, text_type);

    if( capi_text == nullptr || capi_text->GetText()->empty() )
    {
        const Language& default_language = assert_cast<const CEntryDriver*>(m_pEngineDriver)->GetQuestMgr()->GetDefaultLanguage();
        capi_text = best_condition->GetText(default_language.GetName(), text_type);

        if( capi_text == nullptr )
            return SharableString();
    }

    SharableString evaluated_text = capi_text->GetText();
    const std::vector<CapiFill>& fills_to_replace = capi_text->GetFills();

    if( !fills_to_replace.empty() )
    {
        const std::map<std::string, int>& fill_expressions = question.GetFillExpressions();

        std::map<std::string, SharableString> replacements;

        for( const CapiFill& fill : fills_to_replace )
        {
            replacements.try_emplace(fill.GetTextToReplace(),
                                     EvaluateQuestionTextFill(symbol, fill_expressions.at(fill.GetTextToReplace())));
        }

        evaluated_text = capi_text->ReplaceFills(replacements);
    }

    return capi_text->GetHtml(std::move(evaluated_text));
}


bool CIntDriver::EvaluateQuestionTextCondition(const Symbol& symbol, const int program_index)
{
    ASSERT(symbol.IsOneOf(SymbolType::Block, SymbolType::Variable));

    // setup execution parameters
    m_iProgType = static_cast<int>(ProcType::OnFocus);
    m_iExSymbol = symbol.GetSymbolIndex();
    m_iExLevel = SymbolCalculator::GetLevelNumber_base1(symbol);

    // these statements clear any preexisting stuff that might have been going on
    m_bSkipStmt = false;
    m_bStopExec = m_bStopProc;
    SetRequestIssued(false);

    try
    {
        return EvaluateConditional(program_index);
    }

    // HTML_QSF_TODO what should happen if exceptions are thrown / movement requests are issued?
    catch( const ProgramControlException& ) { }

    return false;
}


SharableString CIntDriver::EvaluateQuestionTextFill(const Symbol& symbol, const int program_index)
{
    ASSERT(symbol.IsOneOf(SymbolType::Block, SymbolType::Variable));

    // setup execution parameters
    m_iProgType = static_cast<int>(ProcType::OnFocus);
    m_iExSymbol = symbol.GetSymbolIndex();
    m_iExLevel = SymbolCalculator::GetLevelNumber_base1(symbol);

    // these statements clear any preexisting stuff that might have been going on
    m_bSkipStmt = false;
    m_bStopExec = m_bStopProc;
    SetRequestIssued(false);

    try
    {
        return EvaluateTextFill(program_index);
    }

    // HTML_QSF_TODO what should happen if exceptions are thrown / movement requests are issued?
    catch( const ProgramControlException& ) {}

    return SharableString();
}
