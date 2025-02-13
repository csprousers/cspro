#include "stdafx.h"
#include "TextConverterDlg.h"
#include <zUtilO/Filedlg.h>
#include <zUtilO/WindowsWS.h>


// 20120123 utility to convert text files from ANSI->UTF8 or from UTF8->ANSI; borrowed liberally from CSConcat


BEGIN_MESSAGE_MAP(CTextConverterDlg, CDialog)
    ON_WM_PAINT()
    ON_WM_QUERYDRAGICON()
    ON_BN_CLICKED(IDC_ADD, &CTextConverterDlg::OnBnClickedAdd)
    ON_BN_CLICKED(IDC_REMOVE, &CTextConverterDlg::OnBnClickedRemove)
    ON_BN_CLICKED(IDC_CLEAR, &CTextConverterDlg::OnBnClickedClear)
    ON_BN_CLICKED(IDOK, &CTextConverterDlg::OnBnClickedOk)
    ON_BN_CLICKED(IDC_UTF8, &CTextConverterDlg::OnBnClickedUtf8)
END_MESSAGE_MAP()


CTextConverterDlg::CTextConverterDlg(CWnd* pParent /* = nullptr*/)
    :   CDialog(CTextConverterDlg::IDD, pParent)
{
    m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}


void CTextConverterDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);

    DDX_Control(pDX, IDC_FILELIST, m_fileList);
}


BOOL CTextConverterDlg::OnInitDialog()
{
    CDialog::OnInitDialog();

    // Set the icon for this dialog.  The framework does this automatically
    //  when the application's main window is not a dialog
    SetIcon(m_hIcon, TRUE);         // Set big icon
    SetIcon(m_hIcon, FALSE);        // Set small icon

    CheckRadioButton(IDC_ANSI,IDC_UTF8,IDC_ANSI); // default to convert to ansi
    UpdateRunButton();

    m_fileList.SetExtendedStyle(LVS_EX_FULLROWSELECT);
    m_fileList.SetHeadings(_T("Name,500;Encoding,100"));
    m_fileList.LoadColumnInfo();

    // set up the callback to allow the dragging of files onto the list of files to convert
    m_fileList.InitializeDropFiles(DropFilesListCtrl::DirectoryHandling::RecurseInto,
        [&](const std::vector<std::wstring>& paths)
        {
            OnDropFiles(paths);
        });

    return TRUE;  // return TRUE  unless you set the focus to a control
}


// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CTextConverterDlg::OnPaint()
{
    if( IsIconic() )
    {
        CPaintDC dc(this); // device context for painting

        SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

        // Center icon in client rectangle
        int cxIcon = GetSystemMetrics(SM_CXICON);
        int cyIcon = GetSystemMetrics(SM_CYICON);
        CRect rect;
        GetClientRect(&rect);
        int x = (rect.Width() - cxIcon + 1) / 2;
        int y = (rect.Height() - cyIcon + 1) / 2;

        // Draw the icon
        dc.DrawIcon(x, y, m_hIcon);
    }

    else
    {
        CDialog::OnPaint();
    }
}


// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CTextConverterDlg::OnQueryDragIcon()
{
    return static_cast<HCURSOR>(m_hIcon);
}


void CTextConverterDlg::OnBnClickedAdd()
{
    SetCurrentDirectory(AfxGetApp()->GetProfileString(_T("Settings"), _T("Last Data Folder")));

    CIMSAFileDialog dlg(TRUE, nullptr, nullptr, OFN_HIDEREADONLY | OFN_ALLOWMULTISELECT,
                        _T("Files To Convert (*.*)|*.*|"), AfxGetApp()->GetMainWnd(), CFD_NO_DIR);
    dlg.m_ofn.lpstrTitle = _T("Select Files To Convert");
    dlg.SetMultiSelectBuffer();

    if( dlg.DoModal() != IDOK )
        return;

    for( int i = 0; i < dlg.m_aFileName.GetSize(); ++i )
        AddFile(dlg.m_aFileName[i]);

    AfxGetApp()->WriteProfileString(_T("Settings"), _T("Last Data Folder"),
                                    PortableFunctions::PathGetDirectory(dlg.m_aFileName.GetAt(0)).c_str());

    UpdateRunButton();
}


void CTextConverterDlg::OnBnClickedRemove()
{
    int first_selected_index = m_fileList.GetSelectionMark();
    std::vector<int> deleted_indices;

    POSITION pos = m_fileList.GetFirstSelectedItemPosition();

    while( pos != nullptr )
        deleted_indices.emplace_back(m_fileList.GetNextSelectedItem(pos));

    for( size_t i = deleted_indices.size() - 1; i < deleted_indices.size(); --i )
        m_fileList.DeleteItem(deleted_indices[i]);

    first_selected_index = std::min(first_selected_index, m_fileList.GetItemCount() - 1);

    m_fileList.SetItemState(first_selected_index, LVIS_SELECTED | LVIS_FOCUSED,LVIS_SELECTED | LVIS_FOCUSED);
    m_fileList.SetFocus();

    UpdateRunButton();
}


