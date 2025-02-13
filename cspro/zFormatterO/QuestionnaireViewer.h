#pragma once

#include <zFormatterO/zFormatterO.h>
#include <zFormatterO/QuestionnaireContentCreator.h>
#include <zHtml/VirtualFileMapping.h>

struct ViewerOptions;


class ZFORMATTERO_API QuestionnaireViewer : public QuestionnaireContentCreator
{
public:
    QuestionnaireViewer();
    virtual ~QuestionnaireViewer();

    QuestionnaireViewer(const QuestionnaireViewer&) = delete;
    QuestionnaireViewer& operator=(const QuestionnaireViewer&) = delete;

    // returns the URL that can be used to view questionnaires
    const std::wstring& GetUrl();

    // returns the parameters that should be returned to the Action Invoker CS.UI.getInputData function
    std::wstring GetInputData();

    // displays the questionnaire view in an embedded browser
    void View(const ViewerOptions* viewer_options = nullptr);

protected:
    // methods that subclasses must override
    virtual std::wstring GetDictionaryName() = 0;
    virtual std::wstring GetCurrentLanguageName() = 0;
    virtual bool ShowLanguageBar() = 0;
    virtual std::wstring GetDirectoryForUrl() = 0;

    // methods that subclasses can override
    virtual const std::wstring& GetCaseUuid() { return SO::EmptyString; }
    virtual const std::wstring& GetCaseKey()  { return SO::EmptyString; }

private:
    std::string m_html;

    using HtmlContentServer = std::variant<VirtualFileMapping, std::unique_ptr<DataVirtualFileMappingHandler<const std::string*>>>;
    std::map<std::wstring, HtmlContentServer> m_htmlContentServers;
};
