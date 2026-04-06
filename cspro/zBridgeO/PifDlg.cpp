#include "StdAfx.h"
#include "PifDlg.h"
#include <zToolsO/FileIO.h>
#include <zToolsO/Tools.h>
#include <zInterfaceF/DictionaryReconcileDlg.h>


/////////////////////////////////////////////////////////////////////////////
// CPifDlg dialog

CPifDlg::CPifDlg(const CArray<PIFINFO*, PIFINFO*>& pffInfo, CString title, CWnd* pParent /*=NULL*/)
    :   CDialog(IDD_PIFDLG, pParent),
        m_pPifFile(nullptr)
{
    m_title = title;
    m_arrPifInfo.Copy(pffInfo);
}

CPifDlg::CPifDlg(const std::vector<std::shared_ptr<PIFINFO>>& pffInfo, CString title, CWnd* pParent /*=NULL*/)
    :   CDialog(IDD_PIFDLG, pParent),
        m_pPifFile(nullptr)
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


bool CPifDlg::IsValidFilePath(std::set<std::string>& nonexistent_directories, const std::wstring& path,
                              const bool must_be_writeable)
{
    if( PortableFunctions::FileExists(path) )
    {
        // if the file exists, potentially check that it is writeable
        if( must_be_writeable )
        {
            const DWORD attr = GetFileAttributes(path.c_str());

            if( ( attr & FILE_ATTRIBUTE_READONLY ) != 0 )
                return false;
        }
    }

    else
    {
        // check that the directory exists
        std::wstring directory = PortableFunctions::PathGetDirectory(path);

        if( !PortableFunctions::FileIsDirectory(directory) )
            nonexistent_directories.insert(UTF8_TODO::GetUtf8(std::move(directory)));
    }

    // Check that name doesn't contain invalid characters
    return ( path.find_first_of(L"\"<>|*?") == wstring_view::npos );
}


