#include "StdAfx.h"
#include "CreateReleaseDlg.h"
#include "MarkdownViewer.h"
#include <zUtilO/DynamicLayoutControlResizer.h>
#include <zUtilO/FileDlg.h>


BEGIN_MESSAGE_MAP(CreateReleaseDlg, DynamicLayoutResizableDlg)
    ON_COMMAND(IDC_VIEW_README, OnViewReadme)
    ON_COMMAND(IDC_SELECT_INSTALLER_32, OnSelectInstaller32)
    ON_COMMAND(IDC_PREVIEW_RELEASE_NOTES, OnPreviewReleaseNotes)
END_MESSAGE_MAP()


CreateReleaseDlg::CreateReleaseDlg(std::unique_ptr<ReleaseCreator> release_creator, CWnd* const pParent/* = nullptr*/)
    :   DynamicLayoutResizableDlg(IDD_CREATE_RELEASE, pParent),
        m_releaseCreator(std::move(release_creator)),
        m_tagName(m_releaseCreator->GetTagName()),
        m_librariesTag(m_releaseCreator->GetLibrariesTag()),
        m_versionText(m_releaseCreator->GetVersionText()),
        m_readmeRepoPath("build-tools/Installer Inputs/readme.txt"),
        m_releaseTitle(*m_tagName),
        m_releaseType(m_releaseCreator->GetPrerelease() ? 1 : 0)
{
    SerializeDialogSize("CreateReleaseDlg");
}


void CreateReleaseDlg::DoDataExchange(CDataExchange* const pDX)
{
    __super::DoDataExchange(pDX);

    DDX_Text(pDX, IDC_TAG_NAME, m_tagName);
    DDX_Text(pDX, IDC_LIBRARIES_TAG, m_librariesTag);
    DDX_Text(pDX, IDC_RELEASE_VERSION, m_versionText);

    DDX_Text(pDX, IDC_README, m_readmeRepoPath, true);
    DDX_Text(pDX, IDC_INSTALLER_32, m_installer32BitFilePath, true);

    DDX_Text(pDX, IDC_RELEASE_TITLE, m_releaseTitle, true);
    DDX_Radio(pDX, IDC_FULL_RELEASE, m_releaseType);
    DDX_Control(pDX, IDC_RELEASE_NOTES, m_releaseNotesLogicCtrl);
}


BOOL CreateReleaseDlg::OnInitDialog()
{
    __super::OnInitDialog();

    WindowHelpers::RemoveDialogSystemIcon(*this);

    m_releaseNotesLogicCtrl.ReplaceCEdit(this, false, false, SCLEX_MARKDOWN);
    m_releaseNotesLogicCtrl.SetText(m_releaseCreator->GetReleaseNotes());

    return TRUE;
}


std::vector<std::tuple<CWnd*, SizingDirection>> CreateReleaseDlg::GetDynamicLayoutControls()
{
    return { { &m_releaseNotesLogicCtrl, SizingDirection::XY } };
}


void CreateReleaseDlg::UpdateAndValidateInputs(const bool validate_files)
{
    UpdateData(TRUE);

    if( SO::IsWhitespace(m_releaseTitle) )
        throw CSProException("Add a release title.");

    m_releaseCreator->SetReleaseTitle(m_releaseTitle);

    m_releaseCreator->SetPrerelease(( m_releaseType == 1 ));

    std::string release_notes = m_releaseNotesLogicCtrl.GetText();

    if( SO::IsWhitespace(release_notes) )
        throw CSProException("Add release notes.");

    m_releaseCreator->SetReleaseNotes(std::move(release_notes));

    if( validate_files )
        UpdateAndValidateFileInputs();
}


void CreateReleaseDlg::UpdateAndValidateFileInputs()
{
    const std::string version_and_filename_id = *m_versionText + m_releaseCreator->GetReleaseFilenameIdentifier();

    // the readme will be named: cspro-<full version><release filename ID>-release-notes.txt
    auto get_readme_filename = [&]()
    {
        return FormatText("cspro-%s-release-notes.txt", version_and_filename_id.c_str());
    };

    // the installer will be named: cspro-<full version><release filename ID>-windows-<architecture>.exe
    auto get_installer_filename = [&](const char* const architecture)
    {
        return FormatText("cspro-%s-windows-%s.exe", version_and_filename_id.c_str(), architecture);
    };

    LoadAndSetAsset("readme", get_readme_filename(), m_readmeRepoPath, true);
    LoadAndSetAsset("installer (32-bit)", get_installer_filename("x86"), m_installer32BitFilePath, false);
}


void CreateReleaseDlg::LoadAndSetAsset(const char* const type, std::string filename, const std::string& path, const bool is_repo_path)
{
    std::shared_ptr<const BinaryBlock> data;

    if( SO::IsWhitespace(path) )
        throw CSProException("Specify the path of the %s", type);

    if( is_repo_path )
    {
        data = m_releaseCreator->GetFileInOpenSourceRepository(path);
    }

    else
    {
        auto lookup = m_loadedFiles.find(path);

        if( lookup == m_loadedFiles.cend() )
            lookup = m_loadedFiles.try_emplace(path, std::make_unique<BinaryBlock>(FileIO::ReadBinary(path))).first;

        data = lookup->second;
    }

    ASSERT(data != nullptr);

    m_releaseCreator->SetAsset(std::move(filename), std::move(data));
}


void CreateReleaseDlg::OnViewReadme()
{
    UpdateData(TRUE);

    try
    {
        const std::shared_ptr<const BinaryBlock> readme = m_releaseCreator->GetFileInOpenSourceRepository(m_readmeRepoPath);

        // write the file to the disk and then show it in its associated application
        m_readmeTemporaryFile = TemporaryFile::FromExistingPath(GetUniqueTempFilePath(Path::GetFilename(m_readmeRepoPath)));

        FileIO::Write(m_readmeTemporaryFile->GetPath(), *readme);

        Viewer().ViewFile(m_readmeTemporaryFile->GetPath());
    }

    catch( const CSProException& exception )
    {
        ErrorMessage::Display(exception);
    }
}


void CreateReleaseDlg::OnSelectInstaller32()
{
    UpdateData(TRUE);

    OpenFileDlg open_file_dlg(0, nullptr, m_installer32BitFilePath, L"Executables (*.exe)|*.exe|All Files (*.*)|*.*||", this);
    open_file_dlg.SetTitle(L"Select the Installer");

    if( open_file_dlg.DoModal() != IDOK )
        return;

    m_installer32BitFilePath = open_file_dlg.GetFilePath();

    UpdateData(FALSE);
}


void CreateReleaseDlg::OnPreviewReleaseNotes()
{
    try
    {
        UpdateAndValidateInputs(false);

        MarkdownViewer::ShowHtmlInDialog(
            MarkdownViewer::MarkdownToHtmlDocument("Release Notes", m_releaseCreator->GetFormattedReleaseNotes())
        );
    }

    catch( const CSProException& exception )
    {
        ErrorMessage::Display(exception);
    }
}


void CreateReleaseDlg::OnOK()
{
    try
    {
        UpdateAndValidateInputs(true);

        __super::OnOK();
    }

    catch( const CSProException& exception )
    {
        ErrorMessage::Display(exception);
    }
}
