#include "StdAfx.h"
#include "QuestionTextEditor.h"
#include <zToolsO/FileIO.h>
#include <zHtml/HtmlEditorCtrl.h>
#include <zHtml/SharedHtmlLocalFileServer.h>


struct QuestionTextHtmlEditor::Data
{
    HtmlEditorCtrl html_editor_ctrl;
    SharedHtmlLocalFileServer file_server;
    std::unique_ptr<VirtualFileMapping> question_text_virtual_file_mapping;
};


QuestionTextHtmlEditor::QuestionTextHtmlEditor()
    :   m_data(std::make_unique<Data>())
{
}


QuestionTextHtmlEditor::~QuestionTextHtmlEditor()
{
}


HtmlEditorCtrl& QuestionTextHtmlEditor::GetHtmlEditorCtrl()
{
    return m_data->html_editor_ctrl;
}


void QuestionTextHtmlEditor::Initialize(Application* /*application*/, const std::string& application_file_path)
{
    // to make relative paths in the question text work, the HTML editor must
    // appear as if it exists in the application directory; we will load the
    // editing HTML once and then issue it as a virtual file
    static SharableString editor_html;

    if( !editor_html.IsSet() )
    {
        try
        {
            const std::string editor_html_file_path = Path::Combine(Html::GetDirectory(Html::Subdirectory::HtmlEditor), "index.html");
            editor_html = FileIO::ReadText(editor_html_file_path);
        }

        catch( const FileIO::Exception& )
        {
            editor_html = "<html><body><p>There was an error loading the HTML editor.</p></body></html>";
        }
    }

    m_data->question_text_virtual_file_mapping = std::make_unique<VirtualFileMapping>(
        m_data->file_server.CreateVirtualHtmlFile(PortableFunctions::PathGetDirectory(application_file_path),
        [&]()
        {
            return editor_html;
        }));

    m_data->html_editor_ctrl.SetUrl(m_data->question_text_virtual_file_mapping->GetUrl());
}


void QuestionTextHtmlEditor::Destroy()
{
    m_data->question_text_virtual_file_mapping.reset();
}


void QuestionTextHtmlEditor::SetStyles(const std::vector<HtmlEditorCtrl::Style>& editor_styles)
{
    m_data->html_editor_ctrl.SetStyles(editor_styles);
}


bool QuestionTextHtmlEditor::IsDirty()
{
    return m_data->html_editor_ctrl.IsDirty();
}


void QuestionTextHtmlEditor::UpdateFillErrorDisplay(const std::map<std::string, CapiEditorViewModel::SyntaxCheckResult>& fill_syntax_check_results)
{
    std::map<std::string, std::string> errors;

    for( const auto& [fill, result] : fill_syntax_check_results )
    {
        if( std::holds_alternative<CapiEditorViewModel::SyntaxCheckError>(result) )
            errors.try_emplace(fill, std::get<CapiEditorViewModel::SyntaxCheckError>(result).error_message);
    }

    m_data->html_editor_ctrl.SetSyntaxErrors(errors);
}


bool QuestionTextHtmlEditor::HasContent()
{
    return !m_data->html_editor_ctrl.GetText()->empty();
}


void QuestionTextHtmlEditor::ClearContent()
{
    m_data->html_editor_ctrl.Clear();
}


void QuestionTextHtmlEditor::SetContent(const SharableString& text)
{
    m_data->html_editor_ctrl.SetText(text);
}


void QuestionTextHtmlEditor::Copy()
{
    m_data->html_editor_ctrl.Copy();
}


bool QuestionTextHtmlEditor::CanCopy()
{
    return m_data->html_editor_ctrl.CanCopy();
}


void QuestionTextHtmlEditor::Cut()
{
    m_data->html_editor_ctrl.Cut();
}


bool QuestionTextHtmlEditor::CanCut()
{
    return m_data->html_editor_ctrl.CanCut();
}


void QuestionTextHtmlEditor::Paste(const bool with_formatting)
{
    return m_data->html_editor_ctrl.Paste(with_formatting);
}


bool QuestionTextHtmlEditor::CanPaste()
{
    return m_data->html_editor_ctrl.CanPaste();
}


void QuestionTextHtmlEditor::SelectAll()
{
    m_data->html_editor_ctrl.SelectAll();
}


void QuestionTextHtmlEditor::Undo()
{
    m_data->html_editor_ctrl.Undo();
}


void QuestionTextHtmlEditor::Redo()
{
    m_data->html_editor_ctrl.Redo();
}


void QuestionTextHtmlEditor::Bold()
{
    ASSERT(!m_data->html_editor_ctrl.GetCodeViewShowing());
    m_data->html_editor_ctrl.Bold();
}


void QuestionTextHtmlEditor::Italic()
{
    ASSERT(!m_data->html_editor_ctrl.GetCodeViewShowing());
    m_data->html_editor_ctrl.Italic();
}


void QuestionTextHtmlEditor::Underline()
{
    ASSERT(!m_data->html_editor_ctrl.GetCodeViewShowing());
    m_data->html_editor_ctrl.Underline();
}


void QuestionTextHtmlEditor::SetForeColor(const COLORREF color)
{
    ASSERT(!m_data->html_editor_ctrl.GetCodeViewShowing());
    m_data->html_editor_ctrl.SetForeColor(color);
}


void QuestionTextHtmlEditor::UnorderedList()
{
    ASSERT(!m_data->html_editor_ctrl.GetCodeViewShowing());
    m_data->html_editor_ctrl.UnorderedList();
}


void QuestionTextHtmlEditor::OrderedList()
{
    ASSERT(!m_data->html_editor_ctrl.GetCodeViewShowing());
    m_data->html_editor_ctrl.OrderedList();
}


void QuestionTextHtmlEditor::InsertImage(const std::string& image_url)
{
    ASSERT(!m_data->html_editor_ctrl.GetCodeViewShowing());
    m_data->html_editor_ctrl.InsertImage(image_url);
}


void QuestionTextHtmlEditor::InsertTable(const int rows, const int columns)
{
    ASSERT(!m_data->html_editor_ctrl.GetCodeViewShowing());
    m_data->html_editor_ctrl.InsertTable(rows, columns);
}


void QuestionTextHtmlEditor::InsertLink(const std::string& text, const std::string& url)
{
    ASSERT(!m_data->html_editor_ctrl.GetCodeViewShowing());
    m_data->html_editor_ctrl.InsertLink(text, url);
}
