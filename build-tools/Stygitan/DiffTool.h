#pragma once


namespace DiffTool
{
    constexpr std::string_view CommandKey_sv  = "diff-tool-command";
    constexpr std::string_view ArgumentKey_sv = "diff-tool-argument";

    constexpr std::string_view OldReplacementParameter_sv = "~~OLD~~";
    constexpr std::string_view NewReplacementParameter_sv = "~~NEW~~";

    void Launch(std::string_view old_file_path_sv, std::string_view new_file_path_sv, const std::string& command, std::string argument) noexcept;
    void Launch(std::string_view old_file_path_sv, std::string_view new_file_path_sv) noexcept;
}
