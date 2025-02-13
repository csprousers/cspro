// MsgDial.cpp : implementation file
//

#include "StdAfx.h"
#include "MsgDial.h"
#include <zUtilO/CustomFont.h> // 20100621
#include <zUtilO/MemoryHelpers.h>


/////////////////////////////////////////////////////////////////////////////
// CMsgDialog dialog

CMsgDialog::CMsgDialog(const CMsgOptions& message_options, CWnd* pParent /*=NULL*/)
    :   CDialog(IDD_MSGBOX_DIALOG, pParent),
        m_messageOptions(message_options)
{
    // Buttons (1/2)
    m_pCSButtonsLabels = m_messageOptions.GetArrayButtons();

    if(  m_pCSButtonsLabels.empty() )
    {
        switch( m_messageOptions.GetType() )
        {
        case MB_ABORTRETRYIGNORE:
            m_pCSButtonsLabels.push_back( _T( " Abort " ) );
            m_pCSButtonsLabels.push_back( _T( " Retry " ) );
            m_pCSButtonsLabels.push_back( _T( " Cancel ") );
            break;

        case MB_OK:
            m_pCSButtonsLabels.push_back( _T(" OK ") );
            break;

        case MB_OKCANCEL:
            m_pCSButtonsLabels.push_back( _T(" OK ") );
            m_pCSButtonsLabels.push_back( _T(" Cancel ") );
            break;

        case MB_RETRYCANCEL :
            m_pCSButtonsLabels.push_back( _T(" Retry ") );
            m_pCSButtonsLabels.push_back( _T(" Cancel ") );
            break;

        case MB_YESNO:
            m_pCSButtonsLabels.push_back( _T(" Yes ") );
            m_pCSButtonsLabels.push_back( _T(" No ") );
            break;

        case MB_YESNOCANCEL:
            m_pCSButtonsLabels.push_back( _T(" Yes ") );
            m_pCSButtonsLabels.push_back( _T(" No ") );
            m_pCSButtonsLabels.push_back( _T(" Cancel ") );
            break;
        }
    }

    // Title
    m_CSTittle = m_messageOptions.GetTitle();
    m_iTittleHeigth = GetSystemMetrics(SM_CYCAPTION); // previously hard-coded to 21

    //Textos :
    CString message_text = m_messageOptions.GetMsg();
    message_text.Replace(_T("&"), _T("&&"));

    m_pCSAMainText = new CStringArray;

    if( !SO::ContainsNewlineCharacter(wstring_view(message_text)) )
    {
        m_pCSAMainText->Add(message_text);
    }

    else
    {
        // if delimiting by newlines, add each line to the string array separately
        SO::ForeachLine(message_text, true,
            [&](wstring_view line_sv)
            {
                m_pCSAMainText->Add(line_sv);
                return true;
            });
    }

    m_iTextTopMargin = 15;
    m_iTextSeparationToButtons = 14;

    m_iMessageFontHeigth = 17;
    m_iMessageFontWidth  =  7;
    m_CSMessageFontName  = _T("Courier New");
    m_msgFontCharSet = DEFAULT_CHARSET; // JH 7/05

    m_iTextLabelFontHeigth     = m_iMessageFontHeigth;
    m_iTextLabelFontWidth      = m_iMessageFontWidth;
    m_iTextVerticalSeparation  = 2;

    // Buttons (2/2)
    m_iButtonsSeparation = 5;   //Default.
    m_iLeftMargin        = 25;  //Default.
    m_iButtonsHeigth     = 25;  //Default.
    m_iBottomMargin      = m_iTextTopMargin;  //Default.
    m_iNumButtons        = 0;   //Default.

    m_iButtonFontHeigth  = m_iTextLabelFontHeigth;
    m_iButtonFontWidth   = m_iTextLabelFontWidth;

    m_pButtonsFont = NULL;

    //
    m_iLastPressedButton = m_messageOptions.GetCancelReturn();  // if I close without press any button
    m_iDefaultButtonFocus = m_messageOptions.GetFocusButton();

    m_iHeigth = -1;

    m_iActiveButtonIndex = m_messageOptions.GetFocusButton(); //Carefull!!: could be out of range :)

    //
    m_pWindowParent = pParent;

    //
    m_bExistDefaultDialogIcon = false;

    //Fonts :
    m_pMessageFont = NULL;

    //INIT 06-Mar-2001 F.A.B.N.
    //Seteo de la máxima cantidad de caracteres por línea :
    m_iMaxWidthAllowed      = (int) ::GetSystemMetrics(SM_CXSCREEN) / 2;
    m_iMaxTextLengthAllowed = (int) ( m_iMaxWidthAllowed - 2*m_iLeftMargin ) / m_iMessageFontWidth;
    //END 06-Mar-2001 F.A.B.N.


    m_bButtonsHorizontalCenter = true;

    //INIT 06-Mar-2001 F.A.B.N.
    m_iTotalButtonsWidth = -1;
    m_iTotalTextsWidth   = -1;

    m_bNothingMoreToDoWithTexts = false;
    //END 06-Mar-2001 F.A.B.N.

    //{{AFX_DATA_INIT(CMsgDialog)
        // NOTE: the ClassWizard will add member initialization here
    //}}AFX_DATA_INIT
}

