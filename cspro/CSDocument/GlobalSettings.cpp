#include "StdAfx.h"
#include "GlobalSettings.h"


namespace Settings
{
    constexpr wstring_view AutomaticallyAssociateDocumentsWithDocSets = _T("AutomaticallyAssociateDocumentsWithDocumentSets");
    constexpr wstring_view BuildDocumentsOnOpen                       = _T("BuildDocumentsOnOpen");
    constexpr wstring_view AutomaticCompilationSeconds                = _T("AutomaticCompilationSeconds");
    constexpr wstring_view HtmlHelpCompilerExe                        = _T("HtmlHelpCompilerExe");
    constexpr wstring_view wkhtmltopdfExe                             = _T("wkhtmltopdfExe");
    constexpr wstring_view CSProCodeDirectory                         = _T("CSProCodeDirectory");
    constexpr wstring_view CloseGenerateDialogOnCompletion            = _T("CloseGenerateDialogOnCompletion");
    constexpr wstring_view BuildWindowProportion                      = _T("BuildWindowProportion");
    constexpr wstring_view HtmlWindowProportion                       = _T("HtmlWindowProportion");
    constexpr wstring_view DocSetTreeWindowProportion                 = _T("DocSetTreeWindowProportion");
}


namespace Default
{
    constexpr bool AutomaticallyAssociateDocumentsWithDocSets = true;
    constexpr bool BuildDocumentsOnOpen                       = true;
    constexpr unsigned AutomaticCompilationSeconds            = 15;
    constexpr bool CloseGenerateDialogOnCompletion            = false;
    constexpr double BuildWindowProportion                    = 0.15;
    constexpr double HtmlWindowProportion                     = 0.40;
    constexpr double DocSetTreeWindowProportion               = 0.30;
}


GlobalSettings::GlobalSettings()
    :   settings_db(CSProExecutables::Program::CSDocument),
        automatically_associate_documents_with_doc_sets(settings_db.ReadOrDefault(Settings::AutomaticallyAssociateDocumentsWithDocSets, Default::AutomaticallyAssociateDocumentsWithDocSets)),
        build_documents_on_open(settings_db.ReadOrDefault(Settings::BuildDocumentsOnOpen, Default::BuildDocumentsOnOpen)),
        automatic_compilation_seconds(settings_db.ReadOrDefault(Settings::AutomaticCompilationSeconds, Default::AutomaticCompilationSeconds)),
        cspro_code_path(settings_db.ReadOrDefault(Settings::CSProCodeDirectory, SO::EmptyString)),
        close_generate_dialog_on_completion(settings_db.ReadOrDefault(Settings::CloseGenerateDialogOnCompletion, Default::CloseGenerateDialogOnCompletion)),
        build_window_proportion(settings_db.ReadOrDefault(Settings::BuildWindowProportion, Default::BuildWindowProportion)),
        html_window_proportion(settings_db.ReadOrDefault(Settings::HtmlWindowProportion, Default::HtmlWindowProportion)),
        doc_set_tree_window_proportion(settings_db.ReadOrDefault(Settings::DocSetTreeWindowProportion, Default::DocSetTreeWindowProportion))
{
    // if this is the first time trying to locate one of the executables, try to set them automatically
    bool save_user_settings = false;

    auto set_executable_from_settings = [&](std::wstring& path, wstring_view key)
    {
        const std::wstring* path_from_settings = settings_db.Read<const std::wstring*>(key);

        if( path_from_settings != nullptr )
        {
            path = *path_from_settings;
            return true;
        }

        return false;
    };

    auto set_executable_if_exists = [&](std::wstring& path, WindowsSpecialFolder folder, std::initializer_list<const TCHAR*> components)
    {
        std::wstring test_path = GetWindowsSpecialFolder(folder);

        for( const TCHAR* component : components )
            test_path = PortableFunctions::PathAppendToPath(test_path, component);

        if( PortableFunctions::FileIsRegular(test_path) )
        {
            path = std::move(test_path);
            save_user_settings = true;
        }
    };

    if( !set_executable_from_settings(html_help_compiler_path, Settings::HtmlHelpCompilerExe) )
        set_executable_if_exists(html_help_compiler_path, WindowsSpecialFolder::ProgramFiles32, { _T("HTML Help Workshop"), _T("hhc.exe") });

    if( !set_executable_from_settings(wkhtmltopdf_path, Settings::wkhtmltopdfExe) )
        set_executable_if_exists(wkhtmltopdf_path, WindowsSpecialFolder::ProgramFiles64, { _T("wkhtmltopdf"), _T("bin"), _T("wkhtmltopdf.exe") });

    if( save_user_settings )
        Save(true);
}


void GlobalSettings::Save(bool save_user_settings)
{
    if( save_user_settings )
    {
        settings_db.Write(Settings::AutomaticallyAssociateDocumentsWithDocSets, automatically_associate_documents_with_doc_sets);
        settings_db.Write(Settings::BuildDocumentsOnOpen, build_documents_on_open);
        settings_db.Write(Settings::AutomaticCompilationSeconds, automatic_compilation_seconds);
        settings_db.Write(Settings::HtmlHelpCompilerExe, html_help_compiler_path);
        settings_db.Write(Settings::wkhtmltopdfExe, wkhtmltopdf_path);
        settings_db.Write(Settings::CSProCodeDirectory, cspro_code_path);
    }

    settings_db.Write(Settings::CloseGenerateDialogOnCompletion, close_generate_dialog_on_completion);
    settings_db.Write(Settings::BuildWindowProportion, build_window_proportion);
    settings_db.Write(Settings::HtmlWindowProportion, html_window_proportion);
    settings_db.Write(Settings::DocSetTreeWindowProportion, doc_set_tree_window_proportion);
}
