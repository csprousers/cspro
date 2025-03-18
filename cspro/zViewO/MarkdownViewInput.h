#pragma once

#include <zViewO/zViewO.h>
#include <zViewO/ViewInput.h>
#include <zHtml/VirtualFileMapping.h>


class ZVIEWO_API MarkdownViewInput : public ViewInput
{
public:
    using ViewInput::ViewInput;

    static std::string ToViewableHtml(const std::string& file_path, std::string_view markdown_sv);

protected:
    void CreateUrl() override;

private:
    std::optional<VirtualFileMapping> m_htmlVirtualFileMapping;
};