void CTextConverterDlg::OnBnClickedClear()
{
    m_fileList.DeleteAllItems();
    UpdateRunButton();
}


void CTextConverterDlg::OnBnClickedOk()
{
    CWaitCursor waitCursor;

    int converted_files = 0;
    int failed_conversions = 0;
    int skipped_files = 0;
    int not_necessary_files = 0;

    bool convert_to_ansi = IsDlgButtonChecked(IDC_ANSI);

    for( int i = 0; i < m_fileList.GetItemCount(); ++i )
    {
        const Encoding encoding = static_cast<Encoding>(m_fileList.GetItemData(i));

        if( encoding != Encoding::Ansi && encoding != Encoding::Utf8 )
        {
            ++skipped_files;
        }

        else
        {
            std::wstring filename = m_fileList.GetItemText(i, 0);

            if( convert_to_ansi )
            {
                if( encoding == Encoding::Ansi )
                {
                    ++not_necessary_files;
                }

                else
                {
                    CStdioFileUnicode::ConvertUTF8ToAnsi(filename) ? ++converted_files :
                                                                     ++failed_conversions;
                }
            }

            else
            {
                if( encoding == Encoding::Utf8 )
                {
                    not_necessary_files++;
                }

                else
                {
                    CStdioFileUnicode::ConvertAnsiToUTF8(filename) ? ++converted_files :
                                                                     ++failed_conversions;
                }
            }
        }
    }

    RefreshEncodings();

    std::vector<std::wstring> messages;

    if( converted_files )
        messages.emplace_back(FormatTextCS2WS(_T("%d files converted to %s successfully."), converted_files, convert_to_ansi ? _T("ANSI") : _T("UTF-8")));

    if( failed_conversions )
        messages.emplace_back(FormatTextCS2WS(_T("%d files failed during the conversion process."), failed_conversions));

    if( skipped_files )
        messages.emplace_back(FormatTextCS2WS(_T("%d files were skipped to due incompatible encodings."), skipped_files));

    if( not_necessary_files )
        messages.emplace_back(FormatTextCS2WS(_T("%d files were not converted as they were already in the correct encoding."), not_necessary_files));

    if( converted_files && convert_to_ansi )
        messages.emplace_back(_T("A loss of data may have occurred during the UTF-8 -> ANSI conversion."));

    AfxMessageBox(SO::CreateSingleString(messages, _T("\n")), MB_ICONINFORMATION);
}


void CTextConverterDlg::UpdateRunButton()
{
    GetDlgItem(IDOK)->EnableWindow(m_fileList.GetItemCount());
    GetDlgItem(IDC_REMOVE)->EnableWindow(m_fileList.GetItemCount());
    GetDlgItem(IDC_CLEAR)->EnableWindow(m_fileList.GetItemCount());
}


void CTextConverterDlg::RefreshEncodings()
{
    for( int i = 0; i < m_fileList.GetItemCount(); ++i )
    {
        Encoding encoding;
        GetFileBOM(m_fileList.GetItemText(i, 0), encoding);

        m_fileList.SetItemText(i, 1, ToString(encoding));
        m_fileList.SetItemData(i, static_cast<DWORD>(encoding));
    }
}


void CTextConverterDlg::OnDropFiles(const std::vector<std::wstring>& filenames)
{
    const int nIndex = m_fileList.GetItemCount();

    for( const std::wstring& filename : filenames )
        AddFile(filename);

    m_fileList.EnsureVisible(nIndex - 1, FALSE);

    UpdateRunButton();
}


void CTextConverterDlg::OnBnClickedUtf8() // 20120620
{
    static bool shown_message = false;

    if( !shown_message )
    {
        AfxMessageBox(_T("CSPro applications automatically upconvert files to UTF-8 if necessary, so this functionality may not be necessary for you.\n")
                      _T("Converting non-text files to UTF-8 is destructive and can ruin the file."));

        shown_message = true;
    }
}


void CTextConverterDlg::AddFile(NullTerminatedString filename) // 20120620
{
    LVFINDINFO rlvFind = { LVFI_STRING, filename.c_str() };

    if( m_fileList.FindItem(&rlvFind) == -1 ) // it's not already in the list
    {
        Encoding encoding;
        GetFileBOM(filename, encoding);

        const int item = m_fileList.AddItem(filename.c_str(), ToString(encoding));
        m_fileList.SetItemData(item, static_cast<DWORD>(encoding));
    }
}
