package gov.census.cspro.maps;


import android.Manifest;
import android.annotation.SuppressLint;
import android.content.pm.PackageManager;
import android.graphics.Bitmap;
import android.graphics.Color;
import android.graphics.drawable.Drawable;
import android.os.Bundle;
import android.text.Html;
import android.text.Spanned;
import android.text.TextUtils;
import android.util.DisplayMetrics;
import android.util.SparseArray;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.ImageButton;
import android.widget.LinearLayout;
import android.widget.TextView;
import android.widget.Toast;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.core.app.ActivityCompat;
import androidx.fragment.app.Fragment;

import com.bumptech.glide.Glide;
import com.bumptech.glide.request.target.CustomTarget;
import com.bumptech.glide.request.transition.Transition;
import com.google.android.gms.location.LocationServices;
import com.google.android.gms.maps.CameraUpdate;
import com.google.android.gms.maps.CameraUpdateFactory;
import com.google.android.gms.maps.GoogleMap;
import com.google.android.gms.maps.SupportMapFragment;
import com.google.android.gms.maps.model.BitmapDescriptorFactory;
import com.google.android.gms.maps.model.CameraPosition;
import com.google.android.gms.maps.model.LatLng;
import com.google.android.gms.maps.model.LatLngBounds;
import com.google.android.gms.maps.model.Marker;
import com.google.android.gms.maps.model.MarkerOptions;
import com.google.android.gms.maps.model.TileOverlay;
import com.google.android.gms.maps.model.TileOverlayOptions;

import java.io.FileOutputStream;
import java.io.IOException;
import java.util.ArrayList;
import java.util.List;

import gov.census.cspro.commonui.ErrorDialogFragment;
import gov.census.cspro.csentry.CSEntry;
import gov.census.cspro.csentry.R;
import gov.census.cspro.engine.BaseMapSelection;
import gov.census.cspro.engine.Messenger;
import gov.census.cspro.engine.Util;
import gov.census.cspro.maps.geojson.FeatureCollection;
import gov.census.cspro.maps.geojson.GeoJsonLayer;
import gov.census.cspro.maps.offline.IOfflineTileReader;
import gov.census.cspro.maps.offline.MBTilesReader;
import gov.census.cspro.maps.offline.OfflineTileProvider;
import gov.census.cspro.maps.offline.TpkTilesReader;

import static gov.census.cspro.maps.MapUtilKt.MARKER_FIT_PADDING;

/**
 * Fragment for displaying interactive Google Map with markers and buttons.
 *
 * Set the map parameters including markers, buttons and basemap by calling {@link #setData(MapData)}
 * To handle events (clicks, drags, close) from the map call {@link #setEventListener(IMapEventListener)}
 */
