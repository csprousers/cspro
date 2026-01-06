#include "StdAfx.h"
#include "DiffTool.h"
#include "MainFrame.h"


void DiffTool::Launch(const std::string_view old_file_path_sv, const std::string_view new_file_path_sv,
                      const std::string& command, std::string argument) noexcept
{
    SO::RecursiveReplace(argument, OldReplacementParameter_sv, old_file_path_sv);
    SO::RecursiveReplace(argument, NewReplacementParameter_sv, new_file_path_sv);

    const HINSTANCE result = ShellExecute(nullptr, nullptr, TC::ToWide(command).c_str(), TC::ToWide(argument).c_str(), nullptr, SW_SHOW);

    if( reinterpret_cast<INT_PTR>(result) < 32 )
        ErrorMessage::PostMessageForDisplay(FormatText("Error launching: %s", command.c_str()));
}


void DiffTool::Launch(const std::string_view old_file_path_sv, const std::string_view new_file_path_sv) noexcept
{
    SettingsDb& global_settings_db = assert_cast<MainFrame*>(AfxGetMainWnd())->GetGlobalSettingsDb();
    const std::string* const command = global_settings_db.Read<std::string*>(CommandKey_sv);

    if( command == nullptr )
    {
        ErrorMessage::PostMessageForDisplay("Go to File -> Properties to specify the diff tool.");
    }

    else
    {
        Launch(old_file_path_sv, new_file_path_sv, *command, global_settings_db.ReadOrDefault<std::string>(ArgumentKey_sv));
    }
}
