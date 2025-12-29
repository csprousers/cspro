#include "StdAfx.h"
#include "EditorConfigApplierView.h"
#include "EditorConfig.h"
#include <zToolsO/DirectoryLister.h>
#include <zToolsO/File.h>
#include <zGit/GitIndex.h>


namespace
{
    constexpr std::string_view DirectoryKey_sv              = "directory";
    constexpr std::string_view ProcessFilesKey_sv           = "process-files";
    constexpr std::string_view UseDefaultEditorConfigKey_sv = "use-default-editorconfig";

    constexpr int ProcessAllFiles             = 0;
    constexpr int ProcessOnlyFilesInGitIndex  = 1;
    constexpr int ProcessOnlyFilesGitModified = 2;
}


IMPLEMENT_DYNCREATE(EditorConfigApplierView, CFormView)


BEGIN_MESSAGE_MAP(EditorConfigApplierView, CFormView)
    ON_COMMAND(IDC_DIRECTORY_SELECT, OnDirectorySelect)
    ON_COMMAND(IDC_CREATE_LIST_OF_APPLICABLE_RULES, OnCreateListOfApplicableRules)
    ON_COMMAND(IDC_CREATE_LIST_OF_GIT_IGNORED_FILES, OnCreateListOfGitIgnoredFiles)
END_MESSAGE_MAP()


EditorConfigApplierView::EditorConfigApplierView()
    :   CFormView(IDD_EDITORCONFIG_APPLIER),
        m_settingsDb("Stygitan.db", "EditorConfigApplier"),
        m_directory(m_settingsDb.ReadOrDefault<std::string>(DirectoryKey_sv)),
        m_processFilesOption(std::max(ProcessAllFiles, std::min(ProcessOnlyFilesGitModified, m_settingsDb.ReadOrDefault(ProcessFilesKey_sv, ProcessOnlyFilesGitModified)))),
        m_useDefaultEditorConfig(m_settingsDb.ReadOrDefault(UseDefaultEditorConfigKey_sv, true))
{
}


EditorConfigApplierView::~EditorConfigApplierView()
{
}


void EditorConfigApplierView::DoDataExchange(CDataExchange* const pDX)
{
    __super::DoDataExchange(pDX);

    DDX_Text(pDX, IDC_DIRECTORY, m_directory, true);
    DDX_Radio(pDX, IDC_PROCESS_ALL_FILES, m_processFilesOption);
    DDX_Check(pDX, IDC_USE_CSPRO_DEFAULT_EDITORCONFIG, m_useDefaultEditorConfig);

    if( pDX->m_bSaveAndValidate )
    {
        Path::MakeToNativeSlash(m_directory);
        m_settingsDb.Write(DirectoryKey_sv, m_directory);
        m_settingsDb.Write(ProcessFilesKey_sv, m_processFilesOption);
        m_settingsDb.Write(UseDefaultEditorConfigKey_sv, m_useDefaultEditorConfig);
    }
}


void EditorConfigApplierView::OnDirectorySelect()
{
    UpdateData(TRUE);

    std::string directory = SelectFolderDialog(L"Select Directory");

    if( directory.empty() )
        return;

    m_directory = std::move(directory);

    UpdateData(FALSE);
}


struct EditorConfigApplierView::Data
{
    std::string directory;
    int process_files_option;
    bool use_default_editorconfig;
    std::string report_base_file_path;
    std::vector<std::string> file_paths;
    std::map<std::string, EditorConfig::Options> file_options_map;
    std::optional<std::vector<std::string>> git_ignored_file_paths;
};


void EditorConfigApplierView::CreateDataForDirectory()
{
    // only create the directory data when the options have changed
    if( m_data == nullptr ||
        m_directory != m_data->directory ||
        m_processFilesOption != m_data->process_files_option ||
        m_useDefaultEditorConfig != m_data->use_default_editorconfig )
    {
        if( !PortableFunctions::FileIsDirectory(m_directory) )
            throw FileIO::Exception::DirectoryNotFound(m_directory);

        // determine the applicable file paths
        std::vector<std::string> file_paths;
        std::optional<GitRepository> repo;

        if( m_processFilesOption != ProcessAllFiles )
        {
            std::string git_directory = Path::Combine(m_directory, ".git");

            if( PortableFunctions::FileIsDirectory(git_directory) )
            {
                const bool bare = ( m_processFilesOption == ProcessOnlyFilesInGitIndex );
                repo.emplace();
                repo->Open(std::move(git_directory), bare);
            }
        }

        if( !repo.has_value() )
        {
            DirectoryLister directory_lister(true);
            directory_lister.AddPaths(file_paths, m_directory);
        }

        else if( m_processFilesOption == ProcessOnlyFilesInGitIndex )
        {
            GetPathsInGitIndex(*repo, file_paths);
        }

        else
        {
            ASSERT(m_processFilesOption == ProcessOnlyFilesGitModified);
            GetPathsModifiedSinceLastGitRemoteCommit(*repo, file_paths);
        }

        m_data.reset(new Data
            {
                m_directory,
                m_processFilesOption,
                m_useDefaultEditorConfig,
                Path::Combine(m_directory, SO::Concatenate("EditorConfigApplier-", IntToString(GetTimestamp<int64_t>()), "-")),
                std::move(file_paths)
            });
    }
}


