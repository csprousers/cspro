#include "StdAfx.h"
#include "Pre76CapiQuestionFile.h"
#include <zToolsO/Special.h>
#include <zUtilO/SpecFile.h>
#include <zUtilF/ProgressDlg.h>
#include <zUtilF/ProgressDlgFactory.h>


#define FILE_TYPE           L"Question"
#define FILE_TYPE2          L"Question File"
#define HEAD_STAT           L"[CAPI QUESTIONS]"
#define HEAD_LANGUAGES      L"[LANGUAGES]"
#define HEAD_QUESTION       L"[QUESTION]"
#define HEAD_HELP           L"[HELP]"
#define HEAD_INSTRUCTION    L"[INSTRUCTION]"
#define CMD_FIELD           L"Field"
#define CMD_CONDITION       L"Condition"
#define CMD_OCCURRENCES     L"Occurrences"


////////////////////////////////////////////////
////////////////////////////////////////////////


CapiPre76::CNewCapiLanguage::CNewCapiLanguage() {
    Init();
}


CapiPre76::CNewCapiLanguage::CNewCapiLanguage(const CNewCapiLanguage& rOther) {
    Copy(rOther);
}


void CapiPre76::CNewCapiLanguage::operator=(const CNewCapiLanguage& rOther) {
    Copy(rOther);
}

void CapiPre76::CNewCapiLanguage::Init() {
    language_name.clear();
    language_label.clear();
}

void CapiPre76::CNewCapiLanguage::Copy(const CNewCapiLanguage& rOther) {
    Init();

    language_name = rOther.language_name;
    language_label = rOther.language_label;
}


////////////////////////////////////////////////
////////////////////////////////////////////////


CapiPre76::CNewCapiText::CNewCapiText() {
    Init();
}


CapiPre76::CNewCapiText::CNewCapiText(const CNewCapiText& rOther) {
    Copy(rOther);
}


void CapiPre76::CNewCapiText::operator=(const CNewCapiText& rOther) {
    Copy(rOther);
}

void CapiPre76::CNewCapiText::Init() {
    language_name.clear();
    text.clear();
}

void CapiPre76::CNewCapiText::Copy(const CNewCapiText& rOther) {
    Init();

    language_name = rOther.language_name;
    text = rOther.text;
}


////////////////////////////////////////////////
////////////////////////////////////////////////

CapiPre76::CNewCapiQuestionHelp::CNewCapiQuestionHelp() {
    Init();
}


CapiPre76::CNewCapiQuestionHelp::CNewCapiQuestionHelp(const CNewCapiQuestionHelp& rOther) {
    Copy(rOther);
}


void CapiPre76::CNewCapiQuestionHelp::operator=(const CNewCapiQuestionHelp& rOther) {
    Copy(rOther);
}

void CapiPre76::CNewCapiQuestionHelp::Init() {
    m_eType = eCapiNewQuestType::None;
    m_symbolName.clear();
    m_iOccMin = -1;
    m_iOccMax = -1;
    m_condition.clear();

    m_aCapiText.clear();
}

void CapiPre76::CNewCapiQuestionHelp::Copy(const CNewCapiQuestionHelp& rOther) {
    Init();

    m_eType = rOther.m_eType;
    m_symbolName = rOther.m_symbolName;
    m_iOccMin = rOther.m_iOccMin;
    m_iOccMax = rOther.m_iOccMax;
    m_condition = rOther.m_condition;
    m_aCapiText = rOther.m_aCapiText;
}

eCapiNewQuestType CapiPre76::CNewCapiQuestionHelp::GetType() {
    return m_eType;
}
void CapiPre76::CNewCapiQuestionHelp::SetType(eCapiNewQuestType eType) {
    m_eType = eType;
}

bool CapiPre76::CNewCapiQuestionHelp::SetSymbolName(std::string symbol_name) {
    if (!CheckSymbol(symbol_name))
        return false;

    m_symbolName = std::move(symbol_name);

    return true;
}

int CapiPre76::CNewCapiQuestionHelp::GetOccMin() const {
    return m_iOccMin;
}

