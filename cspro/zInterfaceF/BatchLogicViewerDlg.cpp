#include "StdAfx.h"
#include "BatchLogicViewerDlg.h"
#include <zToolsO/FileIO.h>
#include <zUtilO/CSProExecutables.h>
#include <zUtilO/FileDlg.h>
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
                                         std::string logic_text, CWnd* const pParent/* = nullptr*/)
    :   CDialog(IDD_BATCH_LOGIC_VIEWER, pParent),
        m_dictionary(dictionary),
        m_logicSettings(logic_settings),
        m_logicText(std::move(logic_text)),
        m_logicCtrl(std::make_unique<CLogicCtrl>())
{
    ASSERT(PortableFunctions::FileIsRegular(m_dictionary.GetFilePath()));
}


BatchLogicViewerDlg::~BatchLogicViewerDlg()
{
}


void BatchLogicViewerDlg::DoDataExchange(CDataExchange* const pDX)
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
    if( SO::StartsWithNoCase(m_dictionary.GetFilePath(), GetTempDirectory()) )
        GetDlgItem(IDC_CREATE_BATCH_APPLICATION)->EnableWindow(FALSE);

    return TRUE;
}


void BatchLogicViewerDlg::OnCopyToClipboard()
{
    m_logicCtrl->CopyAllText();
}


void BatchLogicViewerDlg::OnCreateBatchApplication()
{
    SaveFileDlg save_file_dlg(0, FileExtensions::BatchApplication, nullptr, L"Batch Application Files (*.bch)|*.bch||", this);
    save_file_dlg.SetTitle(L"New Batch Application Name");

    if( save_file_dlg.DoModal() != IDOK )
        return;

    try
    {
        // create the batch application and associated files
        const std::string& application_file_path = save_file_dlg.GetFilePath();

        Application batch_application;
        batch_application.SetEngineAppType(EngineAppType::Batch);
        batch_application.SetLogicSettings(m_logicSettings);

        // set the name / label
        {
            std::string application_label = Path::GetFilenameWithoutExtension(application_file_path);
            batch_application.SetName(CIMSAString::MakeName(application_label));
            batch_application.SetLabel(std::move(application_label));
        }


        // create and save the order file
        {
            std::string order_file_path = PortableFunctions::PathReplaceFileExtension(application_file_path, FileExtensions::Order);
            CDEFormFile order(UTF8_TODO::GetCString(order_file_path), UTF8_TODO::GetCString(m_dictionary.GetFilePath()));

            order.CreateOrderFile(m_dictionary, true);

            if( !order.Save(order_file_path) )
                return;

            batch_application.AddForm(std::move(order_file_path));
        }


        // save the logic
        {
            std::string logic_file_path = PortableFunctions::PathAppendFileExtension(application_file_path, FileExtensions::Logic);
            FileIO::WriteText(logic_file_path, m_logicText, true);

            batch_application.AddCodeFile(CodeFile(CodeType::LogicMain, std::make_unique<TextSource>(std::move(logic_file_path))));
        }


        // save the application and see if the user wants to open it
        batch_application.Save(application_file_path);

        const std::string message = FormatText("Would you like to open '%s' now?",
                                               PortableFunctions::PathGetFilename(application_file_path).c_str());

        if( AfxMessageBox(message, MB_YESNO) == IDYES )
        {
            CSProExecutables::RunProgramOpeningFile(CSProExecutables::Program::CSPro, application_file_path);
            OnCancel();
        }
    }

    catch( const CSProException& exception )
    {
        ErrorMessage::Display(exception);
    }
}
