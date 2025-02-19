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

    // Returns the questionnaire view HTML, or a default page if there are errors reading the HTML.
    static std::string GetQuestionnaireViewHtml();

    // Returns the URL that can be used to view questionnaires.
    const std::string& GetUrl();

    // Returns the parameters that should be returned to the Action Invoker CS.UI.getInputData function.
    std::string GetInputData();

    // Displays the questionnaire view in an embedded browser.
    void View(const ViewerOptions* viewer_options = nullptr);

protected:
    // methods that subclasses must override
    virtual std::string GetDictionaryName() = 0;
    virtual std::string GetCurrentLanguageName() = 0;
    virtual bool ShowLanguageBar() = 0;
    virtual std::string GetDirectoryForUrl() = 0;

    // methods that subclasses can override
    virtual const std::string& GetCaseUuid() { return SO::Empty_string; }
    virtual const std::string& GetCaseKey()  { return SO::Empty_string; }

private:
    SharableString m_html;

    using HtmlContentServer = std::variant<VirtualFileMapping, std::unique_ptr<DataVirtualFileMappingHandler<const std::string*>>>;
    std::map<std::string, HtmlContentServer> m_htmlContentServers;
};
