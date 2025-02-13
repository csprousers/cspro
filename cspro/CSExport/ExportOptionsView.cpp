// ExportOptionsView.cpp : implementation file
//

#include "StdAfx.h"
#include "ExportOptionsView.h"
#include "ExptDoc.h"
#include "ExptView.h"
#include <zInterfaceF/UniverseDlg.h>


bool bLockEditUniverse = false;


namespace
{
    class ExportUniverseDlgActionResponder : public UniverseDlg::ActionResponder
    {
    public:
        ExportUniverseDlgActionResponder(CExportOptionsView& view, const CString& saved_universe)
            :   m_view(view),
                m_savedUniverse(saved_universe)
        {
        }

        bool CheckSyntax(const std::wstring& universe) override
        {
            return m_view.CheckUniverseSyntax(WS2CS(universe));
        }

        void ToggleNamesInTree() override
        {
            AfxGetMainWnd()->SendMessage(WM_COMMAND, ID_TOGGLE);
        }

    private:
        CExportOptionsView& m_view;
        CString m_savedUniverse;
    };
}


/////////////////////////////////////////////////////////////////////////////
// CExportOptionsView

IMPLEMENT_DYNCREATE(CExportOptionsView, CFormView)

CExportOptionsView::CExportOptionsView()
    :   CFormView(CExportOptionsView::IDD)
{
    m_csUniverse                        = _T("");
    m_bSingleFile                       = true;
    m_bAllInOneRecord                   = true;
    m_bJoinSingleWithMultipleRecords    = false;
    m_iExportRecordType                 = (int)ExportRecordType::None;
    m_iExportItems                      = (int)ExportItemsSubitems::ItemsOnly;
    m_iExportFormat                     = ExportOptionsView_ExportFormat_TabDelimited;
    m_bForceANSI = true; // GHM 20120416
    m_bCommaDecimal = false;

    m_pDoc                              = NULL;
}

CExportOptionsView::~CExportOptionsView()
{
}

void CExportOptionsView::DoDataExchange(CDataExchange* pDX)
{
    CFormView::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_EDIT_UNIVERSE, m_cEditUniverse);
}


BEGIN_MESSAGE_MAP(CExportOptionsView, CFormView)
    ON_EN_CHANGE(IDC_EDIT_UNIVERSE, OnChangeEditUniverse)
    ON_BN_CLICKED(ID_EDIT_UNIVERSE_BUTTON, OnEditUniverseButton)
    ON_BN_CLICKED(IDC_FILES_ONE, OnSelectionBasic)
    ON_BN_CLICKED(IDC_FILES_MULTIPLE, OnSelectionBasic)
    ON_BN_CLICKED(IDC_OCCS_ONE, OnSelectionBasic)
    ON_BN_CLICKED(IDC_OCCS_SEPARATE, OnSelectionBasic)
    ON_BN_CLICKED(IDC_JOIN, OnSelectionBasic)
    ON_BN_CLICKED(IDC_RT_NO, OnSelectionBasic)
    ON_BN_CLICKED(IDC_RT_BEFORE, OnSelectionBasic)
    ON_BN_CLICKED(IDC_RT_AFTER, OnSelectionBasic)
    ON_BN_CLICKED(IDC_ITEMS_ONLY, OnSelectionBasic)
    ON_BN_CLICKED(IDC_SUBITEMS_ONLY, OnSelectionBasic)
    ON_BN_CLICKED(IDC_ITEMS_SUBITEMS, OnSelectionBasic)
    ON_BN_CLICKED(IDC_UNICODE, OnSelectionBasic)
    ON_BN_CLICKED(IDC_COMMA, OnSelectionBasic)
    ON_BN_CLICKED(IDC_FMT_TAB, OnSelectionCheckFileExtension)
    ON_BN_CLICKED(IDC_FMT_COMMA, OnSelectionCheckFileExtension)
    ON_BN_CLICKED(IDC_FMT_SEMICOLON, OnSelectionCheckFileExtension)
    ON_BN_CLICKED(IDC_FMT_CSPRO, OnSelectionCheckFileExtension)
    ON_BN_CLICKED(IDC_FMT_SPSS, OnSelectionCheckFileExtension)
    ON_BN_CLICKED(IDC_FMT_SAS, OnSelectionCheckFileExtension)
    ON_BN_CLICKED(IDC_FMT_STATA, OnSelectionCheckFileExtension)
    ON_BN_CLICKED(IDC_FMT_R, OnSelectionCheckFileExtension)
    ON_BN_CLICKED(IDC_FMT_ALL, OnSelectionCheckFileExtension)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CExportOptionsView diagnostics

