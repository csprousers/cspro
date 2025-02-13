#pragma once

/// <summary>
/// Specialized tool bar combo box that is not linked to other toolbar combo-boxes with same command ID
/// </summary>
/// When the selected item or text in a CMFCToolBarComboBoxButton changes the default implementation
/// makes the same update to all CMFCToolBarComboBoxButtons that share the same command ID.
/// This class reimplements the NotifyCommand method and removes the mirroring functionality.
class QSFEditToolBarComboBoxButton : public CMFCToolBarComboBoxButton
{
    DECLARE_SERIAL(QSFEditToolBarComboBoxButton)

public:
    QSFEditToolBarComboBoxButton();
    QSFEditToolBarComboBoxButton(UINT uiID, int iImage, DWORD dwStyle = CBS_DROPDOWNLIST, int iWidth = 0);

    virtual BOOL NotifyCommand(int iNotifyCode);

    void Invalidate();
};

