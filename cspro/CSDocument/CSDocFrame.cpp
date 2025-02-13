#include "StdAfx.h"
#include "CSDocFrame.h"
#include "CSDocCompiler.h"
#include "CSDocExportDlg.h"
#include "DocSetSpec.h"
#include "GenerateDlg.h"


IMPLEMENT_DYNCREATE(CSDocFrame, DocSetBuildHandlerFrame)

BEGIN_MESSAGE_MAP(CSDocFrame, DocSetBuildHandlerFrame)

    // Document menu
    ON_COMMAND(ID_COMPILE, OnCompile)
    ON_COMMAND(ID_EXPORT_CSDOC, OnExport)
    ON_COMMAND(ID_BUILD_TO_CLIPBOARD, OnBuildToClipboard)
    ON_COMMAND(ID_ASSOCIATE_WITH_DOCSET, OnAssociateWithDocSet)
    ON_UPDATE_COMMAND_UI(ID_ASSOCIATE_WITH_DOCSET, OnUpdateDocumentMustBeSavedToDisk)
    ON_COMMAND(ID_DISASSOCIATE_WITH_DOCSET, OnDisassociateWithDocSet)
    ON_UPDATE_COMMAND_UI(ID_DISASSOCIATE_WITH_DOCSET, OnUpdateDisassociateWithDocSet)

    // Format menu
    ON_COMMAND(ID_FORMAT_VIEW_SYNTAX, OnViewSyntax)
    ON_COMMAND_RANGE(ID_FORMAT_BOLD, ID_FORMAT_SYNTAX_ARG, OnFormatStyle)
    ON_COMMAND(ID_FORMAT_LINK, OnFormatLink)
    ON_COMMAND(ID_FORMAT_LIST_BUILDER, OnFormatListBuilder)

END_MESSAGE_MAP()


void CSDocFrame::AddFrameSpecificItemsToBuildMenu(DynamicMenuBuilder& dynamic_menu_builder)
{
    CSDocDoc& csdoc_doc = GetCSDocDoc();
    const DocSetSpec* doc_set_spec = csdoc_doc.GetAssociatedDocSetSpec();

    dynamic_menu_builder.AddOption(ID_COMPILE, _T("&Build Document\tCtrl+K"));

    dynamic_menu_builder.AddSeparator();
    dynamic_menu_builder.AddOption(ID_BUILD_TO_CLIPBOARD, _T("Build Document to &Clipboard"));
    dynamic_menu_builder.AddOption(ID_EXPORT_CSDOC, _T("Export Document..."));

    dynamic_menu_builder.AddSeparator();

    if( doc_set_spec == nullptr )
    {
        dynamic_menu_builder.AddOption(ID_ASSOCIATE_WITH_DOCSET, _T("Link With Document Set"));
    }

    else
    {
        const std::wstring doc_set_spec_title = doc_set_spec->GetTitleOrFilenameWithoutExtension();
        dynamic_menu_builder.AddOption(ID_DISASSOCIATE_WITH_DOCSET, SO::CreateParentheticalExpression(_T("Remove Document Set Link"), doc_set_spec_title));
    }
}


std::shared_ptr<DocSetSpec> CSDocFrame::CreateOrCompileDocSetSpec()
{
    CSDocDoc& csdoc_doc = GetCSDocDoc();
    std::shared_ptr<DocSetSpec> doc_set_spec = csdoc_doc.GetSharedAssociatedDocSetSpec();

    // if not associated with a Document Set, return a dummy one
    if( doc_set_spec == nullptr )
    {
        return std::make_shared<DocSetSpec>();
    }

    // otherwise make sure it is compiled
    else
    {
        GetMainFrame().CompileDocSetSpecIfNecessary(*doc_set_spec, DocSetCompiler::SpecCompilationType::DataForCSDocCompilation,
                                                    DocSetCompiler::ThrowErrors { });
        return doc_set_spec;
    }
}


void CSDocFrame::OnCompile()
{
    OnCompile(true);
}


