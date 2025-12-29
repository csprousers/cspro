#include "StdAfx.h"
#include "CodePurifierView.h"
#include <zToolsO/WinClipboard.h>


namespace Update
{
    constexpr WPARAM All           = 0xff;
    constexpr WPARAM BranchCopies  = 0x01;
    constexpr WPARAM Commits       = 0x02;
    constexpr WPARAM ModifiedFiles = 0x04;
}


namespace
{
    constexpr std::string_view CreateBranchCopyBeforeResetKey_sv = "create-branch-copy-before-reset";
}


IMPLEMENT_DYNCREATE(CodePurifierView, CFormView)


BEGIN_MESSAGE_MAP(CodePurifierView, CFormView)
    ON_MESSAGE(UWM::Stygitan::AppActivated, OnAppActivated)
    ON_MESSAGE(UWM::Stygitan::UpdateUI, OnUpdateUI)
    ON_NOTIFY(NM_CLICK, IDC_WORKING_DIRECTORY, OnWorkingDirectoryClick)
    ON_NOTIFY(NM_RETURN, IDC_WORKING_DIRECTORY, OnWorkingDirectoryClick)
    ON_COMMAND(IDC_CREATE_BRANCH_COPY, OnCreateBranchCopy)
    ON_COMMAND(IDC_DELETE_BRANCH_COPIES, OnDeleteBranchCopies)
    ON_NOTIFY(NM_CUSTOMDRAW, IDC_COMMITS, OnCommitsCustomDraw)
    ON_NOTIFY(NM_RCLICK, IDC_COMMITS, OnCommitsRightClick)
    ON_COMMAND(ID_SET_CLEAN_COMMIT, OnSetCleanCommit)
    ON_COMMAND(IDC_RESET_BRANCH_TO_CLEAN_COMMIT, OnResetBranchToCleanCommit)
    ON_COMMAND(IDC_CREATE_BRANCH_COPY_BEFORE_RESET, OnCreateCreateBranchCopyBeforeResetClick)
    ON_NOTIFY(NM_DBLCLK, IDC_MODIFIED_FILES, OnModifiedFilesDoubleOrRightClick)
    ON_NOTIFY(NM_RCLICK, IDC_MODIFIED_FILES, OnModifiedFilesDoubleOrRightClick)
    ON_COMMAND(ID_MODIFIED_FILE_OPEN, OnModifiedFileOpen)
    ON_COMMAND(ID_MODIFIED_FILE_OPEN_CONTAINING_FOLDER, OnModifiedFileOpenContainingFolder)
    ON_COMMAND(ID_MODIFIED_FILE_COPY_PATH, OnModifiedFileCopyPath)
END_MESSAGE_MAP()


CodePurifierView::CodePurifierView()
    :   CFormView(IDD_CODE_PURIFIER),
        m_settingsDb("Stygitan.db", "CodePurifier"),
        m_lastFullRefreshTime(0),
        m_cleanCommitIndex(0),
        m_createBranchCopyBeforeReset(m_settingsDb.ReadOrDefault(CreateBranchCopyBeforeResetKey_sv, true))
{
}


void CodePurifierView::OnInitialUpdate()
{
    __super::OnInitialUpdate();

    const CodePurifierDoc& cp_doc = GetDoc();

    // add the working directory as a link
    WindowsUtf8::SetText(m_hWnd, IDC_WORKING_DIRECTORY, SO::Concatenate(
        "<a>",
        Path::RemoveTrailingSlash(cp_doc.m_repo.GetWorkingDirectory()),
        "</a>"
    ));

    m_commitsListCtrl.SetExtendedStyle(LVS_EX_FULLROWSELECT);
    m_commitsListCtrl.SetHeadings(L"Date,120;Message,435");
    m_commitsListCtrl.LoadColumnInfo();

    m_modifiedFilesListCtrl.SetExtendedStyle(LVS_EX_FULLROWSELECT);
    m_modifiedFilesListCtrl.SetHeadings(L"Path,475;Status,95");
    m_modifiedFilesListCtrl.LoadColumnInfo();

    PostMessage(UWM::Stygitan::UpdateUI, Update::All);
}


void CodePurifierView::DoDataExchange(CDataExchange* const pDX)
{
    __super::DoDataExchange(pDX);

    DDX_Control(pDX, IDC_BRANCH_COPIES, m_branchCopiesListBox);
    DDX_Control(pDX, IDC_COMMITS, m_commitsListCtrl);
    DDX_Check(pDX, IDC_CREATE_BRANCH_COPY_BEFORE_RESET, m_createBranchCopyBeforeReset);
    DDX_Control(pDX, IDC_MODIFIED_FILES, m_modifiedFilesListCtrl);
}


