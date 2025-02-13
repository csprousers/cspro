#pragma once

#include <zDesignerF/zDesignerF.h>
#include <zUToolO/oxtbvw.h>
#include <zEdit2O/MessageEditCtrl.h>
#include <zDesignerF/ApplicationChildWnd.h>
#include <zUtilO/TextSourceEditable.h>


class CLASS_DECL_ZDESIGNERF MessageEditTabViewPage : public COXTabViewPage<MessageEditCtrl>, public TextSourceEditable::SourceModifier
{
public:
    MessageEditTabViewPage(ApplicationChildWnd* application_child_wnd);
    ~MessageEditTabViewPage();

    void OnTabChange();

protected:
    void SetModified(bool modified = true) override;

    void SyncTextSource() override;
    void OnTextSourceSave() override;

private:
    ApplicationChildWnd* m_applicationChildWnd;
    std::shared_ptr<TextSourceEditable> m_textSourceEditable;
};
