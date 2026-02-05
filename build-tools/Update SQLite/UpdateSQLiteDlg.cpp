#include "StdAfx.h"
#include "UpdateSQLiteDlg.h"
#include "resource.h"
#include "SQLiteSourceUpdater.h"
#include <zToolsO/FileIO.h>
#include <zToolsO/Tools.h>
#include <zUtilO/DataExchange.h>
#include <zUtilO/Viewers.h>
#include <zUtilO/WindowHelpers.h>
#include <zUtilO/WindowsUtf8.h>


namespace
{
    constexpr std::string_view SqliteSEELoginUrl_sv = "https://www.sqlite.org/see/login";
}


BEGIN_MESSAGE_MAP(UpdateSQLiteDlg, ResizableDlg)
    ON_NOTIFY(NM_CLICK, IDC_SEE_WEBSITE, OnSeeWebsiteClick)
    ON_NOTIFY(NM_RETURN, IDC_SEE_WEBSITE, OnSeeWebsiteClick)
END_MESSAGE_MAP()


UpdateSQLiteDlg::UpdateSQLiteDlg(CWnd* const pParent/* = nullptr*/)
    :   ResizableDlg(IDD_UPDATE_SEE, pParent),
        m_seeEncryptionVariant("see.c")
{
    SerializeDialogSize("UpdateSQLiteDlg");
}



void UpdateSQLiteDlg::DoDataExchange(CDataExchange* const pDX)
{
    __super::DoDataExchange(pDX);

    DDX_Text(pDX, IDC_SEE_DIRECTORY, m_seeDirectory, true);
    DDX_Text(pDX, IDC_SEE_ENCRYPTION_VARIANT, m_seeEncryptionVariant, true);
}


BOOL UpdateSQLiteDlg::OnInitDialog()
{
    __super::OnInitDialog();

    WindowHelpers::RemoveDialogSystemIcon(*this);

    WindowsUtf8::SetText(m_hWnd, IDC_SEE_WEBSITE, SO::Concatenate("<a>", SqliteSEELoginUrl_sv, "</a>"));

    return TRUE;
}


void UpdateSQLiteDlg::OnSeeWebsiteClick(NMHDR* const /*pNMHDR*/, LRESULT* const pResult)
{
    Viewer().ViewHtmlUrl(std::string(SqliteSEELoginUrl_sv));

    *pResult = 0;
}


void UpdateSQLiteDlg::OnOK()
{
    UpdateData(TRUE);

    try
    {
        if( SO::IsBlank(m_seeDirectory) || SO::IsBlank(m_seeEncryptionVariant) )
            throw CSProException("Specify the SEE directory and encryption variant.");

        // read the sources
        std::string sqlite_h = FileIO::ReadText(Path::Combine(m_seeDirectory, "sqlite3.h"));
        std::string sqlite_c = FileIO::ReadText(Path::Combine(m_seeDirectory, "sqlite3-" + m_seeEncryptionVariant));

        // prepare them for CSPro
        SQLiteSourceUpdater::Update(sqlite_h, true, SQLiteSourceUpdater::Version::SEE);
        SQLiteSourceUpdater::Update(sqlite_c, false, SQLiteSourceUpdater::Version::SEE);

        // write them to the external sources directory
        const std::string sqlite_directory = MakeFullPath(PortableFunctions::PathGetDirectory(__FILE__),
                                                          "..\\..\\cspro\\external\\SQLite");

        FileIO::WriteText(Path::Combine(sqlite_directory, "sqlite3.h"), sqlite_h, true);
        FileIO::WriteText(Path::Combine(sqlite_directory, "sqlite3.c"), sqlite_c, true);

        AfxMessageBox(L"Successfully updated SQLite with the SQLite Encryption Extension (SEE).");

        __super::OnOK();
    }

    catch( const CSProException& exception )
    {
        ErrorMessage::Display(exception);
    }
}