void CSDocFrame::OnCompile(bool manual_compile)
{
    CMainFrame& main_frame = GetMainFrame();
    CSDocumentBuildWnd* build_wnd = main_frame.GetBuildWnd(manual_compile);
    HtmlOutputWnd* html_output_wnd = main_frame.GetHtmlOutputWnd();

    if( build_wnd == nullptr || html_output_wnd == nullptr )
        return;

    CSDocDoc& csdoc_doc = GetCSDocDoc();
    CLogicCtrl* logic_ctrl = GetTextEditView().GetLogicCtrl();

    build_wnd->Initialize(logic_ctrl, &csdoc_doc, _T("CSPro Document compilation"));

    std::wstring result_url;

    try
    {
        CWaitCursor wait_cursor;

        std::shared_ptr<DocSetSpec> doc_set_spec;

        try
        {
            doc_set_spec = CreateOrCompileDocSetSpec();
        }

        catch( const CSProException& exception )
        {
            // report Document Set compilation errors as warnings
            build_wnd->AddWarning(exception);

            doc_set_spec = csdoc_doc.GetSharedAssociatedDocSetSpec();
            ASSERT(doc_set_spec != nullptr);
        }

        CSDocCompilerSettingsForCSDocumentPreview settings(std::move(doc_set_spec), build_wnd);

        CSDocCompiler csdoc_compiler;
        const std::wstring html = csdoc_compiler.CompileToHtml(settings, CS2WS(csdoc_doc.GetPathName()), logic_ctrl->GetText());

        result_url = main_frame.CreateHtmlPage(csdoc_doc, html);
    }

    catch( const CSProException& exception )
    {
        build_wnd->AddError(exception.GetErrorMessage());

        result_url = main_frame.CreateHtmlCompilationErrorPage(csdoc_doc);
    }

    build_wnd->Finalize();

    ShowHtmlAndRestoreScrollbarState(html_output_wnd->GetHtmlViewCtrl(), std::move(result_url));
}


void CSDocFrame::OnExport()
{
    std::shared_ptr<DocSetSpec> doc_set_spec;

    try
    {
         doc_set_spec = CreateOrCompileDocSetSpec();
    }

    catch( const CSProException& exception )
    {
        ErrorMessage::Display(exception);
        return;
    }

    CLogicCtrl* logic_ctrl = GetTextEditView().GetLogicCtrl();

    CSDocExportDlg csdoc_export_dlg(std::move(doc_set_spec), CS2WS(GetCSDocDoc().GetPathName()), *logic_ctrl, this);

    if( csdoc_export_dlg.DoModal() != IDOK )
        return;

    std::unique_ptr<GenerateTask> generate_task = csdoc_export_dlg.ReleaseGenerateTask();
    ASSERT(generate_task != nullptr);

    // reset the cache before the build
    CacheableCalculator::ResetCache();

    GenerateDlg generate_dlg(*generate_task, this);
    generate_dlg.DoModal();
}


void CSDocFrame::OnBuildToClipboard()
{
    CSDocDoc& csdoc_doc = GetCSDocDoc();
    CLogicCtrl* logic_ctrl = GetTextEditView().GetLogicCtrl();

    try
    {
        CSDocCompilerSettingsForBuilding settings(CreateOrCompileDocSetSpec(), DocBuildSettings::SettingsForCSDocBuildToClipboard(), std::wstring());

        CSDocCompiler csdoc_compiler;
        const std::wstring html = csdoc_compiler.CompileToHtml(settings, CS2WS(csdoc_doc.GetPathName()), logic_ctrl->GetText());

        WinClipboard::PutHtml(html);
    }

    catch( const CSProException& exception )
    {
        ErrorMessage::Display(exception);
    }
}


