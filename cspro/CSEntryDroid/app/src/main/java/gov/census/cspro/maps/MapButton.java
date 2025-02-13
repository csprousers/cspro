package gov.census.cspro.maps;

import android.os.Parcel;
import android.os.Parcelable;

public class MapButton implements Parcelable
{
    private final int m_id;
    private String m_imagePath;

    private int m_imageResourceId;
    private String m_label;
    private int m_onClickCallback;

    MapButton(int id, String imagePath, String label, int onClickCallback)
    {
        m_id = id;
        m_imagePath = imagePath;
        m_label = label;
        m_onClickCallback = onClickCallback;
    }

    MapButton(int id, int imageResourceId, String label, int onClickCallback)
    {
        m_id = id;
        m_imageResourceId = imageResourceId;
        m_label = label;
        m_onClickCallback = onClickCallback;
    }

    MapButton(MapButton rhs)
    {
        m_id = rhs.m_id;
        m_imagePath = rhs.m_imagePath;
        m_label = rhs.m_label;
        m_onClickCallback = rhs.m_onClickCallback;
        m_imageResourceId = rhs.m_imageResourceId;
    }

    public int getId()
    {
        return m_id;
    }

    public String getLabel()
    {
        return m_label;
    }

    String getImagePath()
    {
        return m_imagePath;
    }

    int getImageResourceId()
    {
        return m_imageResourceId;
    }

    int getOnClickCallback()
    {
        return m_onClickCallback;
    }

    @Override
    public int describeContents()
    {
        return 0;
    }

    @Override
    public void writeToParcel(Parcel dest, int flags)
    {
        dest.writeInt(this.m_id);
        dest.writeString(this.m_imagePath);
        dest.writeString(this.m_label);
        dest.writeInt(this.m_onClickCallback);
    }

    private MapButton(Parcel in)
    {
        this.m_id = in.readInt();
        this.m_imagePath = in.readString();
        this.m_label = in.readString();
        this.m_onClickCallback = in.readInt();
    }

    public static final Creator<MapButton> CREATOR = new Creator<MapButton>()
    {
        @Override
        public MapButton createFromParcel(Parcel source)
        {
            return new MapButton(source);
        }

        @Override
        public MapButton[] newArray(int size)
        {
            return new MapButton[size];
        }
    };

    @Override
    public boolean equals(Object o)
    {
        if (this == o) return true;
        if (o == null || getClass() != o.getClass()) return false;

        MapButton mapButton = (MapButton) o;

        return m_id == mapButton.m_id;
    }

    @Override
    public int hashCode()
    {
        return m_id;
    }
}
