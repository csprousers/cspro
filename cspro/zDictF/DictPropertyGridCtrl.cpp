#include "StdAfx.h"
#include "DictPropertyGridCtrl.h"
#include "DictPropertyGridDictionaryManager.h"
#include "DictPropertyGridItemManager.h"
#include "DictPropertyGridLevelManager.h"
#include "DictPropertyGridRecordManager.h"
#include "DictPropertyGridValueManager.h"
#include "DictPropertyGridValueSetManager.h"


IMPLEMENT_DYNAMIC(CDictPropertyGridCtrl, CMFCPropertyGridCtrl)


CDictPropertyGridCtrl::CDictPropertyGridCtrl()
    :   m_updateControl(true)
{
}

CDictPropertyGridCtrl::~CDictPropertyGridCtrl()
{
}


void CDictPropertyGridCtrl::Initialize(CDDDoc* pDDDoc, DictBase* dict_base)
{
    if( !m_updateControl )
        return;

    // set the right property manager for this type
    if( dict_base == nullptr )
        m_propertyManager.reset();

    else if( dict_base->GetElementType() == DictElementType::Dictionary )
        m_propertyManager = std::make_unique<DictPropertyGridDictionaryManager>(pDDDoc, assert_cast<CDataDict&>(*dict_base));

    else if( dict_base->GetElementType() == DictElementType::Level )
        m_propertyManager = std::make_unique<DictPropertyGridLevelManager>(pDDDoc, assert_cast<DictLevel&>(*dict_base));

    else if( dict_base->GetElementType() == DictElementType::Record )
        m_propertyManager = std::make_unique<DictPropertyGridRecordManager>(pDDDoc, assert_cast<CDictRecord&>(*dict_base));

    else if( dict_base->GetElementType() == DictElementType::Item )
        m_propertyManager = std::make_unique<DictPropertyGridItemManager>(pDDDoc, assert_cast<CDictItem&>(*dict_base));

    else if( dict_base->GetElementType() == DictElementType::ValueSet )
        m_propertyManager = std::make_unique<DictPropertyGridValueSetManager>(pDDDoc, assert_cast<DictValueSet&>(*dict_base));

    else if( dict_base->GetElementType() == DictElementType::Value)
        m_propertyManager = std::make_unique<DictPropertyGridValueManager>(pDDDoc, assert_cast<DictValue&>(*dict_base));

    else
        ASSERT(false);

    // remove any properties previously displayed and then set up the
    // columns and add the appropriate properties
    SetRedraw(FALSE);

    RemoveAll();

    EnableDescriptionArea(TRUE);
    SetVSDotNetLook(TRUE);

    if( m_propertyManager != nullptr )
    {
        EnableHeaderCtrl(TRUE, _T("Property"), _T("Value"));
        m_propertyManager->SetupProperties(*this);
    }

    else
    {
        EnableHeaderCtrl(TRUE, _T("Property"), _T("Value"));
    }

    // adjusting the layout will ensure that all subitems show automatically expanded
    AdjustLayout();

    SetRedraw(TRUE);
    Invalidate(TRUE);
}


void CDictPropertyGridCtrl::OnPropertyChanged(CMFCPropertyGridProperty* pProp) const
{
    // calling updateallviews of dictionary after property is modified invokes the initialize function of the control while in between updates
    // this causes a crash. Added UpdateControl flag to prevent initialization while updating a property. UpdateAllViews call is required to reflect changes in the
    // property grid control
    m_updateControl = false;

    CMFCPropertyGridCtrl::OnPropertyChanged(pProp);

    m_propertyManager->OnPropertyChanged(pProp);

    m_updateControl = true;
}


void CDictPropertyGridCtrl::OnClickButton(CPoint point)
{
    m_updateControl = false;

    CMFCPropertyGridCtrl::OnClickButton(point);

    if( m_pSel != nullptr )
        m_propertyManager->OnClickButton(m_pSel);

    m_updateControl = true;
}
