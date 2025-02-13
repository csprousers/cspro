package gov.census.cspro.engine.functions;

import android.app.Activity;

import gov.census.cspro.engine.Messenger;
import gov.census.cspro.maps.MapManager;
import gov.census.cspro.maps.MapUI;

public class MapHideFunction implements EngineFunction
{
    private final MapUI m_map;

    public MapHideFunction(MapUI map)
    {
        m_map = map;
    }

    @Override
    public void runEngineFunction(Activity activity)
    {
        MapManager.getInstance().hideMap(m_map, activity, new MapManager.HideMapCompletedListener()
        {
            @Override
            public void onMapHidden()
            {
                Messenger.getInstance().engineFunctionComplete(1);
            }
        });
    }
}
