#pragma once

#include <zAction/ActionInvoker.h>

namespace ActionInvoker { class PortableRunner; }


class ActionInvoker::PortableRunner
{
public:
    // Clipboard
    static SharableString Clipboard_GetText();
    static void Clipboard_PutText(const std::string_view text_sv);

    // Path
    static Result Path_ShowNativeFileDialog(const std::string& start_directory, bool open_file_dialog, bool confirm_overwrite,
                                            const std::optional<std::string>& name, const std::optional<std::string>& filter,
                                            const JsonNode& json_node);

    // System
    static void System_CreateShortcut(const std::string& shortcut_id, const std::string& target_file_path, const std::optional<std::string>& icon_file_path,
                                      const std::string& label, const std::optional<std::string>& long_label);

    static std::vector<std::tuple<std::string, std::string>> System_ShowSelectDocumentDialog(const std::vector<std::string>& mime_types, bool multiple);
};
