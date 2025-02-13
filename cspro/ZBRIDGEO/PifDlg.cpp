#include "StdAfx.h"
#include "PifDlg.h"
#include "DictReconcileDlg.h"
#include <zToolsO/Tools.h>
#include <zUtilO/FileUtil.h>


/////////////////////////////////////////////////////////////////////////////
// CPifDlg dialog

CPifDlg::CPifDlg(const CArray<PIFINFO*, PIFINFO*>& pffInfo, CString title, CWnd* pParent /*=NULL*/)
    :   CDialog(IDD_PIFDLG, pParent)
{
    m_title = title;
    m_arrPifInfo.Copy(pffInfo);
}

CPifDlg::CPifDlg(const std::vector<std::shared_ptr<PIFINFO>>& pffInfo, CString title, CWnd* pParent /*=NULL*/)
    :   CDialog(IDD_PIFDLG, pParent)
{
    m_title = title;

    for( const auto& pifInfo : pffInfo )
        m_arrPifInfo.Add(pifInfo.get());
}


/////////////////////////////////////////////////////////////////////////////
// CPifDlg message handlers

BOOL CPifDlg::OnInitDialog()
{
    BOOL bRet = CDialog::OnInitDialog();
    if(!bRet)
        return bRet;

    SetWindowText(m_title);

    CWnd* pWnd = GetDlgItem(IDC_MYGRID);

    pWnd->GetClientRect(&m_Rect);

    m_pifgrid.m_iCols = 2;
    m_pifgrid.m_iRows = m_arrPifInfo.GetSize();
    m_pifgrid.m_sPifFileName = m_pPifFile->GetPifFileName();

    //Attach the grid to the control
    pWnd->SetFocus();
    m_pifgrid.AttachGrid(this,IDC_MYGRID);
    SetGridData();

    //Show the grid
    m_pifgrid.SetFocus();
    m_pifgrid.ShowWindow(SW_SHOW);
    m_pifgrid.GotoCell(0,0);
    m_pifgrid.StartEdit();
    m_pifgrid.m_pPifEdit.SetFocus();
    m_pifgrid.m_pPifEdit.SetCaretPos(CPoint(0,0));

    return bRet;  // return TRUE unless you set the focus to a control
                  // EXCEPTION: OCX Property Pages should return FALSE
}


void CPifDlg::SetGridData()
{
    m_pifgrid.SetGridData(m_arrPifInfo, m_Rect.Width());
}

void CPifDlg::OnOK()
{
    if( Validate() )
    {
        SaveAssociations();
        CDialog::OnOK();
    }
}


bool CPifDlg::IsValidFilePath(NullTerminatedStringView path, bool must_be_writeable)
{
    if( PortableFunctions::FileExists(path) )
    {
        // If the file exists, check that it is writeable (unless read only access)
        DWORD attr = GetFileAttributes(path.c_str());

        if( must_be_writeable && ( attr & FILE_ATTRIBUTE_READONLY ) != 0 )
            return false;
    }

    else
    {
        // Check that directory exists and is writeable (if required)
        std::wstring directory = PortableFunctions::PathGetDirectory(path);

        if( !PortableFunctions::FileIsDirectory(directory) )
            return false;
    }

    // Check that name doesn't contain invalid characters
    return ( path.find_first_of(_T("\"<>|*?")) == wstring_view::npos );
}


