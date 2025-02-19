#include "stdafx.h"
#include "PortableRunner.h"


SharableString ActionInvoker::PortableRunner::Clipboard_GetText()
{
    throw CSProException("Not implemented: PortableRunner::Clipboard_GetText");
}


void ActionInvoker::PortableRunner::Clipboard_PutText(std::string_view /*text_sv*/)
{
    throw CSProException("Not implemented: PortableRunner::Clipboard_PutText");
}


ActionInvoker::Result ActionInvoker::PortableRunner::Path_ShowNativeFileDialog(const std::string& /*start_directory*/, bool /*open_file_dialog*/, bool /*confirm_overwrite*/,
                                                                               const std::optional<std::string>& /*name*/, const std::optional<std::string>& /*filter*/,
                                                                               const JsonNode& /*json_node*/)
{
    throw CSProException("Not implemented: PortableRunner::Path_ShowNativeFileDialog");
}


void ActionInvoker::PortableRunner::System_CreateShortcut(const std::string& /*shortcut_id*/, const std::string& /*target_file_path*/,
                                                          const std::optional<std::string>& /*icon_file_path*/,
                                                          const std::string& /*label*/, const std::optional<std::string>& /*long_label*/)
{
    throw CSProException("Not implemented: PortableRunner::System_CreateShortcut");
}


std::vector<std::tuple<std::string, std::string>> ActionInvoker::PortableRunner::System_ShowSelectDocumentDialog(const std::vector<std::string>& /*mime_types*/, bool /*multiple*/)
{
    throw CSProException("Not implemented: PortableRunner::System_ShowSelectDocumentDialog");
}
