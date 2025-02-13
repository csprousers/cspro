package gov.census.cspro.maps;

import androidx.annotation.NonNull;
import androidx.recyclerview.widget.DiffUtil;
import androidx.recyclerview.widget.RecyclerView;
import android.text.Html;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ImageView;
import android.widget.TextView;

import com.bumptech.glide.RequestManager;

import java.util.ArrayList;
import java.util.List;

import gov.census.cspro.csentry.R;
import gov.census.cspro.engine.Util;

public class MapListAdapter extends RecyclerView.Adapter<MapListAdapter.ViewHolder>
{
    private List<MapMarker> m_markers = new ArrayList<>();
    private OnItemClickListener m_onItemClickListener;
    private final RequestManager m_glide;
    private final MarkerTextIconGenerator m_textIconGenerator;

    MapListAdapter(RequestManager glide, MarkerTextIconGenerator textIconGenerator)
    {
        m_glide = glide;
        m_textIconGenerator = textIconGenerator;
    }

    List<MapMarker> getMarkers()
    {
        return m_markers;
    }

    void setMarkers(List<MapMarker> markers)
    {
        final MapMarkerDiffCallback markerDiffCallback = new MapMarkerDiffCallback(m_markers, markers);
        final DiffUtil.DiffResult diffResult = DiffUtil.calculateDiff(markerDiffCallback);
        m_markers.clear();
        for (int i = 0; i < markers.size(); i++) {
            m_markers.add(new MapMarker(markers.get(i)));
        }
        diffResult.dispatchUpdatesTo(this);
    }

    interface OnItemClickListener
    {
        void OnClick(MapMarker marker);
    }

    void setItemOnClickListener(OnItemClickListener listener)
    {
        m_onItemClickListener = listener;
    }

    private void onItemClick(int position) {
        if (m_onItemClickListener != null)
            m_onItemClickListener.OnClick(m_markers.get(position));
    }

    @NonNull
    @Override
    public ViewHolder onCreateViewHolder(@NonNull ViewGroup parent, int viewType)
    {
        View v = LayoutInflater.from(parent.getContext())
                               .inflate(R.layout.map_list_item, parent, false);
        return new ViewHolder(v);
    }

    @Override
    public void onBindViewHolder(@NonNull ViewHolder holder, int position)
    {
        MapMarker marker = m_markers.get(position);

        if (Util.stringIsNullOrEmpty(marker.getDescription())) {
            holder.m_description.setText(null);
        }
        else {
            holder.m_description.setText(MapFragment.textToMapSupportedHtml(marker.getDescription()));
        }

        if (!Util.stringIsNullOrEmpty(marker.getImagePath()))
        {
            holder.m_icon.setVisibility(View.VISIBLE);
            m_glide.load(marker.getImagePath())
                   .fitCenter()
                   .into(holder.m_icon);
        } else
        {
            m_glide.clear(holder.m_icon);
            if (!Util.stringIsNullOrEmpty(marker.getText()))
            {
                holder.m_icon.setVisibility(View.VISIBLE);
                holder.m_icon.setImageBitmap(m_textIconGenerator.makeIcon(marker));
            } else
            {
                holder.m_icon.setVisibility(View.GONE);
            }
        }
    }

    @Override
    public int getItemCount()
    {
        return m_markers == null ? 0 : m_markers.size();
    }

    class ViewHolder extends RecyclerView.ViewHolder
    {
        TextView m_description;
        ImageView m_icon;

        ViewHolder(@NonNull View v)
        {
            super(v);
            m_description = v.findViewById(R.id.textview_description);
            m_icon = v.findViewById(R.id.imageview_icon);
            v.setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View view) {
                    MapListAdapter.this.onItemClick(getAdapterPosition());
                }
            });
        }
    }
}
