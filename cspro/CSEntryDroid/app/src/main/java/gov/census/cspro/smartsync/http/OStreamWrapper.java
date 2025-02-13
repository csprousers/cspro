package gov.census.cspro.smartsync.http;

import java.io.IOException;
import java.io.OutputStream;

/**
 * Wraps a native ostream pointer so that it can be used as
 * a java OutputStream.
 *
 */
public class OStreamWrapper extends OutputStream {

	private long nativeOStream;

	/**
	 * Create instance of wrapper
	 * @param nativeOutputStreamPtr C pointer to ostream
	 */
	public OStreamWrapper(long nativeOutputStreamPtr)
	{
		this.nativeOStream = nativeOutputStreamPtr;
	}
	
	@Override
	public native void write(int oneByte) throws IOException;

	@Override
	public void write(byte[] b) throws IOException
	{
		write(b, 0, b.length);
	}
	
	@Override
	public native void write(byte[] b, int off, int len) throws IOException;
	
	@Override
	public native void flush() throws IOException;
}
