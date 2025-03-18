#include "StdAfx.h"
#include "ViewDoc.h"
#include "NoDocumentViewInput.h"
#include <zAppO/PFF.h>
#include <zViewO/ViewInputCreator.h>


IMPLEMENT_DYNCREATE(ViewDoc, CDocument)


ViewDoc::ViewDoc()
{
}


BOOL ViewDoc::OnNewDocument()
{
    ASSERT(m_viewInput == nullptr);

    try
    {
        m_viewInput = std::make_unique<NoDocumentViewInput>();

        return TRUE;
    }

    catch( const CSProException& exception )
    {
        ErrorMessage::Display(exception);
        return FALSE;
    }
}


BOOL ViewDoc::OnOpenDocument(LPCTSTR lpszPathName)
{
    try
    {
        std::unique_ptr<ViewInput> view_input = ViewInputCreator::CreateInputFromFilePath(TC::ToUtf8(lpszPathName));

        ProcessCloseDocument();

        m_viewInput = std::move(view_input);

        SetPathName(TC::ToWide(m_viewInput->GetFilePath()).c_str());

        return TRUE;
    }

    catch( const CSProException& exception )
    {
        ErrorMessage::Display(exception);
        return FALSE;
    }
}


void ViewDoc::OnCloseDocument()
{
    ProcessCloseDocument();

    __super::OnCloseDocument();
}


std::string ViewDoc::GetDescription() const
{
    ASSERT(m_viewInput != nullptr);
    return m_viewInput->GetDescription();
}


std::string ViewDoc::GetUrl()
{
    ASSERT(m_viewInput != nullptr);
    return m_viewInput->GetUrl();
}


void ViewDoc::ProcessCloseDocument()
{
    if( m_viewInput != nullptr && m_viewInput->GetPff() != nullptr )
        m_viewInput->GetPff()->ExecuteOnExitPff();
}
