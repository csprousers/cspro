package gov.census.cspro.util;

import java.util.Date;

import android.os.Parcel;
import android.os.Parcelable;

public class FileInfo implements Parcelable {
	
	private final String m_name;
	private final boolean m_isDirectory;
	private final long m_size;
	private Date m_lastModified;
	
	public FileInfo(String name, boolean isDirectory)
	{
		m_name = name;
		m_isDirectory = isDirectory;
		m_size = -1;
		m_lastModified = new Date();
	}
	
	public FileInfo(String name, boolean isDirectory, long size)
	{
		m_name = name;
		m_isDirectory = isDirectory;
		m_size = size;
		m_lastModified = new Date();
	}
	
	public FileInfo(String name, boolean isDirectory, long size, Date lastModified)
	{
		m_name = name;
		m_isDirectory = isDirectory;
		m_size = size;
		m_lastModified = lastModified;
	}
	
	public String getName()
	{
		return m_name;
	}
	
	public boolean getIsDirectory()
	{
		return m_isDirectory;
	}
	
	public long getSize()
	{
		return m_size;
	}
	
	public Date getLastModified()
	{
		return m_lastModified;
	}
	
	public long getLastModifiedTimeSeconds()
	{
		return m_lastModified.getTime()/1000;
	}
	
	public static FileInfo find(String filename, Iterable<FileInfo> infos)
	{
		for (FileInfo info : infos) {
			if (info.getName().equals(filename)) {
				return info;
			}
		}
		return null;
	}

    protected FileInfo(Parcel in) {
        m_name = in.readString();
        m_isDirectory = in.readByte() != 0x00;
        m_size = in.readLong();
        m_lastModified = new Date(in.readLong());
    }

    @Override
    public int describeContents() {
        return 0;
    }

    @Override
    public void writeToParcel(Parcel dest, int flags) {
        dest.writeString(m_name);
        dest.writeByte((byte) (m_isDirectory ? 0x01 : 0x00));
        dest.writeLong(m_size);
        dest.writeLong(m_lastModified.getTime());
    }

    public static final Parcelable.Creator<FileInfo> CREATOR = new Parcelable.Creator<FileInfo>() {
        @Override
        public FileInfo createFromParcel(Parcel in) {
            return new FileInfo(in);
        }

        @Override
        public FileInfo[] newArray(int size) {
            return new FileInfo[size];
        }
    };
}