#ifdef _DEBUG
void CExportOptionsView::AssertValid() const
{
    CFormView::AssertValid();
}

void CExportOptionsView::Dump(CDumpContext& dc) const
{
    CFormView::Dump(dc);
}
#endif //_DEBUG


bool CExportOptionsView::CheckUniverseSyntax(const CString& sUniverseStatement)
{
    // from the universe dialog, change the universe
    m_csUniverse = sUniverseStatement;
    ToDoc(GetDoc());

    bool success = GetDoc()->CompileApp();

    // restore the universe from the view's UI
    Gui2Data();
    
    if( !success )
        AfxMessageBox(_T("Invalid Universe"));

    return success;
}

/////////////////////////////////////////////////////////////////////////////
// CExportOptionsView message handlers

void CExportOptionsView::OnEditUniverseButton()
{
    Gui2Data();

    CExportDoc* pDoc = GetDoc();

    ExportUniverseDlgActionResponder universe_dlg_action_responder(*this, m_csUniverse);

    UniverseDlg universe_dlg(pDoc->GetSharedDictionary(), CS2WS(m_csUniverse), universe_dlg_action_responder);

    if( universe_dlg.DoModal() == IDOK )
    {
        m_csUniverse = WS2CS(universe_dlg.GetUniverse());
        Data2Gui();
        ToDoc(GetDoc());
    }
}


void CExportOptionsView::OnSelectionBasic()
{
    Gui2Data();
}


void CExportOptionsView::OnSelectionCheckFileExtension()
{
    Gui2Data();
    CheckFileExtension();
}


void CExportOptionsView::UpdateEnabledDisabledCtrlsStatus(bool* pbJoinValue /*NULL*/ ){

    CExportDoc* pDoc            = GetDoc();
    bool        bDisableJoin    = pDoc ? !pDoc->CanEnableJoin( m_bAllInOneRecord ) : true;

    if(  bDisableJoin ){
        ((CButton*)GetDlgItem(IDC_JOIN))->EnableWindow(FALSE);
    } else {
        ((CButton*)GetDlgItem(IDC_JOIN))->EnableWindow(TRUE);
    }

    CButton* pButton_Join = (CButton*) GetDlgItem(IDC_JOIN);
    m_bJoinSingleWithMultipleRecords    = pButton_Join->IsWindowEnabled() ? pButton_Join->GetCheck()==1 : false;
    if( pbJoinValue )
        *pbJoinValue = m_bJoinSingleWithMultipleRecords;

    CButton* pButtonCommaDecimal = (CButton*)GetDlgItem(IDC_COMMA); // GHM 20120416

    if( m_iExportFormat == ExportOptionsView_ExportFormat_CommaDelimited || m_iExportFormat == ExportOptionsView_ExportFormat_CsPro )
    {
        pButtonCommaDecimal->SetCheck(0);
        pButtonCommaDecimal->EnableWindow(0);
    }

    else
        pButtonCommaDecimal->EnableWindow();

    CButton* pButtonUnicode= (CButton*)GetDlgItem(IDC_UNICODE);

    if( m_iExportFormat == ExportOptionsView_ExportFormat_R || m_iExportFormat == ExportOptionsView_ExportFormat_SPSS_SAS_STATA_R )
    {
        pButtonUnicode->SetCheck(0);
        pButtonUnicode->EnableWindow(0);
    }

    else
        pButtonUnicode->EnableWindow();
}