CMsgDialog::~CMsgDialog() {
    Clean();
}


BEGIN_MESSAGE_MAP(CMsgDialog, CDialog)
    //{{AFX_MSG_MAP(CMsgDialog)
    ON_WM_CLOSE()
    ON_WM_CHAR()
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMsgDialog message handlers

BOOL CMsgDialog::OnInitDialog()
{
    CDialog::OnInitDialog();

    // 20100621 to allow for the dynamic setting of fonts for error messages

    // for these system controlled error messages I'm not going to invest much time
    // in making things look good, I won't allow the user to modify the size of the font
    // so the font size is set to be the font size specified for the dialog box
    // but i do want to make it more dynamic, enable (and supplement) the code below

    // 20111026 on a trevor request i will now try to make these message look good
    UserDefinedFonts* pUserFonts = nullptr;
    AfxGetMainWnd()->SendMessage(WM_IMSA_GET_USER_FONTS, (WPARAM)&pUserFonts);

    bool is_user_message = ( m_messageOptions.GetMessageType().has_value() && *m_messageOptions.GetMessageType() == MessageType::User );

    if( is_user_message && pUserFonts != nullptr && pUserFonts->IsFontDefined(UserDefinedFonts::FontType::ErrMsg) ) // user has defined a particular font
    {
        // the actual font gets set later, here we just adjust the height/width variables
        LOGFONT lf;
        pUserFonts->GetFont(UserDefinedFonts::FontType::ErrMsg)->GetLogFont(&lf);

        m_iTextLabelFontHeigth = abs(lf.lfHeight);

        m_iTextLabelFontWidth = abs(lf.lfWidth); // 20120207 changed for unicode

        m_iButtonFontHeigth = m_iMessageFontHeigth = m_iTextLabelFontHeigth; // 20111026
        m_iButtonFontWidth = m_iMessageFontWidth = m_iTextLabelFontWidth;
        m_iButtonsHeigth = m_iButtonFontHeigth + 10;
    }

    InitFonts(is_user_message);

    // Dimensions

    // WARN: The dialog height contains the title border, but the zero is posicionating under the title
    m_iHeigth =     m_iTextTopMargin
                    + NumTextLabels()*(m_iTextLabelFontHeigth /*INIT 12-Mar-2001 F.A.B.N.*/+ GetAditionalFontHeigth( m_CSMessageFontName ) /*END 12-Mar-2001 F.A.B.N.*/ )
                    + (NumTextLabels()-1)*m_iTextVerticalSeparation
                    + m_iTextSeparationToButtons
                    + m_iButtonsHeigth
                    + m_iBottomMargin
                    + m_iTittleHeigth;

    m_iNumButtons   = m_pCSButtonsLabels.size();

    if( m_iNumButtons ) // 20120207 for unicode
    {
        CClientDC pDC(this);
        pDC.SelectObject(m_pButtonsFont);

        m_iButtonsWidth = 0;

        for( int i = 0; i < m_iNumButtons; i++ )
        {
            int iTextWidth = pDC.GetTextExtent(m_pCSButtonsLabels.at(i)).cx;

            if( iTextWidth > m_iButtonsWidth )
                m_iButtonsWidth = iTextWidth;
        }

        m_iButtonsWidth += 2 * m_iLeftMargin;
    }

    m_iWidth = GetDialogWidth();

    int  iNumMsgLinesBefore;
    int  iNumMsgLinesAfter;

    bool bAcceptWidth = false;
    while( ! bAcceptWidth ){

        //si el numero de lineas antes y despues de hacer un ResizeMsgList coincide, entonces el ancho
        //debido a los textos no puede ser reducido => el menor ancho de entre todas las "palabras" excede
        //la constante m_iMaxWidthAllowed.

        //
        iNumMsgLinesBefore = m_pCSAMainText->GetSize();

        ResizeMsgList();

        iNumMsgLinesAfter  = m_pCSAMainText->GetSize();
        //

        m_iWidth = GetDialogWidth();

        m_iTotalButtonsWidth =                          2*m_iLeftMargin
                               +          m_iNumButtons*m_iButtonsWidth
                               + (m_iNumButtons-1)*m_iButtonsSeparation;



        // m_iTotalTextsWidth   = 2*m_iLeftMargin + MaxTextLength()*m_iTextLabelFontWidth;

        // 20120207 changed for unicode
        CClientDC pDC(this);
        pDC.SelectObject(m_pMessageFont);

        int maxTextWidth = 0;

        for( int i = 0; i < NumTextLabels(); i++ )
        {
            int thisTextWidth = pDC.GetTextExtent(m_pCSAMainText->GetAt(i)).cx;

            if( thisTextWidth > maxTextWidth )
                maxTextWidth = thisTextWidth;
        }

        m_iTotalTextsWidth = 2 * m_iLeftMargin + maxTextWidth;


        m_iWidth = ( m_iTotalButtonsWidth > m_iTotalTextsWidth ) ? m_iTotalButtonsWidth : m_iTotalTextsWidth;


        if( m_iWidth<=m_iMaxWidthAllowed ){

            bAcceptWidth = true;

        } else {

            if( m_iWidth>m_iMaxWidthAllowed ){

                if( ( m_iWidth==m_iTotalButtonsWidth )  ||
                    ( iNumMsgLinesBefore==iNumMsgLinesAfter ) ||
                    ( ( m_iWidth==m_iTotalTextsWidth) && (m_bNothingMoreToDoWithTexts) ) ){

                    bAcceptWidth = true;
                }
            }
        }

        //if( bAcceptWidth == false ) AfxMessageBox("false");
    }

    m_iHeigth =   m_iTextTopMargin
                + NumTextLabels()*(m_iTextLabelFontHeigth /*INIT 12-Mar-2001 F.A.B.N.*/+ GetAditionalFontHeigth( m_CSMessageFontName ) /*END 12-Mar-2001 F.A.B.N.*/ )
                + (NumTextLabels()-1)*m_iTextVerticalSeparation
                + m_iTextSeparationToButtons
                + m_iButtonsHeigth
                + m_iBottomMargin
                + m_iTittleHeigth;


    //END 06-Mar-2001 F.A.B.N.


    //Posicionamiento :
    if( m_pWindowParent==NULL ){

        int iScreenWidth  = ::GetSystemMetrics(SM_CXSCREEN);
        int iScreenHeight = ::GetSystemMetrics(SM_CYSCREEN);

        m_iXo = (iScreenWidth  - m_iWidth)/2;
        m_iYo = (iScreenHeight - m_iHeigth)/2;

    } else {

        RECT c;
        m_pWindowParent->GetWindowRect( &c );

        m_iXo = c.left + ( c.right - c.left - m_iWidth )/2;
        m_iYo = c.top  + ( c.bottom - c.top - m_iHeigth)/2;
    }

    SetWindowPos(0, m_iXo,m_iYo, m_iWidth,m_iHeigth, SWP_NOZORDER);

    //Título :
    SetWindowText( m_CSTittle );

    //Textos :
    DisplayTexts();

    //Botones :
    DisplayButtons();

    //ícono del diálogo (arriba, a la izquierda)
    if( m_bExistDefaultDialogIcon==true ){
        SetIcon( m_DefaultDialogIcon , false );
    }

    if( m_iDefaultButtonFocus>=0 &&
        m_iDefaultButtonFocus<m_CS_ArrayButtons.GetSize() ){

        //primero le damos el foco al botón por defecto.
        m_CS_ArrayButtons.GetAt(m_iDefaultButtonFocus)->SetFocus();

        //segundo, le pintamos un borde que haga ver que es el botón por defecto.
        m_CS_ArrayButtons.GetAt(m_iDefaultButtonFocus)->SetButtonStyle( BS_DEFPUSHBUTTON );

        m_bExistActiveButton = true;
        return FALSE;

    } else {
        //En caso de que el índice del botón por defecto venga fuera de rango,
        //le damos el foco al diálogo.
        SetFocus();
        m_bExistActiveButton = false;
        return TRUE;
    }
}

