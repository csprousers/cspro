#pragma once

#include <zUtilF/zUtilF.h>
#include <zHtml/CSHtmlDlgRunner.h>


class CLASS_DECL_ZUTILF ChoiceDlg : public CSHtmlDlgRunner
{
public:
    ChoiceDlg(int starting_choice_index);

    void SetTitle(SharableString title) { m_title = std::move(title); }

    template<typename T>
    int AddChoice(T&& choice)
    {
        int choice_index = m_startingChoiceIndex + static_cast<int>(m_choices.size());
        m_choices.emplace_back(std::forward<T>(choice));
        return choice_index;
    }

    void SetChoices(std::vector<SharableString> choices) { m_choices = std::move(choices); }

    void SetDefaultChoiceIndex(int default_choice_index) { m_defaultChoiceIndex = default_choice_index; }

    int GetSelectedChoiceIndex() const { return m_selectedChoiceIndex; }

    const SharableString& GetSelectedChoiceText() const;

protected:
    std::string GetDialogName() override;
    SharableString GetJsonArgumentsText() override;
    void ProcessJsonResults(const JsonNode& json_results) override;

private:
    SharableString m_title;
    std::vector<SharableString> m_choices;
    int m_startingChoiceIndex;
    std::optional<int> m_defaultChoiceIndex;
    int m_selectedChoiceIndex;
};
