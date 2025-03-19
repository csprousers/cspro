#pragma once

#include <zViewO/zViewO.h>
#include <zViewO/ViewInput.h>
#include <zHtml/VirtualFileMapping.h>


class ZVIEWO_API MarkdownViewInput : public ViewInput
{
public:
    using ViewInput::ViewInput;

    static std::string ToHtml(const std::string& file_path, std::string_view markdown_sv, bool embed_css);
    static std::string ToSaveableHtml(const std::string& file_path, std::string_view markdown_sv) { return ToHtml(file_path, markdown_sv, true); }
    static std::string ToViewableHtml(const std::string& file_path, std::string_view markdown_sv) { return ToHtml(file_path, markdown_sv, false); }

protected:
    void CreateUrl() override;

private:
    std::optional<VirtualFileMapping> m_htmlVirtualFileMapping;
};
