#include "StdAfx.h"
#include "TextEditDoc.h"
#include "CSDocDoc.h"


BEGIN_MESSAGE_MAP(TextEditDoc, CDocument)
    ON_UPDATE_COMMAND_UI(ID_FILE_SAVE, OnUpdateFileSave)
END_MESSAGE_MAP()


TextEditDoc::TextEditDoc()
    :   m_textEditView(nullptr)
{
}


void TextEditDoc::SetAssociatedDocSetSpec(std::shared_ptr<DocSetSpec> doc_set_spec)
{
    // only CSPro Documents can change their associated Document Set
    ASSERT(IsKindOf(RUNTIME_CLASS(CSDocDoc)));
    m_docSetSpec = std::move(doc_set_spec);
}


void TextEditDoc::UpdateTitle()
{
    std::wstring title = GetPathName().IsEmpty() ? SO::TrimLeft(GetTitle(), '*') :
                                                   PortableFunctions::PathGetFilename(GetPathName());

    SetTitle(IsModified() ? ( _T("*") + title ).c_str() :
                            title.c_str());
}


void TextEditDoc::SetModifiedFlag(BOOL modified/* = TRUE*/)
{
    if( IsModified() == modified )
        return;

    __super::SetModifiedFlag(modified);

    if( m_textSource != nullptr )
        m_textSource->SetModified();

    UpdateTitle();
}


BOOL TextEditDoc::OnOpenDocument(LPCTSTR lpszPathName)
{
    try
    {
        m_textSource = TextSourceEditable::FindOpenOrCreate(lpszPathName);
    }

    catch( const CSProException& exception )
    {
        ErrorMessage::Display(exception);
        return FALSE;
    }

    UpdateTitle();

    return TRUE;
}


BOOL TextEditDoc::OnSaveDocument(LPCTSTR lpszPathName)
{
    try
    {
        ASSERT(m_textEditView != nullptr);
        CLogicCtrl* logic_ctrl = m_textEditView->GetLogicCtrl();

        // if saving to the same file, use the existing text source
        if( GetPathName() == lpszPathName )
        {
            ASSERT(m_textSource != nullptr && SO::EqualsNoCase(m_textSource->GetFilename(), lpszPathName));
            m_textSource->Save();
        }

        // otherwise create a new one
        else
        {
            // Save As is only enabled for CSPro Documents
            ASSERT(IsKindOf(RUNTIME_CLASS(CSDocDoc)));

            if( m_textSource != nullptr )
                m_textSource->SetSourceModifier(nullptr);

            m_textSource = std::make_shared<TextSourceEditable>(lpszPathName, logic_ctrl->GetText());
            m_textSource->SetSourceModifier(this);
        }

        SetModifiedFlag(FALSE);
        logic_ctrl->SetSavePoint();

        return TRUE;
    }

    catch( const CSProException& exception )
    {
        ErrorMessage::Display(exception);
        return FALSE;
    }
}


void TextEditDoc::OnUpdateFileSave(CCmdUI* pCmdUI)
{
    pCmdUI->Enable(IsModified());
}


std::wstring TextEditDoc::OnViewInitialUpdate(TextEditView& text_edit_view)
{
    m_textEditView = &text_edit_view;

    std::wstring initial_text;

    if( m_textSource != nullptr )
    {
        initial_text = m_textSource->GetText();
        m_textSource->SetSourceModifier(this);
    }

    return initial_text;
}


void TextEditDoc::OnViewDeactivate()
{
    // when using the SourceModifier with a modified document, mark the document as modified at the time of the
    // deactivation to ensure that any other users of this content will get the most recent version of the text buffer
    if( m_textSource != nullptr && IsModified() )
        m_textSource->SetModified();
}


void TextEditDoc::ReloadFromDisk()
{
    ASSERT(m_textSource != nullptr && m_textEditView != nullptr);

    try
    {
        const std::wstring& text = m_textSource->ReloadFromDisk();
        m_textEditView->SetTextAndSetSavePoint(text);

        SetModifiedFlag(FALSE);
    }

    catch( const CSProException& exception )
    {
        ErrorMessage::Display(exception);
    }
}


void TextEditDoc::SyncTextSource()
{
    ASSERT(m_textSource != nullptr && m_textEditView != nullptr);

    std::wstring text = m_textEditView->GetLogicCtrl()->GetText();

    if( ConvertTabsToSpacesAndTrimRightEachLine() )
        SO::ConvertTabsToSpacesAndTrimRightEachLine(text);

    m_textSource->SetText(std::move(text));
}
