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

    void Initialize(CLogicCtrl* source_logic_ctrl, const std::string& source_title, std::string action, bool reset_flags = false);
    void Initialize(CLogicCtrl* source_logic_ctrl, const CDocument* source_doc, std::string action, bool reset_flags = false);

    void Finalize();

    void AddMessage(CompilerMessageType compiler_message_type, const std::string& text, int line_number_base1 = -1)                              { AddMessage(compiler_message_type, nullptr, text, line_number_base1); }
    void AddMessage(CompilerMessageType compiler_message_type, const std::string& file_path, const std::string& text, int line_number_base1 = -1) { AddMessage(compiler_message_type, &file_path, text, line_number_base1); }
    void AddMessage(const Logic::ParserMessage& parser_message);

    void AddInfo(const std::string& text, int line_number_base1 = -1)    { AddMessage(CompilerMessageType::Info, text, line_number_base1); }
    void AddError(const std::string& text, int line_number_base1 = -1)   { AddMessage(CompilerMessageType::Error, text, line_number_base1); }
    void AddWarning(const std::string& text, int line_number_base1 = -1) { AddMessage(CompilerMessageType::Warning, text, line_number_base1); }

    void AddInfo(const std::string& file_path, const std::string& text, int line_number_base1 = -1)    { AddMessage(CompilerMessageType::Info, file_path, text, line_number_base1); }
    void AddError(const std::string& file_path, const std::string& text, int line_number_base1 = -1)   { AddMessage(CompilerMessageType::Error, file_path, text, line_number_base1); }
    void AddWarning(const std::string& file_path, const std::string& text, int line_number_base1 = -1) { AddMessage(CompilerMessageType::Warning, file_path, text, line_number_base1); }

    void AddError(const CSProException& exception, bool dynamic_cast_exception_to_add_details = true)   { AddMessage(CompilerMessageType::Error, exception, dynamic_cast_exception_to_add_details); }
    void AddWarning(const CSProException& exception, bool dynamic_cast_exception_to_add_details = true) { AddMessage(CompilerMessageType::Warning, exception, dynamic_cast_exception_to_add_details); }

    const std::vector<std::string>& GetErrors() const { return m_errors; }

protected:
    CLogicCtrl* GetSourceLogicSource() { return m_sourceLogicSource; }

    // subclasses must override
    virtual CLogicCtrl* ActivateDocumentAndGetLogicCtrl(std::variant<const CLogicCtrl*, const std::string*> source_logic_ctrl_or_file_path) = 0;

protected:
	DECLARE_MESSAGE_MAP()

	int OnCreate(LPCREATESTRUCT lpCreateStruct);
	void OnSize(UINT nType, int cx, int cy);

private:
    void AddMessage(CompilerMessageType compiler_message_type, const std::string* file_path, const std::string& text, int line_number_base1);
    void AddMessage(CompilerMessageType compiler_message_type, const CSProException& exception, bool dynamic_cast_exception_to_add_details);

    static std::unique_ptr<std::string> GetIndentedMessageIfNecessary(std::string_view text_sv, const std::string& indent_text1, size_t intent_text2_length);

    void ProgressMessageClick(int build_wnd_line_number_base0);

private:
    std::unique_ptr<BuildWndReadOnlyEditCtrl> m_editCtrl;

    bool m_addNewlinesBetweenErrorsAndWarnings;
    bool m_indentMessageLinesAfterFirstLine;

    CLogicCtrl* m_sourceLogicSource;
    std::string m_sourceDocFilePath;
    std::string m_currentAction;
    size_t m_currentSeparatorWideLength;

    struct MessageDetails
    {
        int build_wnd_line_number_base0;
        int compiled_buffer_line_number_base0;
        std::string file_path;
    };

    std::vector<MessageDetails> m_messageDetails;
    std::vector<std::string> m_errors;
    int m_warningCount;
};
