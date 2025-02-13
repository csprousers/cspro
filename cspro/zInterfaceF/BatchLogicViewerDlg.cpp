#include "StdAfx.h"
#include "BatchLogicViewerDlg.h"
#include <zToolsO/FileIO.h>
#include <zToolsO/PortableFunctions.h>
#include <zUtilO/CSProExecutables.h>
#include <zUtilO/Filedlg.h>
#include <zUtilO/Interapp.h>
#include <zAppO/Application.h>
#include <zFormO/FormFile.h>
#include <zEdit2O/LogicCtrl.h>


IMPLEMENT_DYNAMIC(BatchLogicViewerDlg, CDialog)

BEGIN_MESSAGE_MAP(BatchLogicViewerDlg, CDialog)
    ON_BN_CLICKED(IDC_COPY_TO_CLIPBOARD, OnCopyToClipboard)
    ON_BN_CLICKED(IDC_CREATE_BATCH_APPLICATION, OnCreateBatchApplication)
END_MESSAGE_MAP()


BatchLogicViewerDlg::BatchLogicViewerDlg(const CDataDict& dictionary, const LogicSettings& logic_settings,
                                         std::wstring logic_text, CWnd* pParent/* = nullptr*/)
    :   CDialog(IDD_BATCH_LOGIC_VIEWER, pParent),
        m_dictionary(dictionary),
        m_logicSettings(logic_settings),
        m_logicText(std::move(logic_text)),
        m_logicCtrl(std::make_unique<CLogicCtrl>())
{
    ASSERT(PortableFunctions::FileIsRegular(m_dictionary.GetFullFileName()));
}


BatchLogicViewerDlg::~BatchLogicViewerDlg()
{
}


void BatchLogicViewerDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);

    DDX_Control(pDX, IDC_BATCH_LOGIC, *m_logicCtrl);
}


BOOL BatchLogicViewerDlg::OnInitDialog()
{
    CDialog::OnInitDialog();

    m_logicCtrl->ReplaceCEdit(this, false, false);
    m_logicCtrl->SetText(m_logicText);
    m_logicCtrl->SetReadOnly(TRUE);

    // disable the Create Batch Application option if the dictionary is in the temp directory (which means
    // this is being run from a tool that opened a CSPro DB file directly instead of using the dictionary)
    if( SO::StartsWithNoCase(m_dictionary.GetFullFileName(), GetTempDirectory()) )
        GetDlgItem(IDC_CREATE_BATCH_APPLICATION)->EnableWindow(FALSE);

    return TRUE;
}


void BatchLogicViewerDlg::OnCopyToClipboard()
{
    m_logicCtrl->CopyAllText();
}


void BatchLogicViewerDlg::OnCreateBatchApplication()
{
    CIMSAFileDialog file_dlg(FALSE, FileExtensions::BatchApplication, nullptr, OFN_HIDEREADONLY | OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT,
                             _T("Batch Application Files (*.bch)|*.bch||"));

    file_dlg.m_ofn.lpstrTitle = _T("New Batch Application Name");

    if( file_dlg.DoModal() != IDOK )
        return;

    try
    {
        // create the batch application and associated files
        const std::wstring application_filename = file_dlg.GetPathName();

        Application batch_application;
        batch_application.SetEngineAppType(EngineAppType::Batch);
        batch_application.SetLogicSettings(m_logicSettings);

        // set the name / label
        {
            std::wstring application_label = PortableFunctions::PathGetFilenameWithoutExtension(application_filename);
            batch_application.SetName(CIMSAString::MakeName(application_label));
            batch_application.SetLabel(std::move(application_label));
        }


        // create and save the order file
        {
            std::wstring order_filename = PortableFunctions::PathRemoveFileExtension(application_filename) + FileExtensions::WithDot::Order;
            CDEFormFile order(WS2CS(order_filename), m_dictionary.GetFullFileName());

            order.CreateOrderFile(m_dictionary, true);

            if( !order.Save(WS2CS(order_filename)) )
                return;

            batch_application.AddFormFilename(std::move(order_filename));
        }


        // save the logic
        {
            std::wstring logic_filename = application_filename + FileExtensions::WithDot::Logic;
            FileIO::WriteText(logic_filename, m_logicText, true);

            batch_application.AddCodeFile(CodeFile(CodeType::LogicMain, std::make_unique<TextSource>(std::move(logic_filename))));
        }


        // save the application and see if the user wants to open it
        batch_application.Save(application_filename);

        if( AfxMessageBox(FormatText(_T("Would you like to open \"%s\" now?"),
                          PortableFunctions::PathGetFilename(application_filename)), MB_YESNO) == IDYES )
        {
            CSProExecutables::RunProgramOpeningFile(CSProExecutables::Program::CSPro, application_filename);
            OnCancel();
        }
    }

    catch( const CSProException& exception )
    {
        ErrorMessage::Display(exception);
    }
}
