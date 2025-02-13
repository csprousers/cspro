package gov.census.cspro.maps;

import androidx.lifecycle.LiveData;
import androidx.lifecycle.ViewModel;


public class MapViewModel extends ViewModel
{
    private final LiveData<MapData> m_mapDataLiveData;

    public MapViewModel()
    {
        m_mapDataLiveData = MapManager.getInstance().getCurrentMapData();
    }

    LiveData<MapData> getMapData()
    {
        return m_mapDataLiveData;
    }
}
