// CRunAplE.cpp: implementation of the CCapiRunAplEntry class.
//
//////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "CRunAplE.h"
#include <zUtilO/AppLdr.h>
#include <zFormO/FormFile.h>
#include <zFormO/Roster.h>
#include <Zentryo/Runaple.h>
#include "TItmInfo.h"
#include "CEUtils.h"
#include <zToolsO/Tools.h>
#include <ZBRIDGEO/npff.h>

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]= __FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CCapiRunAplEntry::CCapiRunAplEntry(CRunAplEntry* pRunAplEntry, CNPifFile* pPifFile, CDEItemBase* pCurItemBase)
{
    m_pRunAplEntry  = pRunAplEntry;
    m_pCPifFile     = pPifFile;
}

CCapiRunAplEntry::~CCapiRunAplEntry()
{
}

//
int CCapiRunAplEntry::GetNNodes()
{
    ASSERT(m_pRunAplEntry);

    //This restriction could be removed (engine)
    if(m_pRunAplEntry->InEnterMode()){
        ASSERT(m_pRunAplEntry->GetFormFileInProcess()->GetNumLevels()==1);
        return 1;
    }

    Case* pCasetainer = &m_pRunAplEntry->GetInputCase();
    Pre74_Case* pCase = pCasetainer->GetPre74_Case();

    return pCase->GetNumberNodes();
}

void CCapiRunAplEntry::GetVal( CDEField* pField, const int iOcc, CIMSAString& sData, int* iStatus, bool bFormatValue, COLORREF* pBkColor ){

    if(!GetRunAplEntry())
        return;

    CRunAplEntry* pRunApl = GetRunAplEntry();
    const CDictItem* pDictItem = pField->GetDictItem();
    int iVar = pDictItem->GetSymbol();
    CONTENT_TYPE_REFACTOR::LOOK_AT();
    bool bNumeric = pField->GetDictItem()->GetContentType() == ContentType::Numeric;

    int iColor = GetRunAplEntry()->GetStatus(iVar, iOcc);

    sData = GetRunAplEntry()->GetVal(iVar, iOcc);

    ASSERT(sData.GetLength() == (int)pDictItem->GetLen());

    if( bFormatValue && bNumeric && pDictItem->GetDecimal() > 0 )
    {
        int iDecPos = pDictItem->GetLen() - pDictItem->GetDecimal() - 1;

        // implicit decimal
        if( !pDictItem->GetDecChar() )
            sData.Insert(iDecPos + 1, _T('.'));

        else if( sData[0] == '*' )
            sData.SetAt(iDecPos, _T('.'));
    }

    ASSERT(sData.GetLength() == (int)pDictItem->GetCompleteLen());

    if( iStatus )
        *iStatus = iColor;

    if( bNumeric ) {

        //if numeric data is retrieved as whites => empty string.
        if( SO::IsBlank(sData) ){
            sData.Empty();

        //if numeric with decimal data is retrieved as whites.whites => empty string.
        } else if( pField->GetDictItem()->GetDecimal()>0 ){

            int i = sData.Find(_T("."));
            if( i!=-1 ){
                CString csIntPart = (i>0)                    ? sData.Left(i)  : _T("");
                CString csDecPart = (i+1)< sData.GetLength() ? sData.Mid(i+1) : _T("");
                if( CCapiEUtils::StripValue(csIntPart,true).IsEmpty() && CCapiEUtils::StripValue(csDecPart,true).IsEmpty() ){
                    sData.Empty();
                }
            }
        }
    }
    if( pBkColor ){
        /*
        "Black"         , RGB( 0x00, 0x00, 0x00));
        "Dark Grey"     , RGB( 0x40, 0x40, 0x40 ));
        "Grey"          , RGB( 0x80, 0x80, 0x80 ));
        "Light Grey"    , RGB( 0xC0, 0xC0, 0xC0 ));
        "White"         , RGB( 0xFF, 0xFF, 0xFF ));
        "Navy"          , RGB( 0x00, 0x00, 0x80 ));
        "Blue"          , RGB( 0x00, 0x00, 0xFF ));
        "Aqua"          , RGB( 0x00, 0xDD, 0xDD ));
        "Cyan"          , RGB( 0x00, 0xFF, 0xFF ));
        "Teal"          , RGB( 0x40, 0x80, 0x80 ));
        "Dark Green"    , RGB( 0x00, 0x40, 0x00 ));
        "Green"         , RGB( 0x00, 0x80, 0x00 ));
        "Lime"          , RGB( 0x00, 0xFF, 0x00 ));
        "Olive"         , RGB( 0x66, 0x66, 0x00 ));
        "Khaki"         , RGB( 0xCC, 0xCC, 0x00 ));
        "Brown"         , RGB( 0x80, 0x40, 0x40 ));
        "Purple"        , RGB( 0x99, 0x66, 0xCC ));
        "Red"           , RGB( 0xFF, 0x00, 0x00 ));
        "Magenta"       , RGB( 0xFF, 0x00, 0xFF ));
        "Maroon"        , RGB( 0x80, 0x00, 0x00 ));
        "Fushcia"       , RGB( 0xFF, 0x00, 0xFF ));
        "Yellow"        , RGB( 0xFF, 0xFF, 0x00 ));
        "Light Yellow"  , RGB( 0xFF, 0xFF, 0x40 ));
        "Pale Yellow"   , RGB( 0xFF, 0xFF, 0x80 ));
        */
        int iColorAux=MY_WHITE_COLOR;
        if( pField->IsProtected() )
            iColorAux = MY_PROTECTED_COLOR;

        else if( pField->IsMirror() )
            iColorAux = MY_PROTECTED_COLOR;

        else if( pField->IsPersistent() || pField->IsAutoIncrement() )
            iColorAux = MY_GRAY_COLOR;

        else {
            switch( iColor /*from getval*/){
                case 0  : iColorAux = MY_WHITE_COLOR; break;
                case 1  : iColorAux = pRunApl && pRunApl->IsPathOn() ? MY_GRAY_COLOR : MY_YELLOW_COLOR; break;
                case 2  : iColorAux = MY_GREEN_COLOR; break;
                default : iColorAux = MY_WHITE_COLOR; break;
            }
        }
        COLORREF& rgbColor = *pBkColor;
        GetFieldColorRGB( iColorAux, rgbColor );
    }
}

