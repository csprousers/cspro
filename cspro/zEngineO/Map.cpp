#include "stdafx.h"
#include "Map.h"
#include "UserFunction.h"
#include <zMapping/IMapUI.h>
#include <zEngineF/EngineUI.h>


// --------------------------------------------------------------------------
// LogicMap
// --------------------------------------------------------------------------

LogicMap::LogicMap(std::string map_name, const EngineData& engine_data)
    :   Symbol(std::move(map_name), SymbolType::Map),
        m_engineData(engine_data),
        m_showing(false),
        m_onClickMapCallbackId(-1),
        m_lastClickLatitude(NOTAPPL),
        m_lastClickLongitude(NOTAPPL)
{
}


LogicMap::LogicMap(const LogicMap& logic_map)
    :   Symbol(logic_map),
        m_engineData(logic_map.m_engineData),
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
    {
        if( m_engineData.application == nullptr )
            return ReturnProgrammingError(nullptr);

        EngineUI::CreateMapUINode create_map_ui_node
        {
            m_mapUI,
            &m_engineData.application->GetApplicationProperties().GetMappingProperties()
        };

        SendEngineUIMessage(EngineUI::Type::CreateMapUI, create_map_ui_node);
    }

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