void CapiPre76::CNewCapiQuestionHelp::SetOccMin(int iOccMin) {
    m_iOccMin = iOccMin;
}

int CapiPre76::CNewCapiQuestionHelp::GetOccMax() const {
    return m_iOccMax;
}

void CapiPre76::CNewCapiQuestionHelp::SetOccMax(int iOccMax) {
    m_iOccMax = iOccMax;
}

bool CapiPre76::CNewCapiQuestionHelp::SetCondition(CString csCondition) {
    if (csCondition.GetLength() > 0) {
        if (!CheckCondition(csCondition))
            return false;
    }

    m_condition = UTF8_TODO::GetUtf8(csCondition);
    return true;
}


bool CapiPre76::CNewCapiQuestionHelp::SetOccurrences(CString csOccurrences) {
    if (csOccurrences.GetLength() > 0) {
        int     iOccMin, iOccMax;
        if (!CheckOccurrences(csOccurrences, iOccMin, iOccMax))
            return false;
        m_iOccMin = iOccMin;
        m_iOccMax = iOccMax;
    }
    else {
        m_iOccMin = -1;
        m_iOccMax = -1;
    }

    return true;
}

const CapiPre76::CNewCapiText* CapiPre76::CNewCapiQuestionHelp::GetText(CString csLangName) const {
    int     iLangIndex = GetLangIndex(csLangName);
    return GetText(iLangIndex);
}

const CapiPre76::CNewCapiText* CapiPre76::CNewCapiQuestionHelp::GetText(int iLangIndex) const {
    if (iLangIndex < 0 || iLangIndex >= (int)m_aCapiText.size())
        return nullptr;
    return &(m_aCapiText[iLangIndex]);
}

int CapiPre76::CNewCapiQuestionHelp::GetNumText() const {
    return (int)m_aCapiText.size();
}

bool CapiPre76::CNewCapiQuestionHelp::SetText(CString csLangName, CString csText, bool bAppend) {
    int     iLangIndex = GetLangIndex(csLangName);

    if (iLangIndex < 0) {
        iLangIndex = GetNumText();
        m_aCapiText.resize(iLangIndex + 1);
    }

    if (bAppend) {
        if (!m_aCapiText[iLangIndex].text.empty()) {
            //m_aCapiText[iLangIndex].m_csText += "\r\n" + csText;
            // RHF COM Oct 07, 2003 m_aCapiText[iLangIndex].m_csText += " \r\n" + csText;
            m_aCapiText[iLangIndex].text.append("\r\n").append(UTF8_TODO::GetUtf8(csText)); // RHF Oct 07, 2003
        }
        else {
            m_aCapiText[iLangIndex].text = UTF8_TODO::GetUtf8(csText);
        }
    }
    else {
        m_aCapiText[iLangIndex].text = UTF8_TODO::GetUtf8(csText);
    }

    m_aCapiText[iLangIndex].language_name = UTF8_TODO::GetUtf8(csLangName);
    return true;
}

//There is zero or one CNewCapiText with the same language for a given CNewCapiQuestionHelp
int CapiPre76::CNewCapiQuestionHelp::GetLangIndex(CString csLangName) const {
    for (int iCapiText = 0; iCapiText < (int)m_aCapiText.size(); iCapiText++) {
        const CNewCapiText& rCapiText = m_aCapiText.at(iCapiText);

        if( rCapiText.language_name == UTF8_TODO::GetUtf8(csLangName) )
            return iCapiText;

        /* before was a sensitive comparation. But some times we must ensure there is insensitive
           (example : to compare the langs in one qsf file with the langs in other qsf file)
        if( rCapiText.language_name == csLangName )
            return iCapiText;*/
    }

    return -1;
}


