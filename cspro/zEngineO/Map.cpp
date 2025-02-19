#include "stdafx.h"
#include "Map.h"
#include "UserFunction.h"
#include <zMapping/IMapUI.h>
#include <zEngineF/EngineUI.h>


// --------------------------------------------------------------------------
// LogicMap
// --------------------------------------------------------------------------

LogicMap::LogicMap(std::string map_name)
    :   Symbol(std::move(map_name), SymbolType::Map),
        m_showing(false),
        m_onClickMapCallbackId(-1),
        m_lastClickLatitude(NOTAPPL),
        m_lastClickLongitude(NOTAPPL)
{
}


LogicMap::LogicMap(const LogicMap& logic_map)
    :   Symbol(logic_map),
        m_showing(false),
        m_onClickMapCallbackId(-1),
        m_lastClickLatitude(NOTAPPL),
        m_lastClickLongitude(NOTAPPL)
{
}


LogicMap::~LogicMap()
{
}


std::unique_ptr<Symbol> LogicMap::CloneInInitialState() const
{
    return std::unique_ptr<LogicMap>(new LogicMap(*this));
}


void LogicMap::Reset()
{
    m_showing = false;
    m_callbacks.clear();
    m_onClickMapCallbackId = -1;
    m_lastClickLatitude = NOTAPPL;
    m_lastClickLongitude = NOTAPPL;

    if( m_mapUI != nullptr )
        m_mapUI->Clear();
}


IMapUI* LogicMap::GetMapUI()
{
    if( m_mapUI == nullptr )
        SendEngineUIMessage(EngineUI::Type::CreateMapUI, m_mapUI);

    return m_mapUI.get();
}


int LogicMap::AddCallback(std::shared_ptr<UserFunctionArgumentEvaluator> argument_evaluator)
{
    ASSERT(argument_evaluator != nullptr);
    m_callbacks.emplace_back(std::move(argument_evaluator));
    return m_callbacks.size() - 1;
}


void LogicMap::SetLastOnClick(const double latitude, const double longitude)
{
    m_lastClickLatitude = latitude;
    m_lastClickLongitude = longitude;
}
