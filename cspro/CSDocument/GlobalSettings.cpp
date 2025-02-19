#include "StdAfx.h"
#include "GlobalSettings.h"


namespace Settings
{
    constexpr std::string_view AutomaticallyAssociateDocumentsWithDocSets_sv = "AutomaticallyAssociateDocumentsWithDocumentSets";
    constexpr std::string_view BuildDocumentsOnOpen_sv                       = "BuildDocumentsOnOpen";
    constexpr std::string_view AutomaticCompilationSeconds_sv                = "AutomaticCompilationSeconds";
    constexpr std::string_view HtmlHelpCompilerExe_sv                        = "HtmlHelpCompilerExe";
    constexpr std::string_view wkhtmltopdfExe_sv                             = "wkhtmltopdfExe";
    constexpr std::string_view CSProCodeDirectory_sv                         = "CSProCodeDirectory";
    constexpr std::string_view CloseGenerateDialogOnCompletion_sv            = "CloseGenerateDialogOnCompletion";
    constexpr std::string_view BuildWindowProportion_sv                      = "BuildWindowProportion";
    constexpr std::string_view HtmlWindowProportion_sv                       = "HtmlWindowProportion";
    constexpr std::string_view DocSetTreeWindowProportion_sv                 = "DocSetTreeWindowProportion";
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
        automatically_associate_documents_with_doc_sets(settings_db.ReadOrDefault(Settings::AutomaticallyAssociateDocumentsWithDocSets_sv, Default::AutomaticallyAssociateDocumentsWithDocSets)),
        build_documents_on_open(settings_db.ReadOrDefault(Settings::BuildDocumentsOnOpen_sv, Default::BuildDocumentsOnOpen)),
        automatic_compilation_seconds(settings_db.ReadOrDefault(Settings::AutomaticCompilationSeconds_sv, Default::AutomaticCompilationSeconds)),
        cspro_code_path(settings_db.ReadOrDefault(Settings::CSProCodeDirectory_sv, SO::Empty_string)),
        close_generate_dialog_on_completion(settings_db.ReadOrDefault(Settings::CloseGenerateDialogOnCompletion_sv, Default::CloseGenerateDialogOnCompletion)),
        build_window_proportion(settings_db.ReadOrDefault(Settings::BuildWindowProportion_sv, Default::BuildWindowProportion)),
        html_window_proportion(settings_db.ReadOrDefault(Settings::HtmlWindowProportion_sv, Default::HtmlWindowProportion)),
        doc_set_tree_window_proportion(settings_db.ReadOrDefault(Settings::DocSetTreeWindowProportion_sv, Default::DocSetTreeWindowProportion))
{
    // if this is the first time trying to locate one of the executables, try to set them automatically
    bool save_user_settings = false;

    auto set_executable_from_settings = [&](std::string& path, const std::string_view key_sv)
    {
        const std::string* const path_from_settings = settings_db.Read<std::string*>(key_sv);

        if( path_from_settings != nullptr )
        {
            path = *path_from_settings;
            return true;
        }

        return false;
    };

    auto set_executable_if_exists = [&](std::string& path, const WindowsSpecialFolder folder, const std::initializer_list<const char*> components)
    {
        std::string test_path = GetWindowsSpecialFolder(folder);

        for( const char* const component : components )
            Path::MakeCombine(test_path, component);

        if( PortableFunctions::FileIsRegular(test_path) )
        {
            path = std::move(test_path);
            save_user_settings = true;
        }
    };

    if( !set_executable_from_settings(html_help_compiler_path, Settings::HtmlHelpCompilerExe_sv) )
        set_executable_if_exists(html_help_compiler_path, WindowsSpecialFolder::ProgramFiles32, { "HTML Help Workshop", "hhc.exe" });

    if( !set_executable_from_settings(wkhtmltopdf_path, Settings::wkhtmltopdfExe_sv) )
        set_executable_if_exists(wkhtmltopdf_path, WindowsSpecialFolder::ProgramFiles64, { "wkhtmltopdf", "bin", "wkhtmltopdf.exe" });

    if( save_user_settings )
        Save(true);
}


void GlobalSettings::Save(const bool save_user_settings)
{
    if( save_user_settings )
    {
        settings_db.Write(Settings::AutomaticallyAssociateDocumentsWithDocSets_sv, automatically_associate_documents_with_doc_sets);
        settings_db.Write(Settings::BuildDocumentsOnOpen_sv, build_documents_on_open);
        settings_db.Write(Settings::AutomaticCompilationSeconds_sv, automatic_compilation_seconds);
        settings_db.Write(Settings::HtmlHelpCompilerExe_sv, html_help_compiler_path);
        settings_db.Write(Settings::wkhtmltopdfExe_sv, wkhtmltopdf_path);
        settings_db.Write(Settings::CSProCodeDirectory_sv, cspro_code_path);
    }

    settings_db.Write(Settings::CloseGenerateDialogOnCompletion_sv, close_generate_dialog_on_completion);
    settings_db.Write(Settings::BuildWindowProportion_sv, build_window_proportion);
    settings_db.Write(Settings::HtmlWindowProportion_sv, html_window_proportion);
    settings_db.Write(Settings::DocSetTreeWindowProportion_sv, doc_set_tree_window_proportion);
}
