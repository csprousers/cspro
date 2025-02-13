package gov.census.cspro.engine.functions.fragments;

import android.app.Activity;

import gov.census.cspro.engine.functions.EngineFunction;
import gov.census.cspro.maps.MapManager;
import gov.census.cspro.maps.MapUI;

public class MapUpdateFunction implements EngineFunction
{
    private final MapUI m_map;

    public MapUpdateFunction(MapUI mapUI)
    {
        m_map = mapUI;
    }

    @Override
    public void runEngineFunction(Activity activity)
    {
        MapManager.getInstance().update(m_map);
        MapManager.getInstance().waitForEvent();

        // EngineFunctionComplete will be called when event is sent
    }
}
