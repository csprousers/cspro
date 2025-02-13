#pragma once
// MsgDial.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CMsgDialog dialog

#include <zUtilF/zUtilF.h>
#include <zUtilF/CSButton.h>
#include <zUtilF/MsgOpt.h>
#include "afxtempl.h"


class CLASS_DECL_ZUTILF CMsgDialog : public CDialog
{
// Construction
public:

    //Constructors :
    CMsgDialog(const CMsgOptions& msg_options, CWnd* pParent=NULL );

    //Destructor :
    ~CMsgDialog();

    //Assignment :
    void SetActiveButtonIndex            ( int    iZeroBasedButtonIndex    );  //Setting of the button that will have focus.
    void SetButtonsHorizontalCenterStatus( bool   bStatus                  );  //bStatus == true => horizontal alignment of buttons / false => left alignment.
    void SetDefaultDialogIcon            ( HICON  iDefaultDialogIcon       );  //Setting of the default dialog icon
    void SetWindowParent                 ( CWnd * pWindowParent            );  //Set the parent of the MsgDialog, for centering purposses
    void SetTextTopMargin                ( int    iTextTopMargin           );  //Set the margin of the first text label from the upper border, under the title.
    void SetTextSeparationToButtons      ( int    iTextSeparationToButtons );  //Set the vertical separation from the last text label , to the upper border of any button.
    void SetTextLabelFontHeigth          ( int    iTextLabelFontHeigth     );  //Set the heigth of the labels font.
    void SetTextLabelFontWidth           ( int    iTextLabelFontWidth      );  //Set the width of the labels font.
    void SetTextVerticalSeparation       ( int    iTextVerticalSeparation  );  //Set separation from the bottom border of the CStatic that contain some text label, to the upper border of the same CStatic for the next text line.
    void SetButtonsSeparation            ( int    iButtonsSeparation       );  //Set the horizontal space taken between two buttons.
    void SetLeftMargin                   ( int    iLeftMargin              );  //Set shortest left (and simetricaly rigth) margin that any label and any button must respect from near dialog border
    void SetButtonsHeigth                ( int    iButtonsHeigth           );  //Set the heigth of the rectangle that contain a button.
    void SetBottomMargin                 ( int    iBottomMargin            );  //Set the margin taken from the bottom border of any button, to bottom border of the MsgDialog.
    void SetButtonFontWidth              ( int    iButtonFontWidth         );  //Set the width of button's font.
    void SetButtonFontHeigth             ( int    iButtonFontHeigth        );  //Set the heigth of button's font.
    void SetMessageFont                  ( LPCTSTR sFontName, BYTE   charSet);  // JH 7/05 - set name and character set for font, see CreateFont for valid values
    //Actions
    void PressButton( int iPressedButton );  //Only called by any CCSButton child

    static int GetOption(CString csTitle, CString csMsg, ...);


    //Extraction :
    int  GetActiveButtonIndex();      //Zero based index of the button that has focus.
    bool ExistActiveButton();         //true when some button has focus.
    int  GetLastPressedButtonIndex(); //It retrieves the last pressed button after exit the DoModal call, or default return value if the user cancel whith the rigth upper cross.
    int  NumTextLabels();             //Retrieves the number of text labels in the pCSAMainText CStringArray. Maybe, it can be different for the initial pCSAMainText.GetSize() depending by the acceptance of the max width of labels.


    // Implementation
protected:
    // Generated message map functions
    //{{AFX_MSG(CMsgDialog)
    BOOL OnInitDialog() override;
    afx_msg void OnClose();
    afx_msg void OnOK();
    afx_msg void OnCancel();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()


private:

    void    ResizeMsgList();
    int     MaxTextLength();
    bool    Is_Separator(CString CSLine, int iZeroBasedIndex);

    int     m_iTotalButtonsWidth;
    int     m_iTotalTextsWidth;
    int     m_iMaxWidthAllowed;
    bool    m_bNothingMoreToDoWithTexts;
    int     m_iMaxTextLengthAllowed;
    int     m_iMessageFontHeigth;
    int     m_iMessageFontWidth;
    CString m_CSMessageFontName;
    BYTE    m_msgFontCharSet;  // character set for message display (see CreateFont for legal values) JH 7/05
    int     GetDialogWidth();

    int  GetAditionalFontHeigth( CString CSFontName );

    void InitFonts(bool userMessage = false);

    //
    void CloseDialog();
    void Clean();
    void DisplayTexts();
    void DisplayButtons();

    //Posici�n inicial :
    int m_iXo;
    int m_iYo;

    //Dimensiones :
    int m_iWidth;
    int m_iHeigth;

    //T�tulo :
    CString m_CSTittle;
    int     m_iTittleHeigth;

    //Textos :
    CStringArray* m_pCSAMainText;
    CArray< CStatic*, CStatic* > m_CS_ArrayTexts;

    int    m_iTextLabelsMargin;
    int    m_iTextTopMargin;
    int    m_iTextLabelFontHeigth;
    int    m_iTextVerticalSeparation;
    int    m_iTextLabelFontWidth;
    int    m_iTextSeparationToButtons;
    int    m_iTextBottomCoordinate;
    int    m_iMaxTextLength;

    //Botones :
    std::vector<CString> m_pCSButtonsLabels;

    CArray< CCSButton* , CCSButton*> m_CS_ArrayButtons;
    int            m_iNumButtons;
    int            m_iButtonsSeparation;
    int            m_iBottomMargin;

    int            m_iButtonsHeigth;
    int            m_iLastPressedButton;
    int            m_iButtonsWidth;

    int            m_iMaxButtonTextLength;

    int            m_iButtonFontWidth;
    int            m_iButtonFontHeigth;

    int            m_iDefaultButtonFocus;
    bool           m_bButtonsHorizontalCenter;

    //
    int            m_iLeftMargin;

    HICON          m_DefaultDialogIcon;
    bool           m_bExistDefaultDialogIcon;
    bool           m_bExistActiveButton;
    int            m_iActiveButtonIndex;

    const CMsgOptions& m_messageOptions;

    //Fonts :
    CFont * m_pButtonsFont;
    CFont * m_pMessageFont;

    //Contenedor :
    CWnd* m_pWindowParent;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.