void CodePurifierView::RefreshDataAndUpdateUI(const WPARAM wParam)
{
    try
    {
        CodePurifierDoc& cp_doc = GetDoc();
        cp_doc.RefreshData();

        PostMessage(UWM::Stygitan::UpdateUI, wParam);
    }

    catch( const CSProException& exception )
    {
        ErrorMessage::Display(exception);
        GetParentFrame()->PostMessage(WM_CLOSE);
    }
}


LRESULT CodePurifierView::OnAppActivated(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
    constexpr int64_t RefreshSecondsInterval = 5;

    // when coming back to this view after some time in another application, refresh the data,
    // closing the view on error;
    // the interval check also prevents refreshing the data immediately after OnInitialUpdate
    if( ( GetTimestamp<int64_t>() - m_lastFullRefreshTime ) >= RefreshSecondsInterval )
    {
        RefreshDataAndUpdateUI(Update::All);
    }

    return 1;
}


LRESULT CodePurifierView::OnUpdateUI(const WPARAM wParam, LPARAM /*lParam*/)
{
    const CodePurifierDoc& cp_doc = GetDoc();

    // update most of the repository information
    if( ( wParam & Update::All ) == Update::All )
    {
        WindowsUtf8::SetText(m_hWnd, IDC_LOCAL_BRANCH, cp_doc.m_currentBranch->GetName());

        WindowsUtf8::SetText(m_hWnd, IDC_REMOTE_BRANCH,
            ( cp_doc.m_remoteBranch != nullptr ) ? cp_doc.m_remoteBranch->GetName() : "<no remote branch>"
        );

        m_lastFullRefreshTime = GetTimestamp<int64_t>();
    }

    // update the branch copies
    if( ( wParam & Update::BranchCopies ) == Update::BranchCopies )
    {
        m_branchCopiesListBox.ResetContent();

        for( const auto& [name, branch] : cp_doc.m_branchCopies )
            m_branchCopiesListBox.AddString(TC::ToWide(name).c_str());
    }

    // update the clean commit and the commits
    if( ( wParam & Update::Commits ) == Update::Commits )
    {
        WindowsUtf8::SetText(m_hWnd, IDC_CLEAN_COMMIT, !cp_doc.m_cleanCommit.has_value()
            ? "<no clean commit>"
            : FormatText("%s\n(%s)", cp_doc.m_cleanCommit->GetMessage().c_str(),
                                     cp_doc.m_cleanCommit->GetAuthor().GetWhen().GetLocalDateTimeString().c_str())
        );

        m_commitsListCtrl.DeleteAllItems();
        m_cleanCommitIndex = 0;
        bool reached_clean_commit = false;

        for( const GitCommit& commit : cp_doc.m_recentCommits )
        {
            m_commitsListCtrl.AddItem(TC::ToWide(commit.GetAuthor().GetWhen().GetLocalDateTimeString()).c_str(),
                                      TC::ToWide(commit.GetMessage()).c_str());

            if( !reached_clean_commit )
            {
                if( commit == cp_doc.m_cleanCommit )
                {
                    reached_clean_commit = true;
                }

                else
                {
                    ++m_cleanCommitIndex;
                }
            }
        }
    }

    // updated the modified files
    if( ( wParam & Update::ModifiedFiles ) == Update::ModifiedFiles )
    {
        m_modifiedFilesListCtrl.DeleteAllItems();

        for( const auto& [file_path, diff_flag] : cp_doc.m_modifiedFiles )
        {
            const wchar_t* const status =
                ( diff_flag == GIT_DELTA_ADDED )     ? L"Added" :
                ( diff_flag == GIT_DELTA_DELETED )   ? L"Deleted" :
                ( diff_flag == GIT_DELTA_MODIFIED )  ? L"Modified" :
                ( diff_flag == GIT_DELTA_UNTRACKED ) ? L"Untracked" :
                                                       ReturnProgrammingError(L"<unknown status>");

            m_modifiedFilesListCtrl.AddItem(TC::ToWide(file_path).c_str(), status);
        }
    }

    return 1;
}


void CodePurifierView::OnWorkingDirectoryClick(NMHDR* const /*pNMHDR*/, LRESULT* const pResult)
{
    GitRepository& repo = GetDoc().m_repo;
    OpenContainingFolder(repo.GetWorkingDirectory());
    *pResult = 0;
}