/*static*/
bool CapiPre76::CNewCapiQuestionHelp::SplitCondition(CString csCondition, CString* csLeft, int* iCond, CString* csRight, eCapiNewConditionType* eCondType) {
    int iLocalCond = -1;
    CString csLocalLeft;
    CString csLocalRight;

    if (eCondType)
        *eCondType = CapiPre76::CNewCapiQuestionHelp::None;

    TCHAR* pLeft = csCondition.GetBuffer();
    TCHAR* p = pLeft;
    TCHAR c = 0;

    while (*p != 0) {
        if (*p == '=') {
            c = *p; *p = 0;
            iLocalCond = 0;
        }
        else if (*p == _T('<') && *(p + 1) == _T('>') ||
            *p == _T('!') && *(p + 1) == '=') {
            c = *p; *p = 0;
            iLocalCond = 1;
        }
        else if (*p == _T('>') && *(p + 1) == _T('=')) {
            c = *p; *p = 0;
            iLocalCond = 4;
        }
        else if (*p == _T('<') && *(p + 1) == _T('=')) {
            c = *p; *p = 0;
            iLocalCond = 5;
        }
        else if (*p == '>') {
            c = *p; *p = 0;
            iLocalCond = 2;
        }
        else if (*p == '<') {
            c = *p; *p = 0;
            iLocalCond = 3;
        }

        if (iLocalCond >= 0) {
            csLocalLeft = pLeft;
            *p = c;

            csLocalLeft.TrimLeft(); csLocalLeft.TrimRight();

            if (iLocalCond == 1 || iLocalCond == 4 || iLocalCond == 5)
                csLocalRight = p + 2;
            else
                csLocalRight = p + 1;

            csLocalRight.TrimLeft(); csLocalRight.TrimRight();


            // Check for " "
            p = csLocalRight.GetBuffer();
            if (*p == '"') {
                if (eCondType)
                    *eCondType = CapiPre76::CNewCapiQuestionHelp::Literal;
                int     iLen = _tcslen(p);

                if (iLen < 2 || *(p + iLen - 1) != '"')
                    iLocalCond = -1; // " mismatched
                else {
                    *(p + iLen - 1) = 0; // Delete last "

                    CString csRightAux;
                    csRightAux = p + 1; // Delete first "

                    csLocalRight = csRightAux;
                }
            }
            else if (CIMSAString::IsNumeric(csLocalRight) || SpecialValues::StringIsSpecial(UTF8_TODO::GetUtf8(csLocalRight))) {
                if (eCondType)
                    *eCondType = CapiPre76::CNewCapiQuestionHelp::Numeric;
            }
            else {
                // RHF COM Oct 28, 2003 iLocalCond = -1; // not numeric
                if (eCondType)
                    *eCondType = CapiPre76::CNewCapiQuestionHelp::Other;
            }

            break;
        }

        p++;
    }

    if (iCond) *iCond = iLocalCond;
    if (csLeft) *csLeft = csLocalLeft;
    if (csRight) *csRight = csLocalRight;

    return iLocalCond >= 0;
}

bool CapiPre76::CNewCapiQuestionHelp::CheckOccurrences(CString& csOccurrences, int& iOccMin, int& iOccMax) {
    CString csOccMin;
    CString csOccMax;
    std::vector<std::wstring> aParts = SO::SplitString(csOccurrences, ':', false);

    bool    bRet = true;
    if (aParts.size() == 2) {
        csOccMin = WS2CS(aParts[0]);
        csOccMax = WS2CS(aParts[1]);
    }
    else if (aParts.size() == 1) {
        csOccMin = WS2CS(aParts[0]);

        //FABN March 31, 2003
        bRet = false;
    }
    else {
        bRet = false;
    }


    if (bRet) {
        csOccMin.TrimLeft(); csOccMin.TrimRight();
        csOccMax.TrimLeft(); csOccMax.TrimRight();

        iOccMin = -1;
        iOccMax = -1;

        if (!CIMSAString::IsNumeric(csOccMin)) {
            bRet = false;
        }
        else {
            iOccMin = (int)CIMSAString::fVal(csOccMin);
            if (CIMSAString::fVal(csOccMin) != (double)iOccMin || iOccMin < 1)
                bRet = false;
            else if (csOccMax.GetLength() > 0) {
                if (!CIMSAString::IsNumeric(csOccMax))
                    bRet = false;
                else {
                    iOccMax = (int)CIMSAString::fVal(csOccMax);
                    if (CIMSAString::fVal(csOccMax) != (double)iOccMax || iOccMax < iOccMin)
                        bRet = false;
                }
            }
        }
    }

    return bRet;
}