void CCapiRunAplEntry::GetFieldColorRGB(int iColor, COLORREF& rgbColor ){
    COLORREF& bkColor = rgbColor;
    switch( iColor ){
        case MY_PROTECTED_COLOR :   bkColor = RGB( 172, 168, 153 ); break;  // light gray (protected fields)
        case MY_GRAY_COLOR      :   bkColor = RGB( 0x80, 0x80, 0x80 ); break;
        case MY_WHITE_COLOR     :   bkColor = RGB( 0xFF, 0xFF, 0xFF ); break;
        case MY_GREEN_COLOR     :   bkColor = RGB(128,255,128); break;
        default : rgbColor = RGB(0xFF, 0xFF, 0xFF); /*white*/ ; break;
    }
}


void CCapiRunAplEntry::GetInfo(bool          bVerified,
                               CDEItemBase*  pItem,                 //input.
                               int           iOccurrenceIndex,      //input.
                               int           iMaxItemLabelLength,   //input.
                               bool          bShortRepresentation,  //input.
                               CTreeItemInfo* pTreeItemInfo,       //input. (si es null ==> se ignora )
                               int         * iNonSelectedIconIndex, //output.
                               int         * iSelectedIconIndex,    //output.
                               int         * iIconColor,            //output.
                               CString     * CSFieldLabel,
                               CString     * CSToolTip)          //output.
{
    CString csToolTip=_T("");

    if( pTreeItemInfo!=NULL && pTreeItemInfo->GetTypeOfInfo()==CS_LEVEL_INFO ){

        if( bShortRepresentation ){

            *CSFieldLabel = pTreeItemInfo->GetLevel()->GetName();
            csToolTip         = pTreeItemInfo->GetLevel()->GetLabel();

        } else {

            *CSFieldLabel = pTreeItemInfo->GetLevel()->GetLabel();
            csToolTip         = pTreeItemInfo->GetLevel()->GetName();
        }


    } else {

        *CSFieldLabel = StringRepresentation( pItem, bShortRepresentation );
    }


    CDEFormBase::eItemType eType;
    if( pTreeItemInfo!=NULL && pTreeItemInfo->GetTypeOfInfo()==CS_LEVEL_INFO ) eType = CDEFormBase::Group;
    else eType = pItem->GetItemType();

    if( eType== CDEFormBase::Field ){ //entonces alteramos CSFieldLabel adjuntándole el valor de la respuesta.

        CDEField*   pField = (CDEField*) pItem;
        int         iColor = 0;
        //

        CIMSAString     csData;

        CCapiRunAplEntry*   pCapiRunAplE    = this;
        if(pCapiRunAplE){
            pCapiRunAplE->GetVal( pField, iOccurrenceIndex , csData, &iColor, true );
        }

        if(bShortRepresentation){
            csToolTip = pField->GetDictItem()->GetLabel();
        } else {
            csToolTip = pField->GetDictItem()->GetName();
        }

        csToolTip.TrimRight();
        csToolTip = csToolTip + _T(" (")+IntToString(iOccurrenceIndex)+_T(")");
        csToolTip = csToolTip + _T(" : ");



        //---------------Extracción del Label que vamos a mostrar en el árbol------//
        if( iMaxItemLabelLength > 0){

            CString csAux = *CSFieldLabel;
            int iCurrentLabelLength = (*CSFieldLabel).GetLength();
            for( int i=0; i<(iMaxItemLabelLength - iCurrentLabelLength); i++ ){

                csAux = csAux + _T(".");
            }
            *CSFieldLabel = csAux;

        }

        (*CSFieldLabel).TrimRight();
        int iFieldLabelLength = (*CSFieldLabel).GetLength();
        if(iFieldLabelLength){
            if( (*CSFieldLabel).GetAt( (*CSFieldLabel).GetLength()-1 )!=':' ){
                *CSFieldLabel = *CSFieldLabel + _T(" : ");
            } else {
                *CSFieldLabel = *CSFieldLabel + _T(" ");
            }
        }

        bool    bVisible            = !pField->IsHidden();
        bool    bInVerifyMode       = InVerify();
        bool    bShowData           = bVisible && (!bInVerifyMode || bVerified );

        if( bShowData && !SO::IsBlank(csData) ){
            CIMSAString csDataAux       = csData;
            csDataAux.TrimRight();
            CONTENT_TYPE_REFACTOR::LOOK_AT();
            CString csStrippedData  = CCapiEUtils::StripValue( csDataAux, pField->GetDictItem()->GetContentType() == ContentType::Numeric);
            *CSFieldLabel           = *CSFieldLabel + csStrippedData;
            csToolTip               = csToolTip + csStrippedData;
        }

        if( pField->IsProtected() ){

            *iNonSelectedIconIndex = CaseTreeIcons::GetIconIndex(IDI_NON_SELECTED_PROTECTED_ICON_3D);
            *iIconColor = MY_PROTECTED_COLOR;

        }
        // BUCEN INIT May 01, 2003
        else if(pField->IsMirror()){
            *iNonSelectedIconIndex = CaseTreeIcons::GetIconIndex(IDI_SELECTED_FIELD_ICON_3D_3);
            *iIconColor = MY_PROTECTED_COLOR;
        }
        else if ( pField->IsPersistent() || pField->IsAutoIncrement() ) {
            *iNonSelectedIconIndex = CaseTreeIcons::GetIconIndex(IDI_NON_SELECTED_FIELD_ICON_3D_3);
             *iIconColor = MY_GRAY_COLOR;

        }
        // BUCEN END May 01, 2003
        else {

            if( iColor<0 || iColor>2 ){

                //AfxMessageBox(_T("WARNING! Color out of range : capturated at FormQuestionsTree::InsertItem (F.A.B.N.)"));
                ASSERT(0);

                iColor = CaseTreeIcons::GetIconIndex(IDI_NON_SELECTED_FIELD_ICON_3D);
                *iIconColor = MY_ERROR_COLOR;

            } else {

                switch( iColor ){

                case 0 : {

                    //Blanco
                    *iNonSelectedIconIndex = CaseTreeIcons::GetIconIndex(IDI_NON_SELECTED_FIELD_ICON_3D_0);
                    *iIconColor            = MY_WHITE_COLOR;

                         } break;

                case 1 : {
                    if( m_pRunAplEntry->IsPathOn() ){

                        //Gris
                        *iNonSelectedIconIndex = CaseTreeIcons::GetIconIndex(IDI_NON_SELECTED_FIELD_ICON_3D_3);
                        *iIconColor            = MY_GRAY_COLOR;

                    } else {

                        //Amarillo
                        *iNonSelectedIconIndex = CaseTreeIcons::GetIconIndex(IDI_NON_SELECTED_FIELD_ICON_3D_1);
                        *iIconColor            = MY_YELLOW_COLOR;
                    }

                         } break;

                case 2 : {

                    //Verde
                    *iNonSelectedIconIndex = CaseTreeIcons::GetIconIndex(IDI_NON_SELECTED_FIELD_ICON_3D_2);
                    *iIconColor            = MY_GREEN_COLOR;

                         } break;

                } //fin del switch.
            } //fin del else referente a iColor.
        } //fin del else referente a si es un pField protegido.
        } //fin caso pItem es un Field.

        else {  //...si pItem no es un Field :

            //------Selección de ícono -----------------------//
            switch( eType ){

            case CDEFormBase::Roster :{ *iNonSelectedIconIndex = CaseTreeIcons::GetIconIndex(IDI_NON_SELECTED_ROSTER_ICON); } break;

            case CDEFormBase::Group  :{ *iNonSelectedIconIndex = CaseTreeIcons::GetIconIndex(IDI_NON_SELECTED_GROUP_ICON_3D); } break;

            case CDEFormBase::Field  :{ /*ya está listo*/           } break;
            }
            //------Selección de ícono -----------------------//

            if(csToolTip==_T("")){
                if(bShortRepresentation){
                    csToolTip = pItem->GetLabel();
                } else {
                    csToolTip = pItem->GetName();
                }
            }
        }

        *iSelectedIconIndex = 1 + *iNonSelectedIconIndex;
        //-FIN--Selección de ícono -----------------------//

        if( csToolTip==_T("") ){
            csToolTip = *CSFieldLabel;
        }
        *CSToolTip  = csToolTip;
}

