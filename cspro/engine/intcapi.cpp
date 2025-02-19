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
#include <zEngineO/Block.h>
#include <zEngineO/UserFunctionArgumentEvaluator.h>
#include <zEngineO/WorkString.h>
#include <zToolsO/Encoders.h>
#include <zToolsO/RaiiHelpers.h>
#include <zToolsO/Tools.h>
#include <zUtilO/AppLdr.h>
#include <zAppO/Application.h>
#include <zMessageO/Messages.h>
#include <Zissalib/CsDriver.h>
#include <zCapiO/CapiQuestionManager.h>
#include <zCapiO/CapiQuestionFilePre76.h>
#include <Zentryo/hreplace.h>
#include <regex>


CString CIntDriver::EvaluateCapiText(const std::wstring& language_name, bool bQuestion, int symbol_index, int iOcc)
{
    CString item_name;
    const Symbol* symbol = NPT(symbol_index);

    if (symbol->IsA(SymbolType::Variable)) {
        const VART* pVarT = assert_cast<const VART*>(symbol);
        item_name = UTF8_TODO::GetCString(pVarT->GetDictItem()->GetQualifiedName());
    } else {
        item_name = UTF8_TODO::GetCString(symbol->GetName());
    }

    CEntryDriver* pEntryDriver = (CEntryDriver*)m_pEngineDriver;
    const std::optional<CapiQuestion> question = pEntryDriver->GetQuestMgr()->GetQuestion(item_name);

    return question.has_value() ? EvaluateCapiText(*question, symbol, language_name, bQuestion) :
                                  CString();
}


CString CIntDriver::EvaluateCapiText(const CapiQuestion& question, const Symbol* symbol, const std::wstring& language_name, bool bQuestion)
{
    const CapiCondition* pBest = nullptr;

    for (const CapiCondition& condition : question.GetConditions()) {
        if (!condition.GetLogicExpression().has_value() || EvaluateQuestionTextCondition(symbol, *condition.GetLogicExpression())) {
            pBest = &condition;
            break;
        }
    }

    if (pBest == nullptr)
        return CString();

    auto text_type = bQuestion ? CapiTextType::QuestionText : CapiTextType::HelpText;
    CapiText questionHelpCapiText = pBest->GetText(language_name, text_type);
    if (questionHelpCapiText.GetText().IsEmpty()) {
        const Language& default_language = ((CEntryDriver*)m_pEngineDriver)->GetQuestMgr()->GetDefaultLanguage();
        questionHelpCapiText = pBest->GetText(UTF8_TODO::GetWide(default_language.GetName()), text_type);
    }

    CString csQuestionHelpCapiText = questionHelpCapiText.GetText();

    const std::vector<CapiFill>& fills_to_replace = questionHelpCapiText.GetFills(CapiText::DefaultDelimiters);
    if (fills_to_replace.empty())
        return csQuestionHelpCapiText;

    const auto& fill_expressions = question.GetFillExpressions();
    if (!fill_expressions.empty()) {
        std::map<CString, CString> replacements;
        for (const CapiFill& fill : fills_to_replace) {
            replacements[fill.GetTextToReplace()] = EvaluateQuestionTextFill(symbol, fill_expressions.at(fill.GetTextToReplace()));
        }
        csQuestionHelpCapiText = questionHelpCapiText.ReplaceFills(CapiText::DefaultDelimiters, replacements);
    }
    return csQuestionHelpCapiText;
}


