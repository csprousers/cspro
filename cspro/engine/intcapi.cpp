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
#include "DefaultParametersOnlyUserFunctionArgumentEvaluator.h"
#include "ProgramControl.h"
#include <zEngineO/Block.h>
#include <zEngineO/UserFunction.h>
#include <zEngineO/WorkString.h>
#include <zToolsO/Encoders.h>
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
        item_name = pVarT->GetDictItem()->GetQualifiedName();
    } else {
        item_name = WS2CS(symbol->GetName());
    }

    CEntryDriver* pEntryDriver = (CEntryDriver*)m_pEngineDriver;
    auto question = pEntryDriver->GetQuestMgr()->GetQuestion(item_name);
    if (!question)
        return CString();

    if (pEntryDriver->GetQuestMgr()->UsePre76ConditionsAndFills()) {
        return EvaluateCapiTextPre76(*question, language_name, bQuestion, symbol_index, iOcc);
    }
    else {
        return EvaluateCapiText(*question, symbol, language_name, bQuestion);
    }
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
        const auto& default_language = ((CEntryDriver*)m_pEngineDriver)->GetQuestMgr()->GetDefaultLanguage();
        questionHelpCapiText = pBest->GetText(default_language.GetName(), text_type);
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

CString CIntDriver::EvaluateCapiTextPre76(const CapiQuestion& question, const std::wstring& language_name, bool bQuestion, int symbol_index, int iOcc)
{
    int iBestScore = 0;
    int iScore = 0;
    const CapiCondition* pBest = nullptr;

    for (const CapiCondition& condition : question.GetConditions()) {
        iScore = GetCapiConditionScorePre76(condition, symbol_index, iOcc);

        // Full match with occurrence and expresion: 8 points
        if (iScore == 8) {
            pBest = &condition;
            break;
        }
        else if (iBestScore < iScore) { // Refresh best score
            iBestScore = iScore;
            pBest = &condition;
        }
    }

    if (pBest == nullptr)
        return CString();

    auto text_type = bQuestion ? CapiTextType::QuestionText : CapiTextType::HelpText;
    CapiText questionHelpCapiText = pBest->GetText(language_name, text_type);
    if (questionHelpCapiText.GetText().IsEmpty()) {
        const auto& default_language = ((CEntryDriver*)m_pEngineDriver)->GetQuestMgr()->GetDefaultLanguage();
        questionHelpCapiText = pBest->GetText(default_language.GetName(), text_type);
    }

    CString csQuestionHelpCapiText = questionHelpCapiText.GetText();

    std::vector<CapiText::Delimiter> delimiters = { CapiText::Delimiter{ _T("%"), true } };

    const std::vector<CapiFill>& fills_to_replace = questionHelpCapiText.GetFills(delimiters);
    if (fills_to_replace.empty())
        return csQuestionHelpCapiText;

    const std::vector<ParsedCapiParam>* params = m_question_text_param_cache.Get(questionHelpCapiText);
    if (params == nullptr) {
        std::vector<ParsedCapiParam> parsed_params;
        ExpandText(questionHelpCapiText.GetText(), false, nullptr, &parsed_params);
        m_question_text_param_cache.Put(questionHelpCapiText, std::move(parsed_params));
        params = m_question_text_param_cache.Get(questionHelpCapiText);
    }

    for (const ParsedCapiParam& param : *params) {

        CString csEvaluatedText;

        // %getocclabel% will translate to the current occurrence label, if available
        if( param.m_eParamType == ParsedCapiParam::ParamType::GetOccLabel ) {
            // see if a symbol with occurrences exists for this symbol
            const Symbol* symbol_with_occs = SymbolCalculator::GetFirstSymbolWithOccurrences(NPT_Ref(symbol_index));
            if( symbol_with_occs != nullptr )
                csEvaluatedText = EvaluateOccurrenceLabel(symbol_with_occs, std::nullopt);
        }

        // getvaluelabel
        else if (param.m_eParamType == ParsedCapiParam::ParamType::GetValueLabel) {
            ASSERT(NPT(param.m_iSymbolVar)->IsA(SymbolType::Variable));
            VART* pVarT = VPT(param.m_iSymbolVar);
            csEvaluatedText = WS2CS(GetValueLabel(symbol_index, pVarT));
        }

        // a variable value or user-defined function call
        else {
            Evaluate(symbol_index, &param, csEvaluatedText);
        }

        // pre 7.6 CAPI text allowed <br/> to add newlines, don't let this get escaped in html
        std::wregex line_break(LR"(<br\s*\/?>)");
        csEvaluatedText = std::regex_replace(csEvaluatedText.GetString(), line_break, L"\n").c_str();

        csQuestionHelpCapiText.Replace(param.m_csTextToReplace, Encoders::ToHtml(csEvaluatedText).c_str());
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

        if (csVarName.GetLength() == 0 || (iSymVar = m_pEngineArea->SymbolTableSearch(csVarName, allowable_symbol_types)) == 0)
        {
            if (csVarName.Compare(_T("GETOCCLABEL")) == 0) // GHM 20140312 display the occurrence label
            {
                ParsedCapiParam cCapiParam(ParsedCapiParam::ParamType::GetOccLabel);
                cCapiParam.m_csTextToReplace.Format(_T("%lc%ls%lc"), HELP_OPENVARCHAR, csVarNameOcc.GetString(), HELP_CLOSEVARCHAR);
                capi_params->emplace_back(cCapiParam);
            }

            else if (csVarName.Compare(_T("GETVALUELABEL")) == 0)
            {
                if (csOcc.GetLength() == 0 || (iSymVar = m_pEngineArea->SymbolTableSearch(csOcc, { SymbolType::Variable })) == 0)
                {
                    if (bShowErrors)
                        issaerror(MessageType::Error, 48003, csOcc.GetString());

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
                    issaerror(MessageType::Error, 48000, csVarName.GetString()); // Invalid expresion in QSF File
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

                if ((iOccVar = m_pEngineArea->SymbolTableSearch(csOcc, { SymbolType::Variable })) == 0) {
                    if (bShowErrors)
                        issaerror(MessageType::Error, 48000, csOcc.GetString()); // Invalid expresion in QSF File
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
    m_iSkipStmt = FALSE;
    m_iStopExec = m_bStopProc;
    SetRequestIssued(false);

    try
    {
        return ConditionalValueIsTrue(evalexpr(iExpr));
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
    m_iSkipStmt = FALSE;
    m_iStopExec = m_bStopProc;
    SetRequestIssued(false);

    try
    {
        return WS2CS(EvaluateTextFill(iExpr));
    }

    // HTML_QSF_TODO what should happen if exceptions are thrown / movement requests are issued?
    catch( const ProgramControlException& ) {}

    return CString();
}

int CIntDriver::GetCapiConditionScorePre76(const CapiCondition& condition, int iSym, int iOcc)
{
    bool    bFitVar = true;
    bool    bFitOcc = (condition.GetMinOcc() <= iOcc && iOcc <= condition.GetMaxOcc());
    bool    bFitCond = EvaluateCapiConditionPre76(condition, iSym);
    int     iScore = 0;

    bool bFit = (
        //( /*pQuestOrHelp->GetSymVar() <= 0 ||*/ bFitVar ) && // Symbol
        (condition.GetMinOcc() < 1 || bFitOcc) && //Occ
        (condition.GetLogic() || bFitCond) // Condition
        );

    if (bFit) {
        // There are 8 options. Each one has a "weight"
        // DIC.VAR & Occurrence & Condition         8 points
        // DIC.VAR &.Occurrence                     7 points
        // DIC.VAR & Condition                      6 points
        // DIC.VAR                                  5 points

        // Not longer used. Only exact field is allowed!
        // Occurrence & Condition                   4 points
        // Occurrence                               3 points
        // Condition                                2 points
        //                                          1 points

        if (bFitOcc) {
            if (bFitCond) {
                iScore = bFitVar ? 8 : 4;
            }
            else {
                iScore = bFitVar ? 7 : 3;
            }
        }
        else if (bFitCond) {
            iScore = bFitVar ? 6 : 2;
        }
        else {
            iScore = bFitVar ? 5 : 1;
        }
    }

    return iScore;
}


bool CIntDriver::EvaluateCapiConditionPre76(const CapiCondition& condition, int iSym)
{
    bool        bMatchCondition = false;
    CIMSAString csLeft;
    int         iCond;
    CIMSAString csRight;
    CapiPre76::CNewCapiQuestionHelp::eCapiNewConditionType  eCondType;

    CapiPre76::CNewCapiQuestionHelp::SplitCondition(condition.GetLogic(), &csLeft, &iCond, &csRight, &eCondType);

    if (iCond >= 0 && iCond <= 5) {
        CIMSAString csLeftEvaluated = "%" + csLeft + "%";
        CIMSAString csRightEvaluated = csRight;

        csLeftEvaluated = ExpandText(csLeftEvaluated, iSym);

        csLeftEvaluated.TrimRight();

        double  dLeft = 0, dRight = 0;

        if (eCondType == CapiPre76::CNewCapiQuestionHelp::Numeric) {
            if (csRight.IsNumeric()) {
                dRight = (double)csRight.fVal();
            }
            else {
                ASSERT(SpecialValues::StringIsSpecial(csRight));
                dRight = SpecialValues::StringToValue(csRight);
            }

            if (csLeftEvaluated.IsNumeric()) {
                dLeft = (double)csLeftEvaluated.fVal();
            }
            else if (SpecialValues::StringIsSpecial(csLeftEvaluated)) {
                dLeft = SpecialValues::StringToValue(csLeftEvaluated);
            }
            else {
                ASSERT(0);
                dLeft = NOTAPPL;
            }
        }
        else if (eCondType == CapiPre76::CNewCapiQuestionHelp::Other) {
            csRightEvaluated = "%" + csRight + "%";

            csRightEvaluated = ExpandText(csRightEvaluated, iSym);

            csRightEvaluated.TrimRight();
        }

        if (iCond == 0) { // '='
            if (eCondType == CapiPre76::CNewCapiQuestionHelp::Numeric) {
                bMatchCondition = (dLeft == dRight);
            }
            else {
                bMatchCondition = (csLeftEvaluated == csRightEvaluated);
            }
        }
        else if (iCond == 1) { // '!=' or <>
            if (eCondType == CapiPre76::CNewCapiQuestionHelp::Numeric) {
                bMatchCondition = (dLeft != dRight);
            }
            else {
                bMatchCondition = (csLeftEvaluated != csRightEvaluated);
            }
        }
        else if (iCond == 2) { // '>'
            if (eCondType == CapiPre76::CNewCapiQuestionHelp::Numeric) {
                bMatchCondition = (dLeft > dRight);
            }
            else {
                bMatchCondition = ((CString)csLeftEvaluated > csRightEvaluated);
            }
        }
        else if (iCond == 3) { // '<'
            if (eCondType == CapiPre76::CNewCapiQuestionHelp::Numeric) {
                bMatchCondition = (dLeft < dRight);
            }
            else {
                bMatchCondition = ((CString)csLeftEvaluated < csRightEvaluated);
            }
        }
        else if (iCond == 4) { // '>='
            if (eCondType == CapiPre76::CNewCapiQuestionHelp::Numeric) {
                bMatchCondition = (dLeft >= dRight);
            }
            else {
                bMatchCondition = ((CString)csLeftEvaluated >= csRightEvaluated);
            }
        }
        else if (iCond == 5) { // '<='
            if (eCondType == CapiPre76::CNewCapiQuestionHelp::Numeric) {
                bMatchCondition = (dLeft <= dRight);
            }
            else {
                bMatchCondition = ((CString)csLeftEvaluated <= csRightEvaluated);
            }
        }
    }

    return bMatchCondition;
}


void CIntDriver::Evaluate(int iCurVar, const ParsedCapiParam* pCapiParam, CString& csEvaluatedText)
{
    int iSymVar = pCapiParam->m_iSymbolVar;
    Symbol* pSymbol = NPT(iSymVar);

    if (pSymbol->GetType() == SymbolType::UserFunction) // replacement text comes from a user-defined function
    {
        UserFunction& user_function = assert_cast<UserFunction&>(*pSymbol);

        // functions cannot have required parameters
        if (user_function.GetNumberRequiredParameters() > 0)
        {
            csEvaluatedText = FormatText(MGF::GetMessageText(48001).c_str(), user_function.GetName().c_str());
        }

        else
        {
            // setting m_iExSymbol to the current symbol will allow functions like curocc to work
            int iSavedExSymbol = m_iExSymbol;
            m_iExSymbol = iCurVar;

            // these statements clear any preexisting stuff that might have been going on
            m_iSkipStmt = FALSE;
            m_iStopExec = m_bStopProc;
            SetRequestIssued(false);

            DefaultParametersOnlyUserFunctionArgumentEvaluator argument_evaluator(this, user_function);
            double dRetValue = CallUserFunction(user_function, argument_evaluator);

            m_iExSymbol = iSavedExSymbol;

            if (user_function.GetReturnType() == SymbolType::WorkVariable)
            {
                csEvaluatedText = WS2CS(DoubleToString(dRetValue));
            }

            else
            {
                csEvaluatedText = CharacterObjectToString<CString>(dRetValue);
            }
        }
    }

    else if (pSymbol->IsA(SymbolType::WorkString))
    {
        const WorkString* work_string = assert_cast<const WorkString*>(pSymbol);
        csEvaluatedText = WS2CS(work_string->GetString());
    }

    else
    {
        int iSymVarOcc = pCapiParam->m_iOccSymbolVarOrCte;
        int iOcc = 0;
        VART* pVarT = pSymbol->IsA(SymbolType::Variable) ? (VART*)pSymbol : NULL;

        // Calculate the contents of the occurrence
        if (pCapiParam->m_bIsOccSymbol && iSymVarOcc > 0) // Occurrence symbol used
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
                VART* pCurrentFieldVarT = VPT(iCurVar);
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
            iOcc = EvaluateCapiVariableCurrentOccurrence(iCurVar, pVarT);
        }

        else // Occurrence defined as constant
        {
            iOcc = iSymVarOcc;
        }


        // Evaluate
        // Get Variable buffer
        csprochar* pAux;

        if (pVarT != NULL && pVarT->IsNumeric() && !pVarT->IsUsed())
        {
            pAux = NULL;
        }

        else
        {
            if (pVarT && pVarT->GetLogicStringPtr()) // GHM 20140326 a variable length string
                pAux = pVarT->GetLogicStringPtr()->GetBuffer();

            else
                pAux = GetVarAsciiValue(iSymVar, iOcc);
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
