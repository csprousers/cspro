#pragma once

class VirtualFileMappingHandler;


class ContentCreator
{
public:
    virtual ~ContentCreator();

    // Returns the command ID for this page.
    virtual UINT GetCommandId() const = 0;

    // Returns the title for the Save View dialog (or nullptr if not saveable).
    virtual const wchar_t* GetSaveTitle() const = 0;

    // Returns a vector of extensions in which the content can be saved.
    // The default extension should be the first in the vector.
    // The default implementation returns HTML.
    virtual std::vector<const char*> GetSaveFormats() const;

    // Returns the suggested filename (without an extension) for a saved file.
    // Invalid characters in the filename will be stripped by the calling function.
    virtual std::string GetSaveSuggestedFilename() const = 0;

    // Returns true based on whether the page needs to be updated based on various changes.
    virtual bool ContentChangesOnCaseListingSettingsChange() const = 0;
    virtual bool ContentChangesOnCaseListingSelectionsChange() const = 0;

    // Returns true if the page uses the Action Invoker.
    // The default implementation returns false.
    virtual bool UsesActionInvoker() const;

    // Returns the page's input data requested by the Action Invoker.
    // The default implementation throws ProgrammingErrorException.
    virtual SharableString GetActionInvokerInputData();

    // The default implementation returns 0 (no additional menu options).
    virtual UINT GetViewOptionsMenuResourceId() const;

    // Returns true if the page should be updated.
    // The default implementation adds nothing to the menu.
    virtual bool ProcessViewOptionsMenu(std::variant<UINT, CCmdUI*> data);

    // The default implementation displays a message that the web message is not supported.
    // Implementations can throw exceptions that will be displayed by CaseHoldingFrame.
    virtual void ProcessWebViewMessage(const JsonNode& json_node);

    // Returns the content creator's text content (if applicable).
    // The default implementation throws ProgrammingErrorException.
    virtual SharableString GetTextContent();

    // Returns the content creator's HTML content.
    // When embed_resources is true, the content should be created without relying on external CSS or JavaScript files.
    // The default implementation throws ProgrammingErrorException.
    virtual SharableString GetHtmlContent(bool embed_resources = false);

    // Returns a URL to access the content creator's HTML content.
    const std::string& GetUrl();

protected:
    // The default implementation creates a virtual file wrapping the content returned by GetHtmlContent.
    virtual void GetUrlWorker();

protected:
    using HtmlContentServer = std::variant<std::unique_ptr<VirtualFileMappingHandler>, VirtualFileMapping>;
    std::optional<HtmlContentServer> m_htmlContentServer;
};
