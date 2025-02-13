package gov.census.cspro.maps;

import android.os.Parcel;
import android.os.Parcelable;
import androidx.annotation.ColorInt;

/**
 * Place mark on a map at particular latlong
 */
public class MapMarker implements Parcelable
{
    private final int m_id;
    private double m_latitude;
    private double m_longitude;
    private String m_text;
    private @ColorInt int m_textColor;
    private @ColorInt int m_backgroundColor = 0xF44343;
    private String m_imagePath;
    private String m_description;
    private int m_onClickCallback = -1;
    private int m_onClickInfoWindowCallback = -1;
    private int m_onDragCallback = -1;

    public MapMarker(int id, double latitude, double longitude)
    {
        m_id = id;
        m_latitude = latitude;
        m_longitude = longitude;
    }

    MapMarker(MapMarker rhs)
    {
        m_id = rhs.m_id;
        m_latitude = rhs.m_latitude;
        m_longitude = rhs.m_longitude;
        m_text = rhs.m_text;
        m_textColor = rhs.m_textColor;
        m_backgroundColor = rhs.m_backgroundColor;
        m_imagePath = rhs.m_imagePath;
        m_description = rhs.m_description;
        m_onClickCallback = rhs.m_onClickCallback;
        m_onClickInfoWindowCallback = rhs.m_onClickInfoWindowCallback;
        m_onDragCallback = rhs.m_onDragCallback;
    }

    int getId()
    {
        return m_id;
    }

    double getLatitude()
    {
        return m_latitude;
    }

    double getLongitude()
    {
        return m_longitude;
    }

    void setLocation(double latitude, double longitude)
    {
        m_latitude = latitude;
        m_longitude = longitude;
    }

    String getText()
    {
        return m_text;
    }

    void setText(String text)
    {
        m_text = text;
        m_imagePath = null; // Can't have both text and image
    }

    String getDescription()
    {
        return m_description;
    }

    public void setDescription(String description)
    {
        m_description = description;
    }

    int getOnClickCallback()
    {
        return m_onClickCallback;
    }

    void setOnClickCallback(int onClickCallback)
    {
        m_onClickCallback = onClickCallback;
    }

    @ColorInt int getTextColor()
    {
        return m_textColor;
    }

    void setTextColor(@ColorInt int textColor)
    {
        m_textColor = textColor;
    }

    @ColorInt int getBackgroundColor()
    {
        return m_backgroundColor;
    }

    public void setBackgroundColor(@ColorInt int backgroundColor)
    {
        m_backgroundColor = backgroundColor;
    }

    String getImagePath()
    {
        return m_imagePath;
    }

    void setImagePath(String imagePath)
    {
        m_imagePath = imagePath;
        m_text = null; // Can't have both image and text
    }

    int getOnClickInfoWindowCallback()
    {
        return m_onClickInfoWindowCallback;
    }

    void setOnClickInfoWindowCallback(int onClickInfoWindowCallback)
    {
        m_onClickInfoWindowCallback = onClickInfoWindowCallback;
    }

    int getOnDragCallback()
    {
        return m_onDragCallback;
    }

    void setOnDragCallback(int onDragCallback)
    {
        m_onDragCallback = onDragCallback;
    }

    private MapMarker(Parcel in)
    {
        m_id = in.readInt();
        m_latitude = in.readDouble();
        m_longitude = in.readDouble();
        m_text = in.readString();
        m_textColor = in.readInt();
        m_backgroundColor = in.readInt();
        m_imagePath = in.readString();
        m_description = in.readString();
        m_onClickCallback = in.readInt();
        m_onClickInfoWindowCallback = in.readInt();
        m_onDragCallback = in.readInt();
    }

    @Override
    public void writeToParcel(Parcel dest, int flags)
    {
        dest.writeInt(m_id);
        dest.writeDouble(m_latitude);
        dest.writeDouble(m_longitude);
        dest.writeString(m_text);
        dest.writeInt(m_textColor);
        dest.writeInt(m_backgroundColor);
        dest.writeString(m_imagePath);
        dest.writeString(m_description);
        dest.writeInt(m_onClickCallback);
        dest.writeInt(m_onClickInfoWindowCallback);
        dest.writeInt(m_onDragCallback);
    }

    @Override
    public int describeContents()
    {
        return 0;
    }

    public static final Creator<MapMarker> CREATOR = new Creator<MapMarker>()
    {
        @Override
        public MapMarker createFromParcel(Parcel in)
        {
            return new MapMarker(in);
        }

        @Override
        public MapMarker[] newArray(int size)
        {
            return new MapMarker[size];
        }
    };


    @Override
    public int hashCode()
    {
        return m_id;
    }

    @Override
    public boolean equals(Object o)
    {
        if (this == o) return true;
        if (o == null || getClass() != o.getClass()) return false;

        MapMarker mapMarker = (MapMarker) o;

        if (m_id != mapMarker.m_id) return false;
        if (Double.compare(mapMarker.m_latitude, m_latitude) != 0) return false;
        if (Double.compare(mapMarker.m_longitude, m_longitude) != 0) return false;
        if (m_textColor != mapMarker.m_textColor) return false;
        if (m_backgroundColor != mapMarker.m_backgroundColor) return false;
        if (m_onClickCallback != mapMarker.m_onClickCallback) return false;
        if (m_onClickInfoWindowCallback != mapMarker.m_onClickInfoWindowCallback) return false;
        if (m_onDragCallback != mapMarker.m_onDragCallback) return false;
        if (m_text != null ? !m_text.equals(mapMarker.m_text) : mapMarker.m_text != null)
            return false;
        if (m_imagePath != null ? !m_imagePath.equals(mapMarker.m_imagePath) : mapMarker.m_imagePath != null)
            return false;
        return m_description != null ? m_description.equals(mapMarker.m_description) : mapMarker.m_description == null;
    }
}
