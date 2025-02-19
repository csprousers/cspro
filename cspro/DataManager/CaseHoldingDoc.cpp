#include "StdAfx.h"
#include "CaseHoldingDoc.h"


CaseHoldingDoc::CaseHoldingDoc()
    :   m_dictionaryAllowsExport(false),
        m_dictionaryUsesBinaryData(false)
{
}


BOOL CaseHoldingDoc::RunPostOpenTasks()
{
    ASSERT(m_dictionary != nullptr &&
           m_caseAccess != nullptr);

    m_dictionaryAllowsExport = m_dictionary->GetAllowExport();
    m_dictionaryUsesBinaryData = m_caseAccess->GetCaseMetadata().UsesBinaryData();

    return TRUE;
}


std::string CaseHoldingDoc::GetDictionaryDirectory() const
{
    if( PortableFunctions::FileIsRegular(m_dictionary->GetFilePath()) )
        return PortableFunctions::PathGetDirectory(m_dictionary->GetFilePath());

    if( m_connectionString.HasFilePath() )
        return PortableFunctions::PathGetDirectory(m_connectionString.GetFilePath());

    return std::string();
}


std::string CaseHoldingDoc::GetDataDirectory() const
{
    if( m_connectionString.HasFilePath() )
        return PortableFunctions::PathGetDirectory(m_connectionString.GetFilePath());

    if( PortableFunctions::FileIsRegular(m_dictionary->GetFilePath()) )
        return PortableFunctions::PathGetDirectory(m_dictionary->GetFilePath());

    return std::string();
}


bool CaseHoldingDoc::DictionaryAllowsExport(const char* const operation/* = "this operation"*/) const
{
    ASSERT(operation != nullptr);

    if( m_dictionaryAllowsExport )
        return true;

    ErrorMessage::PostMessageForDisplay(FormatText("The security restrictions for the dictionary '%s' prohibit %s.",
                                                    m_dictionary->GetName().c_str(),
                                                    operation));
    return false;
}
