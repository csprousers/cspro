#include "StdAfx.h"
#include "Csdfdoc.h"
#include "Csdfview.h"
#include "CSDiff.h"
#include "Filebrow.h"
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
    :   m_diffSpec(std::make_unique<DiffSpec>()),
        m_bRetSave(false)
{
    m_diffSpec->SetShowLabels(!SharedSettings::ViewNamesInTree());

    m_pff.SetAppType(APPTYPE::COMPARE_TYPE);
    m_pff.SetViewResultsFlag(false);
    m_pff.SetViewListing(ALWAYS);
}


BOOL CCSDiffDoc::OnOpenDocument(LPCTSTR lpszPathName)
{
    if( !__super::OnOpenDocument(lpszPathName) )
        return FALSE;

    try
    {
        const std::string file_path = TC::ToUtf8(lpszPathName);
        const std::string extension = PortableFunctions::PathGetFileExtension(file_path);

        if( SO::EqualsNoCase(extension, FileExtensions::Pff) )
        {
            m_pff.SetPifFileName(UTF8_TODO::GetCString(file_path));

            if( m_pff.LoadPifFile() )
                RunBatchDiff();

            return FALSE;
        }

        else if( SO::EqualsNoCase(extension, FileExtensions::CompareSpec) )
        {
            if( !OpenSpecFile(file_path) )
                return FALSE;

            AfxGetApp()->WriteProfileString(L"Settings", L"Last Open", lpszPathName);

            m_pff.SetAppFName(UTF8_TODO::GetCString(file_path));
            m_pff.SetListingFName(UTF8_TODO::GetCString(PortableFunctions::PathAppendFileExtension(file_path, FileExtensions::Listing)));

            const std::string pff_file_path = PortableFunctions::PathAppendFileExtension(file_path, FileExtensions::Pff);
            m_pff.SetPifFileName(UTF8_TODO::GetCString(pff_file_path));

            if( PortableFunctions::FileIsRegular(pff_file_path) && m_pff.LoadPifFile() )
            {
                if( !SO::EqualsNoCase(file_path, m_pff.GetAppFName()) )
                    throw CSProException("Spec file in %s\ndoes not match %s", pff_file_path.c_str(), file_path.c_str());
            }
        }

        else if( SO::EqualsNoCase(extension, FileExtensions::Dictionary) )
        {
            m_pff.SetAppFName(L"");
            m_diffSpec->SetDictionary(CDataDict::InstantiateAndOpen(file_path, false));

            AfxGetApp()->WriteProfileString(L"Settings", L"Last Open", lpszPathName);
        }

        else
        {
            throw CSProException("Invalid file type.");
        }

        return TRUE;
    }

    catch( const CSProException& exception )
    {
        ErrorMessage::Display(exception);
        return FALSE;
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
    std::string file_path = UTF8_TODO::GetUtf8(m_pff.GetAppFName()); // BMD 14 Mar 2002

    // if no spec file path exists, base it on the dictionary's file path
    if( file_path.empty() )
        file_path = PortableFunctions::PathReplaceFileExtension(GetDictionary().GetFilePath(), FileExtensions::CompareSpec);

    SaveFileDlg save_file_dlg(0, FileExtensions::CompareSpec, file_path, L"Compare Specification Files (*.cmp)|*.cmp|All Files (*.*)|*.*||");
    save_file_dlg.SetTitle(L"Save Compare Specification File");

    if( save_file_dlg.DoModal() == IDOK )
    {
        m_pff.SetAppFName(UTF8_TODO::GetCString(save_file_dlg.GetFilePath()));
        AfxGetMainWnd()->SetWindowText(TC::ToWide(CCSDiffView::CreateWindowTitle(UTF8_TODO::GetUtf8(m_pff.GetAppFName()), &GetDictionary()).c_str()).c_str());
        SaveSpecFile();
        SetModifiedFlag(FALSE);
        AfxGetApp()->AddToRecentFileList(TC::ToWide(save_file_dlg.GetFilePath()).c_str());
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
        m_diffSpec->Save(UTF8_TODO::GetUtf8(m_pff.GetAppFName()));
    }

    catch( const CSProException& exception )
    {
        ErrorMessage::Display(exception);
    }
}


bool CCSDiffDoc::OpenSpecFile(const std::string& file_path)
{
    try
    {
        auto new_diff_spec = std::make_unique<DiffSpec>();
        new_diff_spec->Load(file_path, false);

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


void CCSDiffDoc::OnUpdateIsDictionaryDefined(CCmdUI* const pCmdUI)
{
    pCmdUI->Enable(m_diffSpec->IsDictionaryDefined());
}


void CCSDiffDoc::OnUpdateOptionsExcluded(CCmdUI* const pCmdUI)
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
            AfxMessageBox(L"No differences were found.\n\n"
                          L"(Note: The system only compares items defined in the data\n"
                          L"dictionary and checked in the dictionary tree.)");
        }
    };
}


void CCSDiffDoc::OnFileRun()
{
    // if the listing file hasn't been defined, put it in the same folder as the dictionary or the PFF
    if( m_pff.GetListingFName().IsEmpty() )
    {
        m_pff.SetListingFName(WS2CS(m_pff.GetPifFileName().IsEmpty() ?
            UTF8_TODO::GetWide(PortableFunctions::PathReplaceFilename(GetDictionary().GetFilePath(), "CSDiff.lst")) :
            PortableFunctions::PathReplaceFileExtension(m_pff.GetPifFileName(), UTF8_TODO::GetCString(FileExtensions::Listing))));
    }

    CFilesBrow file_dlg(this, m_pff);

    if( file_dlg.DoModal() != IDOK )
        return;

    // save the PFF
    if( !m_pff.GetAppFName().IsEmpty() )
    {
        m_pff.SetPifFileName(UTF8_TODO::GetCString(PortableFunctions::PathAppendFileExtension(UTF8_TODO::GetUtf8(m_pff.GetAppFName()), FileExtensions::Pff)));
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
            throw CSProException("PFF file '%s' was not read correctly. Check the file for parameters invalid to CSDiff.",
                                 UTF8_TODO::GetUtf8(m_pff.GetPifFileName()).c_str());
        }

        Differ().Run(m_pff, true);

        m_pff.ExecuteOnExitPff();
    }

    catch( const CSProException& exception )
    {
        ErrorMessage::Display(exception);
    }
}