CString CCapiRunAplEntry::StringRepresentation(CDEItemBase* pItem, bool bShortRepresentation)
{
    CString CStringRepresentation;
    bool    bTypeFounded = true;

    CDEFormBase::eItemType eType = pItem->GetItemType();

    switch( eType ){


    case CDEFormBase::Roster  : {
        CDERoster* pRoster = (CDERoster*) pItem;
        if( bShortRepresentation ) CStringRepresentation = (CString) pRoster->GetName();
        else CStringRepresentation = (CString) pRoster->GetLabel();
                   } break;

    case CDEFormBase::Group   : {        CDEGroup    * pGroup = (CDEGroup*) pItem;
        if( bShortRepresentation ) CStringRepresentation = (CString) pGroup->GetName();
        else CStringRepresentation = (CString) pGroup->GetLabel();
                   } break;

    case CDEFormBase::Field   : {
        CDEField*   pField = (CDEField*) pItem;
        if( bShortRepresentation ) CStringRepresentation = (CString) pField->GetDictItem()->GetName();
        else CStringRepresentation = (CString) pField->GetDictItem()->GetLabel();
                   } break;

    default : { bTypeFounded= false; } break;

    }

    if( bTypeFounded ){
        return CStringRepresentation;
    } else {
        AfxMessageBox(_T("WARNING! There is an unknown pItem type (F.A.B.N. CFormQuestionsTree::StringRepresentation 27-11-00) "));
        return _T(" WARNING! unknown pItem type in CFormQuestionsTree::StringRepresentation 27-11-00 F.A.B.N.");
    }
}


