#pragma once

class DocSetSpec;


// a base class for the CSDocDoc, DocSetSpecDoc, and DocSetComponentDoc documents

class TextEditDoc : public CDocument, public TextSourceEditable::SourceModifier
{
protected:
    TextEditDoc();

public:
    virtual ~TextEditDoc() { }

    TextSourceEditable* GetTextSource()                               { return m_textSource.get(); }
    std::shared_ptr<TextSourceEditable> GetSharedTextSourceEditable() { return m_textSource; }

    DocSetSpec* GetAssociatedDocSetSpec()                       { return m_docSetSpec.get(); }
    std::shared_ptr<DocSetSpec> GetSharedAssociatedDocSetSpec() { return m_docSetSpec; }
    void SetAssociatedDocSetSpec(std::shared_ptr<DocSetSpec> doc_set_spec);

    TextEditView* GetTextEditView() { return m_textEditView; }

    // returns the initial text and sets up the SourceModifier
    std::wstring OnViewInitialUpdate(TextEditView& text_edit_view);

    void OnViewDeactivate();

    void ReloadFromDisk();

    void SetModifiedFlag(BOOL modified = TRUE) override;

    // methods for subclasses to override
    virtual int GetLexerLanguage() const = 0;

    // methods that subclasses can override
    virtual bool ConvertTabsToSpacesAndTrimRightEachLine() const { return false; }

protected:
    DECLARE_MESSAGE_MAP()

    BOOL OnOpenDocument(LPCTSTR lpszPathName) override;
    BOOL OnSaveDocument(LPCTSTR lpszPathName) override;

    void OnUpdateFileSave(CCmdUI* pCmdUI);

private:
    void UpdateTitle();

    // TextSourceEditable::SourceModifier overrides
    void SyncTextSource() override;
    void OnTextSourceSave() override { }

protected:
    std::shared_ptr<DocSetSpec> m_docSetSpec;

private:
    std::shared_ptr<TextSourceEditable> m_textSource;
    TextEditView* m_textEditView;
};
