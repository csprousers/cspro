#include "StdAfx.h"
#include "CodePurifierView.h"
#include "DiffTool.h"
#include "EditorConfigApplier.h"
#include <zToolsO/Hash.h>
#include <zToolsO/WinClipboard.h>


namespace
{
    constexpr std::string_view UseDefaultEditorConfigKey_sv      = "use-default-editorconfig";
    constexpr std::string_view CreateBranchCopyBeforeResetKey_sv = "create-branch-copy-before-reset";
}


IMPLEMENT_DYNCREATE(CodePurifierView, CFormView)


BEGIN_MESSAGE_MAP(CodePurifierView, CFormView)
    ON_WM_MDIACTIVATE()
    ON_WM_DESTROY()
    ON_MESSAGE(UWM::Stygitan::UpdateUI, OnUpdateUI)
    ON_NOTIFY(NM_CLICK, IDC_WORKING_DIRECTORY, OnWorkingDirectoryClick)
    ON_NOTIFY(NM_RETURN, IDC_WORKING_DIRECTORY, OnWorkingDirectoryClick)
    ON_COMMAND(IDC_CREATE_BRANCH_COPY, OnCreateBranchCopy)
    ON_COMMAND(IDC_DELETE_BRANCH_COPIES, OnDeleteBranchCopies)
    ON_NOTIFY(NM_CUSTOMDRAW, IDC_COMMITS, OnCommitsCustomDraw)
    ON_NOTIFY(NM_RCLICK, IDC_COMMITS, OnCommitsRightClick)
    ON_COMMAND(ID_SET_CLEAN_COMMIT, OnSetCleanCommit)
    ON_COMMAND(IDC_APPLY_EDITORCONFIG_RULES, OnApplyEditorConfigRules)
    ON_COMMAND(IDC_USE_CSPRO_DEFAULT_EDITORCONFIG, OnUseDefaultEditorConfigClick)
    ON_COMMAND(IDC_RESET_BRANCH_TO_CLEAN_COMMIT, OnResetBranchToCleanCommit)
    ON_COMMAND(IDC_CREATE_BRANCH_COPY_BEFORE_RESET, OnCreateBranchCopyBeforeResetClick)
    ON_NOTIFY(NM_DBLCLK, IDC_MODIFIED_FILES, OnModifiedFilesDoubleOrRightClick)
    ON_NOTIFY(NM_RCLICK, IDC_MODIFIED_FILES, OnModifiedFilesDoubleOrRightClick)
    ON_COMMAND(ID_MODIFIED_FILE_OPEN, OnModifiedFileOpen)
    ON_COMMAND(ID_MODIFIED_FILE_OPEN_CONTAINING_FOLDER, OnModifiedFileOpenContainingFolder)
    ON_COMMAND(ID_MODIFIED_FILE_COPY_PATH, OnModifiedFileCopyPath)
    ON_COMMAND(ID_MODIFIED_FILE_DIFF, OnModifiedFileDiff)
END_MESSAGE_MAP()


CodePurifierView::CodePurifierView()
    :   CFormView(IDD_CODE_PURIFIER),
        m_settingsDb("Stygitan.db", "CodePurifier"),
        m_useDefaultEditorConfig(m_settingsDb.ReadOrDefault(UseDefaultEditorConfigKey_sv, true)),
        m_createBranchCopyBeforeReset(m_settingsDb.ReadOrDefault(CreateBranchCopyBeforeResetKey_sv, true)),
        m_cleanCommitIndex(0)
{
}


void CodePurifierView::OnInitialUpdate()
{
    __super::OnInitialUpdate();

    CodePurifierDoc& cp_doc = GetDoc();

    // add the working directory as a link
    WindowsUtf8::SetText(m_hWnd, IDC_WORKING_DIRECTORY, SO::Concatenate(
        "<a>",
        Path::RemoveTrailingSlash(cp_doc.GetRepositoryWorkingDirectory()),
        "</a>"
    ));

    m_commitsListCtrl.SetExtendedStyle(LVS_EX_FULLROWSELECT);
    m_commitsListCtrl.SetHeadings(L"Date,120;Message,435");
    m_commitsListCtrl.LoadColumnInfo();

    m_modifiedFilesListCtrl.SetExtendedStyle(LVS_EX_FULLROWSELECT);
    m_modifiedFilesListCtrl.SetHeadings(L"Path,475;Status,95");
    m_modifiedFilesListCtrl.LoadColumnInfo();

    // because it may take a while to generate the list of modified files,
    // add an indication that this list is pending
    m_modifiedFilesListCtrl.AddItem(L"Identifying modified files...", L"");

    // start Git processing, with updates posted here using the message UWM::Stygitan::UpdateUI
    cp_doc.StartGitProcessing(this);
}


