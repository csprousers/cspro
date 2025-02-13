#pragma once

#include <zDesignerF/BuildWnd.h>

class TextEditDoc;
class TextEditView;


class CSDocumentBuildWnd : public BuildWnd
{
protected:
    CLogicCtrl* ActivateDocumentAndGetLogicCtrl(std::variant<const CLogicCtrl*, const std::wstring*> source_logic_ctrl_or_filename) override;

private:
    TextEditView* FindTextEditView(const std::variant<const CLogicCtrl*, const std::wstring*>& source_logic_ctrl_or_filename);

    TextEditDoc* OpenDocumentOnMessageClick(const std::wstring& filename);
};