/*static*/bool CapiPre76::CNewCapiQuestionHelp::CheckCondition(CString& csCondition) {

    //FABN March 12, 2003
    if (csCondition.GetLength() == 0) {
        return true;
    }

    CString csLeft;
    CString csRight;
    int iCond;
    eCapiNewConditionType  eCondType;
    bool bRet = CapiPre76::CNewCapiQuestionHelp::SplitCondition(csCondition, &csLeft, &iCond, &csRight, &eCondType);

    if (bRet) {
        TCHAR* p = csLeft.GetBuffer();
        TCHAR* q;

        // Delete ( if any
        if ((q = _tcschr(p, _T('('))) != nullptr)
            *q = 0;

        CString csLeftAux = p;

        //bRet = CheckSymbol( csLeftAux );
        bRet = CapiPre76::CNewCapiQuestionHelp::CheckSymbol(UTF8_TODO::GetUtf8(csLeftAux));

        if (bRet && eCondType == CapiPre76::CNewCapiQuestionHelp::Other) {
            p = csRight.GetBuffer();

            // Delete ( if any
            if ((q = _tcschr(p, _T('('))) != nullptr)
                *q = 0;

            CString csRightAux = p;

            bRet = CapiPre76::CNewCapiQuestionHelp::CheckSymbol(UTF8_TODO::GetUtf8(csRightAux));
        }
    }

    return bRet;
}


// DIC.VAR or VAR return true
/*static*/bool CapiPre76::CNewCapiQuestionHelp::CheckSymbol(const std::string_view symbol_name_sv)
{
    const std::vector<std::string_view> part_svs = SO::SplitString<std::string_view>(symbol_name_sv, '.');

    if( part_svs.size() > 2 )
        return false;

    for( const std::string_view part_sv : part_svs )
    {
        if( !CIMSAString::IsName(part_sv) )
            return false;
    }

    return true;
}


////////////////////////////////////////////////
////////////////////////////////////////////////


CapiPre76::CNewCapiQuestionFile::CNewCapiQuestionFile() {
    Init();
}


CapiPre76::CNewCapiQuestionFile::CNewCapiQuestionFile(const CNewCapiQuestionFile& rOther) {
    Copy(rOther);
}


void CapiPre76::CNewCapiQuestionFile::operator=(const CNewCapiQuestionFile& rOther) {
    Copy(rOther);
}

void CapiPre76::CNewCapiQuestionFile::Init() {
    m_aLangs.clear();
    m_aQuestions.clear();
    m_aHelps.clear();
}

void CapiPre76::CNewCapiQuestionFile::Copy(const CNewCapiQuestionFile& rOther) {
    Init();

    m_aLangs = rOther.m_aLangs;
    m_aQuestions = rOther.m_aQuestions;
    m_aHelps = rOther.m_aHelps;
}

void CapiPre76::CNewCapiQuestionFile::AddLanguage(CNewCapiLanguage& rNewCapiLanguage) {
    m_aLangs.emplace_back(rNewCapiLanguage);
}

const CapiPre76::CNewCapiLanguage& CapiPre76::CNewCapiQuestionFile::GetLanguage(int iLangNum) {
    return m_aLangs[iLangNum];
}

CapiPre76::CNewCapiLanguage* CapiPre76::CNewCapiQuestionFile::GetLanguage(const std::string& language_name)
{
    for (int iLang = 0; iLang < (int)m_aLangs.size(); iLang++) {
        CNewCapiLanguage& rNewCapiLanguage = m_aLangs[iLang];

        if( rNewCapiLanguage.language_name == language_name )
            return &rNewCapiLanguage;

        /* before was a sensitive comparation. But some times we must ensure there is insensitive
           (example : to compare the langs in one qsf file with the langs in other qsf file)
        if( rNewCapiLanguage.language_name == csLangName )
            return &rNewCapiLanguage;*/
    }

    return nullptr;
}