void CodePurifierView::DoDataExchange(CDataExchange* const pDX)
{
    __super::DoDataExchange(pDX);

    DDX_Control(pDX, IDC_BRANCH_COPIES, m_branchCopiesListBox);
    DDX_Control(pDX, IDC_COMMITS, m_commitsListCtrl);
    DDX_Check(pDX, IDC_USE_CSPRO_DEFAULT_EDITORCONFIG, m_useDefaultEditorConfig);
    DDX_Check(pDX, IDC_CREATE_BRANCH_COPY_BEFORE_RESET, m_createBranchCopyBeforeReset);
    DDX_Control(pDX, IDC_MODIFIED_FILES, m_modifiedFilesListCtrl);
}


void CodePurifierView::OnActivateView(const BOOL bActivate, CView* const pActivateView, CView* const pDeactiveView)
{
    __super::OnActivateView(bActivate, pActivateView, pDeactiveView);

    if( bActivate )
    {
        CodePurifierDoc& cp_doc = GetDoc();
        cp_doc.RefreshGit();
    }
}


void CodePurifierView::OnDestroy()
{
    CodePurifierDoc& cp_doc = GetDoc();
    cp_doc.StopGitProcessing();
}


LRESULT CodePurifierView::OnUpdateUI(const WPARAM wParam, LPARAM /*lParam*/)
{
    switch( wParam )
    {
        case CP::Update::BranchDetails:
            UpdateBranchDetails();
            break;

        case CP::Update::BranchCopies:
            UpdateBranchCopies();
            break;

        case CP::Update::CleanCommit:
            UpdateCleanCommit();
            break;

        case CP::Update::RecentCommits:
            UpdateRecentCommits();
            break;

        case CP::Update::ModifiedFiles:
            UpdateModifiedFiles();
            break;

        default:
            return ReturnProgrammingError(0);
    }

    return 1;
}


void CodePurifierView::UpdateBranchDetails()
{
    const CodePurifierDoc& cp_doc = GetDoc();
    const std::shared_ptr<const CP::BranchDetails> branch_details = cp_doc.GetBranchDetails();

    WindowsUtf8::SetText(m_hWnd, IDC_LOCAL_BRANCH,
        ( branch_details != nullptr ) ? branch_details->current_branch.GetName(): "<no branch>"
    );

    WindowsUtf8::SetText(m_hWnd, IDC_REMOTE_BRANCH,
        ( branch_details != nullptr && branch_details->remote_branch != nullptr ) ? branch_details->remote_branch->GetName() : "<no remote branch>"
    );
}


void CodePurifierView::UpdateBranchCopies()
{
    const CodePurifierDoc& cp_doc = GetDoc();
    const std::shared_ptr<const std::map<std::string, GitBranch>> branch_copies = cp_doc.GetBranchCopies();

    m_branchCopiesListBox.ResetContent();

    if( branch_copies == nullptr )
        return;

    for( const auto& [name, branch] : *branch_copies )
        m_branchCopiesListBox.AddString(TC::ToWide(name).c_str());
}


void CodePurifierView::UpdateCleanCommit()
{
    const CodePurifierDoc& cp_doc = GetDoc();
    const std::shared_ptr<const GitCommit> clean_commit = cp_doc.GetCleanCommit();

    if( clean_commit != nullptr )
    {
        const std::string commit_text = FormatText("%s\n(%s)",
            clean_commit->GetMessage().c_str(),
            clean_commit->GetAuthor().GetWhen().GetLocalDateTimeString().c_str()
        );

        WindowsUtf8::SetText(m_hWnd, IDC_CLEAN_COMMIT, commit_text);
    }

    else
    {
        GetDlgItem(IDC_CLEAN_COMMIT)->SetWindowText(L"<no clean commit>");
    }
}