bool CPifDlg::Validate()
{
    std::vector<CString> processed_filenames;

    for( long lRowIndex = 0; lRowIndex < m_pifgrid.GetNumberRows(); lRowIndex++ )
    {
        CUGCell cellGrid;
        m_pifgrid.GetCell(-1, lRowIndex, &cellGrid);

        PIFINFO** ppPifInfo = (PIFINFO**)cellGrid.GetExtraMemPtr();

        if( ppPifInfo != nullptr )
        {
            PIFINFO* pifInfo = *ppPifInfo;
            std::vector<CString> filenames_to_add;

            // data files
            if( !pifInfo->dictionary_filename.IsEmpty() )
            {
                if( pifInfo->connection_strings.size() == 0 && pifInfo->sUName.CompareNoCase(OUTPFILE) != 0 )
                {
                    AfxMessageBox(FormatText(_T("You must specify a data file for %s"), pifInfo->sDisplay.GetString()));
                    return false;
                }

                if( pifInfo->connection_strings.size() > 1 && ( pifInfo->uOptions & PIF_MULTIPLE_FILES ) == 0 )
                {
                    // see comments in PifInfoPopulator::GetPifInfo for why the output data doesn't have this flag set
                    if( pifInfo->sUName.CompareNoCase(OUTPFILE) != 0 )
                    {
                        AfxMessageBox(FormatText(_T("You cannot specify multiple data files for %s"), pifInfo->sDisplay.GetString()));
                        return false;
                    }
                }

                for( const ConnectionString& connection_string : pifInfo->connection_strings )
                {
                    if( connection_string.IsFilenamePresent() )
                    {
                        if( PathHasWildcardCharacters(connection_string.GetFilename()) )
                        {
                            if( ( pifInfo->uOptions & PIF_MULTIPLE_FILES ) == 0 )
                            {
                                AfxMessageBox(_T("File names may not contain wildcards (* or ?)."));
                                return false;
                            }

                            std::wstring directory = PortableFunctions::PathGetDirectory(connection_string.GetFilename());

                            if( !PortableFunctions::FileIsDirectory(directory) )
                            {
                                AfxMessageBox(FormatText(_T("Directory %s not found"), directory.c_str()));
                                return false;
                            }
                        }

                        else
                        {
                            if( ( pifInfo->uOptions & PIF_FILE_MUST_EXIST ) != 0 && !PortableFunctions::FileIsRegular(connection_string.GetFilename()) )
                            {
                                AfxMessageBox(FormatText(_T("File %s not found"), connection_string.GetFilename().c_str()));
                                return false;
                            }

                            if( !IsValidFilePath(connection_string.GetFilename(), !( pifInfo->uOptions & PIF_READ_ONLY )) )
                            {
                                AfxMessageBox(FormatText(_T("%s is not a valid file name. Check that the directory exists ")
                                                         _T("and that the name does not contain invalid characters."),
                                                         connection_string.GetFilename().c_str()));
                                return false;
                            }

                            if( pifInfo->sUName.CompareNoCase(OUTPFILE) != 0 && !DictionaryChangesIfAnyAreOk(connection_string, pifInfo->dictionary_filename) )
                                return false;
                            
                            filenames_to_add.emplace_back(WS2CS(connection_string.GetFilename()));
                        }
                    }
                }
            }

            // non-data files
            else
            {
                std::vector<CString> filenames_to_process;

                if( ( pifInfo->uOptions & PIF_MULTIPLE_FILES ) != 0 && !m_pifgrid.m_arrMultFiles.IsEmpty() )
                {
                    ASSERT(pifInfo->sDisplay == INPUTTBD);
                    for( int i = 0; i < m_pifgrid.m_arrMultFiles.GetSize(); i++ )
                        filenames_to_process.emplace_back(m_pifgrid.m_arrMultFiles[i]);
                }

                else
                    filenames_to_process.emplace_back(pifInfo->sFileName);

                for( const auto& filename : filenames_to_process )
                {
                    filenames_to_add.emplace_back(filename);

                    if( filename.IsEmpty() && ( pifInfo->uOptions & PIF_ALLOW_BLANK ) == 0 )
                    {
                        AfxMessageBox(FormatText(_T("File Associations incomplete.\nFile name missing for %s."), pifInfo->sDisplay.GetString()));
                        return false;
                    }

                    if( filename.FindOneOf(_T("*?")) >= 0 && ( pifInfo->uOptions & PIF_ALLOW_WILDCARDS ) == 0 )
                    {
                        AfxMessageBox(_T("File names may not contain wildcards (* or ?)."));
                        return false;
                    }

                    // Check valid path except if name is empty or is multiple files (containing ")
					if( !filename.IsEmpty() && filename.Find(_T('"')) < 0 &&
                        !IsValidFilePath(filename, !( pifInfo->uOptions & PIF_READ_ONLY )) )
                    {
                        AfxMessageBox(FormatText(_T("%s is not a valid file name. Check that the directory exists ")
                                                 _T("and that the name does not contain invalid characters."), filename.GetString()));
                        return false;
                    }

                    if( !filename.IsEmpty() && ( pifInfo->uOptions & PIF_FILE_MUST_EXIST ) != 0 && !PortableFunctions::FileExists(filename) )
                    {
                        AfxMessageBox(FormatText(_T("File %s not found"), filename.GetString()));
                        return false;
                    }
                }
            }

            // check for duplicate files
            for( const CString& filename : filenames_to_add )
            {
                if( !filename.IsEmpty() &&
                    std::find_if(processed_filenames.cbegin(), processed_filenames.cend(),
                    [&](const CString& added_filename)
                    { return ( filename.CompareNoCase(added_filename) == 0 ); }) != processed_filenames.cend() )
                {
                    AfxMessageBox(FormatText(_T("You cannot use the file name %s more than once."), filename.GetString()));
                    return false;
                }

                processed_filenames.emplace_back(filename);
            }
        }
    }

    return true;
}

