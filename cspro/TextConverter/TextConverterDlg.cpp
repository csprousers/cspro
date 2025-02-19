#include "stdafx.h"
#include "TextConverterDlg.h"
#include <zUtilO/FileDlg.h>
#include <zUtilO/WindowsWS.h>


// 20120123 utility to convert text files from ANSI->UTF8 or from UTF8->ANSI; borrowed liberally from CSConcat


BEGIN_MESSAGE_MAP(TextConverterDlg, CDialog)
    ON_WM_PAINT()
    ON_WM_QUERYDRAGICON()
    ON_BN_CLICKED(IDC_ADD, OnBnClickedAdd)
    ON_BN_CLICKED(IDC_REMOVE, OnBnClickedRemove)
    ON_BN_CLICKED(IDC_CLEAR, OnBnClickedClear)
    ON_BN_CLICKED(IDOK, OnBnClickedOk)
    ON_BN_CLICKED(IDC_UTF8, OnBnClickedUtf8)
END_MESSAGE_MAP()


TextConverterDlg::TextConverterDlg(CWnd* const pParent/* = nullptr*/)
    :   CDialog(IDD_TEXTCONVERTER, pParent),
        m_hIcon(AfxGetApp()->LoadIcon(IDR_MAINFRAME))
{
}


void TextConverterDlg::DoDataExchange(CDataExchange* const pDX)
{
    __super::DoDataExchange(pDX);

    DDX_Control(pDX, IDC_FILELIST, m_fileList);
}


BOOL TextConverterDlg::OnInitDialog()
{
    __super::OnInitDialog();

    // Set the icon for this dialog.  The framework does this automatically
    //  when the application's main window is not a dialog
    SetIcon(m_hIcon, TRUE);         // Set big icon
    SetIcon(m_hIcon, FALSE);        // Set small icon

    CheckRadioButton(IDC_ANSI, IDC_UTF8, IDC_ANSI); // default to convert to ansi
    UpdateRunButton();

    m_fileList.SetExtendedStyle(LVS_EX_FULLROWSELECT);
    m_fileList.SetHeadings(L"Name,500;Encoding,100");
    m_fileList.LoadColumnInfo();

    // set up the callback to allow the dragging of files onto the list of files to convert
    m_fileList.InitializeDropFiles(DropFilesListCtrl::DirectoryHandling::RecurseInto,
        [&](const std::vector<std::string>& paths)
        {
            OnDropFiles(paths);
        });

    return TRUE;  // return TRUE  unless you set the focus to a control
}


void TextConverterDlg::OnPaint()
{
    // If you add a minimize button to your dialog, you will need the code below
    //  to draw the icon.  For MFC applications using the document/view model,
    //  this is automatically done for you by the framework.
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
        __super::OnPaint();
    }
}


HCURSOR TextConverterDlg::OnQueryDragIcon()
{
    // The system calls this function to obtain the cursor to display while the user drags
    //  the minimized window.
    return static_cast<HCURSOR>(m_hIcon);
}


void TextConverterDlg::OnBnClickedAdd()
{
    SetCurrentDirectory(AfxGetApp()->GetProfileString(L"Settings", L"Last Data Folder"));

    OpenFileDlg open_file_dlg(0, nullptr, nullptr, L"Files To Convert (*.*)|*.*||", this);
    open_file_dlg.SetTitle(L"Select Files To Convert")
                 .SetMultiSelectBuffer();

    if( open_file_dlg.DoModal() != IDOK )
        return;

    for( const std::string& file_path : open_file_dlg.GetFilePaths() )
        AddFile(file_path);

    AfxGetApp()->WriteProfileString(L"Settings", L"Last Data Folder",
                                    TC::ToWide(PortableFunctions::PathGetDirectory(open_file_dlg.GetFilePaths().front())).c_str());

    UpdateRunButton();
}


void TextConverterDlg::OnBnClickedRemove()
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