bool CPifDlg::Validate()
{
    while( true )
    {
        std::vector<CString> processed_filenames;
        std::set<std::string> nonexistent_directories;

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
                if( !pifInfo->dictionary_file_path.empty() )
                {
                    if( pifInfo->connection_strings.size() == 0 && pifInfo->sUName.CompareNoCase(OUTPFILE) != 0 )
                    {
                        AfxMessageBox(L"You must specify a data source for " + pifInfo->sDisplay);
                        return false;
                    }

                    if( pifInfo->connection_strings.size() > 1 && ( pifInfo->uOptions & PIF_MULTIPLE_FILES ) == 0 )
                    {
                        // see comments in PifInfoPopulator::GetPifInfo for why the output data doesn't have this flag set
                        if( pifInfo->sUName.CompareNoCase(OUTPFILE) != 0 )
                        {
                            AfxMessageBox(L"You cannot specify multiple data sources for " + pifInfo->sDisplay);
                            return false;
                        }
                    }

                    for( const ConnectionString& connection_string : pifInfo->connection_strings )
                    {
                        if( connection_string.HasFilePath() )
                        {
                            if( Path::HasWildcardCharacters(connection_string.GetFilePath()) )
                            {
                                if( ( pifInfo->uOptions & PIF_MULTIPLE_FILES ) == 0 )
                                {
                                    AfxMessageBox(L"File names may not contain wildcards (* or ?).");
                                    return false;
                                }

                                std::string directory = PortableFunctions::PathGetDirectory(connection_string.GetFilePath());

                                if( !PortableFunctions::FileIsDirectory(directory) )
                                    nonexistent_directories.insert(std::move(directory));
                            }

                            else
                            {
                                if( ( pifInfo->uOptions & PIF_FILE_MUST_EXIST ) != 0 &&
                                    !PortableFunctions::FileIsRegular(connection_string.GetFilePath()) )
                                {
                                    AfxMessageBox(FormatText("File %s not found", connection_string.GetFilePath().c_str()));
                                    return false;
                                }

                                if( !IsValidFilePath(nonexistent_directories,
                                                     UTF8_TODO::GetWide(connection_string.GetFilePath()),
                                                     !( pifInfo->uOptions & PIF_READ_ONLY )) )
                                {
                                    AfxMessageBox(FormatText("%s is not a valid file name. "
                                                             "Check that the name does not contain invalid characters.",
                                                             connection_string.GetFilePath().c_str()));
                                    return false;
                                }

                                if( pifInfo->sUName.CompareNoCase(OUTPFILE) != 0 &&
                                    !DictionaryReconcileDlg::DictionaryChangesIfAnyAreOk(connection_string, pifInfo->dictionary_file_path) )
                                {
                                    return false;
                                }

                                filenames_to_add.emplace_back(UTF8_TODO::GetCString(connection_string.GetFilePath()));
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
                    {
                        filenames_to_process.emplace_back(pifInfo->sFileName);
                    }

                    for( const CString& filename : filenames_to_process )
                    {
                        filenames_to_add.emplace_back(filename);

                        if( filename.IsEmpty() && ( pifInfo->uOptions & PIF_ALLOW_BLANK ) == 0 )
                        {
                            AfxMessageBox(FormatText(L"File Associations incomplete.\nFile name missing for %s.", pifInfo->sDisplay.GetString()));
                            return false;
                        }

                        if( filename.FindOneOf(L"*?") >= 0 && ( pifInfo->uOptions & PIF_ALLOW_WILDCARDS ) == 0 )
                        {
                            AfxMessageBox(L"File names may not contain wildcards (* or ?).");
                            return false;
                        }

                        // Check valid path except if name is empty or is multiple files (containing ")
                        if( !filename.IsEmpty() && filename.Find('"') < 0 &&
                            !IsValidFilePath(nonexistent_directories,
                                             CS2WS(filename),
                                             !( pifInfo->uOptions & PIF_READ_ONLY )) )
                        {
                            AfxMessageBox(FormatText(L"%s is not a valid file name. "
                                                     L"Check that the name does not contain invalid characters.",
                                                     filename.GetString()));
                            return false;
                        }

                        if( !filename.IsEmpty() && ( pifInfo->uOptions & PIF_FILE_MUST_EXIST ) != 0 && !PortableFunctions::FileExists(filename) )
                        {
                            AfxMessageBox(FormatText(L"File %s not found", filename.GetString()));
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
                        AfxMessageBox(FormatText(L"You cannot use the file name %s more than once.", filename.GetString()));
                        return false;
                    }

                    processed_filenames.emplace_back(filename);
                }
            }
        }

        // if there are any nonexistent directories, check if the user wants to create them
        if( !nonexistent_directories.empty() &&
            !QueryAndCreateNonexistentDirectories(nonexistent_directories) )
        {
            return false;
        }

        return true;
    }
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
            if( !pifInfo->dictionary_file_path.empty() )
            {
                if( lRowIndex == 0 )
                {
                    m_pPifFile->ClearAndAddInputDataConnectionStrings(pifInfo->connection_strings);
                }

                else
                {
                    if( pifInfo->sUName.CompareNoCase(OUTPFILE) == 0 )
                    {
                        if( pifInfo->connection_strings.empty() )
                        {
                            m_pPifFile->SetSingleOutputDataConnectionString(ConnectionString::CreateNullRepositoryConnectionString());
                        }

                        else
                        {
                            m_pPifFile->ClearAndAddOutputDataConnectionStrings(pifInfo->connection_strings);
                        }
                    }

                    else
                    {
                        ASSERT(pifInfo->connection_strings.size() == 1);

                        if( pifInfo->sUName.CompareNoCase(IMPUTESTATFILE) == 0 )
                        {
                            m_pPifFile->SetImputeStatConnectionString(pifInfo->connection_strings.front());
                        }

                        else
                        {
                            m_pPifFile->SetExternalDataConnectionString(pifInfo->sUName, pifInfo->connection_strings.front());
                        }
                    }
                }
            }

            else if( pifInfo->eType == PIFUSRFILE )
            {
                m_pPifFile->SetUsrDatAssoc(pifInfo->sUName, pifInfo->sFileName);
            }

            else if( pifInfo->eType == FILE_NONE )
            {
                if( pifInfo->sUName.CompareNoCase(WRITEFILE) == 0 )
                {
                    m_pPifFile->SetWriteFName(pifInfo->sFileName);
                }

                else if( pifInfo->sUName.CompareNoCase(FREQFILE) == 0 )
                {
                    m_pPifFile->SetFrequenciesFilename(pifInfo->sFileName);
                }

                else if( pifInfo->sUName.CompareNoCase(IMPUTEFILE) == 0 )
                {
                    m_pPifFile->SetImputeFrequenciesFilename(pifInfo->sFileName);
                }

                else if( pifInfo->sUName.CompareNoCase(SAVEARRAYFILE) == 0 )
                {
                    m_pPifFile->SetSaveArrayFilename(pifInfo->sFileName);
                }

                else if( pifInfo->sUName.CompareNoCase(LISTFILE) == 0 )
                {
                    m_pPifFile->SetListingFName(pifInfo->sFileName);
                }

                else if( pifInfo->sUName.CompareNoCase(PARADATAFILE) == 0 )
                {
                    m_pPifFile->SetParadataFilename(pifInfo->sFileName);
                }

                else if( pifInfo->sUName.CompareNoCase(INPUTTBD) == 0 )
                {
                    m_pPifFile->ClearConInputFilenames();

                    if( m_pifgrid.m_arrMultFiles.IsEmpty() )
                    {
                        m_pPifFile->AddConInputFilenames(pifInfo->sFileName);
                    }

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


BOOL CPifDlg::PreTranslateMessage(MSG* pMsg) // 20110805
{
    if( pMsg->message == WM_KEYDOWN )
    {
        bool bCtrl = GetKeyState(VK_CONTROL) < 0;

        if( bCtrl )
        {
            // 20110805 shortcuts so that the user doesn't have to click on the ellipses
            if( pMsg->wParam >= '1' && pMsg->wParam <= '9' )
            {
                int rowNum = pMsg->wParam - '1';

                if( rowNum < m_pifgrid.GetNumberRows() )
                {
                    GetDlgItem(IDOK)->SetFocus(); // there was a problem if the grid cell to be changed had focus
                    m_pifgrid.ForceButtonClick(rowNum);
                    m_pifgrid.RedrawCell(0,rowNum);
                    return TRUE;
                }
            }

            else if (pMsg->wParam == VK_SPACE)
            {
                // Default input data filename based on app name
                GetDlgItem(IDOK)->SetFocus(); // there was a problem if the grid cell to be changed had focus
                CString app_name = m_pPifFile->GetAppFName();
                CString data_name = UTF8_TODO::GetCString(PortableFunctions::PathReplaceFileExtension(UTF8_TODO::GetUtf8(app_name), FileExtensions::Data::CSProDB));
                m_pifgrid.SetDefaultInputDataFilename(data_name);
                return TRUE;
            }
        }
    }

    return CDialog::PreTranslateMessage(pMsg);
}


bool CPifDlg::QueryAndCreateNonexistentDirectories(const std::set<std::string>& nonexistent_directories)
{
    ASSERT(!nonexistent_directories.empty());

    const std::string prompt = FormatText(
        "The following %s not exist:\n\n%s\n\nWould you like to create %s?",
        PluralizeWord(nonexistent_directories.size(), "directory does", "directories do"),
        SO::CreateSingleString(nonexistent_directories, SO::Newline_lf_sv).c_str(),
        PluralizeWord(nonexistent_directories.size(), "it", "them")
    );

    if( AfxMessageBox(prompt, MB_YESNOCANCEL) != IDYES )
        return false;

    try
    {
        for( const std::string& nonexistent_directory : nonexistent_directories )
            FileIO::CreateDirectories(nonexistent_directory);
    }

    catch( const CSProException& exception)
    {
        ErrorMessage::Display(exception);
        return false;
    }

    return true;
}