int CapiPre76::CNewCapiQuestionFile::GetNumLanguages() {
    return (int)m_aLangs.size();
}

void CapiPre76::CNewCapiQuestionFile::AddLanguages(CNewCapiQuestionHelp& rNewCapiQuestionHelp) {
    // Add Language
    for (int iText = 0; iText < rNewCapiQuestionHelp.GetNumText(); iText++) {
        const CNewCapiText* pNewCapiText = rNewCapiQuestionHelp.GetText(iText);

        if (GetLanguage(pNewCapiText->language_name) == nullptr) {
            // Add the new language
            CNewCapiLanguage    cNewCapiLanguage;

            cNewCapiLanguage.language_name = pNewCapiText->language_name;
            cNewCapiLanguage.language_label.clear();

            AddLanguage(cNewCapiLanguage);
        }
    }
}


int CapiPre76::CNewCapiQuestionFile::AddQuestion(CNewCapiQuestionHelp& rNewCapiQuestionHelp) {
    ASSERT(rNewCapiQuestionHelp.GetType() == eCapiNewQuestType::Question);

    int iQuestIdx = (int)m_aQuestions.size();
    m_aQuestions.emplace_back(rNewCapiQuestionHelp);

    AddLanguages(rNewCapiQuestionHelp);

    return iQuestIdx;
}


CapiPre76::CNewCapiQuestionHelp* CapiPre76::CNewCapiQuestionFile::GetQuestion(int iQuestNum) {
    return &m_aQuestions[iQuestNum];
}

int CapiPre76::CNewCapiQuestionFile::GetNumQuestions() {
    return (int)m_aQuestions.size();
}


int CapiPre76::CNewCapiQuestionFile::AddHelp(CNewCapiQuestionHelp& rNewCapiQuestionHelp) {

    ASSERT(rNewCapiQuestionHelp.GetType() == eCapiNewQuestType::Help);
    int iHelpIdx = (int)m_aHelps.size();
    m_aHelps.emplace_back(rNewCapiQuestionHelp);

    AddLanguages(rNewCapiQuestionHelp);

    return iHelpIdx;    //FABN
}


CapiPre76::CNewCapiQuestionHelp* CapiPre76::CNewCapiQuestionFile::GetHelp(int iHelpNum) {
    return &m_aHelps[iHelpNum];
}

int CapiPre76::CNewCapiQuestionFile::GetNumHelps() {
    return (int)m_aHelps.size();
}


int CompareCNewCapiQuestionHelp(const void* const arg1, const void* const arg2) // 20120229 for sorting questions by: 1) field 2) min occ 3) max occ
{
    const CapiPre76::CNewCapiQuestionHelp* const q1 = static_cast<const CapiPre76::CNewCapiQuestionHelp*>(arg1);
    const CapiPre76::CNewCapiQuestionHelp* const q2 = static_cast<const CapiPre76::CNewCapiQuestionHelp*>(arg2);

    const int name_comparison = SO::CompareNoCase(q1->GetSymbolName(), q2->GetSymbolName());

    return ( name_comparison != 0 )              ? name_comparison :
           ( q1->GetOccMin() < q2->GetOccMin() ) ? -1 :
           ( q1->GetOccMin() > q2->GetOccMin() ) ? 1 :
           ( q1->GetOccMax() < q2->GetOccMax() ) ? -1 :
           ( q1->GetOccMax() > q2->GetOccMax() ) ? 1 :
                                                   SO::CompareNoCase(q1->GetCondition(), q2->GetCondition());
}


