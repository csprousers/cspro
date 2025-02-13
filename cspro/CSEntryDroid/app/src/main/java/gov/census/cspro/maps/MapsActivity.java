package gov.census.cspro.maps;

import android.content.Intent;
import android.os.Bundle;
import android.text.TextUtils;
import android.view.KeyEvent;
import android.view.View;
import android.view.ViewGroup;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.appcompat.app.AppCompatActivity;
import androidx.appcompat.widget.SearchView;
import androidx.coordinatorlayout.widget.CoordinatorLayout;
import androidx.lifecycle.Observer;
import androidx.lifecycle.ViewModelProvider;

import com.google.android.material.bottomsheet.BottomSheetBehavior;

import java.util.ArrayList;
import java.util.List;
import java.util.regex.Pattern;

import gov.census.cspro.commonui.ErrorDialogFragment;
import gov.census.cspro.csentry.R;
import gov.census.cspro.csentry.ui.EntryActivity;
import gov.census.cspro.engine.EngineInterface;
import gov.census.cspro.engine.Util;
import gov.census.cspro.engine.functions.GPSFunction;

public class MapsActivity extends AppCompatActivity implements
    MapListFragment.OnFragmentInteractionListener,
    IMapEventListener,
    ErrorDialogFragment.OnErrorFragmentDismissed
{
    private static final String SEARCH_TEXT = "SEARCH_TEXT";
    private BottomSheetBehavior m_bottomSheetBehavior;
    private MapViewModel m_viewModel;
    private View m_searchViewContainer;
    private SearchView m_searchView;
    private String m_searchText;

    @Override
    protected void onCreate(Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);

        // If is killed in background while this activity is shown, Android will try to restore
        // directly to this activity but that causes issues since the EntryActivity is not restored
        // to the point where it called to display the map. So just close the map activity so
        // that the EntryActivity is restored at the start of the questionnaire.
        if (EngineInterface.getInstance() == null || !EngineInterface.getInstance().isApplicationOpen())
        {
            finish();
        }

        if (getSupportActionBar() != null)
            getSupportActionBar().hide();

        if (savedInstanceState != null)
        {
            m_searchText = savedInstanceState.getString(SEARCH_TEXT);
        }
        setContentView(R.layout.activity_maps);

        getMapFragment().setEventListener(this);

        // Bottom sheet only exists in portrait
        m_bottomSheetBehavior = getBottomSheetBehavior(findViewById(R.id.place_list));

        m_viewModel = new ViewModelProvider(this).get(MapViewModel.class);

        m_viewModel.getMapData().observe(this, new Observer<MapData>()
        {
            @Override
            public void onChanged(@Nullable MapData mapData)
            {
                updateMap(mapData);
            }
        });

        m_searchView = findViewById(R.id.search_view);
        m_searchView.setOnQueryTextListener(new SearchView.OnQueryTextListener()
        {
            @Override
            public boolean onQueryTextSubmit(String query)
            {
                Util.hideInputMethod(MapsActivity.this);
                updateSearch(query);
                return false;
            }

            @Override
            public boolean onQueryTextChange(String newText)
            {
                return true;
            }
        });
        m_searchView.setOnCloseListener(new SearchView.OnCloseListener()
        {
            @Override
            public boolean onClose()
            {
                clearSearch();
                return false;
            }
        });

        m_searchView.setMaxWidth( Integer.MAX_VALUE );
        m_searchViewContainer = findViewById(R.id.search_container);
        m_searchViewContainer.setVisibility(View.GONE); // Made visible when tapping on search button
    }

    private void updateMap(MapData mapData)
    {
        if (mapData == null || TextUtils.isEmpty(m_searchText)) {
            getMapFragment().setData(mapData);
            updateListMarkers(mapData);
        } else {
            List<MapMarker> filteredMarkers = filterMarkers(mapData.getMarkers(), m_searchText);
            MapData filteredData = new MapData(mapData);
            filteredData.setMarkers(filteredMarkers);
            getMapFragment().setData(filteredData);
            updateListMarkers(filteredData);
        }
    }

    @Override
    public void onStop()
    {
        MapManager.getInstance().onActivityStoppped();
        super.onStop();
    }

    @Override
    public void onListItemClicked(@NonNull MapMarker marker)
    {
        // Collapse the list
        if (m_bottomSheetBehavior != null)
        {
            m_bottomSheetBehavior.setState(BottomSheetBehavior.STATE_COLLAPSED);

            getMapListFragment().showMarker(marker);
        }

        // simulate click on map marker
        getMapFragment().simulateMarkerClick(marker);
    }

    /**
     * Retreive bottom sheet behavior from bottom sheet view
     *
     * @param bottomSheet View that has bottom sheet behavior attached to it in layout
     * @return behavior or null if none is attached
     */
    @Nullable
    private static BottomSheetBehavior getBottomSheetBehavior(@NonNull View bottomSheet)
    {
        // This is adapted from BottomSheetBehavior.from() which does the same
        // checks but throws exceptions if there is no coordinator layout or bottom
        // sheet behavior.
        ViewGroup.LayoutParams params = bottomSheet.getLayoutParams();
        if (params instanceof CoordinatorLayout.LayoutParams)
        {
            CoordinatorLayout.Behavior behavior = ((CoordinatorLayout.LayoutParams) params)
                .getBehavior();
            if (behavior instanceof BottomSheetBehavior)
            {
                return (BottomSheetBehavior) behavior;
            }
        }
        return null;
    }

    private MapFragment getMapFragment()
    {
        return (MapFragment) getSupportFragmentManager().findFragmentById(R.id.map_fragment);
    }

    private MapListFragment getMapListFragment()
    {
        return (MapListFragment) getSupportFragmentManager().findFragmentById(R.id.place_list);
    }

    @Override
    public boolean onKeyDown(int keyCode, KeyEvent event)
    {
        if (keyCode == KeyEvent.KEYCODE_BACK )
        {
            // Let the engine know that user is exiting map
            MapManager.getInstance().hideMap(MapManager.getInstance().currentMap(), this, new MapManager.HideMapCompletedListener() {

                @Override
                public void onMapHidden()
                {
                    MapManager.getInstance().onMapEvent(new MapEvent(MapEvent.MAP_CLOSED, getMapFragment().getCameraPosition()));
                }
            });
            return true;
        }

        return super.onKeyDown(keyCode, event);
    }

    private void updateListMarkers(MapData mapData)
    {
        ArrayList<MapMarker> listMarkers = new ArrayList<>();
        if (mapData != null)
        {
            for (MapMarker m : mapData.getMarkers())
            {
                if (!Util.stringIsNullOrEmpty(m.getDescription()))
                {
                    listMarkers.add(m);
                }
            }
        }

        getMapListFragment().setMarkers(listMarkers);
        updateListVisibility(listMarkers);
    }

    private void updateListVisibility(List<MapMarker> markers)
    {
        MapListFragment listFragment = getMapListFragment();
        if (markers.size() == 0)
        {
            if (listFragment.isVisible())
                getSupportFragmentManager().beginTransaction().hide(listFragment).commit();

            if (TextUtils.isEmpty(m_searchText))
                m_searchViewContainer.setVisibility(View.GONE);
        } else
        {
            if (!listFragment.isVisible())
                getSupportFragmentManager().beginTransaction().show(listFragment).commit();
            m_searchViewContainer.setVisibility(View.VISIBLE);
        }
    }

    private void updateSearch(String query)
    {
        m_searchText = query;
        updateMap(m_viewModel.getMapData().getValue());
        getMapFragment().zoomToMarkers();
    }

    private void clearSearch()
    {
        m_searchText = null;
        m_searchView.setQuery("", false);
        Util.hideInputMethod(this);
        updateMap(m_viewModel.getMapData().getValue());
    }

    private static List<MapMarker> filterMarkers(@NonNull List<MapMarker> allMarkers, @NonNull String searchText)
    {
        List<MapMarker> filteredMarkers;
        if (TextUtils.isEmpty(searchText)) {
            filteredMarkers = allMarkers;
        } else
        {
            Pattern searchPattern = Pattern.compile(searchText, Pattern.CASE_INSENSITIVE | Pattern.LITERAL);

            filteredMarkers = new ArrayList<>();
            for (MapMarker m : allMarkers)
            {
                boolean matches = (m.getDescription() != null && searchPattern.matcher(m.getDescription()).find()) ||
                    (m.getText() != null && searchPattern.matcher(m.getText()).find());
                if (matches)
                {
                    filteredMarkers.add(m);
                }
            }
        }
        return filteredMarkers;
    }

    @Override
    public void errorDialogDismissed(int code)
    {

    }

    @Override
    public void onActivityResult(int requestCode, int resultCode, Intent intent)
    {
        super.onActivityResult(requestCode, resultCode, intent);

        if (requestCode == EntryActivity.EnableLocationInSettings)
        {
            // Returned from location settings, inform GPS reader
            GPSFunction.getReader().onSettingsResult(this);
        }
    }

    @Override
    public boolean onMapEvent(MapEvent e)
    {
        // for marker click scroll the list view to show the marker
        if (e.eventCode == MapEvent.MARKER_CLICKED)
            getMapListFragment().showMarker(e.marker);

        boolean requiresCallback;
        switch (e.eventCode) {
            case MapEvent.MARKER_CLICKED:
            case MapEvent.BUTTON_CLICKED:
            case MapEvent.MARKER_DRAGGED:
            case MapEvent.MARKER_INFO_WINDOW_CLICKED:
                requiresCallback = true;
                break;
            case MapEvent.MAP_CLOSED:
            case MapEvent.MAP_CLICKED:
            default:
                requiresCallback = false;
                break;
        }
        // Pass the event to the map manager which sends it to engine
        if (!requiresCallback || e.getCallback() >= 0)
        {
            MapManager.getInstance().onMapEvent(e);
            return true;
        } else {
            return false;
        }
    }

    public void saveSnapshot(String imagePath)
    {
        getMapFragment().saveSnapshot(imagePath);
    }
}
