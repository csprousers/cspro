#pragma once

#include <zEngineO/zEngineO.h>
#include <zLogicO/Symbol.h>

struct IMapUI;
class UserFunctionArgumentEvaluator;


class ZENGINEO_API LogicMap : public Symbol
{
private:
    LogicMap(const LogicMap& logic_map);

public:
    LogicMap(std::wstring map_name);
    ~LogicMap();

    IMapUI* GetMapUI();

    bool IsShowing() const          { return m_showing; }
    void SetIsShowing(bool showing) { m_showing = showing; }

    UserFunctionArgumentEvaluator& GetCallback(size_t index) const { return *m_callbacks[index]; }
    int AddCallback(std::shared_ptr<UserFunctionArgumentEvaluator> user_function_argument_evaluator);

    int GetOnClickCallbackId() const     { return m_onClickMapCallbackId; }
    void SetOnClickCallbackId(int index) { m_onClickMapCallbackId = index; }

    double GetLastClickLatitude() const  { return m_lastClickLatitude; }
    double GetLastClickLongitude() const { return m_lastClickLongitude; }

    void SetLastOnClick(double latitude, double longitude)
    {
        m_lastClickLatitude = latitude;
        m_lastClickLongitude = longitude;
    }

    // Symbol overrides
    std::unique_ptr<Symbol> CloneInInitialState() const override;

    void Reset() override;

private:
    std::unique_ptr<IMapUI> m_mapUI;
    bool m_showing;
    std::vector<std::shared_ptr<UserFunctionArgumentEvaluator>> m_callbacks;
    int m_onClickMapCallbackId;
    double m_lastClickLatitude;
    double m_lastClickLongitude;
};