CString CIntDriver::ExpandText(const CString& csText, bool bShowErrors, bool* bSomeErr, std::vector<ParsedCapiParam>* capi_params)
{
    CReplace                    hReplace;
    int                         iOcc;
    bool                        bError = false;
    bool                        bOccCte;
    TCHAR* p, * pVarNameOcc, * pOcc;
    CString                     csVarName;   /* %c%s%c */
    CString                     csOcc;
    CString                     csVarNameOcc;
    std::vector<CReplace>       aReplaceVar;

    const static std::vector<SymbolType> allowable_symbol_types
    {
        SymbolType::Variable,
        SymbolType::WorkVariable,
        SymbolType::UserFunction,
        SymbolType::WorkString
    };

    bool bGenerate = (capi_params != NULL);

    auto pText = csText.GetString();

    if (!bGenerate)
        aReplaceVar.clear();

    // Generate variable list
    while (1) {
        bOccCte = false;
        csVarName = _T("");
        csOcc = _T("");
        if ((p = _tcschr(const_cast<TCHAR*>(pText), HELP_OPENVARCHAR)) == NULL) {
            break;
        }
        else {
            pVarNameOcc = p + 1;
            if ((p = _tcschr(pVarNameOcc, HELP_CLOSEVARCHAR)) == NULL) {
                break;
            }
            else {
                *p = 0;
                csVarNameOcc = pVarNameOcc;
                *p = _T('%');
                pText = p + 1;

                if ((p = _tcschr(csVarNameOcc.GetBuffer(), HELP_OPENPAREN)) != NULL) {
                    *p = 0;
                    csVarName = csVarNameOcc;
                    *p = HELP_OPENPAREN;
                    pOcc = p + 1;

                    if ((p = _tcschr(pOcc, HELP_CLOSEPAREN)) == NULL) {
                        continue; // ignore
                    }
                    else {
                        *p = 0;
                        csOcc = pOcc;
                        *p = HELP_CLOSEPAREN;
                        //pText = p + 1;
                    }
                }
                else {
                    csOcc = _T("");
                    csVarName = csVarNameOcc;
                }
            }
        }

        csVarName.MakeUpper();

        csVarName.Trim();
        csOcc.Trim();

        // Cannot search in an empty container
        if (m_pEngineArea == 0)
            ASSERT(0);

        // Check for valid varname
        int iSymVar = 0;

        if (csVarName.GetLength() == 0 || (iSymVar = m_pEngineArea->SymbolTableSearch(UTF8_TODO::GetUtf8(csVarName), allowable_symbol_types)) == 0)
        {
            if (csVarName.Compare(_T("GETOCCLABEL")) == 0) // 20140312 display the occurrence label
            {
                ParsedCapiParam cCapiParam(ParsedCapiParam::ParamType::GetOccLabel);
                cCapiParam.m_csTextToReplace.Format(_T("%lc%ls%lc"), HELP_OPENVARCHAR, csVarNameOcc.GetString(), HELP_CLOSEVARCHAR);
                capi_params->emplace_back(cCapiParam);
            }

            else if (csVarName.Compare(_T("GETVALUELABEL")) == 0)
            {
                if (csOcc.GetLength() == 0 || (iSymVar = m_pEngineArea->SymbolTableSearch(UTF8_TODO::GetUtf8(csOcc), { SymbolType::Variable })) == 0)
                {
                    if (bShowErrors)
                        issaerror(MessageType::Error, 48003, UTF8_TODO::GetUtf8(csOcc).c_str());

                    bError = true;
                }

                else
                {
                    ParsedCapiParam cCapiParam(ParsedCapiParam::ParamType::GetValueLabel);
                    cCapiParam.m_csTextToReplace.Format(_T("%lc%ls%lc"), HELP_OPENVARCHAR, csVarNameOcc.GetString(), HELP_CLOSEVARCHAR);
                    cCapiParam.m_iSymbolVar = iSymVar;
                    capi_params->emplace_back(cCapiParam);
                }
            }

            else if (csVarName.GetLength() > 0)
            {
                if (bShowErrors)
                    issaerror(MessageType::Error, 48000, UTF8_TODO::GetUtf8(csVarName).c_str()); // Invalid expresion in QSF File
                bError = true;
            }

            continue; // ignore;
        }

        if (csVarNameOcc.GetLength() >= HELP_MAXREPLACETEXTLEN)
            continue; // ignore

        if (!bGenerate)
            hReplace.csIn.Format(_T("%lc%ls%lc"), HELP_OPENVARCHAR, csVarNameOcc.GetString(), HELP_CLOSEVARCHAR);

        Symbol* pSymbol = NPT(iSymVar);
        VART* pVarT = pSymbol->IsA(SymbolType::Variable) ? (VART*)pSymbol : NULL;

        // Get iOcc
        if (csOcc.GetLength() > 0) {

            p = csOcc.GetBuffer();

            bool    bCte = true;
            while (*p != 0) {
                if (!(*p >= _T('0') && *p <= '9')) {
                    bCte = false;
                    break;
                }
                p++;
            }

            bOccCte = bCte;

            if (bCte) {
                iOcc = _ttoi(csOcc);
            }
            else {
                int     iOccVar;
                int     iCurOcc = 0;

                if ((iOccVar = m_pEngineArea->SymbolTableSearch(UTF8_TODO::GetUtf8(csOcc), { SymbolType::Variable })) == 0) {
                    if (bShowErrors)
                        issaerror(MessageType::Error, 48000, UTF8_TODO::GetUtf8(csOcc).c_str()); // Invalid expresion in QSF File
                    bError = true;
                    continue; // ignore
                }

                if (NPT(iOccVar)->IsA(SymbolType::Variable)) {
                    VART* pOccVarT;

                    pOccVarT = VPT(iOccVar);

                    // Index must be numeric
                    if (!pOccVarT->IsNumeric()) {
                        if (bShowErrors)
                            issaerror(MessageType::Error, 48002, NPT(iOccVar)->GetName().c_str()); // Invalid expresion in QSF File
                        bError = true;
                        continue; // ignore
                    }

                    if (!bGenerate)
                        iCurOcc = pOccVarT->GetOwnerGPT()->GetCurrentOccurrences();
                }

                if (bGenerate) {
                    iOcc = iOccVar;
                }
                else {
                    iOcc = (int)GetVarValue(iOccVar, iCurOcc, false);
                } // !bGenerate
            } // !bCte
        } //csOcc.GetLength() > 0
        else { //csOcc.GetLength() == 0
            iOcc = 0;

            if (bGenerate) {
                iOcc = -1;
            }
            else {
                // RHF INIC Jan 08, 2003
                if (pVarT != NULL) {
                    GROUPT* pGroupT = pVarT->GetOwnerGPT();

                    // Hidden group
                    bool bUseSectionOcc = (pGroupT->GetSource() == GROUPT::Source::DcfFile);

                    if (pVarT->IsArray() && bUseSectionOcc) {
                        SECT* pSecT = pVarT->GetSPT();
                        int         iGroupNum = 0;
                        GROUPT* pGroupTAux;
                        int         iSectionOcc = 0;

                        while ((pGroupTAux = pSecT->GetGroup(iGroupNum)) != NULL) {
                            if (pGroupTAux->GetSource() == GROUPT::Source::FrmFile)
                            {
                                iSectionOcc = std::max(iSectionOcc, pGroupTAux->GetCurrentExOccurrence());
                            }

                            iGroupNum++;
                        }

                        iOcc = iSectionOcc;
                    }
                }
                // RHF END Jan 08, 2003
            } // !bGenerate
        }

        if (bGenerate)
        {
            if (pVarT != NULL && pVarT->IsNumeric() && !pVarT->IsUsed())
                continue;

            ParsedCapiParam cCapiParam(ParsedCapiParam::ParamType::VariableOrUserFunction);
            cCapiParam.m_csTextToReplace.Format(_T("%lc%ls%lc"), HELP_OPENVARCHAR, csVarNameOcc.GetString(), HELP_CLOSEVARCHAR);
            cCapiParam.m_iSymbolVar = iSymVar;
            cCapiParam.m_bIsOccSymbol = !bOccCte;
            cCapiParam.m_iOccSymbolVarOrCte = iOcc; // == -1 --> no occurrence

            capi_params->emplace_back(cCapiParam);

            continue;
        }

        // Get Variable buffer
        csprochar* pAux;

        if (pVarT != NULL && pVarT->IsNumeric() && !pVarT->IsUsed()) {
            pAux = NULL;
        }
        else {
            pAux = GetVarAsciiValue(iSymVar, iOcc, true);
        }

        ASSERT(!bGenerate);

        hReplace.pszOutBuff = pAux;
        if (pAux == NULL)
            continue;

        hReplace.csOut = CString(pAux);

        hReplace.csOut.TrimLeft();
        hReplace.csOut.TrimRight();

        aReplaceVar.emplace_back(hReplace);
    } // while(1)


    CIMSAString csExpandedText;
    if (!bGenerate) {
        csExpandedText = csText;
        for (int i = 0; i < (int)aReplaceVar.size(); i++) {
            hReplace = aReplaceVar[i];

            csExpandedText.Replace(hReplace.csIn, hReplace.csOut);

            free(hReplace.pszOutBuff);
            hReplace.pszOutBuff = NULL;
        }

        aReplaceVar.clear();
    }

    if (bSomeErr != NULL)
        *bSomeErr = bError;

    return csExpandedText;
}