void CMsgDialog::OnClose()
{
    CloseDialog();
}



void CMsgDialog::PressButton(int iPressedButton)
{
    m_iLastPressedButton = iPressedButton;
    CloseDialog();
}


void CMsgDialog::DisplayButtons()
{
    CCSButton* pCS_MessageButton;

    int iLeftButtonMargin;
    if( m_bButtonsHorizontalCenter ){

        //Total horizontal space occupated by buttons, including horizontal space button to button.
        int iTotalButtonsWidth = m_iNumButtons*m_iButtonsWidth+ (m_iNumButtons-1)*m_iButtonsSeparation;

        //New value for left buttons margin
        iLeftButtonMargin = ( m_iWidth - iTotalButtonsWidth ) / 2;

    } else iLeftButtonMargin = m_iLeftMargin;

    for( int iButtonIndex=0; iButtonIndex<m_iNumButtons; iButtonIndex++){

        //Creación de un nuevo botón :
        pCS_MessageButton = new CCSButton;
        pCS_MessageButton->SetParent( this );

        CString CSButtonLabel = m_pCSButtonsLabels.at(iButtonIndex);

        pCS_MessageButton->SetIndex( iButtonIndex  );
        pCS_MessageButton->SetLabel( CSButtonLabel );


        int Xo = iLeftButtonMargin + iButtonIndex*(m_iButtonsWidth + m_iButtonsSeparation);
        int Yo = m_iTextBottomCoordinate + m_iTextSeparationToButtons +1;
        int X1 = Xo + m_iButtonsWidth  -1;
        int Y1 = Yo + m_iButtonsHeigth -1;


        pCS_MessageButton->Create(  CSButtonLabel,
                                   WS_CHILD | WS_VISIBLE,
                                   CRect( Xo,Yo, X1,Y1),
                                   this,
                                   1000 );

        pCS_MessageButton->SetFont( m_pButtonsFont, true );

        m_CS_ArrayButtons.Add( pCS_MessageButton );
    }
}

