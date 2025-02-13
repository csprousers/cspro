#pragma once


struct HtmlDialogTemplate
{
    std::wstring filename;
    std::wstring description;
    std::wstring subdescription;

    struct Sample
    {
        std::wstring description;
        std::wstring input;
    };

    std::vector<Sample> samples;
};


class HtmlDialogTemplateFile
{
public:
    HtmlDialogTemplateFile();

    const std::vector<HtmlDialogTemplate>& GetTemplates() const { return m_htmlDialogTemplates; }

    std::optional<std::wstring> GetDefaultInputText(const std::wstring& filename) const;

private:
    static std::vector<HtmlDialogTemplate> ReadTemplates();

private:
    static std::vector<HtmlDialogTemplate> m_htmlDialogTemplates;
};