void TextConverterDlg::OnBnClickedClear()
{
    m_fileList.DeleteAllItems();
    UpdateRunButton();
}


void TextConverterDlg::OnBnClickedOk()
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
            const std::string file_path = TC::ToUtf8(m_fileList.GetItemText(i, 0));

            if( convert_to_ansi )
            {
                if( encoding == Encoding::Ansi )
                {
                    ++not_necessary_files;
                }

                else
                {
                    CStdioFileUnicode::ConvertUTF8ToAnsi(file_path) ? ++converted_files :
                                                                      ++failed_conversions;
                }
            }

            else
            {
                if( encoding == Encoding::Utf8 )
                {
                    ++not_necessary_files;
                }

                else
                {
                    CStdioFileUnicode::ConvertAnsiToUTF8(file_path) ? ++converted_files :
                                                                      ++failed_conversions;
                }
            }
        }
    }

    RefreshEncodings();

    std::vector<std::string> messages;

    if( converted_files )
        messages.emplace_back(FormatText("%d files converted to %s successfully.", converted_files, convert_to_ansi ? "ANSI" : "UTF-8"));

    if( failed_conversions )
        messages.emplace_back(FormatText("%d files failed during the conversion process.", failed_conversions));

    if( skipped_files )
        messages.emplace_back(FormatText("%d files were skipped to due incompatible encodings.", skipped_files));

    if( not_necessary_files )
        messages.emplace_back(FormatText("%d files were not converted as they were already in the correct encoding.", not_necessary_files));

    if( converted_files && convert_to_ansi )
        messages.emplace_back("A loss of data may have occurred during the UTF-8 -> ANSI conversion.");

    AfxMessageBox(SO::CreateSingleString(messages, SO::Newline_lf_sv), MB_ICONINFORMATION);
}


void TextConverterDlg::UpdateRunButton()
{
    GetDlgItem(IDOK)->EnableWindow(m_fileList.GetItemCount());
    GetDlgItem(IDC_REMOVE)->EnableWindow(m_fileList.GetItemCount());
    GetDlgItem(IDC_CLEAR)->EnableWindow(m_fileList.GetItemCount());
}


void TextConverterDlg::RefreshEncodings()
{
    for( int i = 0; i < m_fileList.GetItemCount(); ++i )
    {
        Encoding encoding;
        GetFileBOM(m_fileList.GetItemText(i, 0), encoding);

        m_fileList.SetItemText(i, 1, TC::ToWide(ToString(encoding)).c_str());
        m_fileList.SetItemData(i, static_cast<DWORD>(encoding));
    }
}


void TextConverterDlg::OnDropFiles(const std::vector<std::string>& paths)
{
    const int nIndex = m_fileList.GetItemCount();

    for( const std::string& path : paths )
        AddFile(path);

    m_fileList.EnsureVisible(nIndex - 1, FALSE);

    UpdateRunButton();
}


void TextConverterDlg::OnBnClickedUtf8() // 20120620
{
    static bool shown_message = false;

    if( !shown_message )
    {
        AfxMessageBox(L"CSPro applications automatically upconvert files to UTF-8 if necessary, so this functionality may not be necessary for you.\n"
                      L"Converting non-text files to UTF-8 is destructive and can ruin the file.");

        shown_message = true;
    }
}


void TextConverterDlg::AddFile(const std::string& file_path) // 20120620
{
    const std::wstring wide_file_path = TC::ToWide(file_path);

    LVFINDINFO rlvFind = { LVFI_STRING, wide_file_path.c_str() };

    if( m_fileList.FindItem(&rlvFind) == -1 ) // it's not already in the list
    {
        Encoding encoding;
        GetFileBOM(file_path, encoding);

        const int item = m_fileList.AddItem(wide_file_path.c_str(), TC::ToWide(ToString(encoding)).c_str());
        m_fileList.SetItemData(item, static_cast<DWORD>(encoding));
    }
}
