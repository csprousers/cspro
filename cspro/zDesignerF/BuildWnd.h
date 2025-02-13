#pragma once

#include <zDesignerF/zDesignerF.h>
#include <afxdockablepane.h>

class CLogicCtrl;
namespace Logic { struct ParserMessage; }

enum class CompilerMessageType { Info,  Error, Warning };


class CLASS_DECL_ZDESIGNERF BuildWnd : public CDockablePane
{
    friend class BuildWndReadOnlyEditCtrl;

public:
    BuildWnd();
    virtual ~BuildWnd();

    // some flags
    void SetAddNewlinesBetweenErrorsAndWarnings(bool flag) { m_addNewlinesBetweenErrorsAndWarnings = flag; }
    void SetIndentMessageLinesAfterFirstLine(bool flag)    { m_indentMessageLinesAfterFirstLine = flag; }

    void Initialize(CLogicCtrl* source_logic_ctrl, const std::wstring& source_title, std::wstring action, bool reset_flags = false);
    void Initialize(CLogicCtrl* source_logic_ctrl, const CDocument* source_doc, std::wstring action, bool reset_flags = false);

    void Finalize();

    void AddMessage(CompilerMessageType compiler_message_type, const std::wstring& text, int line_number_base1 = -1)                               { AddMessage(compiler_message_type, nullptr, text, line_number_base1); }
    void AddMessage(CompilerMessageType compiler_message_type, const std::wstring& filename, const std::wstring& text, int line_number_base1 = -1) { AddMessage(compiler_message_type, &filename, text, line_number_base1); }
    void AddMessage(const Logic::ParserMessage& parser_message);

    void AddInfo(const std::wstring& text, int line_number_base1 = -1)    { AddMessage(CompilerMessageType::Info, text, line_number_base1); }
    void AddError(const std::wstring& text, int line_number_base1 = -1)   { AddMessage(CompilerMessageType::Error, text, line_number_base1); }
    void AddWarning(const std::wstring& text, int line_number_base1 = -1) { AddMessage(CompilerMessageType::Warning, text, line_number_base1); }

    void AddInfo(const std::wstring& filename, const std::wstring& text, int line_number_base1 = -1)    { AddMessage(CompilerMessageType::Info, filename, text, line_number_base1); }
    void AddError(const std::wstring& filename, const std::wstring& text, int line_number_base1 = -1)   { AddMessage(CompilerMessageType::Error, filename, text, line_number_base1); }
    void AddWarning(const std::wstring& filename, const std::wstring& text, int line_number_base1 = -1) { AddMessage(CompilerMessageType::Warning, filename, text, line_number_base1); }

    void AddError(const CSProException& exception, bool dynamic_cast_exception_to_add_details = true)   { AddMessage(CompilerMessageType::Error, exception, dynamic_cast_exception_to_add_details); }
    void AddWarning(const CSProException& exception, bool dynamic_cast_exception_to_add_details = true) { AddMessage(CompilerMessageType::Warning, exception, dynamic_cast_exception_to_add_details); }

    const std::vector<std::wstring>& GetErrors() const { return m_errors; }

protected:
    CLogicCtrl* GetSourceLogicSource() { return m_sourceLogicSource; }

    // subclasses must override
    virtual CLogicCtrl* ActivateDocumentAndGetLogicCtrl(std::variant<const CLogicCtrl*, const std::wstring*> source_logic_ctrl_or_filename) = 0;

protected:
	DECLARE_MESSAGE_MAP()

	int OnCreate(LPCREATESTRUCT lpCreateStruct);
	void OnSize(UINT nType, int cx, int cy);

private:
    void AddMessage(CompilerMessageType compiler_message_type, const std::wstring* filename, const std::wstring& text, int line_number_base1);
    void AddMessage(CompilerMessageType compiler_message_type, const CSProException& exception, bool dynamic_cast_exception_to_add_details);

    static std::unique_ptr<std::wstring> GetIndentedMessageIfNecessary(wstring_view text_sv, size_t indent_size = SO::DefaultSpacesPerTab);

    void ProgressMessageClick(int build_wnd_line_number_base0);

private:
    std::unique_ptr<BuildWndReadOnlyEditCtrl> m_editCtrl;

    bool m_addNewlinesBetweenErrorsAndWarnings;
    bool m_indentMessageLinesAfterFirstLine;

    CLogicCtrl* m_sourceLogicSource;
    std::wstring m_sourceDocFilename;
    std::wstring m_currentAction;
    size_t m_currentSeparatorLength;

    struct MessageDetails
    {
        int build_wnd_line_number_base0;
        int compiled_buffer_line_number_base0;
        std::wstring filename;
    };

    std::vector<MessageDetails> m_messageDetails;
    std::vector<std::wstring> m_errors;
    int m_warningCount;
};