void CSDocFrame::OnAssociateWithDocSet()
{
    CSDocDoc& csdoc_doc = GetCSDocDoc();
    const DocSetSpec* current_doc_set_spec = csdoc_doc.GetAssociatedDocSetSpec();

    const std::wstring filter = FormatTextCS2WS(_T("CSPro Document Sets (%s)|%s||"), FileExtensions::Wildcard::CSDocumentSet, FileExtensions::Wildcard::CSDocumentSet);

    CIMSAFileDialog file_dlg(TRUE, nullptr, ( current_doc_set_spec != nullptr ) ? current_doc_set_spec->GetFilename().c_str() : nullptr,
                             OFN_HIDEREADONLY, filter.c_str(), nullptr, CFD_NO_DIR);
    file_dlg.m_ofn.lpstrTitle = _T("Select the Associated Document Set");

    if( file_dlg.DoModal() != IDOK )
        return;

    try
    {
        CMainFrame& main_frame = GetMainFrame();

        std::shared_ptr<DocSetSpec> new_doc_set_spec = main_frame.FindSharedDocSetSpec(CS2WS(file_dlg.GetPathName()), true);
        ASSERT(new_doc_set_spec != nullptr);

        // make sure that this document is part of the Document Set
        if( !main_frame.IsCSDocPartOfDocSet(*new_doc_set_spec, CS2WS(csdoc_doc.GetPathName())) )
        {
            throw CSProException(_T("The CSPro Document '%s' is not part of the Document Set '%s'. Add it to the Table of Contents or Documents and try again."),
                                 PortableFunctions::PathGetFilename(csdoc_doc.GetPathName()), PortableFunctions::PathGetFilename(new_doc_set_spec->GetFilename()));
        }

        csdoc_doc.SetAssociatedDocSetSpec(std::move(new_doc_set_spec));
    }

    catch( const CSProException& exception )
    {
        ErrorMessage::Display(exception);
    }
}


void CSDocFrame::OnDisassociateWithDocSet()
{
    CSDocDoc& csdoc_doc = GetCSDocDoc();
    csdoc_doc.SetAssociatedDocSetSpec(nullptr);
}


void CSDocFrame::OnUpdateDisassociateWithDocSet(CCmdUI* pCmdUI)
{
    CSDocDoc& csdoc_doc = GetCSDocDoc();
    pCmdUI->Enable(csdoc_doc.GetAssociatedDocSetSpec() != nullptr);
}


void CSDocFrame::OnViewSyntax()
{
    const bool control_pressed = ( GetKeyState(VK_CONTROL) < 0 );
    const std::wstring syntax_file = PortableFunctions::PathAppendToPath(Html::GetDirectory(Html::Subdirectory::Document), _T("csdoc-syntax.html"));

    if( !PortableFunctions::FileIsRegular(syntax_file) )
    {
        ErrorMessage::Display(_T("The syntax file could not be found: ") + syntax_file);
        return;
    }

    if( !control_pressed )
    {
        CMainFrame& main_frame = GetMainFrame();
        HtmlOutputWnd* html_output_wnd = main_frame.GetHtmlOutputWnd();

        if( html_output_wnd != nullptr )
        {
            // HELP_TODO suspend automatic compilation, until the next manual compile, so the syntax doesn't get hidden
            const std::wstring url = main_frame.GetSharedHtmlLocalFileServer().GetFilenameUrl(syntax_file);
            ShowHtmlAndRestoreScrollbarState(html_output_wnd->GetHtmlViewCtrl(), url);
            return;
        }
    }

    // if the user presses the Ctrl key, or there is a failure to use the HTML output window, display the syntax in an external browser
    ShellExecute(nullptr, _T("open"), EscapeCommandLineArgument(syntax_file).c_str(), nullptr, nullptr, SW_SHOW);
}


void CSDocFrame::WrapSelectionInTags(const char* start_tag, const char* end_tag)
{
    CLogicCtrl* logic_ctrl = GetTextEditView().GetLogicCtrl();

    const Sci_Position start_pos = logic_ctrl->GetSelectionStart();
    const Sci_Position end_pos = logic_ctrl->GetSelectionEnd();

    logic_ctrl->BeginUndoAction();
    logic_ctrl->InsertText(end_pos, end_tag);
    logic_ctrl->InsertText(start_pos, start_tag);
    logic_ctrl->EndUndoAction();

    // adjust the selection to account for the inserted start tag
    const size_t start_tag_length = strlen(start_tag);
    logic_ctrl->SetSelection(start_pos + start_tag_length, end_pos + start_tag_length);
}