bool CapiPre76::CNewCapiQuestionFile::Open(const std::string& file_path)
{
    constexpr bool bSilent = false;

    CSpecFile cCapiQuestFile;
    bool bRetVal = false;

    try {
        if (!PortableFunctions::FileIsRegular(file_path)) {
            return bRetVal;
        }

        if (cCapiQuestFile.Open(UTF8_TODO::GetCString(file_path), CFile::modeRead)) {
            CString csCmd, csArg;

            std::shared_ptr<ProgressDlg> dlgProgress;
            if (!bSilent) {
                dlgProgress = ProgressDlgFactory::Instance().Create();
                CString csWaitString;
                csWaitString.Format(_T("Checking %s File ... please wait"), FILE_TYPE);

                dlgProgress->SetStatus(csWaitString);
                dlgProgress->SetStep(1);

                int     iHigh = int(cCapiQuestFile.GetLength() / 100);
                if (iHigh == 0)
                    iHigh = 1;

                dlgProgress->SetRange(0, iHigh);   // avoid probs with DD's > 65K bytes
            }


            bRetVal = Build(cCapiQuestFile, bSilent ? nullptr : dlgProgress);

            if (!bSilent) {
                if (dlgProgress->CheckCancelButton() || !bRetVal) {
                    CString     csMsg;
                    csMsg.Format(_T("%s load canceled"), FILE_TYPE2);
                    ErrorMessage::Display(csMsg);
                    bRetVal = false;
                }
                else {
                    dlgProgress->SetPos(int(cCapiQuestFile.GetLength() / 100));
                }
            }
            cCapiQuestFile.Close();

            // 20120229 sort the occurrences so that they're sorted by the Min Occ
            qsort(m_aQuestions.data(), m_aQuestions.size(), sizeof(CNewCapiQuestionHelp), CompareCNewCapiQuestionHelp);
        }
    }
    catch (...) {
        bRetVal = false;
    }

    return bRetVal;
}


