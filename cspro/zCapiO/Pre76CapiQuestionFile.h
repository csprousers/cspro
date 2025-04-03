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
    friend class CNewCapiQuestionFile;

private:
    eCapiNewQuestType       m_eType;
    std::string             m_symbolName;
    int                     m_iOccMin;      // negative, 1,2,...n
    int                     m_iOccMax;
    std::string             m_condition;  // For expresions %VAR=VALUE%. NULL si no hay
    std::vector<CNewCapiText> m_aCapiText;


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
    bool    SetSymbolName(std::string symbol_name);

    int     GetOccMin() const;
    void    SetOccMin(int iOccMin);

    int     GetOccMax() const;
    void    SetOccMax(int iOccMax);

    const std::string& GetCondition() const { return m_condition; }
    bool    SetCondition(CString csCondition);
    static bool SplitCondition(CString csCondition, CIMSAString* csLeft = NULL, int* iCond = NULL, CIMSAString* csRight = NULL, eCapiNewConditionType* eCondType = NULL);

    bool    SetOccurrences(CString csOccurrences);

    const CNewCapiText* GetText(CString csLangName) const;
    const CNewCapiText* GetText(int iLangIndex) const;
    bool    SetText(CString csLangName, CString csText, bool bAppend = false);
    int     GetNumText() const;

    //The index of csLangName in CNewcapiQuestionHelp
    int     GetLangIndex(CString csLangName) const;

    static bool CheckCondition(CString& csCondition);         //FABN March 19, 2003 -> public/static
    static bool CheckSymbol(std::string_view symbol_name_sv); //FABN March 19, 2003 -> public/static
};


class CapiPre76::CNewCapiQuestionFile
{
private:
    CString m_csFileName;
    std::vector<CNewCapiLanguage> m_aLangs;
    std::vector<CNewCapiQuestionHelp> m_aQuestions;
    std::vector<CNewCapiQuestionHelp> m_aHelps;
    bool m_bIsModified;

    void Init();
    void Copy(const CNewCapiQuestionFile& rOther);

public:

    bool Open(const std::string& file_path);
    bool Build(CSpecFile& cCapiQuestFile, std::shared_ptr<ProgressDlg> pDlgProgress);

    // Others
    void AddLanguages(CNewCapiQuestionHelp& rNewCapiQuestionHelp);

    CNewCapiQuestionFile();
    CNewCapiQuestionFile(const CNewCapiQuestionFile& rOther);
    void operator=(const CNewCapiQuestionFile& rOther);

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
};