void CMsgDialog::DisplayTexts()
{
    int iTopMargin = m_iTextTopMargin;

    //m_iTextLabelsMargin = (m_iWidth - MaxTextLength() * m_iTextLabelFontWidth) / 2;

    CStatic * pStaticTextLabel;
    int Xo,Yo,X1,Y1;

    // 20120207 calculate the maximum text margin needed
    CClientDC pDC(this);
    pDC.SelectObject(m_pMessageFont);
    int maxWidth = 0;

    for( int iTextLabelIndex = 0; iTextLabelIndex < NumTextLabels(); iTextLabelIndex++ )
    {
        int thisWidth = pDC.GetTextExtent(m_pCSAMainText->GetAt(iTextLabelIndex)).cx;

        if( thisWidth > maxWidth )
            maxWidth = thisWidth;
    }

    m_iTextLabelsMargin = ( m_iWidth - maxWidth ) / 2;


    for( int iTextLabelIndex=0; iTextLabelIndex<NumTextLabels(); iTextLabelIndex++){

        pStaticTextLabel = new CStatic;

        Xo = m_iTextLabelsMargin;
        Yo = iTopMargin + iTextLabelIndex*(m_iTextLabelFontHeigth /*INIT 12-Mar-2001 F.A.B.N.*/
                                                                  + GetAditionalFontHeigth( m_CSMessageFontName )
                                                                  /*END 12-Mar-2001 F.A.B.N.*/
                                                                  +m_iTextVerticalSeparation);
        X1 = m_iWidth;
        Y1 = Yo + m_iTextLabelFontHeigth/*INIT 12-Mar-2001 F.A.B.N.*/+ GetAditionalFontHeigth( m_CSMessageFontName )/*END 12-Mar-2001 F.A.B.N.*/ -1;

        m_iTextBottomCoordinate = Y1;

        // JH - 7/05 Arabic/Russian support
        // if non english font (set via SetMessageFont), use unicode version of static text control
        // non-unicode version doesn't support non-Western fonts.
        if (m_msgFontCharSet == DEFAULT_CHARSET) {
            pStaticTextLabel->Create(  m_pCSAMainText->GetAt( iTextLabelIndex ),
                                    WS_CHILD | WS_VISIBLE,
                                    CRect( Xo,Yo, X1,Y1 ),
                                    this,
                                    2000);

            pStaticTextLabel->SetFont( m_pMessageFont, true );

       } else {
           // Have to create with CreateWindowW to get unicode version
            HWND hWndStatic = ::CreateWindowW(_T("STATIC"), _T("stat1"), WS_CHILD | WS_VISIBLE, Xo, Yo, X1 - Xo, Y1 - Yo, GetSafeHwnd(), (HMENU)(UINT_PTR) 2000, NULL, NULL );
            pStaticTextLabel->Attach(hWndStatic);

            pStaticTextLabel->SetFont( m_pMessageFont, true );

            // have to translate text to unicode and then use SetWindowTextW to get static to display
            // non-western text correctly
            pStaticTextLabel->SetWindowText(m_pCSAMainText->GetAt( iTextLabelIndex ));
        }

        m_CS_ArrayTexts.Add( pStaticTextLabel );
    }


}

