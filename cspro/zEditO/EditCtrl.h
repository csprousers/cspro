#pragma once

#include <zEditO/zEditO.h>
#include <zEditO/LogicCtrl.h>


class CLASS_DECL_ZEDITO EditCtrl : public CLogicCtrl
{
public:
    EditCtrl();

    bool Create(DWORD dwStyle, CWnd* pParentWnd);

    void SetAccelerators(UINT nId);

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
