#include "StdAfx.h"
#include "VersionShifterDlg.h"
#include <zToolsO/DirectoryLister.h>
#include <zToolsO/WinRegistry.h>


BEGIN_MESSAGE_MAP(VersionShifterDlg, CDialog)
    ON_BN_CLICKED(IDOK, OnBnClickedOk)
END_MESSAGE_MAP()


VersionShifterDlg::VersionShifterDlg(CWnd* pParent/* = nullptr*/)
    :   CDialog(VersionShifterDlg::IDD, pParent)
{
    // search in the (32-bit) Program Files folder for CSPro installations
    m_versionPaths = DirectoryLister().SetRecursive(false)
                                      .SetIncludeFiles(false)
                                      .SetIncludeDirectories(true)
                                      .SetNameFilter(_T("CSPro*"))
                                      .GetPaths(GetWindowsSpecialFolder(WindowsSpecialFolder::ProgramFiles32));

    // see what version is currently being used
    WinRegistry registry;
    CString current_exe;

    if( registry.Open(HKEY_LOCAL_MACHINE,_T("Software\\Microsoft\\Windows\\CurrentVersion\\App Paths\\CSPro.exe")) )
        registry.ReadString(nullptr, &current_exe);

    // make sure that each folder has CSPro.exe and get the index of the current selection
    for( auto version_path_itr = m_versionPaths.begin(); version_path_itr != m_versionPaths.end(); )
    {
        if( !PortableFunctions::FileIsRegular(PortableFunctions::PathAppendToPath(*version_path_itr, _T("CSPro.exe"))) )
        {
            version_path_itr = m_versionPaths.erase(version_path_itr);
        }

        else
        {
            if( SO::StartsWithNoCase(current_exe, *version_path_itr) )
                m_currentIndex = std::distance(m_versionPaths.begin(), version_path_itr);

            ++version_path_itr;
        }
    }
}


BOOL VersionShifterDlg::OnInitDialog()
{
    CDialog::OnInitDialog();

    CComboBox* pCombo = (CComboBox*)GetDlgItem(IDC_ELIGIBLE_VERSIONS);

    for( const std::wstring& version_path : m_versionPaths )
    {
        // add just the folder name
        pCombo->AddString(PortableFunctions::PathGetFilename(PortableFunctions::PathRemoveTrailingSlash(version_path)));
    }

    // if no version was found in the registry, select the last item in the list
    if( !m_currentIndex.has_value() && !m_versionPaths.empty() )
        m_currentIndex = m_versionPaths.size() - 1;

    if( m_currentIndex.has_value() )
        pCombo->SetCurSel((int)*m_currentIndex);

    return TRUE;
}


bool SetRegistryStringValue(HKEY hKey, TCHAR* value_name, NullTerminatedString value)
{
    return RegSetValueEx(hKey, value_name, 0, REG_SZ, (BYTE*)value.c_str(), sizeof(TCHAR) * ( value.length() + 1 )) == ERROR_SUCCESS;
}

