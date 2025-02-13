#pragma once

#include <zEdit2O/zEdit2O.h>
#include <zEdit2O/LogicCtrl.h>


class CLASS_DECL_ZEDIT2O EditCtrl : public CLogicCtrl
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

    void OnEditDeleteLine();

    void OnEditDuplicateLine();

    void OnEditCommentLine();

protected:
    virtual void InitializeControl() = 0;

    virtual bool CanCommentLine() = 0;

private:
    HACCEL m_hAccel;
};
