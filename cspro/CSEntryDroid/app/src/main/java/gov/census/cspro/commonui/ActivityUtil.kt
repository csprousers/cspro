package gov.census.cspro.commonui

import android.app.Activity
import gov.census.cspro.engine.EngineMessage
import gov.census.cspro.engine.IEngineMessageCompletedListener
import gov.census.cspro.engine.Messenger
import kotlinx.coroutines.suspendCancellableCoroutine
import kotlin.coroutines.Continuation
import kotlin.coroutines.resume
import kotlin.coroutines.resumeWithException

suspend fun <T> Activity.runEngine(f: () -> T): T = suspendCancellableCoroutine { continuation: Continuation<T> ->

    var returnValue: T? = null

    val completedListener = IEngineMessageCompletedListener {
        continuation.resume(returnValue!!)
    }
    Messenger.getInstance().sendMessage(object : EngineMessage(this, completedListener) {
        override fun run() {
            try {
                returnValue = f()
            } catch (e: Throwable) {
                continuation.resumeWithException(e)
            }
        }
    })
}