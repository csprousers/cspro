#pragma once

#include <CSCode/LanguageSettings.h>
#include <CSCode/ProcessorHtml.h>
#include <CSCode/ProcessorJavaScript.h>
#include <CSCode/RunOperation.h>

class CodeFrame;
class CodeView;
class TextSourceEditable;


class CodeDoc : public CDocument
{
    DECLARE_DYNCREATE(CodeDoc)

protected:
    CodeDoc(); // create from serialization only

public:
    ~CodeDoc();

    CodeView& GetPrimaryCodeView();
    CodeView* GetSecondaryCodeView();

    CodeFrame& GetCodeFrame();

    void SetModifiedFlag(BOOL modified = TRUE) override;
    void SetModifiedFlag(BOOL modified, CWnd* scintilla_editor_parent);

    const std::string& GetInitialText() const;

    const LanguageSettings& GetLanguageSettings() const { return m_languageSettings; }
    LanguageSettings& GetLanguageSettings()             { return m_languageSettings; }

    const std::string& GetFilePath() const;

    // if the document is not saved, a temporary file path is returned
    std::string GetActualOrTempFilePath(const char* extension) const;

    std::tuple<bool, int64_t> GetFileModificationTimeParameters() const;

    void ReloadFromDisk();

    ProcessorHtml& GetHtmlProcessor();
    ProcessorJavaScript& GetJavaScriptProcessor();

    bool IsRunOperationInProgress() const { return ( m_runOperation != nullptr && m_runOperation->IsRunning() ); }
    void RegisterRunOperation(std::shared_ptr<RunOperation> run_operation);

protected:
    DECLARE_MESSAGE_MAP()

    BOOL OnNewDocument() override;
    BOOL OnOpenDocument(LPCTSTR lpszPathName) override;
    BOOL OnSaveDocument(LPCTSTR lpszPathName) override;
    void OnCloseDocument() override;

    BOOL DoSave(LPCTSTR lpszPathName, BOOL bReplace = TRUE) override;

    // File menu
    void OnFileReloadFromDisk();
    void OnUpdateFileReloadFromDisk(CCmdUI* pCmdUI);

    void OnUpdateFileSave(CCmdUI* pCmdUI);

private:
    void UpdateTitle(cs::cref_optional<std::string> file_path_or_title = std::nullopt);

private:
    std::unique_ptr<TextSourceEditable> m_textSource;
    std::optional<std::string> m_baseModifiedTitle;

    LanguageSettings m_languageSettings;

    std::unique_ptr<ProcessorHtml> m_processorHtml;
    std::map<std::string, std::unique_ptr<ProcessorJavaScript>> m_processorJavaScript;

    std::shared_ptr<RunOperation> m_runOperation;
};