void CodePurifierView::OnCreateBranchCopy()
{
    try
    {
        CodePurifierDoc& cp_doc = GetDoc();
        GitRepository& repo = cp_doc.m_repo;

        const GitCommit commit = repo.LookupCommit(cp_doc.m_currentBranch->GetTarget());

        // the branch name will be: [commit time]-CP-[branch name]
        const std::string branch_name = SO::Concatenate(
            commit.GetAuthor().GetWhen().GetLocalDateTimeString("%m%d%H%M"),
            "-CP-",
            cp_doc.m_currentBranch->GetName()
        );

        // make sure no such branch already exists
        if( cp_doc.m_branchCopies.find(branch_name) != cp_doc.m_branchCopies.cend() )
            return;

        cp_doc.m_branchCopies.try_emplace(branch_name, repo.CreateBranch(branch_name, commit));

        PostMessage(UWM::Stygitan::UpdateUI, Update::BranchCopies);
    }

    catch( const CSProException& exception )
    {
        ErrorMessage::Display(exception);
    }
}


void CodePurifierView::OnDeleteBranchCopies()
{
    try
    {
        CodePurifierDoc& cp_doc = GetDoc();

        if( cp_doc.m_branchCopies.empty() )
            return;

        if( *cp_doc.m_initialNumberOfBranchCopies != 0 )
        {
            const std::string query = FormatText(
                "There %s %d copied branch%s created prior to loading the Code Purifier.\n\n"
                "Do you want to continue deleting the branch copies?",
                PluralizeWord(*cp_doc.m_initialNumberOfBranchCopies, "was", "were"),
                static_cast<int>(*cp_doc.m_initialNumberOfBranchCopies),
                PluralizeWord(*cp_doc.m_initialNumberOfBranchCopies)
            );

            if( AfxMessageBox(query, MB_YESNO | MB_DEFBUTTON1) == IDNO )
                return;
        }

        cp_doc.m_initialNumberOfBranchCopies = 0;

        while( !cp_doc.m_branchCopies.empty() )
        {
            auto name_and_branch = cp_doc.m_branchCopies.begin();
            name_and_branch->second.Delete();
            cp_doc.m_branchCopies.erase(name_and_branch);
        }
    }

    catch( const CSProException& exception )
    {
        ErrorMessage::Display(exception);
    }

    PostMessage(UWM::Stygitan::UpdateUI, Update::BranchCopies);
}


void CodePurifierView::OnCommitsCustomDraw(NMHDR* const pNMHDR, LRESULT* const pResult)
{
    NMLVCUSTOMDRAW* const pLVCD = reinterpret_cast<NMLVCUSTOMDRAW*>(pNMHDR);

    if( pLVCD->nmcd.dwDrawStage == CDDS_PREPAINT )
    {
        *pResult = CDRF_NOTIFYITEMDRAW;
        return;
    }

    if( pLVCD->nmcd.dwDrawStage == CDDS_ITEMPREPAINT )
    {
        // color commits newer than the clean commit with a light green background
        const int item_index = static_cast<int>(pLVCD->nmcd.dwItemSpec);

        if( item_index < m_cleanCommitIndex )
            pLVCD->clrTextBk = RGB(200, 255, 200);
    }

    *pResult = CDRF_DODEFAULT;
}


void CodePurifierView::OnCommitsRightClick(NMHDR* const pNMHDR, LRESULT* const pResult)
{
    NMITEMACTIVATE* const pNMItemActivate = reinterpret_cast<NMITEMACTIVATE*>(pNMHDR);

    *pResult = FALSE;

    if( pNMItemActivate->iItem == -1 )
        return;

    CMenu popup_menu;
    popup_menu.CreatePopupMenu();
    popup_menu.AppendMenu(MF_STRING, ID_SET_CLEAN_COMMIT, L"Set as Clean Commit");

    CPoint point = pNMItemActivate->ptAction;
    m_commitsListCtrl.ClientToScreen(&point);
    popup_menu.TrackPopupMenu(TPM_RIGHTBUTTON, point.x, point.y, this);
}


void CodePurifierView::OnSetCleanCommit()
{
    CodePurifierDoc& cp_doc = GetDoc();

    const size_t index = static_cast<size_t>(m_commitsListCtrl.GetSelectionMark());
    ASSERT(index < cp_doc.m_recentCommits.size());

    cp_doc.m_cleanCommitOverride = cp_doc.m_recentCommits[index].GetObjectId();

    RefreshDataAndUpdateUI(Update::Commits | Update::ModifiedFiles);
}


