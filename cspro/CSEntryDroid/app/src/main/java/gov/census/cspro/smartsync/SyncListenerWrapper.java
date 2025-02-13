package gov.census.cspro.smartsync;

/**
 * Wrapper around native ISyncListener
 * so that it can be called from Java
 *
 */
public class SyncListenerWrapper {

	private long nativeListener;
	
	/**
	 * Create new wrapper from native listener.
	 * Does not take ownership so caller must ensure that
	 * it is not destroyed while wrapper is in use.
	 * 
	 * @param nativeListenerPtr pointer to native listener (ISyncListener*)
	 */
	SyncListenerWrapper(long nativeListenerPtr)
	{
		this.nativeListener = nativeListenerPtr;
	}

	public native void setTotal(long total);

	public native long getTotal();

	public native void onProgress(long progress);
	
	public native boolean isCancelled();
}
