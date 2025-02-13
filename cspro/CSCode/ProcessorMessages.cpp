#include "StdAfx.h"
#include "ProcessorMessages.h"
#include <zUtilO/TextSourceString.h>
#include <zMessageO/MessageFile.h>


void ProcessorMessages::Compile(CodeView& code_view)
{
    CLogicCtrl* logic_ctrl = code_view.GetLogicCtrl();
    ASSERT(logic_ctrl->GetLexer() == SCLEX_CSPRO_MESSAGE_V8_0);

    CMainFrame* main_frame = assert_cast<CMainFrame*>(AfxGetMainWnd());
    CSCodeBuildWnd* build_wnd = main_frame->GetBuildWnd();

    if( build_wnd == nullptr )
        return;

    build_wnd->Initialize(code_view, _T("Message compilation"));

    try
    {
        const TextSourceString text_source(code_view.GetCodeDoc().GetPathNameOrFakeTempName(FileExtensions::Message),
                                           logic_ctrl->GetText());

        MessageFile message_file;
        message_file.Load(text_source, LogicSettings::Version::V8_0);

        for( const Logic::ParserMessage& parser_message : message_file.GetLoadParserMessages() )
            build_wnd->AddMessage(parser_message);
    }

    catch( const CSProException& exception )
    {
        build_wnd->AddError(exception);
    }

    build_wnd->Finalize();
}
