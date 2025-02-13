package gov.census.cspro.smartsync.http;

import java.io.IOException;
import java.io.InputStream;

/**
 * Wraps native std::istream in a Java InputStream
 * 
 */
public class IStreamWrapper extends InputStream {

	private long nativeIStream;

	/**
	 * Create new wrapper from std::istream
	 * Wrapper does not take ownership of the istream
	 * so client must ensure not to delete it while
	 * wrapper is in use.
	 * 
	 * @param nativeIStreamPtr pointer to istream (std::istream*)
	 */
	public IStreamWrapper(long nativeIStreamPtr) {
		this.nativeIStream = nativeIStreamPtr;
	}
	
	@Override
	public native int read() throws IOException;

	@Override
	public native int read(byte[] b, int off, int len) throws IOException;
	
	@Override
	public int read(byte[] b) throws IOException {
		return read(b, 0, b.length);
	}
}
