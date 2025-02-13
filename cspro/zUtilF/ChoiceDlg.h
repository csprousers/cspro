#pragma once

#include <zUtilF/zUtilF.h>
#include <zHtml/CSHtmlDlgRunner.h>


class CLASS_DECL_ZUTILF ChoiceDlg : public CSHtmlDlgRunner
{
public:
    ChoiceDlg(int starting_choice_index);

    void SetTitle(std::wstring title) { m_title = std::move(title); }

    int AddChoice(std::wstring choice)
    {
        int choice_index = m_startingChoiceIndex + (int)m_choices.size();
        m_choices.emplace_back(std::move(choice));
        return choice_index;
    }

    void SetChoices(std::vector<std::wstring> choices) { m_choices = std::move(choices); }

    void SetDefaultChoiceIndex(int default_choice_index) { m_defaultChoiceIndex = default_choice_index; }

    int GetSelectedChoiceIndex() const { return m_selectedChoiceIndex; }

    const std::wstring& GetSelectedChoiceText() const;

protected:
    const TCHAR* GetDialogName() override;
    std::wstring GetJsonArgumentsText() override;
    void ProcessJsonResults(const JsonNode<wchar_t>& json_results) override;

private:
    std::wstring m_title;
    std::vector<std::wstring> m_choices;
    int m_startingChoiceIndex;
    std::optional<int> m_defaultChoiceIndex;
    int m_selectedChoiceIndex;
};