void CMsgDialog::Clean()
{
    //Liberando memoria pedida por botones :
    for( int iButtonIndex=0; iButtonIndex<m_CS_ArrayButtons.GetSize(); iButtonIndex++)
    {
        CCSButton* pCS_MessageButton = m_CS_ArrayButtons.GetAt(iButtonIndex);
        delete pCS_MessageButton;
    }

    m_CS_ArrayButtons.RemoveAll();
    m_iNumButtons = 0;

    //Liberando memoria pedida por textos :
    for( int iTextLabelIndex=0; iTextLabelIndex<m_CS_ArrayTexts.GetSize(); iTextLabelIndex++){

        CStatic* pStaticTextLabel = m_CS_ArrayTexts.GetAt( iTextLabelIndex );

        if( pStaticTextLabel!=NULL ) {
            // JH - 7/05 Arabic/Russian support
            if (m_msgFontCharSet != DEFAULT_CHARSET) {
                pStaticTextLabel->Detach();
            }

            delete pStaticTextLabel;
        }
    }

    m_CS_ArrayTexts.RemoveAll();

    //Fonts :
    safe_delete(m_pButtonsFont);
    safe_delete(m_pMessageFont);

    safe_delete(m_pCSAMainText);
}

void CMsgDialog::CloseDialog()
{
    Clean();
    CDialog::OnOK();
}


void CMsgDialog::SetWindowParent(CWnd *pWindowParent)
{
    m_pWindowParent = pWindowParent;
}

//
void CMsgDialog::SetTextTopMargin(int iTextTopMargin)
{
    m_iTextTopMargin = iTextTopMargin;
}

