#include "StdAfx.h"
#include "CodeView.h"
#include "ProcessorActionInvoker.h"
#include "ProcessorJson.h"
#include "ProcessorMessages.h"


IMPLEMENT_DYNCREATE(CodeView, CLogicView)

BEGIN_MESSAGE_MAP(CodeView, CLogicView)

    // editor handlers
    ON_COMMAND(ID_EDIT_FIND_NEXT, OnFindNext)
    ON_COMMAND(ID_COMMENT_CODE, OnCommentCode)

    // Code menu
    ON_COMMAND(ID_CODE_PASTE_STRING_LITERAL, OnPasteStringLiteral)
    ON_UPDATE_COMMAND_UI(ID_CODE_PASTE_STRING_LITERAL, OnUpdatePasteStringLiteral)

    ON_COMMAND_RANGE(ID_CODE_FOLDING_LEVEL_NONE, ID_CODE_FOLDING_LEVEL_ALL, OnCodeFoldingLevel)
    ON_UPDATE_COMMAND_UI_RANGE(ID_CODE_FOLDING_LEVEL_NONE, ID_CODE_FOLDING_LEVEL_ALL, OnUpdateCodeFoldingLevel)
    ON_COMMAND_RANGE(ID_CODE_FOLDING_FOLD_ALL, ID_CODE_FOLDING_TOGGLE, OnCodeFoldingAction)
    ON_UPDATE_COMMAND_UI_RANGE(ID_CODE_FOLDING_FOLD_ALL, ID_CODE_FOLDING_TOGGLE, OnUpdateCodeFoldingAction)

    // Run menu
    ON_COMMAND(ID_RUN_COMPILE_OR_VALIDATE, OnRunCompileOrValidate)
    ON_UPDATE_COMMAND_UI(ID_RUN_COMPILE_OR_VALIDATE, OnUpdateRunCompileOrValidate)

    ON_COMMAND(ID_RUN_VALIDATE_JSON_ONLY, OnRunValidateJsonOnly)
    ON_UPDATE_COMMAND_UI(ID_RUN_VALIDATE_JSON_ONLY, OnUpdateRunValidateJsonOnly)

    ON_COMMAND_RANGE(ID_RUN_FORMAT_JSON, ID_RUN_COMPRESS_JSON, OnRunFormatJson)
    ON_UPDATE_COMMAND_UI_RANGE(ID_RUN_FORMAT_JSON, ID_RUN_COMPRESS_JSON, OnUpdateRunFormatJson)

    ON_COMMAND(ID_RUN_CSPRO_DOWNGRADE_SPEC_FILE, OnRunSpecFileDowngrade)
    ON_UPDATE_COMMAND_UI(ID_RUN_CSPRO_DOWNGRADE_SPEC_FILE, OnUpdateRunSpecFileDowngrade)

END_MESSAGE_MAP()


CodeView::CodeView()
{
}


CodeView::~CodeView()
{
}


std::variant<const CDocument*, std::wstring> CodeView::GetDocumentOrTitleForBuildWnd() const
{
    return GetDocument();
}


void CodeView::SetTextAndSetSavePoint(const std::wstring& text)
{
    CLogicCtrl* logic_ctrl = GetLogicCtrl();

    logic_ctrl->SetText(text);
    logic_ctrl->SetSavePoint();
}


void CodeView::RefreshLogicControlLexer()
{
    const LanguageSettings& view_language_settings = GetLanguageSettings();
    CLogicCtrl* logic_ctrl = GetLogicCtrl();

    // set the proper lexer
    logic_ctrl->InitLogicControl(true, view_language_settings.CanCompileOrValidateCode(), view_language_settings.GetLexerLanguage());

    // enable folding if possible
    if( Lexers::CanFoldCode(view_language_settings.GetLexerLanguage()) )
        logic_ctrl->SetFolding(true);

    // update the margin as it tended to initially be too wide
    UpdateMarginWidth(true);

    // recolorize the entire document
    logic_ctrl->ColorizeEntireDocument();
}


void CodeView::OnInitialUpdate()
{
    const CodeDoc& code_doc = GetCodeDoc();

    OnInitialUpdateWorker(code_doc.GetInitialText());

    // make sure multiple code views are created (if appropriate)
    GetParentFrame()->PostMessage(UWM::CSCode::SyncCodeFrameSplitter);
}


void CodeView::OnInitialUpdateWorker(const std::wstring& initial_text)
{
    __super::OnInitialUpdate();

    // set up the logic control
    RefreshLogicControlLexer();

    // set the text
    SetTextAndSetSavePoint(initial_text);
    GetLogicCtrl()->EmptyUndoBuffer();
}


void CodeView::OnSavePointReached(_Inout_ Scintilla::NotificationData* /*pSCNotification*/)
{
    CodeDoc& code_doc = GetCodeDoc();
    code_doc.SetModifiedFlag(FALSE, this);
}


