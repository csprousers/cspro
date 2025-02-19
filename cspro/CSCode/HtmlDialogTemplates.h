#pragma once


struct HtmlDialogTemplate
{
    std::string filename;
    std::string description;
    std::string subdescription;

    struct Sample
    {
        std::string description;
        SharableString input;
    };

    std::vector<Sample> samples;
};


class HtmlDialogTemplateFile
{
public:
    HtmlDialogTemplateFile();

    const std::vector<HtmlDialogTemplate>& GetTemplates() const { return m_htmlDialogTemplates; }

    SharableString GetDefaultInputText(const std::string& file_path) const;

private:
    static std::vector<HtmlDialogTemplate> ReadTemplates();

private:
    static std::vector<HtmlDialogTemplate> m_htmlDialogTemplates;
};