//
void CMsgDialog::SetTextSeparationToButtons(int iTextSeparationToButtons)
{
    m_iTextSeparationToButtons = iTextSeparationToButtons;
}

//
void CMsgDialog::SetTextLabelFontHeigth(int iTextLabelFontHeigth)
{
    m_iTextLabelFontHeigth = iTextLabelFontHeigth;
}

//
void CMsgDialog::SetTextLabelFontWidth(int iTextLabelFontWidth)
{
    m_iTextLabelFontWidth = iTextLabelFontWidth;
}

//
void CMsgDialog::SetTextVerticalSeparation(int iTextVerticalSeparation)
{
    m_iTextVerticalSeparation   = iTextVerticalSeparation;
}

//
void CMsgDialog::SetButtonsSeparation(int iButtonsSeparation)
{
    m_iButtonsSeparation    = iButtonsSeparation;
}

//
void CMsgDialog::SetLeftMargin(int iLeftMargin)
{
    m_iLeftMargin = iLeftMargin;
}

//
void CMsgDialog::SetButtonsHeigth(int iButtonsHeigth)
{
    m_iButtonsHeigth = iButtonsHeigth;
}

void CMsgDialog::SetButtonFontWidth(int iButtonFontWidth)
{
    m_iButtonFontWidth = iButtonFontWidth;
}

void CMsgDialog::SetButtonFontHeigth(int iButtonFontHeigth)
{
    m_iButtonFontHeigth = iButtonFontHeigth;
}

//
void CMsgDialog::SetBottomMargin(int iBottomMargin)
{
    m_iBottomMargin = iBottomMargin;
}


void CMsgDialog::InitFonts(bool userMessage)
{
    // 20100621 to allow for the dynamic setting of fonts for error messages
    UserDefinedFonts* pUserFonts = nullptr;

    // 20100708 on macro's request user-defined fonts will only be used for user error messages
    // if( NumTextLabels() == 1 && m_pCSAMainText->GetAt(0).Left(2).CompareNoCase("U ") == 0 )

    //if( m_pCSAMainText->GetAt(0).Left(2).CompareNoCase(_T("U ")) == 0 ) // 20111026 so that custom fonts can work on long system-controlled messages
    if( userMessage )
        AfxGetMainWnd()->SendMessage(WM_IMSA_GET_USER_FONTS, (WPARAM)&pUserFonts);

    if( pUserFonts != nullptr && pUserFonts->GetFont(UserDefinedFonts::FontType::ErrMsg) ) // user has defined a particular font
    {
        LOGFONT lf;
        pUserFonts->GetFont(UserDefinedFonts::FontType::ErrMsg)->GetLogFont(&lf);

        lf.lfHeight = m_iButtonFontHeigth;
        lf.lfWidth = m_iButtonFontWidth;

        m_pButtonsFont = new CFont();
        m_pButtonsFont->CreateFontIndirect(&lf);

        lf.lfHeight = m_iMessageFontHeigth;
        lf.lfWidth = m_iMessageFontWidth;

        m_pMessageFont = new CFont();
        m_pMessageFont->CreateFontIndirect(&lf);
        return;
    }

    m_pButtonsFont = new CFont();
    m_pButtonsFont->CreateFont (        m_iButtonFontHeigth,
                                        m_iButtonFontWidth,
                                        0,
                                        0,
                                        FW_NORMAL,
                                        FALSE,
                                        FALSE,
                                        0,
                                        DEFAULT_CHARSET,
                                        OUT_STRING_PRECIS,
                                        CLIP_STROKE_PRECIS,
                                        PROOF_QUALITY,
                                        FF_DONTCARE,
                                        _T("Courier New"));

    m_pMessageFont= new CFont();
    m_pMessageFont->CreateFont (        m_iTextLabelFontHeigth,
                                        m_iTextLabelFontWidth,
                                        0,
                                        0,
                                        FW_NORMAL,
                                        FALSE,
                                        FALSE,
                                        0,
                                        m_msgFontCharSet,   // JH 7/05
                                        OUT_STRING_PRECIS,
                                        CLIP_STROKE_PRECIS,
                                        PROOF_QUALITY,
                                        FF_DONTCARE,
                                        m_CSMessageFontName);

}

