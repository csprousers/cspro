#include "StdAfx.h"
#include "QSFEditNumericSortToolBarComboBoxButton.h"

IMPLEMENT_SERIAL(QSFEditNumericSortToolBarComboBoxButton, QSFEditToolBarComboBoxButton, 1)

QSFEditNumericSortToolBarComboBoxButton::QSFEditNumericSortToolBarComboBoxButton()
{
}

QSFEditNumericSortToolBarComboBoxButton::QSFEditNumericSortToolBarComboBoxButton(UINT uiID, int iImage, DWORD dwStyle, int iWidth)
    : QSFEditToolBarComboBoxButton(uiID, iImage, dwStyle, iWidth)
{
}

int QSFEditNumericSortToolBarComboBoxButton::Compare(LPCTSTR lpszItem1, LPCTSTR lpszItem2)
{
    return _ttoi(lpszItem1) - _ttoi(lpszItem2);
}
