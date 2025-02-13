package gov.census.cspro.maps;

import android.content.Context;
import android.graphics.Bitmap;
import android.text.Html;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.TextView;

import com.google.maps.android.ui.IconGenerator;

class MarkerTextIconGenerator
{
    private final IconGenerator m_iconFactory;
    private final TextView m_iconTextView;

    MarkerTextIconGenerator(Context context)
    {
        m_iconFactory = new IconGenerator(context);
        View iconView = LayoutInflater.from(context).inflate(com.google.maps.android.R.layout.amu_text_bubble, null);
        m_iconTextView = iconView.findViewById(com.google.maps.android.R.id.amu_text);
        m_iconFactory.setContentView(iconView);
    }

    Bitmap makeIcon(MapMarker marker)
    {
        m_iconFactory.setColor(marker.getBackgroundColor());
        m_iconTextView.setTextColor(marker.getTextColor());
        return m_iconFactory.makeIcon(MapFragment.textToMapSupportedHtml(marker.getText()));
    }
}
