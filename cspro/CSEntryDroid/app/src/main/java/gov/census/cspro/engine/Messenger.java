/*
 *
 * CSEntry for Android
 *
 * Module:      Messenger.java
 *
 * Description: Java/JNI/Engine messaging component used for communicating from the
 *              native software layer to the Java user interface.  This component
 *              exposes static methods invoked directly from native code using
 *              JNI exported methods.  The primary means of messaging is performed
 *              using a standalone worker thread in which messages are stored FIFO.
 *              Users most supply a context if a dialog box is to be presented
 *              and an optional listener can be supplied so that callers can be
 *              signaled when a message has completed execution.
 *
 *              There are two main threads of execution the main Android rendering
 *              thread and the engine thread. The engine thread is used for running
 *              the native CSPro engine code. To schedule code to run in the engine
 *              thread from the main thread call Messenger.sendMessage() and pass
 *              it an object that extends EngineMessage. The message has a run method
 *              that will be called in the engine thread. EngineMessages are run
 *              asynchronously however the engine message can take a listener
 *              that will be executed in the main thread upon completion of the run
 *              method in the engine thread.
 *
 *              The messenger is also used for code in the engine thread to
 *              run code in the main thread, to show dialogs or modify the UI.
 *              This is done by calling Messenger.runLongEngineFunction or
 *              Messenger.runStringEngineFunction and passing it an object
 *              that extends EngineFunction. The EngineFunction has a run
 *              method that will be executed in the main (UI) thread.
 *              Unlike EngineMessages, EngineFunctions are run synchronously.
 *              Messenger.runLongEngineFunction and Messenger.runStringEngineFunction
 *              cause the engine thread to wait until the function has completed in
 *              the main thread. In order for an EngineFunction to signal that is
 *              has completed and to supply a return value it must call
 *              Messenger.engineFunctionComplete. Simply allowing the run
 *              method to finish does not stop the engine thread from waiting.
 *              This allows creation of engine functions that wait on UI events
 *              such as a button press or completion of an activity.
 *
 *              To execute code in the engine thread
 *
 **************************************************************************************/

package gov.census.cspro.engine;

import android.app.Activity;
import android.app.Application;
import android.content.Intent;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;

import androidx.activity.ComponentActivity;
import androidx.activity.result.ActivityResult;
import androidx.activity.result.ActivityResultCallback;
import androidx.activity.result.ActivityResultLauncher;
import androidx.activity.result.ActivityResultRegistry;
import androidx.activity.result.contract.ActivityResultContracts;
import androidx.annotation.MainThread;
import androidx.annotation.NonNull;

import org.jetbrains.annotations.NotNull;

import java.util.LinkedList;
import java.util.Queue;

import gov.census.cspro.engine.functions.EngineFunction;
import timber.log.Timber;


public class Messenger implements Runnable, Application.ActivityLifecycleCallbacks
{
    private static Messenger        m_messenger     = null;

    private boolean                 m_stop          = true;
    private EngineMessage           m_currentMsg    = null;
    private final Queue<EngineMessage>  m_msgQ          = new LinkedList<>();

    private Object m_engineFunctionReturnValue;
    private volatile boolean m_engineFunctionRunningOnUIThread = false;
    private final Object m_runEngineFunctionOnUiThreadLock = new Object();
    private ActiveActivityTracker m_activeActivities = new ActiveActivityTracker();
    private Queue<EngineFunction> m_queuedEngineFunctions = new LinkedList<>();
    private Queue<EngineMessage> m_queuedMessageCompletedListeners = new LinkedList<>();
    private Handler m_handler = new Handler(Looper.getMainLooper());
    public static void CreateMessengerInstance()
    {
        m_messenger = new Messenger();
    }

    public static Messenger getInstance()
    {
        return m_messenger;
    }

    public EngineMessage getCurrentMessage()
    {
        return m_currentMsg;
    }

    private EngineMessage getMessage() throws InterruptedException
    {
        EngineMessage msg = null;

        if(!m_stop)
        {
            synchronized (m_msgQ)
            {
                if(m_msgQ.size() == 0)
                {
                    // if there is nothing in the Q
                    // wait until we get some work
                    // we're waiting forever here, if we should fall through
                    // for some reason then we can always change this value
                    m_msgQ.wait();
                }

                // we only want to process work if we're continuing to run
                // we could have work items in the Q, but we shouldn't do
                // anything with them
                if(!m_stop && m_msgQ.size() > 0)
                {
                    msg = m_msgQ.remove();
                }
            }
        }

        return msg;
    }

