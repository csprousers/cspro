#pragma once

#include <zReportO/zReportO.h>
#include <external/gumbo/gumbo.h>


class ZREPORTO_API HtmlTagModifier
{
public:
    virtual ~HtmlTagModifier() { }

    std::string Process(cs::string_sz html_input);

protected:
    virtual void ProcessTag(std::string& start_tag, std::string* end_tag) = 0;

private:
    void ProcessNode(const GumboNode* node);
    void ProcessElement(const GumboElement& element);

private:
    std::string* m_html = nullptr;
};