bool CIntDriver::EvaluateQuestionTextCondition(const Symbol* symbol, int iExpr)
{
    ASSERT(symbol->IsOneOf(SymbolType::Block, SymbolType::Variable));

    // setup execution parameters
    m_iProgType = (int)ProcType::OnFocus;
    m_iExSymbol = symbol->GetSymbolIndex();
    m_iExLevel = SymbolCalculator::GetLevelNumber_base1(*symbol);

    // these statements clear any preexisting stuff that might have been going on
    m_bSkipStmt = false;
    m_bStopExec = m_bStopProc;
    SetRequestIssued(false);

    try
    {
        return EvaluateConditional(iExpr);
    }

    // HTML_QSF_TODO what should happen if exceptions are thrown / movement requests are issued?
    catch( const ProgramControlException& ) {}

    return false;
}


CString CIntDriver::EvaluateQuestionTextFill(const Symbol* symbol, int iExpr)
{
    ASSERT(symbol->IsOneOf(SymbolType::Block, SymbolType::Variable));

    // setup execution parameters
    m_iProgType = (int)ProcType::OnFocus;
    m_iExSymbol = symbol->GetSymbolIndex();
    m_iExLevel = SymbolCalculator::GetLevelNumber_base1(*symbol);

    // these statements clear any preexisting stuff that might have been going on
    m_bSkipStmt = false;
    m_bStopExec = m_bStopProc;
    SetRequestIssued(false);

    try
    {
        return UTF8_TODO::GetCString(*EvaluateTextFill(iExpr));
    }

    // HTML_QSF_TODO what should happen if exceptions are thrown / movement requests are issued?
    catch( const ProgramControlException& ) {}

    return CString();
}