    private class EngineMessageCompletedRunnable implements Runnable {

        // Stash the message in the runnable to avoid case where m_currentMsg
        // gets changed before UI thread runs the runnable.
        private final EngineMessage m_message;

        EngineMessageCompletedRunnable(EngineMessage msg)
        {
            m_message = msg;
        }

        @MainThread
        @Override
        public void run()
        {
            if (m_activeActivities.isActive(m_message.getActivity()))
            {
                Timber.d("Notify message completed listener on UI Thread: %s", m_message.getClass().getName());
                m_message.getCompletedListener().onMessageCompleted(m_message);
                Timber.d("Done notifying message completed listener on UI Thread");
            } else {
                Timber.d("Message completed but current activity in background, queuing for when activity is brought back: %s", m_message.getClass().getName());
                m_queuedMessageCompletedListeners.add(m_message);
            }
        }
    }

    @Override
    public void run()
    {
        // set the run flag for infinitum
        m_stop = false;

        while(!m_stop)
        {
            try
            {
                // get the next message off of the Q
                m_currentMsg = getMessage();
                // this is a double check for own purposes
                if(m_currentMsg != null)
                {
                    Timber.d("Start running message %s", m_currentMsg.getClass().getSimpleName());
                    // run the runnable
                    m_currentMsg.run();
                    // if the caller supplied a listener object
                    // notify them that the task has completed
                    if(m_currentMsg.getCompletedListener() != null)
                    {
                        Runnable r = new EngineMessageCompletedRunnable(m_currentMsg);
                        //post to UI thread -*IMPORTANT* //This allows mods of UI in OnMessageCompleted
                        m_handler.post(r);
                    }
                    Timber.d("Finished running message %s", m_currentMsg.getClass().getSimpleName());
                }
            }
            catch(InterruptedException ie)
            {
                // an interrupted execption occurs when there is a problem
                // with the synchronization framework in java, this is more
                // serious than some user created exception
                // stop processing
                m_stop = true;
                Timber.e(ie, "An Error Occurred While Processing message %s", m_currentMsg.getClass().getSimpleName());
            }
            catch (Exception ex)
            {
                // this catch block is for user created exceptions in their own code
                // consume the exception but log what's going on
                Timber.e(ex, "An Error Occurred While Processing message %s", m_currentMsg != null ? m_currentMsg.getClass().getSimpleName() : "null");
            }
        }
    }

    @MainThread
    public void sendMessage(EngineMessage msg)
    {
        synchronized (m_msgQ)
        {
            // push the message on the Q
            m_msgQ.add(msg);
            // notify callers that they have work to do
            m_msgQ.notify();
        }
    }

    void postMessage(EngineMessage msg)
    {
        synchronized( m_msgQ )
        {
            m_msgQ.add(msg);
        }
    }

    private void signalComplete()
    {
        synchronized (m_runEngineFunctionOnUiThreadLock)
        {
            // signal callers waiting on the Q
            m_engineFunctionRunningOnUIThread = false;
            m_runEngineFunctionOnUiThreadLock.notify();
        }
    }

    public void stop()
    {
        synchronized (m_msgQ)
        {
            m_stop = true;
            m_msgQ.notifyAll();
        }
    }

    @Override
    public void onActivityCreated(Activity activity, Bundle bundle)
    {
        Timber.d("Create activity: %s", activity.getClass().getName());
    }

    @Override
    public void onActivityStarted(Activity activity)
    {
        Timber.d("Start activity: %s", activity.getClass().getName());
        m_activeActivities.onActivityStarted(activity);
    }

