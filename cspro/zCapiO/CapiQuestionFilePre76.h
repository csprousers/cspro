#pragma once

#include <zCapiO/zCapiO.h>

class CSpecFile;
class ProgressDlg;
class Serializer;


namespace CapiPre76 {

    class CNewCapiQuestionFile;
    class CNewCapiLanguage;
    class CNewCapiQuestionHelp;

    /////////////////////////////
    class CLASS_DECL_ZCAPIO CNewCapiText {
    private:
        void Init();
        void Copy(const CNewCapiText& rOther);
    public:
        CString        m_csLangName;
        CIMSAString     m_csText; // Separator \r\n.

        CNewCapiText();
        CNewCapiText(const CNewCapiText& rOther);
        void operator=(const CNewCapiText& rOther);
        ~CNewCapiText();

        //FABN Apr 10, 2003 need support to delete only a single language for a given CNewCapiQuestionHelp
        bool    m_bDeleted;

        void serialize(Serializer& ar); // 20121109

    };

    /////////////////////////////
    class CLASS_DECL_ZCAPIO CNewCapiLanguage {
    private:
        void Init();
        void Copy(const CNewCapiLanguage& rOther);

    public:
        int         m_iLangIndex;
        CString     m_csLangName;
        CString     m_csLangLabel;

        static      bool CheckLanguageName(CIMSAString& csLanguage);
        CNewCapiLanguage();
        CNewCapiLanguage(const CNewCapiLanguage& rOther);
        void operator=(const CNewCapiLanguage& rOther);
        ~CNewCapiLanguage();

        void serialize(Serializer& ar); // 20121109
    };

    enum class eCapiNewQuestType { None, Question, Help };

    /////////////////////////////
    class CLASS_DECL_ZCAPIO CNewCapiQuestionHelp {
        friend class CNewCapiQuestionFile;  // For save method
    private:
        eCapiNewQuestType       m_eType;
        CString                 m_csSymbolName;
        int                     m_iSymVar;
        int                     m_iOccMin;      // negative, 1,2,...n
        int                     m_iOccMax;
        CString                 m_csCondition;  // For expresions %VAR=VALUE%. NULL si no hay
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

        ~CNewCapiQuestionHelp();

        void Init();

        eCapiNewQuestType     GetType();
        void    SetType(eCapiNewQuestType eType);

        CString GetSymbolName();
        bool    SetSymbolName(CString csSymbolName);

        int     GetSymVar();
        void    SetSymVar(int iSymVar);

        int     GetOccMin() const;
        void    SetOccMin(int iOccMin);

        int     GetOccMax() const;
        void    SetOccMax(int iOccMax);

        CString GetCondition();
        bool    SetCondition(CString csCondition);
        static bool SplitCondition(CString csCondition, CIMSAString* csLeft = NULL, int* iCond = NULL, CIMSAString* csRight = NULL, eCapiNewConditionType* eCondType = NULL);

        CString GetOccurrences();
        bool    SetOccurrences(CString csOccurrences);

        CNewCapiText* GetText(CString csLangName);
        CNewCapiText* GetText(int iLangIndex);
        bool    SetText(CString csLangName, CString csText, bool bAppend = false);
        int     GetNumText();

        //The index of csLangName in CNewcapiQuestionHelp
        int     GetLangIndex(CString csLangName, bool bCaseSensitive = true);

        void    SetMaxLanguages(int iNumLanguages);

        void    RemoveTextAt(int iLangIndex);

        static bool CheckCondition(CString& csCondition);     //FABN March 19, 2003 -> public/static
        static bool CheckSymbol(const CString& csSymbolName); //FABN March 19, 2003 -> public/static


        void serialize(Serializer& ar); // 20121109
    };

    class CLASS_DECL_ZCAPIO CNewCapiQuestionFile {
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

        ~CNewCapiQuestionFile();

        // FileName
        void        SetFileName(CString csFileName);
        CString     GetFileName();

        // Languages
        void        AddLanguage(CNewCapiLanguage& rNewCapiLanguage);
        CNewCapiLanguage* GetLanguage(int iLangNum);
        CNewCapiLanguage* GetLanguage(CString csLangName, bool bCaseSensitive = true);
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

        void serialize(Serializer& ar); // 20121109
    };
}
