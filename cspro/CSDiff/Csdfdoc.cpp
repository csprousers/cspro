#include "StdAfx.h"
#include "Csdfdoc.h"
#include "Csdfview.h"
#include "CSDiff.h"
#include "Filebrow.h"
#include <zUtilO/Filedlg.h>
#include <zDiffO/Differ.h>


IMPLEMENT_DYNCREATE(CCSDiffDoc, CDocument)

BEGIN_MESSAGE_MAP(CCSDiffDoc, CDocument)
    ON_UPDATE_COMMAND_UI(ID_FILE_SAVE, OnUpdateIsDictionaryDefined)
    ON_COMMAND(ID_FILE_SAVE, OnFileSave)
    ON_UPDATE_COMMAND_UI(ID_FILE_SAVE_AS, OnUpdateIsDictionaryDefined)
    ON_COMMAND(ID_FILE_SAVE_AS, OnFileSaveAs)
    ON_UPDATE_COMMAND_UI(ID_FILE_RUN, OnUpdateIsDictionaryDefined)
    ON_COMMAND(ID_FILE_RUN, OnFileRun)
    ON_COMMAND(ID_OPTIONS_EXCLUDED, OnOptionsExcluded)
    ON_UPDATE_COMMAND_UI(ID_OPTIONS_EXCLUDED, OnUpdateOptionsExcluded)
END_MESSAGE_MAP()


CCSDiffDoc::CCSDiffDoc()
    :   m_diffSpec(std::make_shared<DiffSpec>()),
        m_bRetSave(false)
{
    m_diffSpec->SetShowLabels(!SharedSettings::ViewNamesInTree());

    m_pff.SetAppType(APPTYPE::COMPARE_TYPE);
    m_pff.SetViewResultsFlag(false);
    m_pff.SetViewListing(ALWAYS);
}


BOOL CCSDiffDoc::OnOpenDocument(LPCTSTR lpszPathName)
{
    if( !CDocument::OnOpenDocument(lpszPathName) )
        return FALSE;

    const std::wstring extension = PortableFunctions::PathGetFileExtension(lpszPathName);

    if( SO::EqualsNoCase(extension, FileExtensions::Pff) )
    {
        m_pff.SetPifFileName(lpszPathName);

        if( m_pff.LoadPifFile() )
            RunBatchDiff();

        return FALSE;
    }

    else if( SO::EqualsNoCase(extension, FileExtensions::CompareSpec) )
    {
        if( !OpenSpecFile(lpszPathName) )
            return FALSE;

        AfxGetApp()->WriteProfileString(_T("Settings"), _T("Last Open"), lpszPathName);

        const std::wstring spec_filename = lpszPathName;
        m_pff.SetAppFName(WS2CS(spec_filename));
        m_pff.SetListingFName(WS2CS(spec_filename) + FileExtensions::WithDot::Listing);

        const std::wstring pff_filename = spec_filename + FileExtensions::WithDot::Pff;
        m_pff.SetPifFileName(WS2CS(pff_filename));

        if( PortableFunctions::FileIsRegular(pff_filename) && m_pff.LoadPifFile() )
        {
            if( !SO::EqualsNoCase(spec_filename, m_pff.GetAppFName()) )
            {
                AfxMessageBox(FormatText(_T("Spec file in %s\ndoes not match %s"), pff_filename.c_str(), spec_filename.c_str()));
                return FALSE;
            }
        }
    }

    else if( SO::EqualsNoCase(extension, FileExtensions::Dictionary) )
    {
        m_pff.SetAppFName(_T(""));

        if( !OpenDictFile(lpszPathName) )
            return FALSE;

        AfxGetApp()->WriteProfileString(_T("Settings"), _T("Last Open"), lpszPathName);
    }

    else
    {
        AfxMessageBox(_T("Invalid file type."));
        return FALSE;
    }

    return TRUE;
}


bool CCSDiffDoc::OpenDictFile(const std::wstring& filename)
{
    try
    {
        m_diffSpec->SetDictionary(CDataDict::InstantiateAndOpen(filename, false));
        return true;
    }

    catch( const CSProException& )
    {
		return false;
    }
}


void CCSDiffDoc::OnFileSave()
{
    if( m_pff.GetAppFName().IsEmpty() )
    {
        OnFileSaveAs();
    }

    else
    {
        SaveSpecFile();
        SetModifiedFlag(FALSE);
        m_bRetSave = true;
    }
}


void CCSDiffDoc::OnFileSaveAs()
{
    std::wstring path = CS2WS(m_pff.GetAppFName()); // BMD 14 Mar 2002

    // if no spec filename exists, base it on the dictionary's filename
    if( path.empty() )
        path = PortableFunctions::PathReplaceFileExtension(GetDictionary().GetFullFileName(), FileExtensions::CompareSpec);

    CIMSAFileDialog file_dlg(FALSE, FileExtensions::CompareSpec,
                             path.c_str(),
                             OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
                             _T("Compare Specification Files (*.cmp)|*.cmp|All Files (*.*)|*.*||"));

    file_dlg.m_ofn.lpstrTitle = _T("Save Compare Specification File");

    if( file_dlg.DoModal() == IDOK )
    {
        m_pff.SetAppFName(file_dlg.GetPathName());
        AfxGetMainWnd()->SetWindowText(CCSDiffView::CreateWindowTitle(CS2WS(m_pff.GetAppFName()), &GetDictionary()).c_str());
        SaveSpecFile();
        SetModifiedFlag(FALSE);
        AfxGetApp()->AddToRecentFileList(file_dlg.GetPathName());
        m_bRetSave = true;
    }

    else
    {
        m_bRetSave = false;
    }
}


