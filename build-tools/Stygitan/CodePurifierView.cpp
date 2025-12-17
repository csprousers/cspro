#include "StdAfx.h"
#include "CodePurifierView.h"


IMPLEMENT_DYNCREATE(CodePurifierView, CFormView)


BEGIN_MESSAGE_MAP(CodePurifierView, CFormView)
    ON_MESSAGE(UWM::Stygitan::AppActivated, OnAppActivated)
    ON_MESSAGE(UWM::Stygitan::UpdateUI, OnUpdateUI)
    ON_NOTIFY(NM_CLICK, IDC_WORKING_DIRECTORY, OnWorkingDirectoryClick)
    ON_NOTIFY(NM_RETURN, IDC_WORKING_DIRECTORY, OnWorkingDirectoryClick)
    ON_COMMAND(IDC_CREATE_BRANCH_COPY, OnCreateBranchCopy)
    ON_COMMAND(IDC_DELETE_BRANCH_COPIES, OnDeleteBranchCopies)
END_MESSAGE_MAP()


namespace Update
{
    constexpr WPARAM All          = 0xff;
    constexpr WPARAM BranchCopies = 0x01;
}


CodePurifierView::CodePurifierView()
    :   CFormView(IDD_CODE_PURIFIER),
        m_lastRefreshTime(0)
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

    PostMessage(UWM::Stygitan::UpdateUI, Update::All);
}


void CodePurifierView::DoDataExchange(CDataExchange* const pDX)
{
    __super::DoDataExchange(pDX);

    DDX_Control(pDX, IDC_BRANCH_COPIES, m_branchCopiesListBox);
}


LRESULT CodePurifierView::OnAppActivated(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
    constexpr int64_t RefreshSecondsInterval = 5;

    // when coming back to this view after some time in another application, refresh the data,
    // closing the view on error;
    // the interval check also prevents refreshing the data immediately after OnInitialUpdate
    if( ( GetTimestamp<int64_t>() - m_lastRefreshTime ) < RefreshSecondsInterval )
        return 1;

    try
    {
        CodePurifierDoc& cp_doc = GetDoc();
        cp_doc.RefreshData();

        PostMessage(UWM::Stygitan::UpdateUI, Update::All);
    }

    catch( const CSProException& exception )
    {
        ErrorMessage::Display(exception);
        GetParentFrame()->PostMessage(WM_CLOSE);
    }

    return 1;
}


LRESULT CodePurifierView::OnUpdateUI(const WPARAM wParam, LPARAM /*lParam*/)
{
    const CodePurifierDoc& cp_doc = GetDoc();

    if( ( wParam & Update::All ) == Update::All )
    {
        WindowsUtf8::SetText(m_hWnd, IDC_LOCAL_BRANCH, cp_doc.m_currentBranch->GetName());

        WindowsUtf8::SetText(m_hWnd, IDC_REMOTE_BRANCH,
            ( cp_doc.m_remoteBranch != nullptr ) ? cp_doc.m_remoteBranch->GetName() : "<no remote branch>"
        );

        m_lastRefreshTime = GetTimestamp<int64_t>();
    }

    if( ( wParam & Update::BranchCopies ) == Update::BranchCopies )
    {
        m_branchCopiesListBox.ResetContent();

        for( const auto& [name, branch] : cp_doc.m_branchCopies )
            m_branchCopiesListBox.AddString(TC::ToWide(name).c_str());
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