void CPifDlg::SaveAssociations()
{
    for( long lRowIndex = 0 ; lRowIndex < m_pifgrid.GetNumberRows() ; lRowIndex++ )
    {
        CUGCell cellGrid;
        m_pifgrid.GetCell(-1, lRowIndex, &cellGrid);

        PIFINFO** ppPifInfo = (PIFINFO**)cellGrid.GetExtraMemPtr();

        if( ppPifInfo != nullptr )
        {
            PIFINFO* pifInfo = *ppPifInfo;

            // data files
            if( !pifInfo->dictionary_filename.IsEmpty() )
            {
                if( lRowIndex == 0 )
                    m_pPifFile->ClearAndAddInputDataConnectionStrings(pifInfo->connection_strings);

                else
                {
                    if( pifInfo->sUName.CompareNoCase(OUTPFILE) == 0 )
                    {
                        if( pifInfo->connection_strings.empty() )
                            m_pPifFile->SetSingleOutputDataConnectionString(ConnectionString::CreateNullRepositoryConnectionString());

                        else
                            m_pPifFile->ClearAndAddOutputDataConnectionStrings(pifInfo->connection_strings);
                    }

                    else
                    {
                        ASSERT(pifInfo->connection_strings.size() == 1);

                        if( pifInfo->sUName.CompareNoCase(IMPUTESTATFILE) == 0 )
                            m_pPifFile->SetImputeStatConnectionString(pifInfo->connection_strings.front());

                        else
                            m_pPifFile->SetExternalDataConnectionString(pifInfo->sUName, pifInfo->connection_strings.front());
                    }
                }
            }

            else if( pifInfo->eType == PIFUSRFILE )
                m_pPifFile->SetUsrDatAssoc(pifInfo->sUName, pifInfo->sFileName);

            else if( pifInfo->eType == FILE_NONE )
            {
                if( pifInfo->sUName.CompareNoCase(WRITEFILE) == 0 )
                    m_pPifFile->SetWriteFName(pifInfo->sFileName);

                else if( pifInfo->sUName.CompareNoCase(FREQFILE) == 0 )
                    m_pPifFile->SetFrequenciesFilename(pifInfo->sFileName);

                else if( pifInfo->sUName.CompareNoCase(IMPUTEFILE) == 0 )
                    m_pPifFile->SetImputeFrequenciesFilename(pifInfo->sFileName);

                else if( pifInfo->sUName.CompareNoCase(SAVEARRAYFILE) == 0 )
                    m_pPifFile->SetSaveArrayFilename(pifInfo->sFileName);

                else if( pifInfo->sUName.CompareNoCase(LISTFILE) == 0 )
                    m_pPifFile->SetListingFName(pifInfo->sFileName);

                else if( pifInfo->sUName.CompareNoCase(PARADATAFILE) == 0 )
                    m_pPifFile->SetParadataFilename(pifInfo->sFileName);

                else if( pifInfo->sUName.CompareNoCase(INPUTTBD) == 0 )
                {
                    m_pPifFile->ClearConInputFilenames();

                    if( m_pifgrid.m_arrMultFiles.IsEmpty() )
                        m_pPifFile->AddConInputFilenames(pifInfo->sFileName);

                    else
                    {
                        for( int i = 0; i < m_pifgrid.m_arrMultFiles.GetSize(); i++ )
                            m_pPifFile->AddConInputFilenames(m_pifgrid.m_arrMultFiles[i]);
                    }
                }
            }
        }
    }
}