void CExportOptionsView::Gui2Data()
{
    //From GUI to Data
    m_csUniverse = WS2CS(m_cEditUniverse.GetText());

    m_bSingleFile     = ((CButton*)GetDlgItem(IDC_FILES_ONE))->GetCheck() ==  1;
    m_bAllInOneRecord = ((CButton*)GetDlgItem(IDC_OCCS_ONE))->GetCheck()  ==  1;

    if( ((CButton*)GetDlgItem(IDC_RT_NO))->GetCheck()==1 ){
        m_iExportRecordType = (int)ExportRecordType::None;

    } else if( ((CButton*)GetDlgItem(IDC_RT_BEFORE))->GetCheck()==1 ){
        m_iExportRecordType = (int)ExportRecordType::BeforeIds;

    } else if( ((CButton*)GetDlgItem(IDC_RT_AFTER))->GetCheck()==1 ){
        m_iExportRecordType = (int)ExportRecordType::AfterIds;
    }

    ((CButton*)GetDlgItem(IDC_RT_NO))->EnableWindow( !m_bAllInOneRecord );
    ((CButton*)GetDlgItem(IDC_RT_BEFORE))->EnableWindow(!m_bAllInOneRecord );
    ((CButton*)GetDlgItem(IDC_RT_AFTER))->EnableWindow( !m_bAllInOneRecord);

    if( ((CButton*)GetDlgItem(IDC_ITEMS_ONLY))->GetCheck()==1 ){
        m_iExportItems = (int)ExportItemsSubitems::ItemsOnly;

    } else if( ((CButton*)GetDlgItem(IDC_SUBITEMS_ONLY))->GetCheck()==1 ){
        m_iExportItems = (int)ExportItemsSubitems::SubitemsOnly;

    } else if( ((CButton*)GetDlgItem(IDC_ITEMS_SUBITEMS))->GetCheck()==1 ){
        m_iExportItems = (int)ExportItemsSubitems::Both;
    }



    if( ((CButton*)GetDlgItem(IDC_FMT_TAB))->GetCheck()==1 ){
        m_iExportFormat = ExportOptionsView_ExportFormat_TabDelimited;

    } else if( ((CButton*)GetDlgItem(IDC_FMT_COMMA))->GetCheck()==1 ){
        m_iExportFormat = ExportOptionsView_ExportFormat_CommaDelimited;
    } else if( ((CButton*)GetDlgItem(IDC_FMT_SEMICOLON))->GetCheck()==1 ){
        m_iExportFormat = ExportOptionsView_ExportFormat_SemiColon;

    } else if( ((CButton*)GetDlgItem(IDC_FMT_CSPRO))->GetCheck()==1 ){
        m_iExportFormat = ExportOptionsView_ExportFormat_CsPro;

    } else if( ((CButton*)GetDlgItem(IDC_FMT_SPSS))->GetCheck()==1 ){
        m_iExportFormat = ExportOptionsView_ExportFormat_SPSS;

    } else if( ((CButton*)GetDlgItem(IDC_FMT_SAS))->GetCheck()==1 ){
        m_iExportFormat = ExportOptionsView_ExportFormat_SAS;

    } else if( ((CButton*)GetDlgItem(IDC_FMT_STATA))->GetCheck()==1 ){
        m_iExportFormat = ExportOptionsView_ExportFormat_STATA;

    } else if( ((CButton*)GetDlgItem(IDC_FMT_R))->GetCheck()==1 ){
        m_iExportFormat = ExportOptionsView_ExportFormat_R;

    } else if( ((CButton*)GetDlgItem(IDC_FMT_ALL))->GetCheck()==1 ){
        m_iExportFormat = ExportOptionsView_ExportFormat_SPSS_SAS_STATA_R;
    }

    m_bForceANSI = (((CButton *)GetDlgItem(IDC_UNICODE))->GetCheck()!=1);
    m_bCommaDecimal = (((CButton *)GetDlgItem(IDC_COMMA))->GetCheck()==1);

    /* FABN COM Jun 2005 -> To decide if Join can be enabled/disabled is needed
                        more information. To see the whole picture the application
                        must see :
                            * the current tree selection status
                            * current options settings (without using the join status)
                        ..and after that, decide about join status.
                        * to decide if join can be enabled/disabled there is a centralized function
                        inside CExportDoc called CanEnableJoin
                        * bAllInOneRecord is passed as a parameter because of at this point can be different
                        from what the document can see, because of ToDoc() has been executed yet

    bool bDisableJoin = m_bSingleFile && m_bAllInOneRecord;
    bDisableJoin = bDisableJoin || (!m_bSingleFile && m_bAllInOneRecord);
    */
    UpdateEnabledDisabledCtrlsStatus();

    //From Data To the Document
    ToDoc( GetDoc() );
}