void CodePurifierView::OnResetBranchToCleanCommit()
{
    try
    {
        const CodePurifierDoc& cp_doc = GetDoc();

        if( !cp_doc.m_cleanCommit.has_value() )
            throw CSProException("There is no clean commit.");

        if( m_createBranchCopyBeforeReset )
            OnCreateBranchCopy();

        cp_doc.m_repo.ResetBranchMixed(*cp_doc.m_cleanCommit);

        RefreshDataAndUpdateUI(Update::Commits | Update::ModifiedFiles);
    }

    catch( const CSProException& exception )
    {
        ErrorMessage::Display(exception);
    }
}


void CodePurifierView::OnCreateCreateBranchCopyBeforeResetClick()
{
    UpdateData(TRUE);

    m_settingsDb.Write(CreateBranchCopyBeforeResetKey_sv, m_createBranchCopyBeforeReset);
}


void CodePurifierView::OnModifiedFilesDoubleOrRightClick(NMHDR* const pNMHDR, LRESULT* const pResult)
{
    NMITEMACTIVATE* const pNMItemActivate = reinterpret_cast<NMITEMACTIVATE*>(pNMHDR);

    *pResult = FALSE;

    if( pNMItemActivate->iItem == -1 )
        return;

    if( pNMItemActivate->hdr.code == NM_DBLCLK )
    {
        OnModifiedFileOpen();
    }

    else
    {
        ASSERT(pNMItemActivate->hdr.code == NM_RCLICK);

        UINT file_exists_flag = MF_ENABLED;
        UINT directory_exists_flag = MF_ENABLED;

        OnModifiedFile([&](const std::string& file_path)
            {
                if( !PortableFunctions::FileIsRegular(file_path) )
                {
                    file_exists_flag = MF_DISABLED;

                    if( !PortableFunctions::FileIsDirectory(PortableFunctions::PathGetDirectory(file_path)) )
                        directory_exists_flag = MF_DISABLED;
                }
            });


        CMenu popup_menu;
        popup_menu.CreatePopupMenu();
        popup_menu.AppendMenu(MF_STRING, ID_MODIFIED_FILE_COPY_PATH, L"Copy Full Path");
        popup_menu.AppendMenu(MF_SEPARATOR);
        popup_menu.AppendMenu(MF_STRING | file_exists_flag, ID_MODIFIED_FILE_OPEN, L"Open in Associated Application");
        popup_menu.AppendMenu(MF_STRING | directory_exists_flag, ID_MODIFIED_FILE_OPEN_CONTAINING_FOLDER, L"Open Containing Folder");

        CPoint point = pNMItemActivate->ptAction;
        m_modifiedFilesListCtrl.ClientToScreen(&point);
        popup_menu.TrackPopupMenu(TPM_RIGHTBUTTON, point.x, point.y, this);
    }
}


template<typename CF>
void CodePurifierView::OnModifiedFile(const CF& callback_function)
{
    const CodePurifierDoc& cp_doc = GetDoc();

    const size_t index = static_cast<size_t>(m_modifiedFilesListCtrl.GetSelectionMark());
    ASSERT(index < cp_doc.m_modifiedFiles.size());

    callback_function(Path::Combine(cp_doc.m_repo.GetWorkingDirectory(),
                                    Path::ToNativeSlash(std::get<0>(cp_doc.m_modifiedFiles[index]))));
}


void CodePurifierView::OnModifiedFileOpen()
{
    OnModifiedFile(
        [](std::string file_path)
        {
            ShellExecute(nullptr, L"open", TC::ToWide(EscapeCommandLineArgument(std::move(file_path))).c_str(), nullptr, nullptr, SW_SHOW);
        });
}


void CodePurifierView::OnModifiedFileOpenContainingFolder()
{
    OnModifiedFile(
        [](const std::string& file_path)
        {
            if( PortableFunctions::FileIsRegular(file_path) )
            {
                OpenContainingFolder(file_path);
            }

            else
            {
                OpenContainingFolder(PortableFunctions::PathGetDirectory(file_path));
            }
        });
}


void CodePurifierView::OnModifiedFileCopyPath()
{
    OnModifiedFile(
        [&](const std::string& file_path)
        {
            WinClipboard::PutText(this, file_path);
        });
}