    @Override
    public void onActivityResumed(Activity activity) {
        Timber.d("Resume activity: %s", activity.getClass().getName());

        // Run message completed listeners that were queued while activity was stopped
        while (!m_queuedMessageCompletedListeners.isEmpty())
        {
            final EngineMessage message = m_queuedMessageCompletedListeners.poll();
            if (message != null) {
                if (m_activeActivities.isActive(message.getActivity())) {
                    Timber.d("Run queued message completed listener: %s", message.getClass().getName());
                    message.getCompletedListener().onMessageCompleted(message);
                } else {
                    Timber.d("Queued message completed listener ignored because Activity is no longer active: %s", message.getClass().getName());
                }
            }
        }

        // Run engine functions that were queued while activity was stopped
        while (!m_queuedEngineFunctions.isEmpty())
        {
            final EngineFunction f = m_queuedEngineFunctions.poll();
            if (f != null) {
                Timber.d("Run queued engine function: %s", f.getClass().getName());
                f.runEngineFunction(m_activeActivities.current());
            }
        }
    }

    @Override
    public void onActivityPaused(Activity activity) {
        Timber.d("Paused activity: %s", activity.getClass().getName());
    }

    @Override
    public void onActivityStopped(Activity activity) {
        Timber.d("Stopped activity: %s", activity.getClass().getName());
        m_activeActivities.onActivityStopped(activity);
    }

    @Override
    public void onActivitySaveInstanceState(Activity activity, @NotNull Bundle bundle) {
        Timber.d("SaveInstanceState activity: %s", activity.getClass().getName());
    }

    @Override
    public void onActivityDestroyed(@NotNull Activity activity)
    {
    }

    class EngineRunnable implements Runnable {

        private EngineFunction m_function;

        private EngineRunnable(EngineFunction function)
        {
            m_function = function;
        }

        public void run()
        {
            // Run with current activity in case function wants to display UI on top
            // of current activity.
            Activity current = m_activeActivities.current();
            if (current == null) {
                // No current activity
                m_queuedEngineFunctions.add(m_function);
                Timber.d("No current activity or current activity stopped, queue message: %s", m_function.getClass().getName());
            } else
            {
                m_function.runEngineFunction(current);
            }
        }
    }


    private void runEngineFunction(@NonNull EngineFunction function)
    {
        synchronized (m_runEngineFunctionOnUiThreadLock) {
            m_engineFunctionRunningOnUIThread = true;
            m_handler.post(new EngineRunnable(function));
            while (m_engineFunctionRunningOnUIThread) {
                try
                {
                    m_runEngineFunctionOnUiThreadLock.wait();
                }
                catch(InterruptedException ignored)
                {
                }
            }
        }
    }

    public long runLongEngineFunction(@NonNull EngineFunction function)
    {
        runEngineFunction(function);
        return (long) m_engineFunctionReturnValue;
    }

    public String runStringEngineFunction(@NonNull EngineFunction function)
    {
        runEngineFunction(function);
        return (String) m_engineFunctionReturnValue;
    }

    public Bundle runBundleEngineFunction(@NonNull EngineFunction function)
    {
        runEngineFunction(function);
        return (Bundle) m_engineFunctionReturnValue;
    }

    public Object runObjectEngineFunction(@NonNull EngineFunction function)
    {
        runEngineFunction(function);
        return m_engineFunctionReturnValue;
    }

    public void runEngineFunctionDirectlyWithoutUsingMessengerQueue(@NonNull EngineFunction function)
    {
        m_handler.post(new EngineRunnable(function));
    }

    public void engineFunctionComplete(long retVal)
    {
        engineFunctionComplete(Long.valueOf(retVal));
    }

    public void engineFunctionComplete(Object retVal)
    {
        if (activityResultLauncher != null) {
            activityResultLauncher.unregister();
            activityResultLauncher = null;
        }

        m_engineFunctionReturnValue = retVal;
        signalComplete();
    }

    public void startActivityForResultFromEngineFunction(Activity activity, final ActivityResultCallback<ActivityResult> onResultCallback, Intent intent) {
        if (activity instanceof ComponentActivity) {
            final ActivityResultRegistry registry = ((ComponentActivity) activity).getActivityResultRegistry();
            activityResultLauncher = registry.register("EngineFunction", new ActivityResultContracts.StartActivityForResult(), onResultCallback);
            activityResultLauncher.launch(intent);
        } else {
            throw new IllegalArgumentException("Can't do startActivityForResultFromEngineFunction with an Activity that isn't a ComponentActivity");
        }
    }

    private ActivityResultLauncher<Intent> activityResultLauncher = null;
}