std::string CIntDriver::EvaluateCapiText(const int current_symbol_index, const ParsedCapiParam& parsed_capi_param)
{
    Symbol& symbol = NPT_Ref(parsed_capi_param.m_iSymbolVar);

    // replacement text coming from a user-defined function
    if( symbol.IsA(SymbolType::UserFunction) )
    {
        UserFunction& user_function = assert_cast<UserFunction&>(symbol);

        // functions cannot have required parameters
        if( user_function.GetNumberRequiredParameters() > 0 )
            return FormatText(MGF::GetMessageText(48001)->c_str(), user_function.GetName().c_str());

        // setting m_iExSymbol to the current symbol will allow functions like curocc to work
        const RAII::SetValueAndRestoreOnDestruction symbol_modifier(m_iExSymbol, current_symbol_index);

        // these statements clear any preexisting stuff that might have been going on
        m_bSkipStmt = false;
        m_bStopExec = m_bStopProc;
        SetRequestIssued(false);

        DefaultParametersOnlyUserFunctionArgumentEvaluator argument_evaluator;
        const double return_value = CallUserFunction(user_function, argument_evaluator);

        if( user_function.GetReturnType() == SymbolType::WorkVariable )
        {
            return DoubleToString(return_value);
        }

        else
        {
            ASSERT(user_function.GetReturnType() == SymbolType::WorkString);
            return GetWorkingString(static_cast<size_t>(return_value));
        }
    }

    else if( symbol.IsA(SymbolType::WorkString) )
    {
        const WorkString& work_string = assert_cast<const WorkString&>(symbol);
        return work_string.GetString();
    }

    else
    {
        int iSymVarOcc = parsed_capi_param.m_iOccSymbolVarOrCte;
        int iOcc = 0;
        VART* pVarT = symbol.IsA(SymbolType::Variable) ? (VART*)&symbol: NULL;

        // Calculate the contents of the occurrence
        if (parsed_capi_param.m_bIsOccSymbol && iSymVarOcc > 0) // Occurrence symbol used
        {
            ASSERT(NPT(iSymVarOcc)->IsA(SymbolType::Variable));

            int iCurOcc = 0;

            if (NPT(iSymVarOcc)->IsA(SymbolType::Variable))
            {
                VART* pOccVarT = VPT(iSymVarOcc);
                ASSERT(pOccVarT->IsNumeric());
                GROUPT* pOccGroupT = pOccVarT->GetOwnerGPT();

                iCurOcc = pOccGroupT->GetCurrentOccurrences();

                // similar to below, if the occurrence variable is on a different group but the same record as the current field, use the current field's
                // occurrence, not the occurrence variable's occurrence, for the evaluation
                VART* pCurrentFieldVarT = VPT(current_symbol_index);
                GROUPT* pCurrentFieldGroupT = pCurrentFieldVarT->GetOwnerGPT();

                if (pOccGroupT != pCurrentFieldGroupT && pOccVarT->GetOwnerSec() == pCurrentFieldVarT->GetOwnerSec()
                    && pOccGroupT->GetDimType() == pCurrentFieldGroupT->GetDimType())
                {
                    int iDesiredOcc = pCurrentFieldGroupT->GetCurrentExOccurrence();

                    // only get occurrences from groups that have data
                    if (iDesiredOcc <= pOccGroupT->GetDataOccurrences())
                        iCurOcc = iDesiredOcc;
                }
            }

            iOcc = (int)GetVarValue(iSymVarOcc, iCurOcc, false);
        }

        else if (iSymVarOcc == -1) // No occurrence defined
        {
            iOcc = EvaluateCapiVariableCurrentOccurrence(current_symbol_index, pVarT);
        }

        else // Occurrence defined as constant
        {
            iOcc = iSymVarOcc;
        }


        // Evaluate
        // Get Variable buffer
        CString csEvaluatedText;
        csprochar* pAux;

        if (pVarT != NULL && pVarT->IsNumeric() && !pVarT->IsUsed())
        {
            pAux = NULL;
        }

        else
        {
            pAux = GetVarAsciiValue(parsed_capi_param.m_iSymbolVar, iOcc);
        }


        if (pAux != NULL)
        {
            csEvaluatedText = pAux;
            csEvaluatedText.Trim();
            free(pAux);
        }

        else
        {
            csEvaluatedText.Empty();
        }

        return UTF8_TODO::GetUtf8(csEvaluatedText);
    }
}