BOOL CPifDlg::PreTranslateMessage(MSG* pMsg) // GHM 20110805
{
    if( pMsg->message == WM_KEYDOWN )
    {
        bool bCtrl = GetKeyState(VK_CONTROL) < 0;

        if( bCtrl )
        {
            // GHM 20110805 shortcuts so that the user doesn't have to click on the ellipses
            if( pMsg->wParam >= _T('1') && pMsg->wParam <= _T('9') )
            {
                int rowNum = pMsg->wParam - _T('1');

                if( rowNum < m_pifgrid.GetNumberRows() )
                {
                    GetDlgItem(IDOK)->SetFocus(); // there was a problem if the grid cell to be changed had focus
                    m_pifgrid.ForceButtonClick(rowNum);
                    m_pifgrid.RedrawCell(0,rowNum);
                    return TRUE;
                }
            }
            else if (pMsg->wParam == VK_SPACE) {
                // Default input data filename based on app name
                GetDlgItem(IDOK)->SetFocus(); // there was a problem if the grid cell to be changed had focus
                CString app_name = m_pPifFile->GetAppFName();
                CString data_name = app_name.Left(app_name.ReverseFind(_T('.'))) + FileExtensions::Data::WithDot::CSProDB;
                m_pifgrid.SetDefaultInputDataFilename(data_name);
                return TRUE;
            }
        }
    }

    return CDialog::PreTranslateMessage(pMsg);
}


bool CPifDlg::DictionaryChangesIfAnyAreOk(const ConnectionString& connection_string, const CString& dictionary_filename)
{
    // Don't compare if the data file can't have an embedded dictionary
    if( !DataRepositoryHelpers::DoesTypeContainEmbeddedDictionary(connection_string.GetType()) )
        return true;

    CDataDict app_dictionary;

    // Don't compare if the dictionary doesn't exist (such as when running from a .pen file),
    // or when the dictionary cannot be opened
    if( !PortableFunctions::FileIsRegular(dictionary_filename) )
        return true;

    try
    {
        app_dictionary.Open(dictionary_filename, true);
    }

    catch(...)
    {
        return true;
    }

    std::unique_ptr<const CDataDict> embedded_dictionary = DataRepositoryHelpers::GetEmbeddedDictionary(connection_string);
    if( embedded_dictionary == nullptr )
        return true;

    // Don't compare if the structure of the dictionary hasn't changed
    // (an efficiency would be to first query the dictionary structure from
    // the data file rather than get the entire dictionary)
    if( embedded_dictionary->GetStructureMd5() == app_dictionary.GetStructureMd5() )
        return true;

    // Compare the dictionaries
    DictionaryComparer comparer(*embedded_dictionary, app_dictionary);
    std::vector<DictionaryDifference> differences = comparer.GetDataRepositorySpecificDifferences(connection_string.GetType());

    if( differences.empty() )
        return true;

    DictReconcileDlg dlg(CS2WS(app_dictionary.GetName()), connection_string, std::move(differences));
    return ( dlg.DoModal() == IDOK );
}
