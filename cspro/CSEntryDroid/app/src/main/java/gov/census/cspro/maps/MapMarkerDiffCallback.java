package gov.census.cspro.maps;

import androidx.recyclerview.widget.DiffUtil;

import java.util.List;

/**
 * DiffCalback implementation for list of map markers for use with DiffUtils
 */
public class MapMarkerDiffCallback extends DiffUtil.Callback
{
    private final List<MapMarker> m_oldList;
    private final List<MapMarker> m_newList;

    MapMarkerDiffCallback(List<MapMarker> oldList, List<MapMarker> newList)
    {
        m_oldList = oldList;
        m_newList = newList;
    }

    @Override
    public int getOldListSize()
    {
        return m_oldList.size();
    }

    @Override
    public int getNewListSize()
    {
        return m_newList.size();
    }

    @Override
    public boolean areItemsTheSame(int oldItemPos, int newItemPos)
    {
        return m_oldList.get(oldItemPos).getId() == m_newList.get(newItemPos).getId();
    }

    @Override
    public boolean areContentsTheSame(int oldItemPos, int newItemPos)
    {
        return m_oldList.get(oldItemPos).equals(m_newList.get(newItemPos));
    }
}
