#include "StdAfx.h"
#include "PathAdjusterDlg.h"
#include <zUtilO/FileUtil.h>
#include <windowsx.h>


BEGIN_MESSAGE_MAP(PathAdjusterDlg, CDialog)
    ON_EN_CHANGE(IDC_PATH, OnPathChange)
    ON_EN_CHANGE(IDC_RELATIVE_TO_PATH, OnRelativeToChange)
    ON_BN_CLICKED(IDC_USE_FORWARD_SLASHES, OnUseForwardSlahes)
    ON_CONTROL_RANGE(BN_CLICKED, IDC_PATH_SELECT, IDC_RELATIVE_TO_PATH_SELECT, OnPathSelect)
    ON_CONTROL_RANGE(BN_CLICKED, IDC_PATH_LOGIC_COPY, IDC_RELATIVE_PATH_LOGIC_COPY, OnCopy)
END_MESSAGE_MAP()


namespace
{
    static constexpr const TCHAR* UseForwardSlashesSetting = _T("Path Adjuster: Use Forward Slashes");
}


PathAdjusterDlg::PathAdjusterDlg(int lexer_language, std::wstring initial_path, CWnd* pParent/* = nullptr*/)
    :   CDialog(PathAdjusterDlg::IDD, pParent),
        m_logicStringEscaper(Lexers::IsNotV0(lexer_language)),
        m_csproLexerLanguage(lexer_language),
        m_useForwardSlashes(WinSettings::Read<DWORD>(UseForwardSlashesSetting, 1) == 1),
        m_relativePathHWnd(nullptr),
        m_useForwardSlashesHWnd(nullptr),
        m_initialized(false)
{
    m_path = AdjustPathSlashes(std::move(initial_path));

    // the lexer must be a CSPro logic lexer
    if( !Lexers::IsCSProLogic(m_csproLexerLanguage) )
        m_csproLexerLanguage = SCLEX_CSPRO_LOGIC_V8_0;
}


void PathAdjusterDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);

    DDX_Control(pDX, IDC_PATH, m_pathEdit);
    DDX_Control(pDX, IDC_PATH_LOGIC, m_pathLogicCtrl);
    DDX_Control(pDX, IDC_RELATIVE_TO_PATH, m_relativeToEdit);
    DDX_Control(pDX, IDC_RELATIVE_PATH_LOGIC, m_relativePathLogicCtrl);
}


BOOL PathAdjusterDlg::OnInitDialog()
{
    CDialog::OnInitDialog();

    m_relativePathHWnd= GetDlgItem(IDC_RELATIVE_PATH)->GetSafeHwnd();
    m_useForwardSlashesHWnd = GetDlgItem(IDC_USE_FORWARD_SLASHES)->GetSafeHwnd();

    // set up the logic controls, overriding the font size and zoom, and turning off the horizontal scrollbar
    for( CLogicCtrl* logic_ctrl : { &m_pathLogicCtrl, &m_relativePathLogicCtrl } )
    {
        logic_ctrl->ReplaceCEdit(this, false, false, m_csproLexerLanguage);

        logic_ctrl->StyleSetSize(STYLE_DEFAULT, 8);
        logic_ctrl->SetZoom(1);

        logic_ctrl->SetHScrollBar(FALSE);
    };

    // set the initial values
    WindowsWS::SetWindowText(m_pathEdit, m_path);
    WindowsWS::SetWindowText(m_relativeToEdit, PortableFunctions::PathGetDirectory(m_path));
    Button_SetCheck(m_useForwardSlashesHWnd, m_useForwardSlashes ? BST_CHECKED : BST_UNCHECKED);

    m_initialized = true;
    UpdatePaths();

    return TRUE;
}


std::wstring PathAdjusterDlg::AdjustPathSlashes(std::wstring path) const
{
    return m_useForwardSlashes ? PortableFunctions::PathToForwardSlash(std::move(path)) : path;
}


void PathAdjusterDlg::UpdatePaths()
{
    if( !m_initialized )
        return;

    auto get_cspro_logic = [&](std::wstring path)
    {
        return !path.empty() ? m_logicStringEscaper.EscapeString(AdjustPathSlashes(std::move(path))) :
                               std::wstring();
    };

    m_pathLogicCtrl.SetReadOnlyText(get_cspro_logic(m_path));

    std::wstring relative_path;

    if( !m_relativeToFilename.empty() )
    {
        ASSERT80(m_relativeToFilename == PortableFunctions::PathToNativeSlash(m_relativeToFilename));
        relative_path = AdjustPathSlashes(GetRelativeFNameForDisplay(m_relativeToFilename, PortableFunctions::PathToNativeSlash(m_path)));
    }
                                                            
    WindowsWS::SetWindowText(m_relativePathHWnd, relative_path);
    m_relativePathLogicCtrl.SetReadOnlyText(get_cspro_logic(std::move(relative_path)));
}


void PathAdjusterDlg::OnPathChange()
{
    m_path = WindowsWS::GetWindowText(m_pathEdit);

    UpdatePaths();
}


void PathAdjusterDlg::OnRelativeToChange()
{
    m_relativeToFilename = PortableFunctions::PathToNativeSlash(WindowsWS::GetWindowText(m_relativeToEdit));
    SO::MakeTrim(m_relativeToFilename);

    // if this is a directory, create a fake filename in the directory, as that is needed for GetRelativeFNameForDisplay
    if( !m_relativeToFilename.empty() &&
        ( PortableFunctions::IsPathCharacter(m_relativeToFilename.back()) || PortableFunctions::FileIsDirectory(m_relativeToFilename) ) )
    {
        m_relativeToFilename = PortableFunctions::PathAppendToPath(m_relativeToFilename, _T("g"));        
    }  

    UpdatePaths();
}


void PathAdjusterDlg::OnUseForwardSlahes()
{
    m_useForwardSlashes = ( Button_GetCheck(m_useForwardSlashesHWnd) == BST_CHECKED );
    WinSettings::Write<DWORD>(UseForwardSlashesSetting, m_useForwardSlashes);

    UpdatePaths();
}


void PathAdjusterDlg::OnPathSelect(UINT nID)
{
    auto [current_path, hWnd] = ( nID == IDC_PATH_SELECT ) ? std::make_tuple(m_path, m_pathEdit.m_hWnd) :
                                                             std::make_tuple(WindowsWS::GetWindowText(m_relativeToEdit), m_relativeToEdit.m_hWnd);

    std::optional<std::wstring> new_path = SelectFileOrFolderDialog(m_hWnd, _T("Select a File or Folder"),
                                                                    PortableFunctions::FileExists(current_path) ? current_path.c_str() : nullptr);

    if( new_path.has_value() )
        WindowsWS::SetWindowText(hWnd, AdjustPathSlashes(*new_path));
}


void PathAdjusterDlg::OnCopy(UINT nID)
{
    std::wstring text = ( nID == IDC_PATH_LOGIC_COPY )           ? m_pathLogicCtrl.GetText() : 
                        ( nID == IDC_RELATIVE_PATH_COPY )        ? WindowsWS::GetWindowText(m_relativePathHWnd) : 
                      /*( nID == IDC_RELATIVE_PATH_LOGIC_COPY )*/  m_relativePathLogicCtrl.GetText();

    WinClipboard::PutText(this, text);
}