void CodeView::OnSavePointLeft(_Inout_ Scintilla::NotificationData* /*pSCNotification*/)
{
    CodeDoc& code_doc = GetCodeDoc();
    code_doc.SetModifiedFlag(TRUE, this);
}


void CodeView::OnFindNext()
{
    SendMessage(WM_COMMAND, ID_EDIT_REPEAT);
}


void CodeView::OnCommentCode()
{
    const LanguageSettings& view_language_settings = GetLanguageSettings();

    if( CodeMenu::CanCommentCode(view_language_settings.GetLexerLanguage()) )
        GetLogicCtrl()->CommentCode();
}


void CodeView::OnPasteStringLiteral()
{
    CodeMenu::OnPasteStringLiteral(*GetLogicCtrl());
}


void CodeView::OnUpdatePasteStringLiteral(CCmdUI* pCmdUI)
{
    const LanguageSettings& view_language_settings = GetLanguageSettings();

    pCmdUI->Enable(WinClipboard::HasText() && CodeMenu::CanPasteStringLiteral(view_language_settings.GetLexerLanguage()));
}


void CodeView::OnCodeFoldingLevel(UINT nID)
{
    CodeMenu::CodeFolding::OnCodeFoldingLevel(nID, GetLogicCtrl());
}


void CodeView::OnUpdateCodeFoldingLevel(CCmdUI* pCmdUI)
{
    CodeMenu::CodeFolding::OnUpdateCodeFoldingLevel(pCmdUI);
}


void CodeView::OnCodeFoldingAction(UINT nID)
{
    CodeMenu::CodeFolding::OnCodeFoldingAction(nID, GetLogicCtrl());
}


void CodeView::OnUpdateCodeFoldingAction(CCmdUI* pCmdUI)
{
    CodeMenu::CodeFolding::OnUpdateCodeFoldingAction(pCmdUI, GetLogicCtrl());
}


void CodeView::OnRunCompileOrValidate()
{
    const LanguageSettings& view_language_settings = GetLanguageSettings();
    LanguageType view_language_type = view_language_settings.GetLanguageType();

    if( view_language_type == LanguageType::JavaScript )
    {
        CodeDoc& code_doc = GetCodeDoc();
        code_doc.GetJavaScriptProcessor().Compile();
    }

    else if( view_language_type == LanguageType::Json )
    {
        ProcessorJson::ValidateJson(*this);
    }

    else if( view_language_type == LanguageType::CSProActionInvoker )
    {
        ProcessorActionInvoker::ValidateJson(*this);
    }

    else if( view_language_type == LanguageType::CSProMessages )
    {
        ProcessorMessages::Compile(*this);
    }

    else if( view_language_type == LanguageType::CSProSpecFileJson )
    {
        ProcessorJson::ValidateSpecFile(*this);
    }

    else
    {
        ASSERT(view_language_settings.CanCompileCode());
        // CODE_TODO compile CSPro
    }
}


void CodeView::OnUpdateRunCompileOrValidate(CCmdUI* pCmdUI)
{
    const CodeDoc& code_doc = GetCodeDoc();
    const LanguageSettings& view_language_settings = GetLanguageSettings();

    pCmdUI->Enable(!code_doc.IsRunOperationInProgress() &&
                   view_language_settings.CanCompileOrValidateCode());
}


void CodeView::OnRunValidateJsonOnly()
{
    ProcessorJson::ValidateJson(*this);
}


void CodeView::OnUpdateRunValidateJsonOnly(CCmdUI* pCmdUI)
{
    const LanguageSettings& view_language_settings = GetLanguageSettings();

    pCmdUI->Enable(view_language_settings.GetLanguageType() == LanguageType::CSProSpecFileJson);
}


void CodeView::OnRunFormatJson(UINT nID)
{
    bool compress_mode = ( nID == ID_RUN_COMPRESS_JSON );

    ProcessorJson::FormatJson(*this, compress_mode);
}


void CodeView::OnUpdateRunFormatJson(CCmdUI* pCmdUI)
{
    const LanguageSettings& view_language_settings = GetLanguageSettings();

    pCmdUI->Enable(view_language_settings.GetLexerLanguage() == SCLEX_JSON);
}


void CodeView::OnRunSpecFileDowngrade()
{
    ProcessorJson::DowngradeSpecFile(*this);
}


void CodeView::OnUpdateRunSpecFileDowngrade(CCmdUI* pCmdUI)
{
    const std::optional<unsigned>& json_spec_file_index = GetLanguageSettings().GetJsonSpecFileIndex();
    ASSERT(json_spec_file_index.has_value());

    pCmdUI->Enable(( *json_spec_file_index == ID_RUN_CSPRO_SPEC_FILE_APP ||
                     *json_spec_file_index == ID_RUN_CSPRO_SPEC_FILE_DCF ));
}