CDEFormFile* CCapiRunAplEntry::GetDEFormFile( bool bPrimaryFlow )
{
    if( m_pRunAplEntry==NULL ){
        return NULL;
    }

    CDEFormFile* pFormFile = bPrimaryFlow ? m_pRunAplEntry->GetPrimaryFormFile() : m_pRunAplEntry->GetFormFileInProcess();
    return       pFormFile;
}

bool CCapiRunAplEntry::InEnterMode()
{
    return m_pRunAplEntry ? m_pRunAplEntry->InEnterMode() : false;
}

CString CCapiRunAplEntry::GetNodeLabel(int iNodeIdx /*zero based*/)
{
    ASSERT(m_pRunAplEntry);

    if(m_pRunAplEntry->InEnterMode()){
        ASSERT(iNodeIdx==0);
        return m_pRunAplEntry->GetCurrentKey(-1);
    }

    Case* pCasetainer = &m_pRunAplEntry->GetInputCase();
    Pre74_Case* pCase = pCasetainer->GetPre74_Case();

    Pre74_CaseLevel* pGivenCaseLevel = pCase->GetCaseLevelAtNodeNumber(iNodeIdx);
    Pre74_CaseLevel* pCurrentCaseLevel = m_pRunAplEntry->GetCaseLevelNode(m_pRunAplEntry->GetCurrentLevel());

    if( pGivenCaseLevel == pCurrentCaseLevel )
        return m_pRunAplEntry->GetCurrentKey(-1);

    int iLevelIdx = GetLevel(iNodeIdx) - 1;

    if( IsOpen(iNodeIdx) )
        return m_pRunAplEntry->GetCurrentKey(1 + iLevelIdx);

    return pCase->GetLevelKey(pGivenCaseLevel,false);
}