int CMsgDialog::GetLastPressedButtonIndex()
{
    return m_iLastPressedButton;
}

void CMsgDialog::SetDefaultDialogIcon(HICON hDefaultDialogIcon)
{
    m_DefaultDialogIcon       = hDefaultDialogIcon;
    m_bExistDefaultDialogIcon = true;
}



void CMsgDialog::SetButtonsHorizontalCenterStatus(bool bStatus)
{
    m_bButtonsHorizontalCenter = bStatus;
}

void CMsgDialog::OnOK() {
    if ( m_bExistActiveButton ){
        CDialog::OnOK();
    }
}

void CMsgDialog::OnCancel() {
    CDialog::OnCancel();
}

//
int CMsgDialog::GetActiveButtonIndex()
{
    return m_iActiveButtonIndex;
}
void CMsgDialog::SetActiveButtonIndex(int iButtonIndex)
{
    //Default :
    m_iActiveButtonIndex = iButtonIndex;

    //Corrections :
    if( iButtonIndex > (m_iNumButtons-1) ){

        m_iActiveButtonIndex = 0;

    } else {

        if( iButtonIndex < 0 ){

            m_iActiveButtonIndex = (m_iNumButtons-1);
        }
    }

    m_CS_ArrayButtons.GetAt( m_iActiveButtonIndex )->SetFocus();
    m_bExistActiveButton = true;
}

bool CMsgDialog::ExistActiveButton()
{
    return m_bExistActiveButton;
}

int CMsgDialog::NumTextLabels()
{
    return m_pCSAMainText->GetSize();
}

int CMsgDialog::GetDialogWidth()
{
    //El ancho se calcula como el máximo valor entre el espacio necesario para
    //posicionar los botones y el valor estimado que ocupa el texto más largo.
    //Márgenes incluidos.
    m_iTotalButtonsWidth =                          2*m_iLeftMargin
                           +          m_iNumButtons*m_iButtonsWidth
                           + (m_iNumButtons-1)*m_iButtonsSeparation;



    m_iTotalTextsWidth   = 2*m_iLeftMargin + MaxTextLength()*m_iTextLabelFontWidth;

    int iDialogWidth = ( m_iTotalButtonsWidth > m_iTotalTextsWidth ) ? m_iTotalButtonsWidth : m_iTotalTextsWidth;

    return iDialogWidth;
}