int CIntDriver::EvaluateCapiVariableCurrentOccurrence(int iCurVar, VART* pVarT)
{
    int iOcc = 0;

    if (pVarT != NULL)
    {
        if (pVarT->IsArray())
        {
            GROUPT* pGroupT = pVarT->GetOwnerGPT();
            bool bUseSectionOcc = (pGroupT->GetSource() == GROUPT::Source::DcfFile);

            if (bUseSectionOcc) // Hidden group
            {
                SECT* pSecT = pVarT->GetSPT();
                int         iGroupNum = 0;
                GROUPT* pGroupTAux;
                int         iSectionOcc = 0;

                while ((pGroupTAux = pSecT->GetGroup(iGroupNum)) != NULL)
                {
                    if (pGroupTAux->GetSource() == GROUPT::Source::FrmFile)
                    {
                        iSectionOcc = std::max(iSectionOcc, pGroupTAux->GetCurrentExOccurrence());
                    }

                    iGroupNum++;
                }

                iOcc = iSectionOcc;
            }

            else
            {
                // if the current field is not in the same group as the symbol, but is on the same record, use
                // the current field's occurrence as the symbol's occurrence; this will allow, for example, the use of
                // a name variable when a population record has been split into several groups
                Symbol* current_symbol = NPT(iCurVar);
                VART* pCurrentFieldVarT;

                if (current_symbol->IsA(SymbolType::Variable))
                {
                    pCurrentFieldVarT = (VART*)current_symbol;
                }

                else
                {
                    ASSERT(current_symbol->IsA(SymbolType::Block));
                    pCurrentFieldVarT = assert_cast<const EngineBlock*>(current_symbol)->GetFirstVarT();
                    ASSERT(pCurrentFieldVarT != nullptr);
                }

                GROUPT* pCurrentFieldGroupT = pCurrentFieldVarT->GetOwnerGPT();

                if (pGroupT != pCurrentFieldGroupT && pVarT->GetOwnerSec() == pCurrentFieldVarT->GetOwnerSec() &&
                    pGroupT->GetDimType() == pCurrentFieldGroupT->GetDimType())
                {
                    int iDesiredOcc = pCurrentFieldGroupT->GetCurrentExOccurrence();

                    // only get occurrences from groups that have data
                    if (iDesiredOcc <= pGroupT->GetDataOccurrences())
                        iOcc = iDesiredOcc;
                }
            }
        }

        if (iOcc <= 0)
            iOcc = pVarT->GetOwnerGPT()->GetCurrentOccurrences();
    }

    return iOcc;
}
