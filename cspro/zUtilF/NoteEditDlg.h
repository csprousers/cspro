#pragma once

#include <zUtilF/zUtilF.h>
#include <zHtml/CSHtmlDlgRunner.h>


class CLASS_DECL_ZUTILF NoteEditDlg : public CSHtmlDlgRunner
{
public:
    NoteEditDlg(std::string title, SharableString note);

    const SharableString& GetNote() const { return m_note; }

protected:
    std::string GetDialogName() override;
    SharableString GetJsonArgumentsText() override;
    void ProcessJsonResults(const JsonNode& json_results) override;

private:
    const std::string m_title;
    SharableString m_note;
};