void SetRegistryFileAssociation(CString csDocumentName, bool* pbSuccess, const std::wstring* pcsOpenExe, int iIconIndex,
                                const std::wstring* pcsRunExe = NULL, const std::wstring* pcsPackExe = NULL, const std::wstring* pcsPublishExe = NULL,
                                const std::wstring* pcsEditExe = NULL, const std::wstring* pcsIconExe = NULL)
{
    if( !*pbSuccess )
        return;

    csDocumentName.Format(_T("CSPro%s.Document"), (LPCTSTR)csDocumentName);

    HKEY hKey;
    CString csKeyName;

    // the icon
    csKeyName.Format(_T("%s\\DefaultIcon"), (LPCTSTR)csDocumentName);
    *pbSuccess = RegOpenKey(HKEY_CLASSES_ROOT,csKeyName,&hKey) == ERROR_SUCCESS;

    if( *pbSuccess )
    {
        CString csIconName = FormatText(_T("%s,%d"), pcsIconExe != NULL ? pcsIconExe->c_str() : pcsOpenExe->c_str(), iIconIndex);
        *pbSuccess = SetRegistryStringValue(hKey,NULL,csIconName);

        RegCloseKey(hKey);
    }

    if( !*pbSuccess )
        return;

    // the open, run, and pack commands
    const TCHAR* aCommands[] = { _T("Open"), _T("Run"), _T("Pack"), _T("Publish"), _T("Edit") };

    for( int i = 0; i < 5; i++ )
    {
        if( ( i == 1 && pcsRunExe == NULL ) || ( i == 2 && pcsPackExe == NULL ) || ( i == 3 && pcsPublishExe == NULL ) || ( i == 4 && pcsEditExe == NULL ) )
            continue;

        csKeyName.Format(_T("%s\\shell\\%s\\command"), (LPCTSTR)csDocumentName, (LPCTSTR)aCommands[i]);
        *pbSuccess = RegOpenKey(HKEY_CLASSES_ROOT,csKeyName,&hKey) == ERROR_SUCCESS;

        if( *pbSuccess )
        {
            CString csCommand;

            if( i != 3 )
            {
                csCommand.Format(_T("\"%s\" \"%%1\""), i == 0 ? pcsOpenExe->c_str() : i == 1 ? pcsRunExe->c_str() : i == 2 ? pcsPackExe->c_str() : pcsEditExe->c_str());
            }

            else // publish
            {
                csCommand.Format(_T("\"%s\" /pen \"%%1\""), pcsPublishExe->c_str());
            }

            *pbSuccess = SetRegistryStringValue(hKey,NULL,csCommand);

            RegCloseKey(hKey);
        }

        if( !*pbSuccess )
            return;
    }
}

