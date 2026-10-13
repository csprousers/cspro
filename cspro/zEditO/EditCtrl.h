#pragma once

#include <zEditO/zEditO.h>
#include <zEditO/LogicCtrl.h>


class CLASS_DECL_ZEDITO EditCtrl : public LogicCtrl
{
public:
    EditCtrl();

    bool Create(DWORD dwStyle, CWnd* pParentWnd);

    // If not specified, IDR_EDIT is used.
    void SetAccelerators(UINT nId = 0);

protected:
    DECLARE_MESSAGE_MAP()

    BOOL PreTranslateMessage(MSG* pMsg) override;

    void OnEditCut();

    void OnEditCopy();

    void OnEditPaste();
    void OnUpdateEditPaste(CCmdUI* pCmdUI);

    void OnEditClear();

    void OnEditSelectAll();

    void OnEditUndo();
    void OnUpdateEditUndo(CCmdUI* pCmdUI);

    void OnEditRedo();
    void OnUpdateEditRedo(CCmdUI* pCmdUI);

    void OnEditCommentLine();

protected:
    virtual void InitializeControl() = 0;

    virtual bool CanCommentLine() = 0;

private:
    HACCEL m_hAccel;
};
