package gov.census.cspro.smartsync.p2p;

import java.io.IOException;
import java.util.concurrent.ExecutionException;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.TimeUnit;

import android.bluetooth.BluetoothSocket;
import gov.census.cspro.util.CancelableTaskRunner;

public class BluetoothObexTransport {

	private BluetoothSocket m_socket;
	private final CancelableTaskRunner m_runner = new CancelableTaskRunner();
	private final ExecutorService m_threadPool = Executors.newSingleThreadExecutor();
	
	public BluetoothObexTransport(BluetoothSocket socket) {
		m_socket = socket;
	}
	
	public void close() {
		try {
			m_socket.close();
		} catch (IOException e) {
		}
	}
	
	public void write(byte[] data) throws IOException {
		m_socket.getOutputStream().write(data);
	}
	
	public int read(byte[] buffer, int byteCount, int timeoutMs) throws IOException {
		try {
			return m_runner.run(m_threadPool, 
					new ReadTask(m_socket, buffer, byteCount),
					null, timeoutMs, TimeUnit.MILLISECONDS);
		} catch (ExecutionException e) {
			throw new IOException(e);
		}
	}
	
	private static class ReadTask implements CancelableTaskRunner.CancelableTask<Integer> {

		private final BluetoothSocket m_socket;
		private final byte[] m_buffer;
		private final int m_byteCount;
		
		ReadTask(BluetoothSocket socket, byte[] buffer, int byteCount) {
			m_socket = socket;
			m_buffer = buffer;
			m_byteCount = byteCount;
		}
		
		@Override
		public Integer run() throws IOException {
			return m_socket.getInputStream().read(m_buffer, 0, m_byteCount);
		}

		@Override
		public void abort() {
			// Closing the socket will cause the synchronous read to
			// throw an exception and terminate.
			try {
				m_socket.close();
			} catch (IOException e) {
			}
		}
	}
}
