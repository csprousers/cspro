package gov.census.cspro.engine;

import android.app.Activity;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import java.lang.ref.WeakReference;
import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;

/**
 * Keep track of which activities are currently active.
 * An Activity is active if start() has been called but stop() has not yet
 * been called. When active, it is ok to modify the UI associated with the Activity.
 *
 * To use, call onActivityStarted() whenever an Activity is started and onActivityStopped()
 * when an activity is stopped. Use Application.ActivityLifecycleCallbacks to get these callbacks
 * globally for all activities.
 *
 * You can then check if an Activity is active using isActive() and get the current (last started)
 * Activity using current().
 */
class ActiveActivityTracker {
    private final List<WeakReference<Activity>> activityStack = new ArrayList<>();

    void onActivityStarted(@NonNull Activity activity)
    {
        add(activity);
    }

    void onActivityStopped(@NonNull Activity activity)
    {
        remove(activity);
    }

    @Nullable Activity current()
    {
        for (int i = activityStack.size() - 1; i >= 0; i--) {
            Activity a = activityStack.get(i).get();
            if (a != null)
                return a;
        }
        return null;
    }

    boolean isActive(@NonNull Activity activity)
    {
        return contains(activity);
    }

    private void add(@NonNull Activity activity)
    {
        remove(activity);
        activityStack.add(new WeakReference<>(activity));
    }

    private void remove(@NonNull Activity activity)
    {
        Iterator<WeakReference<Activity>> i = activityStack.iterator();
        while (i.hasNext()) {
            WeakReference<Activity> activityWeakReference = i.next();
            if (activityWeakReference.get() == activity)
                i.remove();
        }
    }

    private boolean contains(@NonNull Activity activity)
    {
        for (WeakReference<Activity> activityWeakReference : activityStack) {
            if (activityWeakReference.get() == activity)
                return true;
        }
        return false;
    }
}
