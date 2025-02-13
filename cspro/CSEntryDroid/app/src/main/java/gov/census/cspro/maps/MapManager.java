package gov.census.cspro.maps;

import android.app.Activity;
import androidx.lifecycle.LiveData;
import androidx.lifecycle.MutableLiveData;
import android.content.Intent;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import java.util.ArrayList;

import gov.census.cspro.engine.Messenger;
import timber.log.Timber;

/**
 *  Manages the currently displayed map and launching of MapsActivity.
 *  To enable nested calls to map.show() in CSPro logic we keep a stack maps that are in the show
 *  state. Only the last one shown is displayed in the MapsActivity. The last one shown will be
 *  on top of the stack. When map.show() is called the map is put on top of the stack.
 */
public class MapManager
{
    private static final MapManager ourInstance = new MapManager();
    private ArrayList<MapUI> m_stack = new ArrayList<>();
    private MutableLiveData<MapData> m_currentMapData = new MutableLiveData<>();
    private HideMapCompletedListener m_hideMapCompletedListener;
    private boolean m_waitingForEvent;

    public static MapManager getInstance()
    {
        return ourInstance;
    }

    private MapManager()
    {
    }

    LiveData<MapData> getCurrentMapData()
    {
        return m_currentMapData;
    }

    /**
     * Show map.
     * Will start MapsActivity if no map is currently showing.
     */
    public void showMap(@NonNull MapUI map, Activity currentActivity)
    {
        boolean needToStartActivity = m_stack.isEmpty();

        m_stack.remove(map); // Just in case someone calls show on a map that is already showing

        m_stack.add(map);

        // Update current map live data so listening activity is updated
        //noinspection ConstantConditions
        m_currentMapData.setValue(top().getMapData());

        if (needToStartActivity) {
            // Current activity is not map, start activity
            Intent intent = new Intent(currentActivity, MapsActivity.class);
            currentActivity.startActivity(intent);
        }
    }

    void onActivityStoppped()
    {
        if (m_hideMapCompletedListener != null) {
            m_hideMapCompletedListener.onMapHidden();
            m_hideMapCompletedListener = null;
        }
    }

    @Nullable MapUI currentMap()
    {
        return top();
    }

    public interface HideMapCompletedListener {

        void onMapHidden();
    }

    /**
     * Remove a map from stack
     * Updates current map.
     * Ends MapsActivity if no other map is showing.
     */
    public void hideMap(MapUI map, Activity currentActivity, HideMapCompletedListener listener)
    {
        MapUI oldTop = top();
        m_stack.remove(map);
        MapUI newTop = top();
        if (newTop == oldTop) {
            // no change
            listener.onMapHidden();
        } else {
            if (newTop == null)
            {
                // No more maps, end the activity
                MapsActivity mapsActivity = (MapsActivity) currentActivity;
                m_hideMapCompletedListener = listener;
                mapsActivity.finish();
            } else
            {
                // Top map was hidden, show new top
                m_currentMapData.setValue(newTop.getMapData());
                listener.onMapHidden();
            }
        }
    }

    public void update(@NonNull MapUI map)
    {
        if (map == top()) {
            m_currentMapData.setValue(map.getMapData());
        }
    }

    private @Nullable MapUI top()
    {
        return m_stack.isEmpty() ? null : m_stack.get(m_stack.size() - 1);
    }

    public void waitForEvent()
    {
        Timber.d("Wait for event");
        m_waitingForEvent = true;
    }

    void onMapEvent(MapEvent event)
    {
        Timber.d("onMapEvent");
        // Don't send events while still processing an earlier event
        // or it will mess up the engine which will could try to interpret a MapEvent
        // as the response from some other engine function.
        if (m_waitingForEvent)
        {
            m_waitingForEvent = false;
            Messenger.getInstance().engineFunctionComplete(event);
        }
    }

    public void saveSnapshot(@NonNull MapUI map, Activity currentActivity, @NonNull String imagePath)
    {
        if( m_stack.isEmpty() || top() != map )
        {
            Messenger.getInstance().engineFunctionComplete("The map must be showing before you can save a snapshot.");
        }

        else
        {
            MapsActivity mapsActivity = (MapsActivity) currentActivity;
            mapsActivity.saveSnapshot(imagePath);
        }
    }
}
