#pragma once

#include <zFormF/QuestionTextProperties.h>
#include <zUtilF/DialogValidators.h>


class QuestionTextPropertiesDlg : public CDialog
{
public:
    QuestionTextPropertiesDlg(const std::optional<CapiText::Format>& default_application_capi_text_format,
                              CWnd* pParent = nullptr);

    const std::optional<CapiText::Format>& GetDefaultApplicationCapiTextFormat() const { return m_defaultApplicationCapiTextFormat; }

    const QuestionTextProperties& GetQuestionTextProperties() const { return m_questionTextProperties; }

protected:
    void DoDataExchange(CDataExchange* pDX) override;

    void OnOK() override;

private:
    std::optional<CapiText::Format> m_defaultApplicationCapiTextFormat;
    QuestionTextProperties m_questionTextProperties;

    RadioEnumHelper<CapiText::Format> m_capiTextFormatRadioEnumHelper;
    RadioEnumHelper<bool> m_errorAnnotationsRadioEnumHelper;
};
