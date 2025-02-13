package gov.census.cspro.util;

import java.util.concurrent.Callable;
import java.util.concurrent.CancellationException;
import java.util.concurrent.ExecutionException;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.FutureTask;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.TimeoutException;
import timber.log.Timber;

/**
 * Execute long running tasks with cancel and timeout.
 * 
 * Run tasks that make synchronous calls that take a long time or can go
 * on forever in a background thread to allow timeouts and cancel.
 * 
 * Pass an implementation of CancellableTask to CancellableTaskRunner.run
 * and it will be run in background thread while the calling thread
 * periodically checks for cancellation and timeout and forces the background
 * task to be cancelled if the task is cancelled or the timeout passes.
 *
 */
public class CancelableTaskRunner {
	
	/**
	 * Task that can be cancelled.
	 * 
	 * @param <T> Return type of task
	 */
	public interface CancelableTask<T> {
	
		/**
		 * Run method is called in background thread to execute long
		 * running task.
		 */
		T run() throws Exception;

		/**
		 * Force the task to end by signaling/closing anything that
		 * the code in run() is waiting on. For example closing a socket
		 * or a calling quit() on a looper.
		 */
		void abort();
	}
	
	/**
	 * Check for cancellation of task.
	 */
	public interface CancelChecker {
		
		/**
		 * Return true if task execution has been cancelled.
		 */
		boolean isCancelled();
	}
	
	/**
	 * Execute long running task in background checking for cancel in calling thread
	 * @param threadPool Thread pool to launch task in (usually Executors.newSingleThreadExecutor())
	 * @param task Long running operation to execute
	 * @param cancelChecker Interface to check for cancellation
	 * @param timeout amount of time to wait before automatically canceling task or -1 for no timeout
	 * @param timeoutUnit unit for value of timeout
	 * @return result task.run()
	 * @throws ExecutionException
	 */
	public <T> T run(final ExecutorService threadPool, final CancelableTask<T> task, final CancelChecker cancelChecker, int timeout, TimeUnit timeoutUnit) throws ExecutionException
	{
		Timber.d("Starting CancelableTask with timeout %d %s", timeout, timeoutUnit);
		
		// Start task in a background thread
		FutureTask<T> futureTask = new FutureTask<>(
				new Callable<T>() {
					@Override
					public T call() throws Exception {
						return task.run();
					}});
		threadPool.execute(futureTask);
		
		// Check for cancel 10 times/second.
		final long timeoutNs = timeout >= 0 ? timeoutUnit.toNanos(timeout) : -1;
		long startTime = System.nanoTime();
		while ((cancelChecker == null || !cancelChecker.isCancelled()) && 
				(timeout < 0 || (System.nanoTime() - startTime <= timeoutNs))) {
			try {
				T result = futureTask.get(100, TimeUnit.MILLISECONDS);
				Timber.d("CancelableTask completed successfully");
				return result;
			} catch (TimeoutException ignored) {
			} catch (InterruptedException e) {
				break;
			}
		}
		if (!futureTask.isDone()) {
			// Cancel the task due to timeout or manual cancel.
			// FutureTask.cancel tries to force an interrupted exception but
		    // won't do anything if the the thread isn't currently
			// in a call to sleep or similar. CancellableTask.abort() should
			// be implemented to do something that will really end the task like
			// close the socket being read.
			Timber.d("Aborting CancelableTask after " + (System.nanoTime() / 1e9) + " seconds and cancel = " + cancelChecker.isCancelled());
			futureTask.cancel(true); 
			task.abort();
			try {
				// Wait for cancel to actually happen in background
				// thread so that we don't get zombie tasks hanging
				// around.
				futureTask.get();
			} catch (InterruptedException ignored) {
			} catch (ExecutionException ignored) {
			} catch (CancellationException ignored) {
			}
		}
		return null;
	}
}