void EditorConfigApplierView::GetPathsInGitIndex(GitRepository& repo, std::vector<std::string>& file_paths) const
{
    const GitIndex index = repo.GetIndex();
    const size_t count = index.GetEntryCount();

    for( size_t i = 0; i < count; ++i )
        file_paths.emplace_back(Path::Combine(m_directory, Path::ToNativeSlash(index.GetPathByIndex(i))));
}


void EditorConfigApplierView::GetPathsModifiedSinceLastGitRemoteCommit(GitRepository& repo, std::vector<std::string>& file_paths) const
{
    const GitBranch current_branch = repo.GetCurrentBranch();
    const std::unique_ptr<const GitBranch> remote_branch = current_branch.GetUpstreamBranch();

    if( remote_branch == nullptr )
        return GetPathsInGitIndex(repo, file_paths);

    const GitCommit remote_commit = repo.LookupCommit(remote_branch->GetTarget());

    repo.ForeachDifferenceInWorkingDirectory(remote_commit,
        [&](std::string path, const unsigned int diff_flag)
        {
            if( diff_flag != GIT_DELTA_DELETED )
                file_paths.emplace_back(Path::Combine(m_directory, Path::ToNativeSlash(std::move(path))));
        });
}


void EditorConfigApplierView::ParseFilesUsingEditorConfig()
{
    ASSERT(m_data != nullptr);

    if( !m_data->file_options_map.empty() )
        return;

    EditorConfig::Evaluator evaluator;

    for( const std::string& file_path : m_data->file_paths )
        m_data->file_options_map.try_emplace(file_path, evaluator.Parse(file_path, m_data->use_default_editorconfig));
}


void EditorConfigApplierView::OnCreateListOfApplicableRules()
{
    try
    {
        UpdateData(TRUE);
        CreateDataForDirectory();
        ParseFilesUsingEditorConfig();

        // sort the files into buckets based on their options
        std::map<EditorConfig::Options, std::vector<const std::string*>> options_file_map;

        for( const auto& [file_path, options] : m_data->file_options_map )
            options_file_map[options].emplace_back(&file_path);

        std::string report_file_path;

        // write out the details for each bucket
        for( auto& [options, file_paths] : options_file_map )
        {
            std::sort(file_paths.begin(), file_paths.end(),
                [&](const std::string* const fp1, const std::string* const fp2)
                {
                    return Helpers::CompareFilePathsByDirectory(*fp1, *fp2);
                });

            report_file_path = SO::Concatenate(
                m_data->report_base_file_path,
                options.IsDefined() ? Path::CreateValidFilename(options.GetShortDescription()) : "ignored",
                ".txt"
            );

            FileIO::TextFile report;
            report.SetTextEncoding(TextEncoding::Type::Utf8);
            report.OpenForTextWritingCreate(report_file_path);

            if( options.IsDefined() )
                report.WriteLine(options.GetLongDescription());

            for( const std::string* const file_path : file_paths )
                report.WriteLine(*file_path);
        }

        if( !report_file_path.empty() )
            OpenContainingFolder(report_file_path);
    }

    catch( const CSProException& exception )
    {
        ErrorMessage::Display(exception);
    }
}


void EditorConfigApplierView::OnCreateListOfGitIgnoredFiles()
{
    try
    {
        UpdateData(TRUE);

        if( m_processFilesOption == ProcessAllFiles )
            throw CSProException("No files are ignored when not using Git.");

        CreateDataForDirectory();

        // calculate files ignored based on only looking at Git files
        if( !m_data->git_ignored_file_paths.has_value() )
        {
            DirectoryLister directory_lister(true);
            m_data->git_ignored_file_paths = directory_lister.GetPaths(m_directory);

            // sort the applicable file paths to enable for quick searching
            std::sort(m_data->file_paths.begin(), m_data->file_paths.end(),
                      [&](const std::string& fp1, const std::string& fp2) { return ( SO::CompareNoCase(fp1, fp2) < 0 ); });

            // remove duplicate paths
            const auto& remove_end = std::remove_if(m_data->git_ignored_file_paths->begin(), m_data->git_ignored_file_paths->end(),
                [&](const std::string& file_path)
                {
                    return std::binary_search(m_data->file_paths.begin(), m_data->file_paths.end(), file_path,
                                                [&](const std::string& fp1, const std::string& fp2) { return ( SO::CompareNoCase(fp1, fp2) < 0 ); });
                });

            m_data->git_ignored_file_paths->erase(remove_end, m_data->git_ignored_file_paths->end());

            std::sort(m_data->git_ignored_file_paths->begin(), m_data->git_ignored_file_paths->end(),
                [&](const std::string& fp1, const std::string& fp2)
                {
                    return Helpers::CompareFilePathsByDirectory(fp1, fp2);
                });
        }

        // write out the details about Git-ignored files
        const std::string report_file_path = m_data->report_base_file_path + "files-git-ignored.txt";

        FileIO::WriteText(
            report_file_path,
            SO::CreateSingleString(*m_data->git_ignored_file_paths, SO::Newline_lf_sv).append(SO::Newline_lf_sv),
            false
        );

        OpenContainingFolder(report_file_path);
    }

    catch( const CSProException& exception )
    {
        ErrorMessage::Display(exception);
    }
}