void CMsgDialog::ResizeMsgList()
{
    //IDEA : Se llama a ResizeMsgList, para asegurarnos que cada línea es de, a lo sumo,
    //       el máximo número de caracteres permitidos por línea.

    CString CSMsgLine;
    CString CSAux_1;
    CString CSAux_2;

    bool bStop =false;
    int iNumLines  = m_pCSAMainText->GetSize();
    for( int i=0; i<iNumLines /*puede ir aumentando*/ && !bStop; i++){

        CSMsgLine = m_pCSAMainText->GetAt(i);
        if( CSMsgLine.GetLength() > m_iMaxTextLengthAllowed ){

            //hacemos crecer la lista en un elemento :
            m_pCSAMainText->Add(_T(""));

            iNumLines++;


            for( int j = (iNumLines-1); j>i; j--){
                m_pCSAMainText->SetAt(j, m_pCSAMainText->GetAt(j-1) );
            }

            CSAux_1 = CString(_T(""));
            CSAux_2 = CString(_T(""));
            m_pCSAMainText->SetAt(i,CString(_T("")));


            //
            int iLen = CSMsgLine.GetLength();
            int iMaxSeparatorIndex = -1;
            for( int k=0; (k<iLen) && (k<m_iMaxTextLengthAllowed); k++){

                if( Is_Separator(CSMsgLine,k) ) iMaxSeparatorIndex = k;
            }

            //

            if( iMaxSeparatorIndex>0){

                for( int l = 0; l<iLen; l++){

                    if( (l<m_iMaxTextLengthAllowed) && (l<iMaxSeparatorIndex) ){

                        CSAux_1 = CSAux_1 + CSMsgLine.GetAt(l);

                    } else {

                        CSAux_2 = CSAux_2 + CSMsgLine.GetAt(l);
                    }
                }

            } else {

                for( int l = 0; l<iLen; l++){

                    if( l<m_iMaxTextLengthAllowed){

                        CSAux_1 = CSAux_1 + CSMsgLine.GetAt(l);

                    } else {

                        CSAux_2 = CSAux_2 + CSMsgLine.GetAt(l);
                    }
                }
            }

            //Extracción de blancos desde el inicio de CSAux_2 :
            int     iAux_2_Length = CSAux_2.GetLength();
            int m = 0;
            while( m<iAux_2_Length && (CString) CSAux_2.GetAt(m)==CString(_T(" "))  ) m++;
            CString CSAux_3;
            CSAux_3 = CString(_T(""));
            for( int n = m; n<iAux_2_Length; n++) CSAux_3 = CSAux_3 + (CString) CSAux_2.GetAt(n);

            //
            m_pCSAMainText->SetAt(i  , CSAux_1);
            m_pCSAMainText->SetAt(i+1, CSAux_3);



            //si eventualmente CSAux_1 es un CString vacio , significa que es token unico, y que
            //desgraciadamente excede las dimensiones permitidas => nada se puede hacer => nos quedamos con la
            //copia previa de m_pCSAMainText, y abortamos el bucle "for".
            if( CSAux_1==CString(_T("")) ){

                bStop = true;
                m_bNothingMoreToDoWithTexts = true;
            }
        }
    }

}

int CMsgDialog::MaxTextLength()
{

    int iMaxLen=0;
    int iTextLen;
    int iNumTextLabels = NumTextLabels();
    for( int i=0; i<iNumTextLabels; i++){

        iTextLen = m_pCSAMainText->GetAt( i ).GetLength();
        if( iTextLen > iMaxLen ) iMaxLen = iTextLen;
    }
    return iMaxLen;
}

bool CMsgDialog::Is_Separator(CString CSLine, int iZeroBasedIndex)
{
    //devuelve verdadero si en el índice iZeroBasedIndex de la línea CSLine, existe un separador

    int iLine_Length = CSLine.GetLength();
    if( 0<=iZeroBasedIndex && iZeroBasedIndex<iLine_Length){

        CString CSChar = CSLine.GetAt( iZeroBasedIndex );

        if( CSChar==_T(" ") || CSChar==_T("\t") ) return true;
            else return false;

    } else {

        AfxMessageBox(_T("WARNING!!! ..Out of Range Index at CCS_MessageBox::Is_Separator(CString,int) F.A.B.N."));
        return false;
    }
}

int  CMsgDialog::GetAditionalFontHeigth( CString CSFontName )
{
    int iAditonalHeigth = 0;

    if( CSFontName==CString(_T("Courier New")) ){

        iAditonalHeigth = 1;
    }


    return iAditonalHeigth;

}

int CMsgDialog::GetOption(CString csTitle, CString csMsg, ...)
{
    std::vector<CString> buttons;

    va_list parg;
    va_start(parg, csMsg);

    const TCHAR* pszButtonText;
    while( ( pszButtonText = (const TCHAR*)va_arg( parg, const TCHAR* ) ) != nullptr )
        buttons.push_back(pszButtonText);

    va_end(parg);

    // The user choose the apropiate action
    CMsgOptions cMsgOptions(csTitle, csMsg, MB_OK, -1, -1, buttons);
    CMsgDialog cMsgDialog(cMsgOptions, AfxGetMainWnd());

    cMsgDialog.DoModal();
    return cMsgDialog.GetLastPressedButtonIndex();
}


// JH 7/05 - set character set for font, see CreateFont for valid values
void CMsgDialog::SetMessageFont(LPCTSTR sFontName, BYTE charSet)
{
    m_CSMessageFontName = sFontName;
    m_msgFontCharSet = charSet;
}