void CCSDiffDoc::SaveSpecFile()
{
    try
    {
        m_diffSpec->Save(CS2WS(m_pff.GetAppFName()));
    }

    catch( const CSProException& exception )
    {
        ErrorMessage::Display(exception);
    }
}


bool CCSDiffDoc::OpenSpecFile(const std::wstring& filename)
{
    try
    {
        auto new_diff_spec = std::make_unique<DiffSpec>();
        new_diff_spec->Load(filename, false);

        m_diffSpec = std::move(new_diff_spec);
        SharedSettings::ToggleViewNamesInTree(!m_diffSpec->GetShowLabels());

        return true;
    }

    catch( const CSProException& exception )
    {
        ErrorMessage::Display(exception);
        return false;
    }
}


BOOL CCSDiffDoc::SaveModified()
{
    if( !IsModified() )
        return TRUE;

    const std::wstring spec_filename = !m_pff.GetAppFName().IsEmpty() ? CS2WS(m_pff.GetAppFName()) :
                                                                        WindowsWS::LoadString(AFX_IDS_UNTITLED);

    const std::wstring prompt = WindowsWS::AfxFormatString1(AFX_IDP_ASK_TO_SAVE, spec_filename);

    switch( AfxMessageBox(prompt, MB_YESNOCANCEL, AFX_IDP_ASK_TO_SAVE) )
    {
        case IDCANCEL:
            return FALSE;       // don't continue

        case IDYES:
            // If so, either Save or Update, as appropriate
            OnFileSave();
            return m_bRetSave;

        case IDNO:
            // If not saving changes, revert the document
            break;

        default:
            ASSERT(FALSE);
            break;
    }

    return TRUE;    // keep going
}


void CCSDiffDoc::OnUpdateIsDictionaryDefined(CCmdUI* pCmdUI)
{
    pCmdUI->Enable(m_diffSpec->IsDictionaryDefined());
}


void CCSDiffDoc::OnUpdateOptionsExcluded(CCmdUI* pCmdUI)
{
    pCmdUI->SetCheck(m_diffSpec->GetSaveExcludedItems());
}


void CCSDiffDoc::OnOptionsExcluded()
{
    m_diffSpec->SetSaveExcludedItems(!m_diffSpec->GetSaveExcludedItems());
    SetModifiedFlag();
}


namespace
{
    class ToolDiffer : public Differ
    {
    public:
        ToolDiffer(std::shared_ptr<DiffSpec> diff_spec)
            :   Differ(std::move(diff_spec))
        {
        }

    protected:
        void HandleNoDifferences(const PFF& /*pff*/) override
        {
            AfxMessageBox(_T("No differences were found.\n\n")
                          _T("(Note: The system only compares items defined in the data\n")
                          _T("dictionary and checked in the dictionary tree.)"));
        }
    };
}


void CCSDiffDoc::OnFileRun()
{
    // if the listing file hasn't been defined, put it in the same folder as the dictionary or the PFF
    if( m_pff.GetListingFName().IsEmpty() )
    {
        m_pff.SetListingFName(WS2CS(m_pff.GetPifFileName().IsEmpty() ?
            PortableFunctions::PathAppendToPath(PortableFunctions::PathGetDirectory(GetDictionary().GetFullFileName()), _T("CSDiff.lst")) :
            PortableFunctions::PathReplaceFileExtension(m_pff.GetPifFileName(), FileExtensions::WithDot::Listing)));
    }

    CFilesBrow file_dlg(this, m_pff);

    if( file_dlg.DoModal() != IDOK )
        return;

    // save the PFF
    if( !m_pff.GetAppFName().IsEmpty() )
    {
        m_pff.SetPifFileName(m_pff.GetAppFName() + FileExtensions::WithDot::Pff);
        m_pff.Save();
    }

    try
    {
        ToolDiffer(m_diffSpec).Run(m_pff, false);
    }

    catch( const CSProException& exception )
    {
        ErrorMessage::Display(exception);
    }
}


void CCSDiffDoc::RunBatchDiff()
{
    try
    {
        if( m_pff.GetAppType() != APPTYPE::COMPARE_TYPE )
        {
            throw CSProException(_T("PFF file '%s' was not read correctly. Check the file for parameters invalid to CSDiff."),
                                 m_pff.GetPifFileName().GetString());
        }

        Differ().Run(m_pff, true);

        m_pff.ExecuteOnExitPff();
    }

    catch( const CSProException& exception )
    {
        ErrorMessage::Display(exception);
    }
}