bool CCapiRunAplEntry::IsOpen( int iNodeIdx )
{
    ASSERT(m_pRunAplEntry);

    //In the near future this code could be removed (engine matter)
    //INIT
    if(m_pRunAplEntry->InEnterMode()){
        ASSERT( GetNNodes()==1 );
        return TRUE;
    }
    //END

    Case* pCasetainer = &m_pRunAplEntry->GetInputCase();
    Pre74_Case* pCase = pCasetainer->GetPre74_Case();

    //step #1 : obtain the given node
    Pre74_CaseLevel* pGivenCaseLevel = pCase->GetCaseLevelAtNodeNumber(iNodeIdx);
    int iGivenLevel = pGivenCaseLevel->GetLevelNum();

    //step #2 : obtain the current node for the level of the given node
    Pre74_CaseLevel* pCurrentCaseLevel = m_pRunAplEntry->GetCaseLevelNode(iGivenLevel);

    return ( pGivenCaseLevel == pCurrentCaseLevel );
}


int CCapiRunAplEntry::GetLevel( int iNodeIdx )
{
    //In the near future this code could be removed (engine matter)
    //INIT
    if(m_pRunAplEntry->InEnterMode()){
        ASSERT( iNodeIdx==0 );
        return 1;
    }
    //END

    Case* pCasetainer = &m_pRunAplEntry->GetInputCase();
    Pre74_Case* pCase = pCasetainer->GetPre74_Case();

    Pre74_CaseLevel* pGivenCaseLevel = pCase->GetCaseLevelAtNodeNumber(iNodeIdx);
    return pGivenCaseLevel->GetLevelNum();
}


bool CCapiRunAplEntry::InVerify()
{
    return m_pRunAplEntry && (m_pRunAplEntry->GetAppMode()==CRUNAPL_VERIFY);
}


CDEItemBase* CCapiRunAplEntry::GetCurItemBase()
{
    return m_pRunAplEntry ? m_pRunAplEntry->GetCurItemBase() : NULL;
}

CDEField*   CCapiRunAplEntry::GetDeField( DEFLD* pFld )
{
    return m_pRunAplEntry ? m_pRunAplEntry->GetDeField(pFld) : NULL;
}

//DESCRIPTIVE ITEM FUNCTIONS
bool CCapiRunAplEntry::IsMirror(CDEItemBase* pItem, int /*iOcc*/)
{
    if(!pItem){
        return false;
    }
    CDEFormBase::eItemType eType = pItem->GetItemType();
    return eType== CDEFormBase::Field ? ((CDEField*)pItem)->IsMirror() : false;
}

bool CCapiRunAplEntry::IsPersistent(CDEItemBase* pItem, int /*iOcc*/)
{
    if(!pItem){
        return false;
    }
    CDEFormBase::eItemType eType = pItem->GetItemType();
    return eType== CDEFormBase::Field ? ((CDEField*)pItem)->IsPersistent() : false;
}

bool CCapiRunAplEntry::IsProtected(CDEItemBase* pItem, int iOcc)
{
    if(!pItem){
        return false;
    }
    CDEFormBase::eItemType eType = pItem->GetItemType();
    return eType == CDEFormBase::Field ? ((CDEField*)pItem)->IsProtected() : false;
}


CRunAplEntry* CCapiRunAplEntry::GetRunAplEntry()
{
    return m_pRunAplEntry;
}


bool CCapiRunAplEntry::IsItemSelectable( CDEItemBase* pItem, int iOcc ){
    CCapiRunAplEntry* m_pCapiRunAplEntry = this;

    /*avoid titles*/
    if(iOcc==0){
        return true;
    }

    /*selection of protected items*/
    if( m_pCapiRunAplEntry->IsProtected(pItem,iOcc) )
        return false;

    /*selection of mirror items*/
    if( m_pCapiRunAplEntry->IsMirror(pItem,iOcc) )
        return false;

    /*selection of persistent items*/
    if( m_pCapiRunAplEntry->IsPersistent(pItem,iOcc) )
        return false;

    if( InVerify() )
        return false;

    return true;
}