void CExportOptionsView::Data2Gui()
{
    auto set_check = [&](int nID, bool check)
    {
        ((CButton*)GetDlgItem(nID))->SetCheck(check ? BST_CHECKED : BST_UNCHECKED);
    };

    bLockEditUniverse = true;
    {
        m_cEditUniverse.SetText(m_csUniverse);
    }
    bLockEditUniverse = false;

    set_check(IDC_FILES_ONE, m_bSingleFile);
    set_check(IDC_FILES_MULTIPLE, !m_bSingleFile);

    set_check(IDC_OCCS_ONE, m_bAllInOneRecord);
    set_check(IDC_OCCS_SEPARATE, !m_bAllInOneRecord);

    bool bDisableJoin = m_bSingleFile && m_bAllInOneRecord;
    bDisableJoin = bDisableJoin || (!m_bSingleFile && m_bAllInOneRecord);
    if( bDisableJoin ){
        set_check(IDC_JOIN, false);
        ((CButton*)GetDlgItem(IDC_JOIN))->EnableWindow(FALSE);
        m_bJoinSingleWithMultipleRecords = false;
    } else {
        ((CButton*)GetDlgItem(IDC_JOIN))->EnableWindow(TRUE);
    }

    set_check(IDC_JOIN, m_bJoinSingleWithMultipleRecords);


    ((CButton*)GetDlgItem(IDC_RT_NO))->EnableWindow( !m_bAllInOneRecord );
    ((CButton*)GetDlgItem(IDC_RT_BEFORE))->EnableWindow( !m_bAllInOneRecord );
    ((CButton*)GetDlgItem(IDC_RT_AFTER))->EnableWindow( !m_bAllInOneRecord);

    if( m_bAllInOneRecord )
        m_iExportRecordType = (int)ExportRecordType::None;
    

    set_check(IDC_RT_NO, ( m_iExportRecordType == (int)ExportRecordType::None ));
    set_check(IDC_RT_BEFORE, ( m_iExportRecordType == (int)ExportRecordType::BeforeIds ));
    set_check(IDC_RT_AFTER, ( m_iExportRecordType == (int)ExportRecordType::AfterIds ));

    set_check(IDC_ITEMS_ONLY, ( m_iExportItems == (int)ExportItemsSubitems::ItemsOnly ));
    set_check(IDC_SUBITEMS_ONLY, ( m_iExportItems == (int)ExportItemsSubitems::SubitemsOnly ));
    set_check(IDC_ITEMS_SUBITEMS, ( m_iExportItems == (int)ExportItemsSubitems::Both ));

    set_check(IDC_FMT_TAB, ( m_iExportFormat == ExportOptionsView_ExportFormat_TabDelimited ));
    set_check(IDC_FMT_COMMA, ( m_iExportFormat == ExportOptionsView_ExportFormat_CommaDelimited ));
    set_check(IDC_FMT_SEMICOLON, ( m_iExportFormat == ExportOptionsView_ExportFormat_SemiColon ));
    set_check(IDC_FMT_CSPRO, ( m_iExportFormat == ExportOptionsView_ExportFormat_CsPro ));
    set_check(IDC_FMT_SPSS, ( m_iExportFormat == ExportOptionsView_ExportFormat_SPSS ));
    set_check(IDC_FMT_SAS, ( m_iExportFormat == ExportOptionsView_ExportFormat_SAS ));
    set_check(IDC_FMT_STATA, ( m_iExportFormat == ExportOptionsView_ExportFormat_STATA ));
    set_check(IDC_FMT_R, ( m_iExportFormat == ExportOptionsView_ExportFormat_R ));
    set_check(IDC_FMT_ALL, ( m_iExportFormat == ExportOptionsView_ExportFormat_SPSS_SAS_STATA_R ));

    set_check(IDC_UNICODE, !m_bForceANSI);
    set_check(IDC_COMMA, m_bCommaDecimal);
}


void CExportOptionsView::OnInitialUpdate()
{
    CFormView::OnInitialUpdate();

    std::optional<int> lexer_language;

    if( m_pDoc != nullptr )
        lexer_language = Lexers::GetLexer_Logic(m_pDoc->GetLogicSettings());

    m_cEditUniverse.ReplaceCEdit(this, false, false, std::move(lexer_language));

    Data2Gui();
}


void CExportOptionsView::RefreshLexer()
{
    m_cEditUniverse.PostMessage(UWM::Edit::RefreshLexer);
}


int ToIntExportFormat(METHOD method)
{
    int i = -1;
    switch( method ){
    case METHOD::TABS       :   i = ExportOptionsView_ExportFormat_TabDelimited;    break;
    case METHOD::COMMADEL   :   i = ExportOptionsView_ExportFormat_CommaDelimited;  break;
    case METHOD::SPSS       :   i = ExportOptionsView_ExportFormat_SPSS;            break;
    case METHOD::SAS        :   i = ExportOptionsView_ExportFormat_SAS;             break;
    case METHOD::STATA      :   i = ExportOptionsView_ExportFormat_STATA;           break;
    case METHOD::SEMI_COLON :   i = ExportOptionsView_ExportFormat_SemiColon;       break;
    case METHOD::CSPRO      :   i = ExportOptionsView_ExportFormat_CsPro;           break;
    case METHOD::R          :   i = ExportOptionsView_ExportFormat_R;               break;
    case METHOD::ALLTYPES   :   i = ExportOptionsView_ExportFormat_SPSS_SAS_STATA_R;break;
    default : ASSERT(0); break;
    }
    return i;

}

