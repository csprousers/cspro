#pragma once

#include <zUtilF/zUtilF.h>
#include <zHtml/CSHtmlDlgRunner.h>


class CLASS_DECL_ZUTILF NoteEditDlg : public CSHtmlDlgRunner
{
public:
    NoteEditDlg(std::wstring title, std::wstring note);

    const std::wstring& GetNote() const { return m_note; }

protected:
    const TCHAR* GetDialogName() override;
    std::wstring GetJsonArgumentsText() override;
    void ProcessJsonResults(const JsonNode<wchar_t>& json_results) override;

private:
    std::wstring m_title;
    std::wstring m_note;
};

