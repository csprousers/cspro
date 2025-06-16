#pragma once

#include <zEngineO/zEngineO.h>
#include <zLogicO/Symbol.h>

class IMapUI;
class UserFunctionArgumentEvaluator;


class ZENGINEO_API LogicMap : public Symbol
{
private:
    LogicMap(const LogicMap& logic_map);

public:
    LogicMap(std::string map_name, const EngineData& engine_data);
    ~LogicMap();

    IMapUI* GetMapUI();

    bool IsShowing() const          { return m_showing; }
    void SetIsShowing(bool showing) { m_showing = showing; }

    UserFunctionArgumentEvaluator& GetCallback(size_t index) const { return *m_callbacks[index]; }
    int AddCallback(std::shared_ptr<UserFunctionArgumentEvaluator> argument_evaluator);

    int GetOnClickCallbackId() const     { return m_onClickMapCallbackId; }
    void SetOnClickCallbackId(int index) { m_onClickMapCallbackId = index; }

    double GetLastClickLatitude() const  { return m_lastClickLatitude; }
    double GetLastClickLongitude() const { return m_lastClickLongitude; }

    void SetLastOnClick(double latitude, double longitude);

    // Symbol overrides
    std::unique_ptr<Symbol> CloneInInitialState() const override;

    void Reset() override;

private:
    const EngineData& m_engineData;
    std::unique_ptr<IMapUI> m_mapUI;
    bool m_showing;
    std::vector<std::shared_ptr<UserFunctionArgumentEvaluator>> m_callbacks;
    int m_onClickMapCallbackId;
    double m_lastClickLatitude;
    double m_lastClickLongitude;
};