METHOD FromIntExportFormat(int iExportFormat)
{
    METHOD i = METHOD::TABS;
    switch( iExportFormat ){
    case ExportOptionsView_ExportFormat_TabDelimited    :   i = METHOD::TABS;       break;
    case ExportOptionsView_ExportFormat_CommaDelimited  :   i = METHOD::COMMADEL;   break;
    case ExportOptionsView_ExportFormat_SPSS            :   i = METHOD::SPSS;       break;
    case ExportOptionsView_ExportFormat_SAS             :   i = METHOD::SAS;        break;
    case ExportOptionsView_ExportFormat_STATA           :   i = METHOD::STATA;      break;
    case ExportOptionsView_ExportFormat_SemiColon       :   i = METHOD::SEMI_COLON; break;
    case ExportOptionsView_ExportFormat_CsPro           :   i = METHOD::CSPRO;      break;
    case ExportOptionsView_ExportFormat_R               :   i = METHOD::R;          break;
    case ExportOptionsView_ExportFormat_SPSS_SAS_STATA_R:   i = METHOD::ALLTYPES;   break;
    default : ASSERT(0); break;
    }
    return i;
}

void CExportOptionsView::FromDoc( CExportDoc* pDoc )
{
    //Update Doc
    SetDoc( pDoc );

    //Update Data
    m_bSingleFile                       = pDoc->m_bmerge;
    m_bAllInOneRecord                   = pDoc->m_bAllInOneRecord;
    m_bJoinSingleWithMultipleRecords    = pDoc->m_bJoinSingleWithMultipleRecords;
    m_iExportRecordType                 = (int)pDoc->m_exportRecordType;
    m_iExportItems                      = (int)pDoc->m_exportItemsSubitems;
    m_iExportFormat                     = ToIntExportFormat( pDoc->m_convmethod );
    m_csUniverse                        = pDoc->m_csUniverse;

    m_bForceANSI                        = pDoc->m_bForceANSI; // GHM 20120416
    m_bCommaDecimal                     = pDoc->m_bCommaDecimal;

    //Update Gui
    Data2Gui();
}

void CExportOptionsView::ToDoc( CExportDoc* pDoc )
{
    ASSERT( pDoc );
    if( !pDoc )
        return;

    //Update Data
    pDoc->m_bmerge                          = m_bSingleFile;
    pDoc->m_bAllInOneRecord                 = m_bAllInOneRecord;

    pDoc->SetJoin( m_bJoinSingleWithMultipleRecords );

    pDoc->m_exportRecordType                = (ExportRecordType)m_iExportRecordType;
    pDoc->m_exportItemsSubitems             = (ExportItemsSubitems)m_iExportItems;
    pDoc->m_convmethod                      = FromIntExportFormat(m_iExportFormat);
    pDoc->m_csUniverse                      = m_csUniverse;
    pDoc->m_bForceANSI                      = m_bForceANSI; // GHM 20120416
    pDoc->m_bCommaDecimal                   = m_bCommaDecimal;

    pDoc->Checks();

    //so, app and bch must be updated
    pDoc->SyncBuff_app();
}

CExportDoc* CExportOptionsView::GetDoc(){
    return m_pDoc;
}
void    CExportOptionsView::SetDoc( CExportDoc* pDoc ){
    m_pDoc = pDoc;
}

void CExportOptionsView::OnChangeEditUniverse()
{
    // TODO: If this is a RICHEDIT control, the control will not
    // send this notification unless you override the CFormView::OnInitDialog()
    // function and call CRichEditCtrl().SetEventMask()
    // with the ENM_CHANGE flag ORed into the mask.

    if( bLockEditUniverse )
        return;

    Gui2Data();
}


void CExportOptionsView::CheckFileExtension(){
    if(!m_pDoc)
        return;

    m_pDoc->CheckFileExtension();
}

void CExportOptionsView::EnableJoinSingleMultiple(bool bEnable){

    if( m_bAllInOneRecord )
        ((CButton*)GetDlgItem(IDC_JOIN))->EnableWindow(false);

    else
        ((CButton*)GetDlgItem(IDC_JOIN))->EnableWindow(bEnable);
}

bool CExportOptionsView::IsJoinEnabled(){

    CButton* pButton = (CButton*) GetDlgItem(IDC_JOIN);
    if(!pButton)
        return false;

    if(!IsWindow( pButton->GetSafeHwnd() ))
        return false;

    return pButton->IsWindowEnabled() == TRUE;
}
