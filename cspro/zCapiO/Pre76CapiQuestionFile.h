#pragma once

class CSpecFile;
class ProgressDlg;
namespace CapiPre76 { class CNewCapiLanguage; class CNewCapiText; class CNewCapiQuestionFile; class CNewCapiQuestionHelp; }


class CapiPre76::CNewCapiText
{
private:
    void Init();
    void Copy(const CNewCapiText& rOther);

public:
    std::string language_name;
    std::string text; // Separator \r\n.

    CNewCapiText();
    CNewCapiText(const CNewCapiText& rOther);
    void operator=(const CNewCapiText& rOther);

    //FABN Apr 10, 2003 need support to delete only a single language for a given CNewCapiQuestionHelp
    bool    m_bDeleted;
};


class CapiPre76::CNewCapiLanguage
{
private:
    void Init();
    void Copy(const CNewCapiLanguage& rOther);

public:
    std::string language_name;
    std::string language_label;

    CNewCapiLanguage();
    CNewCapiLanguage(const CNewCapiLanguage& rOther);
    void operator=(const CNewCapiLanguage& rOther);
};

enum class eCapiNewQuestType { None, Question, Help };

/////////////////////////////
class CapiPre76::CNewCapiQuestionHelp
{
    friend class CNewCapiQuestionFile;  // For save method

private:
    eCapiNewQuestType       m_eType;
    std::string             m_symbolName;
    int                     m_iSymVar;
    int                     m_iOccMin;      // negative, 1,2,...n
    int                     m_iOccMax;
    std::string             m_condition;  // For expresions %VAR=VALUE%. NULL si no hay
    CString                 m_csOccurrences; // Ascii text for occurrences
    std::vector<CNewCapiText> m_aCapiText;
    bool                    m_bDeleted;     //FABN March 14, 2003


    void Copy(const CNewCapiQuestionHelp& rOther);
    bool    CheckOccurrences(CString& csCondition, int& iOccMin, int& iOccMax);

    //FABN March 19, 2003 -> moved to public/static
    //bool    CheckSymbol( CString& csSymbolName );

    //FABN March 19, 2003 -> moved to public/static
    //bool CheckSymbol( CString& csSymbolName );

public:
    typedef enum { None, Literal, Numeric, Other } eCapiNewConditionType;

    CNewCapiQuestionHelp();
    CNewCapiQuestionHelp(const CNewCapiQuestionHelp& rOther);
    void operator=(const CNewCapiQuestionHelp& rOther);

    void Init();

    eCapiNewQuestType     GetType();
    void    SetType(eCapiNewQuestType eType);

    const std::string& GetSymbolName() const { return m_symbolName; }
    bool    SetSymbolName(CString csSymbolName);

    int     GetSymVar();
    void    SetSymVar(int iSymVar);

    int     GetOccMin() const;
    void    SetOccMin(int iOccMin);

    int     GetOccMax() const;
    void    SetOccMax(int iOccMax);

    const std::string& GetCondition() const { return m_condition; }
    bool    SetCondition(CString csCondition);
    static bool SplitCondition(CString csCondition, CIMSAString* csLeft = NULL, int* iCond = NULL, CIMSAString* csRight = NULL, eCapiNewConditionType* eCondType = NULL);

    CString GetOccurrences();
    bool    SetOccurrences(CString csOccurrences);

    CNewCapiText* GetText(CString csLangName);
    CNewCapiText* GetText(int iLangIndex);
    bool    SetText(CString csLangName, CString csText, bool bAppend = false);
    int     GetNumText();

    //The index of csLangName in CNewcapiQuestionHelp
    int     GetLangIndex(CString csLangName);

    void    SetMaxLanguages(int iNumLanguages);

    void    RemoveTextAt(int iLangIndex);

    static bool CheckCondition(CString& csCondition);     //FABN March 19, 2003 -> public/static
    static bool CheckSymbol(const CString& csSymbolName); //FABN March 19, 2003 -> public/static
};


class CapiPre76::CNewCapiQuestionFile
{
private:
    CString m_csFileName;
    std::vector<CNewCapiLanguage> m_aLangs;
    std::vector<CNewCapiQuestionHelp> m_aQuestions;
    std::vector<CNewCapiQuestionHelp> m_aHelps;
    bool m_bIsModified;

    void Init(bool bOnlyArrays);
    void Copy(const CNewCapiQuestionFile& rOther);

public:

    bool Open(const CString& csFileName, bool bSilent);
    bool Build(CSpecFile& cCapiQuestFile, std::shared_ptr<ProgressDlg> pDlgProgress);

    // Others
    void AddLanguages(CNewCapiQuestionHelp& rNewCapiQuestionHelp);

    CNewCapiQuestionFile();
    CNewCapiQuestionFile(const CNewCapiQuestionFile& rOther);
    void operator=(const CNewCapiQuestionFile& rOther);

    // FileName
    void        SetFileName(CString csFileName);
    CString     GetFileName();

    // Languages
    void        AddLanguage(CNewCapiLanguage& rNewCapiLanguage);
    const CNewCapiLanguage& GetLanguage(int iLangNum);
    CNewCapiLanguage* GetLanguage(const std::string& language_name);
    int         GetNumLanguages();

    // Questions
    int         AddQuestion(CNewCapiQuestionHelp& rNewCapiQuestionHelp);
    CNewCapiQuestionHelp* GetQuestion(int iQuestNum);
    int         GetNumQuestions();

    // Helps
    int         AddHelp(CNewCapiQuestionHelp& rNewCapiQuestionHelp);
    CNewCapiQuestionHelp* GetHelp(int iHelpNum);
    int         GetNumHelps();

    // Others

    //FABN Apr 14, 2003 - you case use this instead of AddQuestion/AddHelp
    int         AddCapiQuest(CNewCapiQuestionHelp& rCapiQuest);

    void SetModifiedFlag(bool bIsModified) { m_bIsModified = bIsModified; }
    bool IsModified() const                { return m_bIsModified; }
};
