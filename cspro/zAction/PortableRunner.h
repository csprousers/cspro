#pragma once

#include <zAction/ActionInvoker.h>


namespace ActionInvoker
{
    class PortableRunner
    {
    public:
        // Clipboard
        static std::optional<std::wstring> ClipboardGetText();
        static void ClipboardPutText(const std::wstring& text);

        // Path
        static Result PathShowNativeFileDialog(const std::wstring& start_directory, bool open_file_dialog, bool confirm_overwrite,
                                               const TCHAR* name, const TCHAR* filter, const JsonNode<wchar_t>& json_node);

        // System
        static std::vector<std::tuple<std::wstring, std::wstring>> SystemShowSelectDocumentDialog(const std::vector<std::wstring>& mime_types, bool multiple);
    };
}