bool CapiPre76::CNewCapiQuestionFile::Build(CSpecFile& cCapiQuestFile, std::shared_ptr<ProgressDlg> pDlgProgress)
{
    CString csCmd, csArg;
    CString csMsg, csError;
    bool bRetVal = true;

    bool    bSilent = (pDlgProgress == nullptr);

    Init(); // Clean All arrays

    int     iNumLines = 0;
    try
    {

        while (cCapiQuestFile.GetLine(csCmd, csArg, false) == SF_OK) { // RHF Jul 10, 2002 Add false for avoid trim
            iNumLines++;
            csCmd.TrimRight(); csCmd.TrimLeft();
            //csArg.Remove( '\n' );
            csArg.TrimRight();

            if (csCmd.GetLength() == 0) continue;

            if (csCmd[0] == '[') {
                if (pDlgProgress) {
                    pDlgProgress->SetPos(int(cCapiQuestFile.GetPosition() / 100));
                    if (pDlgProgress->CheckCancelButton())
                        return false;
                }

                if (csCmd.CompareNoCase(HEAD_STAT) == 0) { // [CAPI QUESTIONS]
                    CString csVersion = Versioning::CSProVersionText;
                    if (!cCapiQuestFile.IsVersionOK_CS(csVersion)) {
                        if (!IsValidCSProVersion(UTF8_TODO::GetUtf8(csVersion), 2.5)) {
                            if (!bSilent) {
                                const std::string message = FormatText("%s is not %s", UTF8_TODO::GetUtf8(FILE_TYPE2).c_str(), Versioning::CSProVersionText); // 20100601 added CSPro version to stop crashes
                                ErrorMessage::Display(message);
                            }
                            return false;
                        }
                    }
                }

                else if (csCmd.CompareNoCase(HEAD_LANGUAGES) == 0) { // [LANGUAGES]
                    while (cCapiQuestFile.GetLine(csCmd, csArg, false) == SF_OK) { // RHF Jul 10, 2002 Add false for avoid trim
                        csCmd.TrimRight(); csCmd.TrimLeft();
                        //csArg.Remove( '\n' );
                        csArg.TrimRight();

                        if (csCmd.GetLength() == 0) continue;

                        if (csCmd[0] == '[') {
                            cCapiQuestFile.UngetLine();
                            break;
                        }
                        else {//Assumes as languages  Example: ENG=English
                            CNewCapiLanguage    cNewCapiLanguage;

                            if (!CIMSAString::IsName(csCmd)) {
                                if (!bSilent) {
                                    csError.Format(_T("Invalid Language Name at line %d"), cCapiQuestFile.GetLineNumber()); // Invalid section heading at line %d:
                                    csError += _T("\n") + csCmd;
                                    ErrorMessage::Display(csError);
                                }
                                bRetVal = false;
                                continue;
                            }

                            if (GetLanguage(UTF8_TODO::GetUtf8(csCmd)) == nullptr) {
                                cNewCapiLanguage.language_name = UTF8_TODO::GetUtf8(csCmd);
                                cNewCapiLanguage.language_label = UTF8_TODO::GetUtf8(csArg);

                                AddLanguage(cNewCapiLanguage);
                            }
                        }
                    }
                }
                else if (csCmd.CompareNoCase(HEAD_QUESTION) == 0 ||
                    csCmd.CompareNoCase(HEAD_HELP) == 0 ||
                    csCmd.CompareNoCase(HEAD_INSTRUCTION) == 0
                    ) { // [QUESTION] / [HELP]
                    bool bQuestion = (csCmd.CompareNoCase(HEAD_QUESTION) == 0);
                    bool bHelp = (csCmd.CompareNoCase(HEAD_HELP) == 0);
                    bool bInstruction = (csCmd.CompareNoCase(HEAD_INSTRUCTION) == 0);

                    CNewCapiQuestionHelp  cNewCapiQuestionHelp;

                    //FABN May 8, 2003
                    std::map<CString, bool> aMapAux;

                    cNewCapiQuestionHelp.SetType(bQuestion ? eCapiNewQuestType::Question :
                        eCapiNewQuestType::Help);


                    while (cCapiQuestFile.GetLine(csCmd, csArg, false) == SF_OK) {
                        csCmd.TrimRight(); csCmd.TrimLeft();
                        //csArg.Remove( '\n' );
                        csArg.TrimRight();

                        if (csCmd.GetLength() == 0) continue;

                        if (csCmd[0] == '[') {
                            cCapiQuestFile.UngetLine();
                            break;
                        }
                        else if (csCmd.CompareNoCase(CMD_FIELD) == 0) {   // "Field"
                            if (!cNewCapiQuestionHelp.CheckSymbol(UTF8_TODO::GetUtf8(csArg))) {
                                if (!bSilent) {
                                    csError.Format(_T("Invalid Symbol at line %d"), cCapiQuestFile.GetLineNumber()); // Invalid section heading at line %d:
                                    csError += _T("\n") + csCmd;
                                    ErrorMessage::Display(csError);
                                }
                                bRetVal = false;
                                continue;
                            }
                            cNewCapiQuestionHelp.SetSymbolName(UTF8_TODO::GetUtf8(csArg));
                        }
                        else if (csCmd.CompareNoCase(CMD_CONDITION) == 0) {   // "Condition"
                            if (!cNewCapiQuestionHelp.CheckCondition(csArg)) {
                                if (!bSilent) {
                                    csError.Format(_T("Invalid Condition at line %d"), cCapiQuestFile.GetLineNumber()); // Invalid section heading at line %d:
                                    csError += _T("\n") + csCmd;
                                    ErrorMessage::Display(csError);
                                }
                                bRetVal = false;
                                continue;
                            }
                            cNewCapiQuestionHelp.SetCondition(csArg);
                        }
                        else if (csCmd.CompareNoCase(CMD_OCCURRENCES) == 0) {  // "Occurrences"
                            int     iOccMin, iOccMax;

                            //FABN March 31, 2003
                            //the format must be CMD_OCCURRENCES=occmin:occmax
                            //CMD_OCCURRENCES=occ <- invalid!


                            if (!cNewCapiQuestionHelp.CheckOccurrences(csArg, iOccMin, iOccMax)) {
                                if (!bSilent) {
                                    csError.Format(_T("Invalid Occurrences at line %d"), cCapiQuestFile.GetLineNumber()); // Invalid section heading at line %d:
                                    csError += _T("\n") + csCmd;
                                    ErrorMessage::Display(csError);
                                }
                                bRetVal = false;
                                continue;
                            }

                            if (iOccMin >= 1)
                                cNewCapiQuestionHelp.SetOccMin(iOccMin);

                            if (iOccMax >= iOccMin)
                                cNewCapiQuestionHelp.SetOccMax(iOccMax);

                            cNewCapiQuestionHelp.SetOccurrences(csArg);
                        }
                        else { //Assumes as question in a specific language Example: ENG=Question Text
                            if (!CIMSAString::IsName(csCmd)) {
                                if (!bSilent) {
                                    csError.Format(_T("Invalid Language Name at line %d"), cCapiQuestFile.GetLineNumber()); // Invalid section heading at line %d:
                                    csError += _T("\n") + csCmd;
                                    ErrorMessage::Display(csError);
                                }
                                bRetVal = false;
                                continue;
                            }

                            if (!cNewCapiQuestionHelp.GetText(csCmd)) {
                                aMapAux[csCmd] = true;
                            }
                            else {
                                bool bRtf = aMapAux[csCmd];
                                if (!bRtf) {
                                    csArg += _T("\\par");
                                }
                            }

                            cNewCapiQuestionHelp.SetText(csCmd, csArg, true);
                        }
                    }

                    //FABN March 15, 2003
#ifdef _DEBUG
                    ASSERT(!cNewCapiQuestionHelp.GetSymbolName().empty());
#endif
                    if (cNewCapiQuestionHelp.GetSymbolName().empty()) {
                        if (!bSilent) {
                            csError.Format(_T("Symbol name not found, at line %d"), cCapiQuestFile.GetLineNumber()); // symbol name not found
                            csError += _T("\n") + csCmd;
                            ErrorMessage::Display(csError);
                        }

                        bRetVal = false;
                        continue;
                    }


                    //FABN March 27, 2003
                    /*to correct corrupted files saved by early versions of qsfedit*/
                    /*so, if no occs nor condition => it must have some text*/
                    bool bAddOK = true;
                    if (cNewCapiQuestionHelp.GetOccMin() == -1 && cNewCapiQuestionHelp.GetOccMax() == -1 && cNewCapiQuestionHelp.GetCondition().empty()) {
                        int iNumLangs = cNewCapiQuestionHelp.GetNumText();

                        bool bExistSomeText = false;
                        for (int iLangIdx = 0; !bExistSomeText && iLangIdx < iNumLangs; iLangIdx++) {
                            bExistSomeText = !cNewCapiQuestionHelp.GetText(iLangIdx)->text.empty();
                        }

                        bAddOK = bAddOK && bExistSomeText;
                    }
                    if (!bAddOK) {
                        continue;
                    }

                    //FABN May 8, 2003
                    CNewCapiText* pTextAux;
                    for( const auto& [csLangNameAux, bWasRtf] : aMapAux ) {
                        if (!bWasRtf) {
                            pTextAux = const_cast<CNewCapiText*>(cNewCapiQuestionHelp.GetText(csLangNameAux));
                            pTextAux->text.push_back('}');
                        }
                    }

                    if (bQuestion)
                        AddQuestion(cNewCapiQuestionHelp);
                    else if (bHelp)
                        AddHelp(cNewCapiQuestionHelp);
                    else if (bInstruction)
                        ; // unimplemented instructions were removed for CSPro 6.2
                    else
                        ASSERT(0);
                }
                else {
                    if (!bSilent) {
                        csError.Format(_T("Invalid section heading at line %d:"), cCapiQuestFile.GetLineNumber()); // Invalid section heading at line %d:
                        csError += _T("\n") + csCmd;
                        ErrorMessage::Display(csError);
                    }
                    return false;
                }
            }

            else {
                if (!bSilent) {
                    csError.Format(_T("Invalid line at %d"), cCapiQuestFile.GetLineNumber()); // Unrecognized command at line %d:
                    csError += _T("\n") + csCmd + _T("=") + csArg;

                    ErrorMessage::Display(csError);
                }
                return false;
            }
        }
    }
    catch (...) {
        return false;
    }


    // FABN Aug 19, 2003
    if (m_aLangs.empty()) {
        if (!bSilent) {
            ErrorMessage::Display(_T("No languages found."));
        }
        return false;
    }
    // FABN Aug 19, 2003

    return (iNumLines > 0); // && bRetVal
}
