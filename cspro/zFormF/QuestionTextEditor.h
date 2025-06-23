#pragma once

#include <zHtml/HtmlEditorCtrl.h>


// --------------------------------------------------------------------------
// QuestionTextEditor
//
// An interface for question text editors. Subclasses include:
//
//     - QuestionTextHtmlEditor: a wrapper around HtmlViewCtrl.
//     - QuestionTextTextEditor: a wrapper around CLogicCtrl.
//
// --------------------------------------------------------------------------

class QuestionTextEditor
{
public:
    virtual ~QuestionTextEditor() { }

    virtual CWnd& GetWnd() = 0;

    virtual void Initialize(CWnd* pParent, const std::string& application_file_path) = 0;
    virtual void Destroy() = 0;

    virtual void UpdateForFormat(const Application* application, CapiText::Format format) = 0;

    virtual bool IsDirty() = 0;

    virtual void ClearCompilationResults() = 0;
    virtual void CompileFillsAndLogic(CapiEditorViewModel& view_model, const CapiText& capi_text) = 0;

    virtual bool HasContent() = 0;
    virtual SharableString GetContent() = 0;
    virtual void ClearContent() = 0;
    virtual void SetContent(const SharableString& text) = 0;

    virtual void Copy() = 0;
    virtual bool CanCopy() = 0;

    virtual void Cut() = 0;
    virtual bool CanCut() = 0;

    virtual void Paste(bool with_formatting) = 0;
    virtual bool CanPaste() = 0;

    virtual void SelectAll() = 0;

    virtual void Undo() = 0;
    virtual void Redo() = 0;

    virtual void Bold() = 0;
    virtual void Italic() = 0;
    virtual void Underline() = 0;

    virtual void SetForeColor(COLORREF color) = 0;

    virtual void UnorderedList() = 0;
    virtual void OrderedList() = 0;

    virtual void InsertImage(const std::string& image_url) = 0;
    virtual void InsertTable(int rows, int columns) = 0;
    virtual void InsertLink(const std::string& text, const std::string& url) = 0;
};



// --------------------------------------------------------------------------
// QuestionTextHtmlEditor
// --------------------------------------------------------------------------

class QuestionTextHtmlEditor : public QuestionTextEditor
{
public:
    QuestionTextHtmlEditor();
    ~QuestionTextHtmlEditor();

    HtmlEditorCtrl& GetHtmlEditorCtrl();

    CWnd& GetWnd() override { return GetHtmlEditorCtrl(); }

    void Initialize(CWnd* pParent, const std::string& application_file_path) override;
    void Destroy() override;

    void UpdateForFormat(const Application* application, CapiText::Format format) override;

    bool IsDirty() override;

    void ClearCompilationResults() override;
    void CompileFillsAndLogic(CapiEditorViewModel& view_model, const CapiText& capi_text) override;

    bool HasContent() override;
    SharableString GetContent() override;
    void ClearContent() override;
    void SetContent(const SharableString& text) override;

    void Copy() override;
    bool CanCopy() override;

    void Cut() override;
    bool CanCut() override;

    void Paste(bool with_formatting) override;
    bool CanPaste() override;

    void SelectAll() override;

    void Undo() override;
    void Redo() override;

    void Bold() override;
    void Italic() override;
    void Underline() override;

    void SetForeColor(COLORREF color) override;

    void UnorderedList() override;
    void OrderedList() override;

    void InsertImage(const std::string& image_url) override;
    void InsertTable(int rows, int columns) override;
    void InsertLink(const std::string& text, const std::string& url) override;

private:
    struct Data;
    std::unique_ptr<Data> m_data;
};



// --------------------------------------------------------------------------
// QuestionTextTextEditor
// --------------------------------------------------------------------------

class QuestionTextTextEditor : public QuestionTextEditor
{
public:
    QuestionTextTextEditor();
    ~QuestionTextTextEditor();

    CLogicCtrl& GetLogicCtrl() { return *m_logicCtrl; }

    CWnd& GetWnd() override { return GetLogicCtrl(); }

    void Initialize(CWnd* pParent, const std::string& application_file_path) override;
    void Destroy() override;

    void UpdateForFormat(const Application* application, CapiText::Format format) override;

    bool IsDirty() override;

    void ClearCompilationResults() override;
    void CompileFillsAndLogic(CapiEditorViewModel& view_model, const CapiText& capi_text) override;

    bool HasContent() override;
    SharableString GetContent() override;
    void ClearContent() override;
    void SetContent(const SharableString& text) override;

    void Copy() override;
    bool CanCopy() override;

    void Cut() override;
    bool CanCut() override;

    void Paste(bool with_formatting) override;
    bool CanPaste() override;

    void SelectAll() override;

    void Undo() override;
    void Redo() override;

    void Bold() override;
    void Italic() override;
    void Underline() override;

    void SetForeColor(COLORREF color) override;

    void UnorderedList() override;
    void OrderedList() override;

    void InsertImage(const std::string& image_url) override;
    void InsertTable(int rows, int columns) override;
    void InsertLink(const std::string& text, const std::string& url) override;

private:
    int GetLexerLanguage(const Application* application) const;

    bool EditingHtml() const     { return ( m_format == CapiText::Format::ReportHtml ); }
    bool EditingMarkdown() const { return ( m_format == CapiText::Format::ReportMarkdown ); }

    void WrapSelection(cs::string_view_sz start_text_sv, const char* end_text);

private:
    class CustomLogicCtrl;
    std::unique_ptr<CLogicCtrl> m_logicCtrl;
    CapiText::Format m_format;
};
