/***************************************************************************************
 * 
 * CSEntry for Android
 * 
 * Module:		IEngineMessageCompletedListener.java
 * 
 * Description: The observer interface used for signaling Messenger users that their
 * 				message execution has completed.
 * 
 **************************************************************************************/
package gov.census.cspro.engine;

public interface IEngineMessageCompletedListener 
{
	public void onMessageCompleted(EngineMessage msg);
}
