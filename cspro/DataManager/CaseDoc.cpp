#include "StdAfx.h"
#include "CaseDoc.h"
#include "DataManager.h"


IMPLEMENT_DYNCREATE(CaseDoc, CDocument)


BOOL CaseDoc::OnNewDocument()
{
    const std::unique_ptr<DataManagerApp::CaseDocData> case_doc_data = assert_cast<DataManagerApp*>(AfxGetApp())->ReleaseCaseDocData();

    if( case_doc_data == nullptr )
        return ReturnProgrammingError(FALSE);

    std::tie(m_dictionary,
             m_caseAccess,
             m_connectionString,
             m_currentCase,
             m_initialPageCommandId) = std::move(*case_doc_data);

    const std::string title = SO::CreateParentheticalExpression(m_currentCase->GetSingleLineKey(), m_dictionary->GetName());
    SetTitle(TC::ToWide(title).c_str());

    return RunPostOpenTasks();
}


UINT CaseDoc::GetDefaultPageCommandId()
{
    return m_initialPageCommandId;
}
