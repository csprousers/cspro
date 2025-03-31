//////////////////////////////////////////////////////////////////////
//
// Capi.cpp: implementation of the CCapi class.
//
//////////////////////////////////////////////////////////////////////
#include "StdAfx.h"
#include "capi.h"
#include "CapiQuestionManager.h"
#include <zToolsO/Tools.h>
#include <zAppO/Application.h>
#include <zEngineO/Block.h>
#include <zEngineO/ResponseProcessor.h>
#include <engine/Entdrv.h>
#include <engine/INTERPRE.H>

#ifdef WIN_DESKTOP
#include "ExtendedControl.h"
#include "RectExtended.h"
#endif


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CCapi::CCapi()
{
    Init();
}

CCapi::~CCapi()
{
    End();
}

void CCapi::SetEntryDriver(CEntryDriver* pEntryDriver)
{
    m_pEntryDriver = pEntryDriver;
    m_pEngineArea  = m_pEntryDriver->m_pEngineArea;
}

const Logic::SymbolTable& CCapi::GetSymbolTable() const
{
    return m_pEngineArea->GetSymbolTable();
}

void CCapi::Init()
{
    m_pEntryDriver = NULL;
    m_pEngineArea  = NULL;

#ifdef WIN_DESKTOP
    m_bHasHlp = false;
    m_pAroundField = NULL;
    m_pFrameWindow = NULL;
    m_pExtendedControl = NULL;
#endif
}

void CCapi::End()
{
#ifdef WIN_DESKTOP
    DeleteLabels();
#endif
}


#ifdef WIN_DESKTOP

void CCapi::DeleteLabels()
{
    if( m_pExtendedControl != NULL )
    {
        m_pExtendedControl->DestroyWindow();
        delete m_pExtendedControl;
        m_pExtendedControl = NULL;
    }
}

void CCapi::DoQuestion(const DEFLD* const pDeField)
{
    m_showingHelp = false;

    ASSERT(m_pAroundField != nullptr);
    const int field_symbol_index = pDeField->GetSymbol();

    // get the field text
    const CapiContent field_capi_content = GetFieldAndBlockCombinedCapiContent(field_symbol_index, CapiContentType::Question);

    // refresh the text
    WindowsDesktopMessage::Send(WM_IMSA_SETCAPITEXT, &field_capi_content.question_text);
}

#endif


CapiContent CCapi::GetCapiContent(const int symbol_index, const CapiContentType capi_content_type) const
{
    CapiContent capi_content;

    // if not using CAPI, exit immediately
    if( !m_pEntryDriver->GetApplication()->GetUseQuestionText() )
        return capi_content;

    // get the current CAPI language
    CEntryDriver* pEntryDriver = (CEntryDriver*)m_pEntryDriver;
    const Language& current_language = pEntryDriver->GetQuestMgr()->GetCurrentLanguage();

    const Symbol& symbol = NPT_Ref(symbol_index);

    for( int type = 0; type < 2; ++type )
    {
        const bool evaluating_question_text = ( type == 0 );

        // skip evaluating the particular CAPI text if it is not requested
        if( ( evaluating_question_text && capi_content_type == CapiContentType::Help ) ||
            ( !evaluating_question_text && capi_content_type == CapiContentType::Question ) )
        {
            continue;
        }

        // if the symbol is a field, check if we should be displaying question text for it
        if( evaluating_question_text &&
            symbol.IsA(SymbolType::Variable) &&
            !assert_cast<const VART&>(symbol).GetShowQuestionText() )
        {
            continue;
        }

        // evaluate the question or help text
        SharableString& text = evaluating_question_text ? capi_content.question_text :
                                                          capi_content.help_text;

        text = m_pEntryDriver->m_pIntDriver->EvaluateCapiText(current_language.GetName(), evaluating_question_text, symbol_index);
    }

    return capi_content;
}


CapiContent CCapi::GetFieldAndBlockCombinedCapiContent(const int field_symbol_index, const CapiContentType capi_content_type) const
{
    // get the field content
    CapiContent field_capi_content = GetCapiContent(field_symbol_index, capi_content_type);

    // if the field is part of a block, get the block content
    VART* const pVarT = VPT(field_symbol_index);

    if( pVarT->GetEngineBlock() == nullptr )
        return field_capi_content;

    CapiContent block_capi_content = GetCapiContent(pVarT->GetEngineBlock()->GetSymbolIndex(), capi_content_type);

    // combine the block and field content
    block_capi_content.question_text.MakeModifiable().append(field_capi_content.question_text.GetString());
    block_capi_content.help_text.MakeModifiable().append(field_capi_content.help_text.GetString());

    return block_capi_content;
}


const std::string& CCapi::GetRuntimeStylesCss()
{
    return m_pEntryDriver->GetQuestMgr()->GetRuntimeStylesCss();
}


#ifdef WIN_DESKTOP

void CCapi::UpdateSelection(const CString& csText)
{
    if( m_pExtendedControl != NULL ) // GHM 20100616
        m_pExtendedControl->UpdateSelection(csText);
}


