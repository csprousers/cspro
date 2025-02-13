#pragma once

#include <zUtilO/SettingsDb.h>


class GlobalSettings
{
public:
    GlobalSettings();

    void Save(bool save_user_settings);

private:
    // the settings database
    SettingsDb settings_db;

public:
    // modifiable by the user in the Global Settings dialog
    bool automatically_associate_documents_with_doc_sets;
    bool build_documents_on_open;
    unsigned automatic_compilation_seconds;
    std::wstring html_help_compiler_path;
    std::wstring wkhtmltopdf_path;
    std::wstring cspro_code_path;

    // modifiable by the user in other areas
    bool close_generate_dialog_on_completion;

    // set automatically by CSDocument
    double build_window_proportion;
    double html_window_proportion;
    double doc_set_tree_window_proportion;
};
