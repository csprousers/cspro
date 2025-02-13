#pragma once

#include <zUtilO/DataTypes.h>
#include <zEngineO/UserFunction.h>


// 20100412, for adding a user bar to the top of a data entry screen

class Userbar
{
public:
    enum class Command
    {
        Hide,
        Show,
        Clear,
        Remove,
        Modify,
        SetColor,
        GetField,
        AddText,
        AddButton,
        AddField,
        AddSpacing
    };

    enum class ControlAction
    {
        Next,
        Previous,
        Advance,
        Note,
        Language,
        Save,
        Help,
        InsertLevelOcc,
        AddLevelOcc,
        DeleteLevelOcc,
        InsertGroupOcc,
        InsertGroupOccAfter,
        DeleteGroupOcc,
        SortGroupOcc,
        PreviousForm,
        NextForm,
        EndGroupOcc,
        EndGroup,
        EndLevelOcc,
        EndLevel,
        FullScreen,
        ToggleResponses,
        ToggleAllResponses
    };

    using Action = std::variant<std::unique_ptr<UserFunctionArgumentEvaluator>, ControlAction>;

    struct FeatureNotImplemented : public std::exception { };

protected:
    // this value can be anything other than 0 but is 60000 to 
    // match what it was when the userbar was first programmed
    constexpr static int StartingId = 60000;

public:
    virtual ~Userbar() { }

    virtual void Show() = 0;
    virtual void Hide() = 0;
    virtual void Clear() = 0;

    virtual void Pause(bool pause) = 0;

    virtual int AddButton(std::wstring text, std::optional<Action> action) = 0;
    virtual int AddField(std::wstring text, std::optional<Action> action) = 0;
    virtual int AddText(std::wstring text) = 0;
    virtual int AddSpacing(int spacing) = 0;

    virtual std::optional<std::wstring> GetFieldText(int id) = 0;
    virtual std::optional<int> GetLastActivatedItem() const = 0;

    virtual bool SetColor(COLORREF color, std::optional<int> id) = 0;

    virtual bool Modify(int id, std::optional<std::wstring> text, std::optional<Action> action, std::optional<int> spacing) = 0;

    virtual bool Remove(int id) = 0;
};
