#pragma once

#include <zDesignerF/BuildWnd.h>

class TextEditDoc;
class TextEditView;


class CSDocumentBuildWnd : public BuildWnd
{
protected:
    CLogicCtrl* ActivateDocumentAndGetLogicCtrl(std::variant<const CLogicCtrl*, const std::string*> source_logic_ctrl_or_file_path) override;

private:
    TextEditView* FindTextEditView(const std::variant<const CLogicCtrl*, const std::string*>& source_logic_ctrl_or_file_path);

    TextEditDoc* OpenDocumentOnMessageClick(const std::string& file_path);
};
