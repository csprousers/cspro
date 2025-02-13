#include "StdAfx.h"
#include "CapiQuestionFilePre76.h"
#include <zToolsO/Special.h>
#include <zUtilO/Specfile.h>
#include <zUtilF/ProgressDlg.h>
#include <zUtilF/ProgressDlgFactory.h>


namespace CapiPre76 {

#define FILE_TYPE           _T("Question")
#define FILE_TYPE2          _T("Question File")
#define HEAD_STAT           _T("[CAPI QUESTIONS]")
#define HEAD_LANGUAGES      _T("[LANGUAGES]")
#define HEAD_QUESTION       _T("[QUESTION]")
#define HEAD_HELP           _T("[HELP]")
#define HEAD_INSTRUCTION    _T("[INSTRUCTION]")
#define CMD_FIELD           _T("Field")
#define CMD_CONDITION       _T("Condition")
#define CMD_OCCURRENCES     _T("Occurrences")



///////////////////////////////////////////////////////
CNewCapiLanguage::CNewCapiLanguage() {
    Init();
}

CNewCapiLanguage::~CNewCapiLanguage() {
}


CNewCapiLanguage::CNewCapiLanguage(const CNewCapiLanguage& rOther) {
    Copy(rOther);
}


void CNewCapiLanguage::operator=(const CNewCapiLanguage& rOther) {
    Copy(rOther);
}

void CNewCapiLanguage::Init() {
    m_iLangIndex = -1;
    m_csLangName.Empty();
    m_csLangLabel.Empty();
}

void CNewCapiLanguage::Copy(const CNewCapiLanguage& rOther) {
    Init();

    m_iLangIndex = rOther.m_iLangIndex;
    m_csLangName = rOther.m_csLangName;
    m_csLangLabel = rOther.m_csLangLabel;
}

bool CNewCapiLanguage::CheckLanguageName(CIMSAString& csLanguage) {
    return csLanguage.IsName();
}

////////////////////////////////////////////////

///////////////////////////////////////////////////////
CNewCapiText::CNewCapiText() {
    Init();
}

CNewCapiText::~CNewCapiText() {
}


CNewCapiText::CNewCapiText(const CNewCapiText& rOther) {
    Copy(rOther);
}


void CNewCapiText::operator=(const CNewCapiText& rOther) {
    Copy(rOther);
}

void CNewCapiText::Init() {
    m_csLangName.Empty();
    m_csText.Empty();

    //FABN Apr 10, 2003
    m_bDeleted = false;
}

void CNewCapiText::Copy(const CNewCapiText& rOther) {
    Init();

    m_csLangName = rOther.m_csLangName;
    m_csText = rOther.m_csText;

    //FABN Apr 10, 2003
    m_bDeleted = rOther.m_bDeleted;
}



///////////////////////////////////////////////////////
CNewCapiQuestionHelp::CNewCapiQuestionHelp() {
    Init();
}

CNewCapiQuestionHelp::~CNewCapiQuestionHelp() {
}


CNewCapiQuestionHelp::CNewCapiQuestionHelp(const CNewCapiQuestionHelp& rOther) {
    Copy(rOther);
}


void CNewCapiQuestionHelp::operator=(const CNewCapiQuestionHelp& rOther) {
    Copy(rOther);
}

void CNewCapiQuestionHelp::Init() {
    m_eType = eCapiNewQuestType::None;
    m_csSymbolName.Empty();
    m_iSymVar = -1;
    m_iOccMin = -1;
    m_iOccMax = -1;
    m_csCondition.Empty();
    m_csOccurrences.Empty();

    m_aCapiText.clear();

    m_bDeleted = false; //FABN March 14, 2003
}

void CNewCapiQuestionHelp::Copy(const CNewCapiQuestionHelp& rOther) {
    Init();

    m_eType = rOther.m_eType;
    m_csSymbolName = rOther.m_csSymbolName;
    m_iSymVar = rOther.m_iSymVar;
    m_iOccMin = rOther.m_iOccMin;
    m_iOccMax = rOther.m_iOccMax;
    m_csCondition = rOther.m_csCondition;
    m_csOccurrences = rOther.m_csOccurrences;
    m_aCapiText = rOther.m_aCapiText;
    m_bDeleted = rOther.m_bDeleted;        //FABN March 14, 2003
}

eCapiNewQuestType     CNewCapiQuestionHelp::GetType() {
    return m_eType;
}
void CNewCapiQuestionHelp::SetType(eCapiNewQuestType eType) {
    m_eType = eType;
}

CString CNewCapiQuestionHelp::GetSymbolName() {
    return m_csSymbolName;
}

bool CNewCapiQuestionHelp::SetSymbolName(CString csSymbolName) {
    if (!CheckSymbol(csSymbolName))
        return false;

    m_csSymbolName = csSymbolName;

    return true;
}

int CNewCapiQuestionHelp::GetSymVar() {
    return m_iSymVar;
}
void CNewCapiQuestionHelp::SetSymVar(int iSymVar) {
    m_iSymVar = iSymVar;
}

int CNewCapiQuestionHelp::GetOccMin() const {
    return m_iOccMin;
}
void CNewCapiQuestionHelp::SetOccMin(int iOccMin) {
    m_csOccurrences.Empty();
    m_iOccMin = iOccMin;
}

int CNewCapiQuestionHelp::GetOccMax() const {
    return m_iOccMax;
}

void CNewCapiQuestionHelp::SetOccMax(int iOccMax) {
    m_csOccurrences.Empty();
    m_iOccMax = iOccMax;
}

CString CNewCapiQuestionHelp::GetCondition() {
    return m_csCondition;
}

bool CNewCapiQuestionHelp::SetCondition(CString csCondition) {
    if (csCondition.GetLength() > 0) {
        if (!CheckCondition(csCondition))
            return false;
    }

    m_csCondition = csCondition;
    return true;
}

CString CNewCapiQuestionHelp::GetOccurrences() {
    return m_csOccurrences;
}

bool CNewCapiQuestionHelp::SetOccurrences(CString csOccurrences) {
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
    m_csOccurrences = csOccurrences;

    return true;
}

CNewCapiText* CNewCapiQuestionHelp::GetText(CString csLangName) {
    int     iLangIndex = GetLangIndex(csLangName);
    return GetText(iLangIndex);
}

CNewCapiText* CNewCapiQuestionHelp::GetText(int iLangIndex) {
    if (iLangIndex < 0 || iLangIndex >= (int)m_aCapiText.size())
        return NULL;
    return &(m_aCapiText[iLangIndex]);
}

int CNewCapiQuestionHelp::GetNumText() {
    return (int)m_aCapiText.size();
}

bool CNewCapiQuestionHelp::SetText(CString csLangName, CString csText, bool bAppend) {
    int     iLangIndex = GetLangIndex(csLangName);

    if (iLangIndex < 0) {
        iLangIndex = GetNumText();
        SetMaxLanguages(iLangIndex + 1);
    }

    if (bAppend) {
        if (m_aCapiText[iLangIndex].m_csText.GetLength() > 0) {
            //m_aCapiText[iLangIndex].m_csText += "\r\n" + csText;
            // RHF COM Oct 07, 2003 m_aCapiText[iLangIndex].m_csText += " \r\n" + csText;
            m_aCapiText[iLangIndex].m_csText += _T("\r\n") + csText; // RHF Oct 07, 2003

#ifdef _DEBUG
            CString csAux = m_aCapiText[iLangIndex].m_csText;
#endif
        }
        else
            m_aCapiText[iLangIndex].m_csText = csText;
    }
    else
        m_aCapiText[iLangIndex].m_csText = csText;
    m_aCapiText[iLangIndex].m_csLangName = csLangName;
    return true;
}

//There is zero or one CNewCapiText with the same language for a given CNewCapiQuestionHelp
int CNewCapiQuestionHelp::GetLangIndex(CString csLangName, bool bCaseSensitive /*=true*/) {
    for (int iCapiText = 0; iCapiText < (int)m_aCapiText.size(); iCapiText++) {
        CNewCapiText& rCapiText = m_aCapiText[iCapiText];

        if ((bCaseSensitive && rCapiText.m_csLangName.Compare(csLangName) == 0) ||
            (!bCaseSensitive && rCapiText.m_csLangName.CompareNoCase(csLangName) == 0)) {
            return iCapiText;
        }
        /* before was a sensitive comparation. But some times we must ensure there is insensitive
           (example : to compare the langs in one qsf file with the langs in other qsf file)
        if( rCapiText.m_csLangName == csLangName )
            return iCapiText;*/
    }

    return -1;
}

void CNewCapiQuestionHelp::RemoveTextAt(int iLangIndex) {
    m_aCapiText.erase(m_aCapiText.begin() + iLangIndex);
}

void CNewCapiQuestionHelp::SetMaxLanguages(int iNumLanguages)
{
    m_aCapiText.resize(iNumLanguages);
}



/*static*/
bool CNewCapiQuestionHelp::SplitCondition(CString csCondition, CIMSAString* csLeft, int* iCond, CIMSAString* csRight, eCapiNewConditionType* eCondType) {
    int         iLocalCond = -1;
    CIMSAString csLocalLeft;
    CIMSAString csLocalRight;

    if (eCondType) *eCondType = CNewCapiQuestionHelp::None;

    csprochar* pLeft = csCondition.GetBuffer();
    csprochar* p = pLeft;
    csprochar    c = 0;

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
                if (eCondType) *eCondType = CNewCapiQuestionHelp::Literal;
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
            else if (csLocalRight.IsNumeric() || SpecialValues::StringIsSpecial(csLocalRight)) {
                if (eCondType) *eCondType = CNewCapiQuestionHelp::Numeric;
            }
            else {
                // RHF COM Oct 28, 2003 iLocalCond = -1; // not numeric
                if (eCondType) *eCondType = CNewCapiQuestionHelp::Other;
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

bool CNewCapiQuestionHelp::CheckOccurrences(CString& csOccurrences, int& iOccMin, int& iOccMax) {
    CIMSAString csOccMin;
    CIMSAString csOccMax;
    std::vector<std::wstring> aParts = SO::SplitString(csOccurrences, ':', false);;

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

        if (!csOccMin.IsNumeric())
            bRet = false;
        else {
            iOccMin = (int)csOccMin.fVal();
            if (csOccMin.fVal() != (double)iOccMin || iOccMin < 1)
                bRet = false;
            else if (csOccMax.GetLength() > 0) {
                if (!csOccMax.IsNumeric())
                    bRet = false;
                else {
                    iOccMax = (int)csOccMax.fVal();
                    if (csOccMax.fVal() != (double)iOccMax || iOccMax < iOccMin)
                        bRet = false;
                }
            }
        }
    }

    return bRet;
}

/*static*/bool CNewCapiQuestionHelp::CheckCondition(CString& csCondition) {

    //FABN March 12, 2003
    if (csCondition.GetLength() == 0) {
        return true;
    }


    CIMSAString  csLeft;

    // RHF COM Oct 28, 2003 bool bRet = CNewCapiQuestionHelp::SplitCondition( csCondition, &csLeft );


    CIMSAString  csRight;
    int          iCond;
    eCapiNewConditionType  eCondType;
    bool bRet = CNewCapiQuestionHelp::SplitCondition(csCondition, &csLeft, &iCond, &csRight, &eCondType);

    if (bRet) {
        csprochar* p = csLeft.GetBuffer();
        csprochar* q;

        // Delete ( if any
        if ((q = _tcschr(p, _T('('))) != NULL)
            *q = 0;

        CIMSAString   csLeftAux;

        csLeftAux = p;

        //bRet = CheckSymbol( csLeftAux );
        bRet = CNewCapiQuestionHelp::CheckSymbol(csLeftAux);

        if (bRet && eCondType == CNewCapiQuestionHelp::Other) {
            p = csRight.GetBuffer();

            // Delete ( if any
            if ((q = _tcschr(p, _T('('))) != NULL)
                *q = 0;

            CIMSAString   csRightAux;

            csRightAux = p;

            bRet = CNewCapiQuestionHelp::CheckSymbol(csRightAux);
        }
    }

    return bRet;
}


// DIC.VAR or VAR return true
/*static*/bool CNewCapiQuestionHelp::CheckSymbol(const CString& csSymbolName)
{
    std::vector<std::wstring> aParts = SO::SplitString(csSymbolName, '.');

    if( aParts.size() > 2 )
        return false;

    for( const std::wstring& part : aParts )
    {
        if( !CIMSAString::IsName(part) )
            return false;
    }

    return true;
}

////////////////////////////////////////////////

////////////////////////////////////////////////


///////////////////////////////////////////////////////
CNewCapiQuestionFile::CNewCapiQuestionFile() {

    m_bIsModified = false;

    Init(false);
}

CNewCapiQuestionFile::~CNewCapiQuestionFile() {
}


CNewCapiQuestionFile::CNewCapiQuestionFile(const CNewCapiQuestionFile& rOther) {
    Copy(rOther);
}


void CNewCapiQuestionFile::operator=(const CNewCapiQuestionFile& rOther) {
    Copy(rOther);
}

void CNewCapiQuestionFile::Init(bool bOnlyArrays) {
    if (!bOnlyArrays) {
        m_csFileName.Empty();
    }

    m_aLangs.clear();
    m_aQuestions.clear();
    m_aHelps.clear();
    m_bIsModified = false;
}

void CNewCapiQuestionFile::Copy(const CNewCapiQuestionFile& rOther) {
    Init(false);

    m_csFileName = rOther.m_csFileName;
    m_aLangs = rOther.m_aLangs;
    m_aQuestions = rOther.m_aQuestions;
    m_aHelps = rOther.m_aHelps;
    m_bIsModified = rOther.m_bIsModified;
}

void CNewCapiQuestionFile::SetFileName(CString csFileName) {
    m_csFileName = csFileName;
}

CString CNewCapiQuestionFile::GetFileName() {
    return m_csFileName;
}

void CNewCapiQuestionFile::AddLanguage(CNewCapiLanguage& rNewCapiLanguage) {
    rNewCapiLanguage.m_iLangIndex = (int)m_aLangs.size();

    m_aLangs.emplace_back(rNewCapiLanguage);
}

CNewCapiLanguage* CNewCapiQuestionFile::GetLanguage(int iLangNum) {
    return &m_aLangs[iLangNum];
}

CNewCapiLanguage* CNewCapiQuestionFile::GetLanguage(CString csLangName, bool bCaseSensitive /*=true*/) {
    for (int iLang = 0; iLang < (int)m_aLangs.size(); iLang++) {
        CNewCapiLanguage& rNewCapiLanguage = m_aLangs[iLang];

        if ((bCaseSensitive && rNewCapiLanguage.m_csLangName.Compare(csLangName) == 0) ||
            (!bCaseSensitive && rNewCapiLanguage.m_csLangName.CompareNoCase(csLangName) == 0)) {
            return &rNewCapiLanguage;
        }
        /* before was a sensitive comparation. But some times we must ensure there is insensitive
           (example : to compare the langs in one qsf file with the langs in other qsf file)
        if( rNewCapiLanguage.m_csLangName == csLangName )
            return &rNewCapiLanguage;*/
    }

    return NULL;
}

int CNewCapiQuestionFile::GetNumLanguages() {
    return (int)m_aLangs.size();
}

void CNewCapiQuestionFile::AddLanguages(CNewCapiQuestionHelp& rNewCapiQuestionHelp) {
    // Add Language
    for (int iText = 0; iText < rNewCapiQuestionHelp.GetNumText(); iText++) {
        CNewCapiText* pNewCapiText = rNewCapiQuestionHelp.GetText(iText);

        if (GetLanguage(pNewCapiText->m_csLangName) == NULL) {
            // Add the new language
            CNewCapiLanguage    cNewCapiLanguage;

            cNewCapiLanguage.m_csLangName = pNewCapiText->m_csLangName;
            cNewCapiLanguage.m_csLangLabel = _T("");

            AddLanguage(cNewCapiLanguage);
        }
    }
}

//FABN Apr 14, 2003
int CNewCapiQuestionFile::AddCapiQuest(CNewCapiQuestionHelp& rCapiQuest)
{
    int iQuestIdx = -1;
    eCapiNewQuestType eCapiType = rCapiQuest.GetType();
    switch (eCapiType) {
    case eCapiNewQuestType::Question:
        iQuestIdx = AddQuestion(rCapiQuest);
        break;

    case eCapiNewQuestType::Help:
        iQuestIdx = AddHelp(rCapiQuest);
        break;

    default:
        ASSERT(0);
        break;
    }
    return iQuestIdx;
}


int CNewCapiQuestionFile::AddQuestion(CNewCapiQuestionHelp& rNewCapiQuestionHelp) {
    ASSERT(rNewCapiQuestionHelp.GetType() == eCapiNewQuestType::Question);

    int iQuestIdx = (int)m_aQuestions.size();
    m_aQuestions.emplace_back(rNewCapiQuestionHelp);

    AddLanguages(rNewCapiQuestionHelp);

    return iQuestIdx;
}


CNewCapiQuestionHelp* CNewCapiQuestionFile::GetQuestion(int iQuestNum) {
    return &m_aQuestions[iQuestNum];
}

int CNewCapiQuestionFile::GetNumQuestions() {
    return (int)m_aQuestions.size();
}


int CNewCapiQuestionFile::AddHelp(CNewCapiQuestionHelp& rNewCapiQuestionHelp) {

    ASSERT(rNewCapiQuestionHelp.GetType() == eCapiNewQuestType::Help);
    int iHelpIdx = (int)m_aHelps.size();
    m_aHelps.emplace_back(rNewCapiQuestionHelp);

    AddLanguages(rNewCapiQuestionHelp);

    return iHelpIdx;    //FABN
}


CNewCapiQuestionHelp* CNewCapiQuestionFile::GetHelp(int iHelpNum) {
    return &m_aHelps[iHelpNum];
}

int CNewCapiQuestionFile::GetNumHelps() {
    return (int)m_aHelps.size();
}







int CompareCNewCapiQuestionHelp(const void* arg1, const void* arg2) // 20120229 for sorting questions by: 1) field 2) min occ 3) max occ
{
    CNewCapiQuestionHelp* q1 = (CNewCapiQuestionHelp*)arg1;
    CNewCapiQuestionHelp* q2 = (CNewCapiQuestionHelp*)arg2;

    int comparison = q1->GetSymbolName().CompareNoCase(q2->GetSymbolName());

    if (comparison)
        return comparison;

    if (q1->GetOccMin() < q2->GetOccMin())
        return -1;

    else if (q1->GetOccMin() > q2->GetOccMin())
        return 1;

    else if (q1->GetOccMax() < q2->GetOccMax())
        return -1;

    else if (q1->GetOccMax() > q2->GetOccMax())
        return 1;

    return q1->GetCondition().CompareNoCase(q2->GetCondition());
}


bool CNewCapiQuestionFile::Open(const CString& csFileName, bool bSilent)
{
    SetFileName(csFileName);

    CSpecFile   cCapiQuestFile;
    bool        bRetVal = false;

    try {
        if (!PortableFunctions::FileIsRegular(csFileName)) {
            if (!bSilent) {
                CString csMsg;
                csMsg.Format(_T("%s %s does not exist"), FILE_TYPE2, (LPCTSTR)csFileName);

                // RHF COM Nov 22, 2002 Uncommented soon! ErrorMessage::Display(csMsg);
            }
            return bRetVal;
        }
        if (cCapiQuestFile.Open(csFileName, CFile::modeRead)) {

            CIMSAString csCmd, csArg;

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


bool CNewCapiQuestionFile::Build(CSpecFile& cCapiQuestFile, std::shared_ptr<ProgressDlg> pDlgProgress)
{
    CIMSAString     csCmd, csArg;
    CString         csMsg, csError;
    bool            bRetVal = true;

    bool    bSilent = (pDlgProgress == NULL);

    Init(true); // Clean All arrays

    int     iNumLines = 0;
    try
    {

        while (cCapiQuestFile.GetLine(csCmd, csArg, false) == SF_OK) { // RHF Jul 10, 2002 Add false for avoid trim
            iNumLines++;
            csCmd.TrimRight(); csCmd.TrimLeft();
            //csArg.Remove( (csprochar) '\n' );
            csArg.TrimRight();

            if (csCmd.GetLength() == 0) continue;

            if (csCmd[0] == '[') {
                if (pDlgProgress) {
                    pDlgProgress->SetPos(int(cCapiQuestFile.GetPosition() / 100));
                    if (pDlgProgress->CheckCancelButton())
                        return false;
                }

                if (csCmd.CompareNoCase(HEAD_STAT) == 0) { // [CAPI QUESTIONS]
                    CIMSAString csVersion = CSPRO_VERSION;
                    if (!cCapiQuestFile.IsVersionOK(csVersion)) {
                        if (!IsValidCSProVersion(csVersion, 2.5)) {
                            if (!bSilent) {
                                csError.Format(_T("%s is not %s"), FILE_TYPE2, CSPRO_VERSION); // 20100601 added CSPro version to stop crashes
                                ErrorMessage::Display(csError);
                            }
                            return false;
                        }
                    }
                }

                else if (csCmd.CompareNoCase(HEAD_LANGUAGES) == 0) { // [LANGUAGES]
                    while (cCapiQuestFile.GetLine(csCmd, csArg, false) == SF_OK) { // RHF Jul 10, 2002 Add false for avoid trim
                        csCmd.TrimRight(); csCmd.TrimLeft();
                        //csArg.Remove( (csprochar) '\n' );
                        csArg.TrimRight();

                        if (csCmd.GetLength() == 0) continue;

                        if (csCmd[0] == '[') {
                            cCapiQuestFile.UngetLine();
                            break;
                        }
                        else {//Assumes as languages  Example: ENG=English
                            CNewCapiLanguage    cNewCapiLanguage;

                            if (!CNewCapiLanguage::CheckLanguageName(csCmd)) {
                                if (!bSilent) {
                                    csError.Format(_T("Invalid Language Name at line %d"), cCapiQuestFile.GetLineNumber()); // Invalid section heading at line %d:
                                    csError += _T("\n") + csCmd;
                                    ErrorMessage::Display(csError);
                                }
                                bRetVal = false;
                                continue;
                            }

                            if (GetLanguage(csCmd) == NULL) {
                                cNewCapiLanguage.m_csLangName = csCmd;
                                cNewCapiLanguage.m_csLangLabel = csArg;

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
                        //csArg.Remove( (csprochar) '\n' );
                        csArg.TrimRight();

                        if (csCmd.GetLength() == 0) continue;

                        if (csCmd[0] == '[') {
                            cCapiQuestFile.UngetLine();
                            break;
                        }
                        else if (csCmd.CompareNoCase(CMD_FIELD) == 0) {   // "Field"
                            if (!cNewCapiQuestionHelp.CheckSymbol(csArg)) {
                                if (!bSilent) {
                                    csError.Format(_T("Invalid Symbol at line %d"), cCapiQuestFile.GetLineNumber()); // Invalid section heading at line %d:
                                    csError += _T("\n") + csCmd;
                                    ErrorMessage::Display(csError);
                                }
                                bRetVal = false;
                                continue;
                            }
                            cNewCapiQuestionHelp.SetSymbolName(csArg);
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
                            if (!CNewCapiLanguage::CheckLanguageName(csCmd)) {
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
                    ASSERT(cNewCapiQuestionHelp.GetSymbolName().GetLength() > 0);
#endif
                    if (cNewCapiQuestionHelp.GetSymbolName().GetLength() == 0) {
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
                    if (cNewCapiQuestionHelp.GetOccMin() == -1 && cNewCapiQuestionHelp.GetOccMax() == -1 && cNewCapiQuestionHelp.GetCondition().GetLength() == 0) {
                        int iNumLangs = cNewCapiQuestionHelp.GetNumText();

                        bool bExistSomeText = false;
                        for (int iLangIdx = 0; !bExistSomeText && iLangIdx < iNumLangs; iLangIdx++) {
                            bExistSomeText = (cNewCapiQuestionHelp.GetText(iLangIdx)->m_csText.GetLength() > 0);
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
                            pTextAux = cNewCapiQuestionHelp.GetText(csLangNameAux);
                            pTextAux->m_csText = pTextAux->m_csText + _T("}");
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


void CNewCapiText::serialize(Serializer& ar) // 20121109
{
    ar & m_csLangName
       & m_csText
       & m_bDeleted;
}

void CNewCapiLanguage::serialize(Serializer& ar) // 20121109
{
    ar & m_iLangIndex
       & m_csLangName
       & m_csLangLabel;
}

void CNewCapiQuestionHelp::serialize(Serializer& ar) // 20121109
{
    ar.SerializeEnum(m_eType)
      & m_csSymbolName
      & m_iSymVar
      & m_iOccMin
      & m_iOccMax
      & m_csCondition
      & m_csOccurrences
      & m_bDeleted
      & m_aCapiText;
}

void CNewCapiQuestionFile::serialize(Serializer& ar) // 20121109
{
    ar.SerializeFilename(m_csFileName)
      & m_aLangs
      & m_aQuestions
      & m_aHelps
      & m_bIsModified;
}

}
