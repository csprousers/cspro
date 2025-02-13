package gov.census.cspro.engine.functions;

import android.app.Activity;

import gov.census.cspro.engine.Messenger;
import gov.census.cspro.maps.MapManager;
import gov.census.cspro.maps.MapUI;

public class MapShowFunction implements EngineFunction
{
    private final MapUI m_map;

    public MapShowFunction(MapUI mapUI)
    {
        m_map = mapUI;
    }

    @Override
    public void runEngineFunction(Activity activity)
    {
        MapManager.getInstance().showMap(m_map, activity);
        Messenger.getInstance().engineFunctionComplete(1);
    }
}