void CodePurifierView::UpdateRecentCommits()
{
    const CodePurifierDoc& cp_doc = GetDoc();

    m_recentCommits = cp_doc.GetRecentCommits();
    m_cleanCommitIndex = 0;

    m_commitsListCtrl.DeleteAllItems();

    if( m_recentCommits == nullptr )
        return;

    const std::shared_ptr<const GitCommit> clean_commit = cp_doc.GetCleanCommit();
    bool reached_clean_commit = false;

    for( const GitCommit& commit : *m_recentCommits )
    {
        m_commitsListCtrl.AddItem(TC::ToWide(commit.GetAuthor().GetWhen().GetLocalDateTimeString()).c_str(),
                                  TC::ToWide(commit.GetMessage()).c_str());

        if( !reached_clean_commit )
        {
            if( clean_commit != nullptr && commit == *clean_commit )
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


void CodePurifierView::UpdateModifiedFiles()
{
    const CodePurifierDoc& cp_doc = GetDoc();

    m_modifiedFiles = cp_doc.GetModifiedFiles();

    m_modifiedFilesListCtrl.DeleteAllItems();

    if( m_modifiedFiles == nullptr )
        return;

    for( const CP::ModifiedFile& modified_file : *m_modifiedFiles )
    {
        m_modifiedFilesListCtrl.AddItem(TC::ToWide(modified_file.git_path).c_str(),
                                        modified_file.GetStatus());
    }
}


void CodePurifierView::OnWorkingDirectoryClick(NMHDR* const /*pNMHDR*/, LRESULT* const pResult)
{
    const CodePurifierDoc& cp_doc = GetDoc();
    OpenContainingFolder(cp_doc.GetRepositoryWorkingDirectory());
    *pResult = 0;
}


void CodePurifierView::OnCreateBranchCopy()
{
    try
    {
        CodePurifierDoc& cp_doc = GetDoc();
        cp_doc.CreateBranchCopy();
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
        cp_doc.DeleteBranchCopies();
    }

    catch( const CSProException& exception )
    {
        ErrorMessage::Display(exception);
    }
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
        // and commits older than the next commit with light gray text
        const int item_index = static_cast<int>(pLVCD->nmcd.dwItemSpec);

        if( item_index < m_cleanCommitIndex )
        {
            pLVCD->clrTextBk = RGB(200, 255, 200);
        }

        else if( item_index > m_cleanCommitIndex )
        {
            pLVCD->clrText= RGB(150, 150, 150);
        }
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
    try
    {
        const size_t index = static_cast<size_t>(m_commitsListCtrl.GetSelectionMark());
        ASSERT(m_recentCommits != nullptr && index < m_recentCommits->size());

        CodePurifierDoc& cp_doc = GetDoc();
        cp_doc.SetCleanCommitOverride(m_recentCommits->at(index));
    }

    catch( const CSProException& exception )
    {
        ErrorMessage::Display(exception);
    }
}


void CodePurifierView::OnApplyEditorConfigRules()
{
    size_t changed = 0;

    try
    {
        if( m_modifiedFiles == nullptr || m_modifiedFiles->empty() )
            throw CSProException("There are no modified files.");

        const CWaitCursor wait_cursor;

        // modified from EditorConfigApplierView::OnApplyRules
        EditorConfig::Evaluator evaluator;
        EditorConfig::Applier editorconfig_applier;

        for( const CP::ModifiedFile& modified_file : *m_modifiedFiles )
        {
            if( modified_file.diff_flag == GIT_DELTA_DELETED )
                continue;

            const std::string file_path = GetFilePathOnDisk(modified_file);
            const EditorConfig::Options options = evaluator.Parse(file_path, m_useDefaultEditorConfig);

            if( options.IsDefined() )
            {
                const BinaryBlock* const processed_file = editorconfig_applier.Process(file_path, options);

                if( processed_file != nullptr )
                {
                    FileIO::Write(file_path, *processed_file);
                    ++changed;
                }
            }
        }

        AfxMessageBox(FormatText("%d file%s modified.", static_cast<int>(changed), PluralizeWord(changed)));
    }

    catch( const CSProException& exception )
    {
        ErrorMessage::Display(exception);
    }

    if( changed != 0 )
    {
        CodePurifierDoc& cp_doc = GetDoc();
        cp_doc.RefreshWorkingDirectory();
    }
}


void CodePurifierView::OnUseDefaultEditorConfigClick()
{
    UpdateData(TRUE);

    m_settingsDb.Write(UseDefaultEditorConfigKey_sv, m_useDefaultEditorConfig);
}


void CodePurifierView::OnResetBranchToCleanCommit()
{
    try
    {
        CodePurifierDoc& cp_doc = GetDoc();
        cp_doc.ResetBranchToCleanCommit(m_createBranchCopyBeforeReset);
    }

    catch( const CSProException& exception )
    {
        ErrorMessage::Display(exception);
    }
}


void CodePurifierView::OnCreateBranchCopyBeforeResetClick()
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

        const auto [file_path, modified_file] = GetSelectedModifiedFile();

        if( !PortableFunctions::FileIsRegular(file_path) )
        {
            file_exists_flag = MF_DISABLED;

            if( !PortableFunctions::FileIsDirectory(PortableFunctions::PathGetDirectory(file_path)) )
                directory_exists_flag = MF_DISABLED;
        }

        CMenu popup_menu;
        popup_menu.CreatePopupMenu();
        popup_menu.AppendMenu(MF_STRING, ID_MODIFIED_FILE_COPY_PATH, L"Copy Full Path");
        popup_menu.AppendMenu(MF_SEPARATOR);
        popup_menu.AppendMenu(MF_STRING | file_exists_flag, ID_MODIFIED_FILE_OPEN, L"Open in Associated Application");
        popup_menu.AppendMenu(MF_STRING | directory_exists_flag, ID_MODIFIED_FILE_OPEN_CONTAINING_FOLDER, L"Open Containing Folder");
        popup_menu.AppendMenu(MF_SEPARATOR);
        popup_menu.AppendMenu(MF_STRING, ID_MODIFIED_FILE_DIFF, L"External Diff\tCtrl+D");

        CPoint point = pNMItemActivate->ptAction;
        m_modifiedFilesListCtrl.ClientToScreen(&point);
        popup_menu.TrackPopupMenu(TPM_RIGHTBUTTON, point.x, point.y, this);
    }
}


std::string CodePurifierView::GetFilePathOnDisk(const CP::ModifiedFile& modified_file)
{
    const CodePurifierDoc& cp_doc = GetDoc();

    return Path::Combine(cp_doc.GetRepositoryWorkingDirectory(),
                         Path::ToNativeSlash(modified_file.git_path));
}


std::tuple<std::string, const CP::ModifiedFile*> CodePurifierView::GetSelectedModifiedFile()
{
    const size_t index = static_cast<size_t>(m_modifiedFilesListCtrl.GetSelectionMark());
    ASSERT(m_modifiedFiles != nullptr && index < m_modifiedFiles->size());

    const CP::ModifiedFile* const modified_file = &m_modifiedFiles->at(index);

    return std::make_tuple(GetFilePathOnDisk(*modified_file),
                           modified_file);
}


void CodePurifierView::OnModifiedFileOpen()
{
    auto [file_path, modified_file] = GetSelectedModifiedFile();

    ShellExecute(nullptr, L"open", TC::ToWide(EscapeCommandLineArgument(std::move(file_path))).c_str(), nullptr, nullptr, SW_SHOW);
}


void CodePurifierView::OnModifiedFileOpenContainingFolder()
{
    const auto [file_path, modified_file] = GetSelectedModifiedFile();

    if( PortableFunctions::FileIsRegular(file_path) )
    {
        OpenContainingFolder(file_path);
    }

    else
    {
        OpenContainingFolder(PortableFunctions::PathGetDirectory(file_path));
    }
}


void CodePurifierView::OnModifiedFileCopyPath()
{
    const auto [file_path, modified_file] = GetSelectedModifiedFile();

    WinClipboard::PutText(this, file_path);
}


void CodePurifierView::OnModifiedFileDiff()
{
    // because this can be called via Ctrl + D, ensure that something is selected
    if( GetFocus() != &m_modifiedFilesListCtrl ||
        m_modifiedFilesListCtrl.GetSelectionMark() < 0 )
    {
        return;
    }

    const auto [file_path, modified_file] = GetSelectedModifiedFile();

    try
    {
        // the default settings work for GIT_DELTA_ADDED or GIT_DELTA_UNTRACKED
        std::string old_file_path;
        const std::string* new_file_path = &file_path;

        // when a file is deleted or modified, we must get the version of the file from the commit's tree
        if( modified_file->diff_flag == GIT_DELTA_DELETED ||
            modified_file->diff_flag == GIT_DELTA_MODIFIED )
        {
            CodePurifierDoc& cp_doc = GetDoc();
            const std::shared_ptr<const GitCommit> clean_commit = cp_doc.GetCleanCommit();

            // if not previously done so (in this session), save the file in the temporary directory;
            // the filename will contain the short Git hash, as well as a short hash of the file path
            // (in case multiple files with the same filename from different directories are accessed)
            const std::string temp_filename_wihout_extension = SO::Concatenate(
                Path::GetFilenameWithoutExtension(file_path),
                "-", ( clean_commit != nullptr ) ? clean_commit->GetObjectId().GetHexHash().substr(0, 7) : ReturnProgrammingError(""),
                "-", Hash::Hash(file_path, 2)
            );

            old_file_path = PortableFunctions::CreateFilePath(
                GetTempDirectory(),
                temp_filename_wihout_extension,
                Path::GetExtension(file_path)
            );

            if( m_comparisonFilePathsForFileDiffs.find(old_file_path) == m_comparisonFilePathsForFileDiffs.cend() )
            {
                cp_doc.SaveFileFromCleanCommit(modified_file->git_path, old_file_path);
                TemporaryFile::RegisterFileForDeletion(old_file_path);
            }

            if( modified_file->diff_flag == GIT_DELTA_DELETED )
                new_file_path = &SO::Empty_string;
        }

        DiffTool::Launch(old_file_path, *new_file_path);
    }

    catch( const CSProException& exception )
    {
        ErrorMessage::Display(exception);
    }
}