void VersionShifterDlg::OnBnClickedOk()
{
    CComboBox* pCombo = (CComboBox*)GetDlgItem(IDC_ELIGIBLE_VERSIONS);

    int iSelection = pCombo->GetCurSel();
    ASSERT(iSelection >= 0);

    const CSProExecutables::Program aeExecutables[] =
    {
        CSProExecutables::Program::CSPro,
        CSProExecutables::Program::CSEntry,
        CSProExecutables::Program::CSBatch,
        CSProExecutables::Program::CSTab,
        CSProExecutables::Program::TblView,
        CSProExecutables::Program::CSSort,
        CSProExecutables::Program::CSFreq,
        CSProExecutables::Program::CSDiff,
        CSProExecutables::Program::CSExport,
        CSProExecutables::Program::RunPff,
        CSProExecutables::Program::CSPack,
        CSProExecutables::Program::ProductionRunner,
        CSProExecutables::Program::SaveArrayViewer,
        CSProExecutables::Program::PffEditor,
        CSProExecutables::Program::DataViewer,
        CSProExecutables::Program::Excel2CSPro,
        CSProExecutables::Program::ParadataViewer,
        CSProExecutables::Program::CSDeploy
    };

    const size_t ExecutablesRequired = 11;

    enum { cspro, csentry, csbatch, cstab, tblview, cssort, csfreq, csdiff, csexport, runpff,
           cspack, productionRunner, saveArrayViewer, pffEditor, dataViewer, excel2CSPro,
           paradataViewer, csdeploy };

    std::vector<std::wstring> executables;

    // make sure that all of the required executables exist
    for( size_t i = 0; i < _countof(aeExecutables); ++i )
    {
        std::wstring exe_filename = PortableFunctions::PathAppendToPath(m_versionPaths[iSelection], CSProExecutables::GetExecutableName(aeExecutables[i]));

        if( i < ExecutablesRequired && !PortableFunctions::FileIsRegular(exe_filename) )
        {
            AfxMessageBox(FormatText(_T("It was not possible to change the version because the file %s was missing."),
                                     (LPCTSTR)CSProExecutables::GetExecutableName(aeExecutables[i])));
            return;
        }

        executables.emplace_back(std::move(exe_filename));
    }

    bool bSuccess = true;

    HKEY hKey;
    bSuccess = RegOpenKey(HKEY_LOCAL_MACHINE,_T("Software\\Microsoft\\Windows\\CurrentVersion\\App Paths\\CSPro.exe"),&hKey) == ERROR_SUCCESS;

    if( bSuccess )
    {
        bSuccess = SetRegistryStringValue(hKey, _T("Path"), m_versionPaths[iSelection]);

        if( bSuccess )
            bSuccess = SetRegistryStringValue(hKey, NULL, executables[cspro]);

        RegCloseKey(hKey);

        SetRegistryFileAssociation(_T("DataEntryApp"),&bSuccess,&executables[cspro],0,&executables[csentry],&executables[cspack],&executables[csentry],NULL,&executables[csentry]); // .ent
        SetRegistryFileAssociation(_T("CompiledDataEntryApp"),&bSuccess,&executables[csentry],20,&executables[csentry],NULL,NULL,NULL,&executables[cspro]); // .pen
        SetRegistryFileAssociation(_T("BatchEditApp"),&bSuccess,&executables[cspro],0,&executables[csbatch],&executables[cspack],NULL,NULL,&executables[csbatch]); // .bch
        SetRegistryFileAssociation(_T("CrossTabApp"),&bSuccess,&executables[cspro],0,NULL,&executables[cspack],NULL,NULL,&executables[cstab]); // .xtb
        SetRegistryFileAssociation(_T("DataDict"),&bSuccess,&executables[cspro],8); // .dcf
        SetRegistryFileAssociation(_T("Form"),&bSuccess,&executables[cspro],9); // .fmf
        SetRegistryFileAssociation(_T("TblView"),&bSuccess,&executables[tblview],0); // .tbw
        SetRegistryFileAssociation(_T("Sort"),&bSuccess,&executables[cssort],0); // .ssf
        SetRegistryFileAssociation(_T("Freq"),&bSuccess,&executables[csfreq],0); // .fqf
        SetRegistryFileAssociation(_T("Cmp"),&bSuccess,&executables[csdiff],0); // .cmp
        SetRegistryFileAssociation(_T("Expt"),&bSuccess,&executables[csexport],0); // .exf

        if( PortableFunctions::FileIsRegular(executables[pffEditor]) )
        {
            SetRegistryFileAssociation(_T("Pff"),&bSuccess,&executables[runpff],0,NULL,NULL,NULL,&executables[pffEditor]); // .pff
        }

        else
        {
            SetRegistryFileAssociation(_T("Pff"),&bSuccess,&executables[runpff],0); // .pff
        }

        if( PortableFunctions::FileIsRegular(executables[productionRunner]) )
            SetRegistryFileAssociation(_T("ProductionRunner"),&bSuccess,&executables[productionRunner],0); // .pffRunner

        if( PortableFunctions::FileIsRegular(executables[saveArrayViewer]) )
            SetRegistryFileAssociation(_T("SaveArray"),&bSuccess,&executables[saveArrayViewer],0); // .sva

        if( PortableFunctions::FileIsRegular(executables[dataViewer]) )
        {
            SetRegistryFileAssociation(_T("DataFile"),&bSuccess,&executables[dataViewer],0); // .csdb
            SetRegistryFileAssociation(_T("EncryptedDataFile"),&bSuccess,&executables[dataViewer],0); // .csdbe
        }

        if( PortableFunctions::FileIsRegular(executables[excel2CSPro]) )
            SetRegistryFileAssociation(_T("Excel2CSPro"),&bSuccess,&executables[excel2CSPro],0); // .xl2cs

        if( PortableFunctions::FileIsRegular(executables[paradataViewer]) )
            SetRegistryFileAssociation(_T("Paradata"),&bSuccess,&executables[paradataViewer],0); // .cslog

        if( PortableFunctions::FileIsRegular(executables[csdeploy]) )
            SetRegistryFileAssociation(_T("Deploy"),&bSuccess,&executables[csdeploy],0); // .csds
    }

    CString csMessage;
    pCombo->GetWindowText(csMessage);
    csMessage.Format(_T("The version was %ssuccessfully set to %s.%s"),bSuccess ? _T("") : _T("un"), (LPCTSTR)csMessage, bSuccess ? _T("") : _T(" You may need to run CSPro as an administrator for this to work."));
    AfxMessageBox(csMessage);

    CDialog::OnOK();
}