public class MapFragment extends Fragment
    implements OnMapAndViewReadyListener.OnGlobalLayoutAndMapReadyListener,
    GoogleMap.OnMarkerClickListener, GoogleMap.OnMarkerDragListener,
    GoogleMap.OnInfoWindowClickListener,
    GoogleMap.OnMapClickListener, GoogleMap.InfoWindowAdapter
{
    private static final int CAMERA_ANIMATE_DURATION_MARKER_CLICK = 250; // 250 is the duration Google Maps uses when you click on a marker according to Stack Overflow
    private static final int CAMERA_ANIMATE_DURATION_ZOOM_TO = 1000;
    private static final int MAX_MARKER_ICON_SIZE_DP = 60;
    protected static final float VECTOR_LAYER_ZINDEX = 1;

    private int m_maxMarkerIconSizePx;
    protected GoogleMap m_map;
    private final SparseArray<Marker> m_googleMapMarkers = new SparseArray<>();
    private boolean m_firstShowing;
    private MarkerTextIconGenerator m_textIconGenerator;
    private TileOverlay m_tileOverlay;
    private OfflineTileProvider m_tileProvider;
    private LatLngBounds m_baseMapInitialExtent;
    private LinearLayout m_buttonContainer;
    private ImageButton m_currentLocationButton;
    private TextView m_titleView;
    private MapData m_mapData;
    private IMapEventListener m_eventListener;
    private final List<GeoJsonLayer> m_geometryLayers = new ArrayList<>();

    public void setEventListener(@NonNull IMapEventListener listener)
    {
        m_eventListener = listener;
    }

    /**
     * Called when map is modified from CSPro logic i.e. new markers, new buttons...
     */
    public void setData(@Nullable MapData mapData)
    {
        if (m_map == null) {
            // Not ready yet, save a copy to set later and ignore for now
            m_mapData = mapData == null ? null : new MapData(mapData);
            return;
        }

        if (mapData != null)
        {
            // Check for any differences with the current map data and make
            // updates
            if (m_mapData == null || !TextUtils.equals(m_mapData.getTitle(), mapData.getTitle()))
                setMapTitle(mapData.getTitle());

            if (m_map != null)
            {
                setShowCurrentLocation(mapData.isShowCurrentLocation());

                setMarkers(mapData.getMarkers());

                setButtons(mapData.getButtons());

                setGeometry(mapData.getGeometry());

                if (m_mapData == null || !mapData.getBaseMapSelection().equals(m_mapData.getBaseMapSelection()))
                {
                    setBaseMap(mapData.getBaseMapSelection());
                }
            }
        }

        // Make a copy so we can do diffs
        m_mapData = mapData == null ? null : new MapData(mapData);

        if (m_mapData != null) {
            setInitialCamera();
        }
    }

    private void setGeometry(List<FeatureCollection> featureCollections) {

        for (GeoJsonLayer layer : m_geometryLayers) {
            layer.removeFromMap();
        }
        m_geometryLayers.clear();

        for (FeatureCollection features : featureCollections) {
            m_geometryLayers.add(new GeoJsonLayer(m_map, features, VECTOR_LAYER_ZINDEX));
        }

    }

    /**
     * Move camera to fit view to the currently displayed markers
     */
    void zoomToMarkers()
    {
        if (m_googleMapMarkers.size() > 0)
        {
            m_map.animateCamera(CameraUpdateFactory.newLatLngBounds(getMarkerBounds(), MARKER_FIT_PADDING), CAMERA_ANIMATE_DURATION_MARKER_CLICK, null);

            // If only a single marker then show the info window
            if (m_googleMapMarkers.size() == 1) {
                m_googleMapMarkers.valueAt(0).showInfoWindow();
            }
        }
    }

    @Override
    public void onCreate(Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);

        m_firstShowing = (savedInstanceState == null);

        m_maxMarkerIconSizePx = Util.dpToPx(MAX_MARKER_ICON_SIZE_DP, getResources());

        m_textIconGenerator = new MarkerTextIconGenerator(getContext());
    }

    public View onCreateView(@NonNull LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState)
    {
        View view = inflater.inflate(R.layout.fragment_map,
            container,
            false);

        m_buttonContainer = view.findViewById(R.id.button_container);
        m_titleView = view.findViewById(R.id.title);

        return view;
    }

    @Override
    public void onActivityCreated(Bundle savedInstanceState) {
        super.onActivityCreated(savedInstanceState);

        new OnMapAndViewReadyListener((SupportMapFragment) getChildFragmentManager().findFragmentById(R.id.map), this);
    }

    @Override
    public void onDestroy()
    {
        if (m_tileProvider != null)
        {
            m_tileProvider.close();
        }

        super.onDestroy();
    }

    /**
     * Manipulates the map once available.
     * This callback is triggered when the map is ready to be used.
     * This is where we can add markers or lines, add listeners or move the camera.
     * If Google Play services is not installed on the device, the user will be prompted to install
     * it inside the SupportMapFragment. This method will only be triggered once the user has
     * installed Google Play services and returned to the app.
     */
    @SuppressLint("MissingPermission")
    @Override
    public void onMapReady(GoogleMap googleMap)
    {
        m_map = googleMap;
        m_map.getUiSettings().setMapToolbarEnabled(false);
        m_map.getUiSettings().setTiltGesturesEnabled(false);
        m_map.getUiSettings().setMyLocationButtonEnabled(false); // will add our own if needed

        addCurrentLocationButton();

        m_map.setOnMarkerClickListener(this);
        m_map.setOnInfoWindowClickListener(this);
        m_map.setOnMarkerDragListener(this);
        m_map.setOnMapClickListener(this);
        m_map.setInfoWindowAdapter(this);

        // If map data was set before map was ready we saved it in m_mapData. Set that data now
        // but set m_mapData to null so that setData() can properly diff the old and new data
        MapData previouslySetData = m_mapData;
        m_mapData = null;
        setData(previouslySetData);
    }

    private void setInitialCamera()
    {
        // Set initial zoom/position of map when map is shown
        MapCameraPosition savedCameraPos = m_mapData.getCameraPosition();
        MapCameraPosition zoomToPoint = m_mapData.getZoomToPoint();
        LatLngBounds zoomToBounds = m_mapData.getZoomToBounds();
        if (savedCameraPos != null)
        {
            // Restore saved camera position
            m_map.moveCamera(CameraUpdateFactory.newCameraPosition(
                new CameraPosition(new LatLng(savedCameraPos.latitude, savedCameraPos.longitude),
                    savedCameraPos.zoom, 0, savedCameraPos.bearing)));
        } else if (zoomToPoint != null) {
            // Logic called map.zoomTo() with point, use that
            CameraUpdate update = getZoomToPointUpdate(new LatLng(zoomToPoint.latitude, zoomToPoint.longitude), zoomToPoint.zoom);
            if (m_firstShowing)
            {
                // If just showing map, don't animate camera
                m_map.moveCamera(update);
            } else {
                m_map.animateCamera(update, CAMERA_ANIMATE_DURATION_ZOOM_TO, null);
            }
        } else if (zoomToBounds != null) {
            // Logic called map.zoomTo() with bounds, use that
            CameraUpdate update = getZoomToBoundsUpdate(zoomToBounds, m_mapData.getZoomToBoundsPaddingPct());
            if (m_firstShowing)
            {
                m_map.moveCamera(update);
            } else {
                m_map.animateCamera(update, CAMERA_ANIMATE_DURATION_ZOOM_TO, null);
            }
        } else if (m_googleMapMarkers.size() > 0) {
            // No saved pos or call to map.zoomTo() - use markers
            m_map.moveCamera(CameraUpdateFactory.newLatLngBounds(getMarkerBounds(), MARKER_FIT_PADDING));
        } else if (getGeometryBounds() != null) {
            // No markers - use geometry layers
            m_map.moveCamera(CameraUpdateFactory.newLatLngBounds(getGeometryBounds(), MARKER_FIT_PADDING));
        } else if (m_baseMapInitialExtent != null) {
            // No geometry - use bounds of custom base map
            m_map.moveCamera(CameraUpdateFactory.newLatLngBounds(m_baseMapInitialExtent, 0));
        } else if (m_mapData.isShowCurrentLocation()) {
            // Use current location
            try
            {
                LocationServices.getFusedLocationProviderClient(getActivity()).getLastLocation().addOnSuccessListener(location -> {
                    if (location != null)
                        m_map.moveCamera(CameraUpdateFactory.newLatLngZoom(new LatLng(location.getLatitude(), location.getLongitude()), 16));
                });
            } catch (SecurityException ignored)
            {}
        }

        m_firstShowing = false;
    }

    private void setMarkers(List<MapMarker> markers)
    {
        for (int i = 0; i < m_googleMapMarkers.size(); ++i)
            m_googleMapMarkers.valueAt(i).remove();
        m_googleMapMarkers.clear();

        for (MapMarker m : markers)
        {
            addMarkerToMap(m);
        }
    }

    private Marker createGoogleMapMarker(@NonNull MapMarker m)
    {
        LatLng latLng = new LatLng(m.getLatitude(), m.getLongitude());

        MarkerOptions markerOptions = new MarkerOptions().position(latLng);

        if (Util.stringIsNullOrEmpty(m.getImagePath()))
        {
            if (!Util.stringIsNullOrEmpty(m.getText()))
            {
                // Text icon
                markerOptions.icon(BitmapDescriptorFactory.fromBitmap(m_textIconGenerator.makeIcon(m)));
            } else {
                // Default icon
                markerOptions.icon(BitmapDescriptorFactory.defaultMarker(colorToHue(m.getBackgroundColor())));
            }
        }

        markerOptions.draggable(m.getOnDragCallback() >= 0);

        Marker googleMapMarker = m_map.addMarker(markerOptions);

        if (googleMapMarker != null)
        {
            googleMapMarker.setTag(m);

            if (!Util.stringIsNullOrEmpty(m.getImagePath()))
            {
                // Image icon
                Glide.with(this).asBitmap().load(m.getImagePath()).centerInside().into(new MarkerIconTarget(m_maxMarkerIconSizePx, m_maxMarkerIconSizePx, googleMapMarker));
            }
        }

        return googleMapMarker;
    }

    private float colorToHue(int color)
    {
        float[] hsv = new float[3];
        Color.colorToHSV(color, hsv);
        return hsv[0];
    }

    void simulateMarkerClick(MapMarker marker)
    {
        Marker clickedMarker = m_googleMapMarkers.get(marker.getId());
        if (clickedMarker != null)
        {
            zoomToPoint(clickedMarker.getPosition());
            clickedMarker.showInfoWindow();
        }
    }

    /**
     * Target for loading Bitmap as map marker icon using Glide
     */
    private static class MarkerIconTarget extends CustomTarget<Bitmap>
    {
        private final Marker m_marker;

        private MarkerIconTarget(int w, int h, Marker marker)
        {
            super(w,h);
            m_marker = marker;
        }

        @Override
        public void onResourceReady(@NonNull Bitmap resource, @Nullable Transition<? super Bitmap> transition)
        {
            try {

                m_marker.setIcon(BitmapDescriptorFactory.fromBitmap(resource));
            }
            catch (Exception exc) {
                Toast.makeText(CSEntry.Companion.getContext(), "Error setting marker icon", Toast.LENGTH_SHORT).show();
            }
        }

        @Override
        public void onLoadCleared(@Nullable Drawable placeholder)
        {

        }
    }

    private void addMarkerToMap(@NonNull MapMarker m)
    {
        Marker googleMapMarker = createGoogleMapMarker(m);
        if (googleMapMarker != null)
            m_googleMapMarkers.put(m.getId(), googleMapMarker);
    }

    @Override
    public void onInfoWindowClick(@NonNull Marker marker)
    {
        MapMarker clickedMarker = (MapMarker) marker.getTag();
        if (clickedMarker != null)
        {
            if (m_eventListener != null)
                m_eventListener.onMapEvent(new MapEvent(MapEvent.MARKER_INFO_WINDOW_CLICKED,
                clickedMarker.getOnClickInfoWindowCallback(),
                clickedMarker,
                getCameraPosition()));
        }
    }

    @Override
    public boolean onMarkerClick(@NonNull Marker marker)
    {
        MapMarker clickedMarker = (MapMarker) marker.getTag();
        if (clickedMarker != null && m_eventListener != null)
        {
            return m_eventListener.onMapEvent(new MapEvent(MapEvent.MARKER_CLICKED,
                clickedMarker.getOnClickCallback(),
                clickedMarker,
                getCameraPosition()));
        }

        return false;
    }

    @Override
    public void onMarkerDragStart(@NonNull Marker marker)
    {
    }

    @Override
    public void onMarkerDrag(@NonNull Marker marker)
    {
    }

    @Override
    public void onMarkerDragEnd(@NonNull Marker marker)
    {
        MapMarker clickedMarker = (MapMarker) marker.getTag();
        if (clickedMarker != null)
        {
            clickedMarker.setLocation(marker.getPosition().latitude, marker.getPosition().longitude);
            if (m_eventListener != null) {
                m_eventListener.onMapEvent(new MapEvent(MapEvent.MARKER_DRAGGED,
                clickedMarker.getOnDragCallback(),
                clickedMarker,
                getCameraPosition()));
            }
        }
    }

    @Override
    public void onMapClick(LatLng latLng)
    {
        if (m_eventListener != null)
            m_eventListener.onMapEvent(new MapEvent(MapEvent.MAP_CLICKED, -1, latLng.latitude, latLng.longitude, getCameraPosition()));
    }

    @Override
    public View getInfoWindow(@NonNull Marker marker)
    {
        return null;
    }

    @Override
    public View getInfoContents(@NonNull Marker marker)
    {
        MapMarker mm = (MapMarker) marker.getTag();
        if (mm == null)
            return null;

        // Only include text if there is an icon, otherwise text is already shown
        // on the icon so it is redundant
        String text = Util.stringIsNullOrEmpty(mm.getImagePath()) ? null : mm.getText();
        String description = mm.getDescription();

        if (Util.stringIsNullOrEmpty(text) && Util.stringIsNullOrEmpty(description)) {
            // Nothing to display, no popup
            return null;
        } else {
            @SuppressLint("InflateParams") View popupView = LayoutInflater.from(getContext()).inflate(R.layout.map_info_window, null);
            TextView tvText = popupView.findViewById(R.id.textview_text);
            if (Util.stringIsNullOrEmpty(text)) {
                tvText.setVisibility(View.GONE);
            } else {
                tvText.setVisibility(View.VISIBLE);
                tvText.setText(textToMapSupportedHtml(text));
            }

            TextView tvDescription = popupView.findViewById(R.id.textview_description);
            if (Util.stringIsNullOrEmpty(description)) {
                tvDescription.setVisibility(View.GONE);
            } else {
                tvDescription.setVisibility(View.VISIBLE);
                tvDescription.setText(textToMapSupportedHtml(description));
            }
            return popupView;
        }
    }

    private void setButtons(List<MapButton> buttons)
    {
        clearButtons();

        for (MapButton mapButton : buttons) {
            addButtonToMap(mapButton);
        }
    }

    protected void clearButtons() {
        for (int i = m_buttonContainer.getChildCount() - 1; i >= 1; --i)
        {
            m_buttonContainer.removeViewAt(i);
        }
    }


    private void addButtonToMap(MapButton mapButton)
    {
        View button = createMapButton(mapButton);

        final int callback = mapButton.getOnClickCallback();
        button.setOnClickListener(view -> {
            if (m_eventListener != null)
                m_eventListener.onMapEvent(new MapEvent(MapEvent.BUTTON_CLICKED,
                callback, getCameraPosition()));
        });

        button.setTag(R.id.button_container, mapButton);
        addToButtonLayout(button);
    }

    @NonNull
    private View createMapButton(MapButton mapButton)
    {
        View button;
        if (!TextUtils.isEmpty(mapButton.getLabel())) {
            Button textButton = new Button(getContext());
            textButton.setAllCaps(false);
            textButton.setText(textToMapSupportedHtml(mapButton.getLabel()));
            button = textButton;
        } else if (!TextUtils.isEmpty(mapButton.getImagePath())) {
            ImageButton imageButton = new ImageButton(getContext());
            button = imageButton;
            Glide.with(this).load(mapButton.getImagePath()).fitCenter().into(imageButton);
        } else {
            ImageButton imageButton = new ImageButton(getContext());
            imageButton.setImageResource(mapButton.getImageResourceId());
            button = imageButton;
        }

        return button;
    }

    protected void addToButtonLayout(View button)
    {
        addToButtonLayout(button, -1);
    }

    protected void addToButtonLayout(View button, int index)
    {
        LinearLayout.LayoutParams layoutParams = new LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT);
        layoutParams.setMargins(0, 12, 24, 12);
        button.setLayoutParams(layoutParams);
        button.setBackgroundResource(R.drawable.mapbutton_background);
		button.setElevation(Util.dpToPx(6, getResources()));
        if (m_buttonContainer != null)
        {
            m_buttonContainer.addView(button, index);
        }
    }

    private void setBaseMap(BaseMapSelection baseMapSelection)
    {
        if (m_map.getMapType() != baseMapSelection.getGoogleMapType())
            m_map.setMapType(baseMapSelection.getGoogleMapType());

        if (m_tileOverlay != null)
        {
            m_tileOverlay.remove();
            m_tileOverlay = null;
            m_tileProvider.close();
            m_tileProvider = null;
        }

        if (baseMapSelection.getFilename() != null)
        {
            try
            {
                IOfflineTileReader reader;

                if (baseMapSelection.getFilename().toLowerCase().endsWith("mbtiles")) {
                    reader = new MBTilesReader(baseMapSelection.getFilename());
                } else if (baseMapSelection.getFilename().toLowerCase().endsWith("tpk")) {
                    reader = new TpkTilesReader(baseMapSelection.getFilename());
                } else {
                    throw new IOException("Invalid tile file format. Only mbtiles and tpk are supported.");
                }

                // No max zoom since reader can scale up bitmaps
                m_map.setMinZoomPreference(reader.getMinZoom());

                LatLngBounds baseMapFullExtent = reader.getFullExtent();
                m_map.setLatLngBoundsForCameraTarget(baseMapFullExtent);

                m_baseMapInitialExtent = reader.getInitialExtent();

                m_tileProvider = new OfflineTileProvider(reader);
                m_tileOverlay = m_map.addTileOverlay(new TileOverlayOptions().tileProvider(m_tileProvider));

            } catch (IOException e) {
                ErrorDialogFragment errorDialog = ErrorDialogFragment.newInstance(
                    String.format(getString(R.string.error_loading_offline_map), baseMapSelection.getFilename()),
                    e.getMessage(),
                    R.string.error_loading_offline_map);
                errorDialog.show(getChildFragmentManager(), "errorDialog");
            }
        } else {
            m_map.setLatLngBoundsForCameraTarget(null);
        }
    }

    private void setShowCurrentLocation(boolean showCurrentLocation)
    {
        if (ActivityCompat.checkSelfPermission(getContext(), Manifest.permission.ACCESS_FINE_LOCATION) != PackageManager.PERMISSION_GRANTED && ActivityCompat.checkSelfPermission(getContext(), Manifest.permission.ACCESS_COARSE_LOCATION) != PackageManager.PERMISSION_GRANTED)
        {
            return;
        }

        m_currentLocationButton.setVisibility(showCurrentLocation ? View.VISIBLE : View.GONE);
        m_map.setMyLocationEnabled(showCurrentLocation);
    }

    private void addCurrentLocationButton()
    {
        m_currentLocationButton = new ImageButton(getContext());
        m_currentLocationButton.setImageResource(R.drawable.ic_my_location);
        m_currentLocationButton.setOnClickListener(view -> MapUtilKt.zoomToCurrentLocation(m_map, getActivity()));
        addToButtonLayout(m_currentLocationButton);
    }

    private void setMapTitle(String title)
    {
        if (!TextUtils.isEmpty(title)) {
            m_titleView.setText(textToMapSupportedHtml(title));
            m_titleView.setVisibility(View.VISIBLE);
        } else {
            m_titleView.setVisibility(View.GONE);
        }
    }

    private void zoomToPoint(@NonNull LatLng position)
    {
        m_map.animateCamera(getZoomToPointUpdate(position, -1), CAMERA_ANIMATE_DURATION_MARKER_CLICK, null);
    }

    private CameraUpdate getZoomToPointUpdate(@NonNull LatLng position, double zoom)
    {
        if (zoom >= 0)
            return CameraUpdateFactory.newLatLngZoom(position, (float) zoom);
        else
            return CameraUpdateFactory.newLatLng(position);
    }

    private CameraUpdate getZoomToBoundsUpdate(@NonNull LatLngBounds bounds, double paddingPercent)
    {
        DisplayMetrics metrics = new DisplayMetrics();
        getActivity().getWindowManager().getDefaultDisplay().getMetrics(metrics);
        int paddingPx = (int) Math.round(paddingPercent*metrics.widthPixels);
        int maxPadding = getView().getMeasuredWidth()/2 - 1;
        paddingPx = Math.min(paddingPx, maxPadding);
        return CameraUpdateFactory.newLatLngBounds(bounds, paddingPx);
    }

    private LatLngBounds getMarkerBounds()
    {
        LatLngBounds.Builder builder = new LatLngBounds.Builder();
        for (int i = 0; i < m_googleMapMarkers.size(); ++i)
        {
            builder.include(m_googleMapMarkers.valueAt(i).getPosition());
        }
        return builder.build();
    }

    private LatLngBounds getGeometryBounds() {
        LatLngBounds.Builder builder = new LatLngBounds.Builder();
        boolean haveBounds = false;
        for (GeoJsonLayer layer : m_geometryLayers) {
            if (layer.getBounds() != null) {
                builder.include(layer.getBounds().northeast);
                builder.include(layer.getBounds().southwest);
                haveBounds = true;
            }
        }
        return haveBounds ? builder.build() : null;
    }

    public MapCameraPosition getCameraPosition()
    {
        if (m_map != null)
        {
            CameraPosition cp = m_map.getCameraPosition();
            return new MapCameraPosition(cp.target.latitude, cp.target.longitude, cp.zoom, cp.bearing);
        } else {
            return null;
        }
    }

    public void saveSnapshot(String imagePath)
    {
        if( m_map == null )
        {
            Messenger.getInstance().engineFunctionComplete("Map is not setup yet.");
        }

        else
        {
            m_map.snapshot(new GoogleMap.SnapshotReadyCallback()
            {
                @Override
                public void onSnapshotReady(Bitmap bitmap)
                {
                    String result = null;

                    try
                    {
                        String extension = imagePath.substring(imagePath.lastIndexOf('.') + 1);

                        if( extension.compareToIgnoreCase("png") == 0 )
                            bitmap.compress(Bitmap.CompressFormat.PNG, 100, new FileOutputStream(imagePath));

                        else if( extension.compareToIgnoreCase("jpg") == 0 || extension.compareToIgnoreCase("jpeg") == 0 )
                            bitmap.compress(Bitmap.CompressFormat.JPEG, 100, new FileOutputStream(imagePath));

                        else
                            throw new IOException("Snapshots can only be saved to JPEG or PNG formats.");
                    }

                    catch( Exception exception )
                    {
                        result = exception.getMessage();
                    }

                    Messenger.getInstance().engineFunctionComplete(result);
                }
            });
        }
    }

    public static Spanned textToMapSupportedHtml(String text) {
        return Html.fromHtml(text.replace("\n", "<br>"));
    }
}