void CSDocFrame::OnFormatStyle(const UINT nID)
{
    ( nID == ID_FORMAT_BOLD )        ? WrapSelectionInTags("<b>",                 "</b>") :
    ( nID == ID_FORMAT_ITALICS )     ? WrapSelectionInTags("<i>",                 "</i>") :
    ( nID == ID_FORMAT_FONT_TAG )    ? WrapSelectionInTags("<font ...>",          "</font>") :
    ( nID == ID_FORMAT_SUBHEADER )   ? WrapSelectionInTags("<subheader>",         "</subheader>") :
    ( nID == ID_FORMAT_CENTER )      ? WrapSelectionInTags("<center>",            "</center>") :
    ( nID == ID_FORMAT_TABLE )       ? WrapSelectionInTags("<table 2>\n\t<cell>", "</cell><cell> </cell>\n</table>") :
    ( nID == ID_FORMAT_LOGIC_COLOR ) ? WrapSelectionInTags("<logiccolor>",        "</logiccolor>") :
    ( nID == ID_FORMAT_SYNTAX_ARG )  ? WrapSelectionInTags("<arg>",               "</arg>") :
                                       ReturnProgrammingError(std::make_tuple("", ""));
}


void CSDocFrame::OnFormatLink()
{
    CSDocDoc& csdoc_doc = GetCSDocDoc();

    // if a URL or a document's path or filename is on the clipboard, automatically add it
    bool surround_link_with_quotes = false;

    auto process_potential_path = [&](std::wstring potential_path)
    {
        if( SO::StartsWith(potential_path, _T("http")) )
        {
            surround_link_with_quotes = true;
            return potential_path;
        }

        // allows filenames to be specified without an extension
        if( !SO::EqualsNoCase(PortableFunctions::PathGetFileExtension(potential_path), FileExtensions::CSDocument) )
            potential_path.append(FileExtensions::WithDot::CSDocument);

        if( PortableFunctions::FileIsRegular(potential_path) )
            return GetRelativeFNameForDisplay(csdoc_doc.GetPathName(), potential_path);

        // if a file with the name did not exist (specified with or without the .csdoc extension), see if it is part of the Document Set's documents
        DocSetSpec* doc_set_spec = csdoc_doc.GetAssociatedDocSetSpec();

        if( doc_set_spec != nullptr && GetMainFrame().IsCSDocPartOfDocSet(*doc_set_spec, potential_path, false) )
            return potential_path;

        return std::wstring();
    };

    std::wstring link = WinClipboard::HasText() ? process_potential_path(WinClipboard::GetText()) :
                                                  std::wstring();

    std::string start_tag = "<link>";

    if( !link.empty() )
    {
        // if a http URL, or if the link uses spaces, wrap in double quotes
        if( surround_link_with_quotes || link.find(' ') != std::wstring::npos )
            link = Encoders::ToLogicString(link);

        start_tag.insert(start_tag.length() - 1, " " + UTF8Convert::WideToUTF8(link));
    }

    WrapSelectionInTags(start_tag.c_str(), "</link>");
}


void CSDocFrame::OnFormatListBuilder()
{
    CLogicCtrl* logic_ctrl = GetTextEditView().GetLogicCtrl();
    std::wstring html = _T("<list>\n");

    SO::ForeachLine(logic_ctrl->GetSelText(), false,
        [&](const wstring_view line_sv)
        {
            ASSERT(!line_sv.empty());

            html.append(_T("\t<li>"));
            html.append(line_sv);
            html.append(_T("</li>\n"));

            return true;
        });

    html.append(_T("</list>"));

    logic_ctrl->ReplaceSel(html);
}
