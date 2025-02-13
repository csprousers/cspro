#pragma once

// an subclass of the tree control where pressing escape will move to the parent

class CFreqDDTreeCtrl : public CTreeCtrl
{
public:

    BOOL PreTranslateMessage(MSG* pMsg) override
    {
        if( pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_ESCAPE )
        {
            HTREEITEM hItem = GetSelectedItem();
            hItem = GetParentItem(hItem);
            if (hItem != NULL) {
                SelectItem(hItem);
            }
            return TRUE;
        }

        return CTreeCtrl::PreTranslateMessage(pMsg);
    }
};