void CCapi::ResponseUnOverlap( CWnd* pWnd, CRect maxRect )
{
    if( m_pExtendedControl != NULL && m_pExtendedControl->GetSafeHwnd() != NULL ) // GHM 20100622
    {
        CWnd* pControlWindow = m_pExtendedControl;

        CRect fieldRect;

        pWnd->GetWindowRect( fieldRect );

        CRect responsesRect;

        pControlWindow->GetWindowRect(responsesRect);

        bool bChanged = CRectExt::UnIntersect( &responsesRect, fieldRect, maxRect );

        if( bChanged ) {
            pControlWindow->SetWindowPos(NULL, responsesRect.left, responsesRect.top,
                responsesRect.Width(), responsesRect.Height(), SWP_NOACTIVATE);
        }
    }
}


void CCapi::ToggleHelp(const DEFLD* const pDeField)
{
    m_showingHelp ? DoQuestion(pDeField) :
                    ShowHelp(pDeField);
}


void CCapi::ShowHelp(const DEFLD* const pDeField)
{
    m_showingHelp = true;

    const int field_symbol_index = pDeField->GetSymbol();

    // get the field text
    const CapiContent field_capi_content = GetFieldAndBlockCombinedCapiContent(field_symbol_index, CapiContentType::Help);

    // refresh the text if there are lines
    if( !field_capi_content.help_text->empty() )
    {
        const COLORREF background_color = RGB(240, 240, 240); // grey background for help
        WindowsDesktopMessage::Send(WM_IMSA_SETCAPITEXT, &field_capi_content.help_text, &background_color);
    }
}


int CCapi::DoLabelsModeless(const DEFLD* const pDeField)
{
    int iRet = -1;

    DeleteLabels();

    VART* pVarT = VPT(pDeField->GetSymbol());

    CaptureInfo evaluated_capture_info = pVarT->GetEvaluatedCaptureInfo();

    if( evaluated_capture_info.GetCaptureType() != CaptureType::TextBox )
    {
        if( evaluated_capture_info.GetCaptureType() == CaptureType::Photo ||
            evaluated_capture_info.GetCaptureType() == CaptureType::Signature ||
            evaluated_capture_info.GetCaptureType() == CaptureType::Audio )
        {
            return iRet; // BINARY_TYPES_TO_ENGINE_TODO temporarily ignoring these capture types
        }

        m_pExtendedControl = new CExtendedControl(m_pFrameWindow ? m_pFrameWindow : AfxGetApp()->GetMainWnd());

        ResponseProcessor* response_processor = m_pEntryDriver->GetResponseProcessor(pDeField);

        iRet = m_pExtendedControl->DoModeless(pVarT, evaluated_capture_info, response_processor,
            m_pAroundField, m_pEntryDriver->GetApplication()->GetDecimalMarkIsComma());
    }

    return iRet;
}


void CCapi::RefreshPosition()
{
    if( m_pExtendedControl != NULL && m_pExtendedControl->GetSafeHwnd() != NULL ) // 20100623
        m_pExtendedControl->RefreshPosition();
}


void CCapi::CheckInZone( bool bRefresh )
{
    // Check windows overlapping. Move labels window if necessary
    if( m_pExtendedControl != NULL && m_pExtendedControl->GetSafeHwnd() != NULL )
    {
        CRect responsesRect;
        CRect maxRect;

        responsesRect.SetRectEmpty();
        maxRect.SetRectEmpty();

        m_pExtendedControl->GetWindowRect(responsesRect);

        if( m_pFrameWindow != NULL )
            m_pFrameWindow->GetWindowRect( maxRect );
        else
            AfxGetApp()->GetMainWnd()->GetWindowRect( maxRect );

        if( CheckInZone( &responsesRect, maxRect ) || bRefresh )
            m_pExtendedControl->MoveWindow(responsesRect);
    }
}


bool CCapi::CheckInZone( CRect* pResponsesRect, CRect maxRect ) {
    CRect   rectUnion;
    CRect   oldResponsesRect;

    oldResponsesRect=*pResponsesRect;
    rectUnion = *pResponsesRect;

    int     iDiffY=0, iDiffX=0;
    if( rectUnion.top < maxRect.top )
        iDiffY = maxRect.top-rectUnion.top;
    else if( rectUnion.bottom > maxRect.bottom )
        iDiffY = maxRect.bottom-rectUnion.bottom;

    if( rectUnion.left < maxRect.left )
        iDiffX = maxRect.left-rectUnion.left;
    else if( rectUnion.right > maxRect.right )
        iDiffX = maxRect.right-rectUnion.right;

    bool    bChanged = false;

    pResponsesRect->top += iDiffY;
    pResponsesRect->bottom += iDiffY;
    pResponsesRect->left += iDiffX;
    pResponsesRect->right += iDiffX;

    bChanged = ( oldResponsesRect != *pResponsesRect ) != 0;

    return bChanged;

}


void CCapi::CheckOverlap( bool bRefresh ) {
    CRect   maxRect;

    if( m_pFrameWindow != NULL )
        m_pFrameWindow->GetWindowRect( maxRect );
    else
        AfxGetApp()->GetMainWnd()->GetWindowRect( maxRect );

    if( m_pAroundField != NULL ) {
        ResponseUnOverlap( m_pAroundField, maxRect );
    }
}

#endif // WIN_DESKTOP
