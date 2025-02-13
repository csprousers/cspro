package gov.census.cspro.maps;

import android.content.Context;
import android.os.Bundle;
import androidx.annotation.NonNull;
import androidx.fragment.app.Fragment;
import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import com.bumptech.glide.Glide;

import java.util.List;

import gov.census.cspro.csentry.R;


/**
 * A simple {@link Fragment} subclass.
 * Activities that contain this fragment must implement the
 * {@link MapListFragment.OnFragmentInteractionListener} interface
 * to handle interaction events.
 * Use the {@link MapListFragment#newInstance} factory method to
 * create an instance of this fragment.
 */
public class MapListFragment extends Fragment
{
    private MapListAdapter m_mapListAdapter;
    private LinearLayoutManager m_layoutManager;
    private OnFragmentInteractionListener mListener;

    public MapListFragment()
    {
        // Required empty public constructor
    }

    /**
     * Use this factory method to create a new instance of
     * this fragment using the provided parameters.
     *
     * @return A new instance of fragment MapListFragment.
     */
    public static MapListFragment newInstance()
    {
        MapListFragment fragment = new MapListFragment();
        Bundle args = new Bundle();
        fragment.setArguments(args);
        return fragment;
    }

    @Override
    public View onCreateView(@NonNull LayoutInflater inflater, ViewGroup container,
                             Bundle savedInstanceState)
    {
        // Inflate the layout for this fragment
        View view = inflater.inflate(R.layout.fragment_map_list, container, false);

        RecyclerView rv = view.findViewById(R.id.marker_recycler_view);
        m_layoutManager = new LinearLayoutManager(getContext());
        rv.setLayoutManager(m_layoutManager);
        m_mapListAdapter = new MapListAdapter(Glide.with(this), new MarkerTextIconGenerator(getContext()));
        rv.setAdapter(m_mapListAdapter);
        m_mapListAdapter.setItemOnClickListener(new MapListAdapter.OnItemClickListener()
        {
            @Override
            public void OnClick(MapMarker m)
            {
                onItemClicked(m);
            }
        });

        return view;
    }

    private void onItemClicked(MapMarker m)
    {
        if (mListener != null)
        {
            mListener.onListItemClicked(m);
        }
    }

    @Override
    public void onAttach(Context context)
    {
        super.onAttach(context);
        if (context instanceof OnFragmentInteractionListener)
        {
            mListener = (OnFragmentInteractionListener) context;
        } else
        {
            throw new RuntimeException(context.toString()
                + " must implement OnFragmentInteractionListener");
        }
    }

    @Override
    public void onDetach()
    {
        super.onDetach();
        mListener = null;
    }

    void showMarker(MapMarker marker)
    {
        int pos = m_mapListAdapter.getMarkers().indexOf(marker);
        if (pos >= 0)
            m_layoutManager.scrollToPositionWithOffset(pos, 0);
    }

    void setMarkers(List<MapMarker> markers)
    {
        m_mapListAdapter.setMarkers(markers);
    }

    /**
     * This interface must be implemented by activities that contain this
     * fragment to allow an interaction in this fragment to be communicated
     * to the activity and potentially other fragments contained in that
     * activity.
     * <p>
     * See the Android Training lesson <a href=
     * "http://developer.android.com/training/basics/fragments/communicating.html"
     * >Communicating with Other Fragments</a> for more information.
     */
    public interface OnFragmentInteractionListener
    {
        void onListItemClicked(MapMarker m);
    }
}
