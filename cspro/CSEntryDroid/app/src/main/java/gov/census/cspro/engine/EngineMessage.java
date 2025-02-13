/***************************************************************************************
 * 
 * CSEntry for Android
 * 
 * Module:		EngineMessage.java
 * 
 * Description: Dispatch object for the Java/JNI/Engine messaging component.  This
 * 				class is abstract by design and forces callers to instantiate an
 * 				execution reference.  This class uses Runnable to perform execution
 * 				theoretically making it suitable for one off thread pool operations.
 * 
 **************************************************************************************/
package gov.census.cspro.engine;

import android.app.Activity;

public abstract class EngineMessage implements Runnable
{
	private long							m_lparam			= -1;
	private long							m_wparam			= -1;
	private Object							m_object			= null;
	private IEngineMessageCompletedListener m_completedListener = null;
	protected Activity						m_activity			= null;
	protected long result;

	protected EngineMessage()
	{
		
	}
	protected EngineMessage(IEngineMessageCompletedListener listener)
	{
		m_completedListener = listener;
	}
	
	protected EngineMessage(Activity activity, IEngineMessageCompletedListener listener)
	{
		this(listener);
		
		m_activity = activity;
	}
	
	public IEngineMessageCompletedListener getCompletedListener()
	{
		return m_completedListener;
	}
	
	public Activity getActivity()
	{
		return m_activity;
	}
	
	public long getLParam()
	{
		return m_lparam;
	}
	
	public void setLParam(long lparam)
	{
		m_lparam = lparam;
	}
	
	public long getWParam()
	{
		return m_wparam;
	}
	
	public void setWParam(long wparam)
	{
		m_wparam = wparam;
	}
	
	public void setObject(Object object)
	{
		m_object = object;
	}

	public Object getObject()
	{
		return m_object;
	}
	
	public long getResult()
	{
		return result;
	}
	
	public void setResult(long value)
	{
		result = value;
	}

}
