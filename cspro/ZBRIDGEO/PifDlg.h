#pragma once
// PifDlg.h : header file
//
#include <ZBRIDGEO/npff.h>
#include <ZBRIDGEO/Pifgrid.h>

enum FILETYPE { FILE_NONE, PIFDICT, PIFUSRFILE };

enum PifInfoOptions
{
    PIF_MULTIPLE_FILES = 1,
    PIF_FILE_MUST_EXIST = 2,
    PIF_DISALLOW_CSPRO_EXTENSIONS = 4,
    PIF_ALLOW_BLANK = 8,
    PIF_ALLOW_WILDCARDS = 16, // CONNECTION_TODO remove as PIF_MULTIPLE_FILES should check this
    PIF_READ_ONLY = 32,
    PIF_USE_REPOSITORY_TYPE = 64,
    PIF_REPOSITORY_MUST_BE_READABLE = 128
};

struct PIFINFO
{
    FILETYPE eType; //Is form or dict?
    CString sUName; //Unique Name
    CString sDisplay;
    UINT uOptions; // Bitwise or of flags from PifInfoOptions
    CString sDefaultFileExtension; // Automatically appended in file open dialog if none provided, leave blank for no default
    CString sFileName; // non-data filename

    std::vector<ConnectionString> connection_strings;
    CString dictionary_filename;

    void SetConnectionString(const ConnectionString& connection_string_)
    {
        ASSERT(connection_strings.empty());

        if( connection_string_.IsDefined() )
            connection_strings.push_back(connection_string_);
    }

    void SetConnectionStrings(const std::vector<ConnectionString>& connection_strings_)
    {
        ASSERT(connection_strings.empty());
        connection_strings = connection_strings_;
    }
};


const TCHAR* const OUTPFILE       = _T("Output Data");
const TCHAR* const PARADATAFILE   = _T("<Paradata Log>");
const TCHAR* const LISTFILE       = _T("<Listing File>");
const TCHAR* const WRITEFILE      = _T("<Write File>");
const TCHAR* const FREQFILE       = _T("<Freq File>");
const TCHAR* const IMPUTEFILE     = _T("<Impute Freq File>");
const TCHAR* const IMPUTESTATFILE = _T("<Impute Stat Data>");
const TCHAR* const SAVEARRAYFILE  = _T("<Save Array File>");
const TCHAR* const INPUTTBD       = _T("<Input TAB>");
const TCHAR* const OUTPUTTBD      = _T("<Output TAB>");
const TCHAR* const OUTPUTTBW      = _T("<Output TBW>");
const TCHAR* const INPUTDATA      = _T("<Input Data>");
const TCHAR* const AREANAMES      = _T("<Area Names>");


/////////////////////////////////////////////////////////////////////////////
// CPifDlg dialog

class CLASS_DECL_ZBRIDGEO CPifDlg : public CDialog
{
// Construction
public:
    CPifDlg(const CArray<PIFINFO*, PIFINFO*>& pffInfo,
        CString title = _T("Define File Associations"), CWnd* pParent = NULL);   // standard constructor
    CPifDlg(const std::vector<std::shared_ptr<PIFINFO>>& pffInfo,
        CString title = _T("Define File Associations"), CWnd* pParent = NULL);   // standard constructor

// Dialog Data
public:
    CPifGrid        m_pifgrid;
    CRect           m_Rect;
    CString         m_title;
    CNPifFile*      m_pPifFile;

    void SetGridData();

// Implementation
protected:

    // Generated message map functions
    //{{AFX_MSG(CPifDlg)
    virtual BOOL OnInitDialog();
    virtual void OnOK();
    //}}AFX_MSG

public:
    virtual BOOL PreTranslateMessage(MSG* pMsg); // GHM 20110805

private:
    CArray<PIFINFO*, PIFINFO*> m_arrPifInfo;
    bool Validate();
    bool IsValidFilePath(NullTerminatedStringView path, bool must_be_writeable);
    void SaveAssociations();
    bool DictionaryChangesIfAnyAreOk(const ConnectionString& connection_string, const CString& dictionary_filename);

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.
