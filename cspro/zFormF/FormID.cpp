//***************************************************************************
//  File name: FormID.cpp
//
//  Description:
//       Forms tree control node implementation
//
//  History:    Date       Author   Comment
//              ---------------------------
//              11 Feb 99   gsf     Created for Measure 1.0
//
//***************************************************************************

#include "StdAfx.h"
#include "FormID.h"


IMPLEMENT_DYNAMIC(CFormID, CObject)

CFormID::CFormID()
    :   m_eNodeType(eINVALID_TYPE),
        m_pFormDoc(nullptr),
        m_iForm(0),
        m_iCol(NONE),
        m_iRosterField(NONE),
        m_pItem(nullptr)
{
}

CFormID::~CFormID()
{
}



// *********************************************************************

IMPLEMENT_DYNAMIC(CFormNodeID, CFormID)

CFormNodeID::CFormNodeID()
    :   m_iCount(0)
{
}

CFormNodeID::~CFormNodeID()
{
}



// *********************************************************************

IMPLEMENT_DYNAMIC(FormExternalCodeID, CFormID)

FormExternalCodeID::FormExternalCodeID(CFormDoc* pDoc, std::optional<CodeFile> code_file)
    :   m_codeFile(std::move(code_file))
{
    SetNodeType(eFFT_EXTERNALCODE);
    SetFormDoc(pDoc);
}


std::optional<CString> FormExternalCodeID::GetName() const
{
    // the heading for the external code nodes will simply be "Logic" because clicking on it will show the main application logic
    return m_codeFile.has_value() ? UTF8_TODO::GetCString(Path::GetFilenameWithoutExtension(m_codeFile->GetFilePath())) :
                                    _T("Logic");
}


std::optional<CString> FormExternalCodeID::GetLabel() const
{
    return GetName();
}


TextSource* FormExternalCodeID::GetTextSource()
{
    return m_codeFile.has_value() ? &m_codeFile->GetTextSource() :
                                    nullptr;
}



// *********************************************************************

IMPLEMENT_DYNAMIC(FormReportID, CFormID)

FormReportID::FormReportID(CFormDoc* pDoc, std::optional<ReportFile> report_file)
    :   m_reportFile(std::move(report_file))
{
    SetNodeType(eFFT_REPORT);
    SetFormDoc(pDoc);
}


std::optional<CString> FormReportID::GetName() const
{
    return m_reportFile.has_value() ? UTF8_TODO::GetCString(m_reportFile->GetName()) :
                                      L"Reports"; // the heading
}


std::optional<CString> FormReportID::GetLabel() const
{
    return m_reportFile.has_value() ? UTF8_TODO::GetCString(Path::GetFilenameWithoutExtension(m_reportFile->GetFilePath())) :
                                      L"Reports"; // the heading
}


TextSource* FormReportID::GetTextSource()
{
    return m_reportFile.has_value() ? &m_reportFile->GetTextSource() :
                                      nullptr; // the heading
}
