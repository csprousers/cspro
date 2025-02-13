package gov.census.cspro.smartsync.ftp;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.io.InputStream;
import java.net.URI;
import java.net.URISyntaxException;

import gov.census.cspro.engine.Util;
import gov.census.cspro.smartsync.SyncCancelException;
import gov.census.cspro.smartsync.SyncListenerWrapper;
import gov.census.cspro.smartsync.SyncLoginDeniedError;
import gov.census.cspro.util.FileInfo;
import it.sauronsoftware.ftp4j.FTPAbortedException;
import it.sauronsoftware.ftp4j.FTPClient;
import it.sauronsoftware.ftp4j.FTPCodes;
import it.sauronsoftware.ftp4j.FTPDataTransferException;
import it.sauronsoftware.ftp4j.FTPDataTransferListener;
import it.sauronsoftware.ftp4j.FTPException;
import it.sauronsoftware.ftp4j.FTPFile;
import it.sauronsoftware.ftp4j.FTPIllegalReplyException;
import it.sauronsoftware.ftp4j.FTPListParseException;
import timber.log.Timber;


@SuppressWarnings("unused")
public class AndroidFtpConnection {

	private SyncListenerWrapper listener;
	private final FTPClient m_ftpClient = new FTPClient();
	//private final CommunicationsListener m_communicationsListener = new CommunicationsListener();
	
	public AndroidFtpConnection() {
		//m_ftpClient.addCommunicationListener(m_communicationsListener);
		
	}
	
	private class TransferListener implements FTPDataTransferListener {

		long transferredSoFar = 0L;

		@Override
		public void aborted() {
		}

		@Override
		public void completed() {
		}

		@Override
		public void failed() {
		}

		@Override
		public void started() {
		}

		@Override
		public void transferred(int additionalBytes) {
			transferredSoFar += additionalBytes;
			listener.onProgress(transferredSoFar);
	    	if (listener.isCancelled()) {
				try {
				    Timber.i("abortCurrentTransfer");
					m_ftpClient.abortCurrentDataTransfer(true);
				} catch (IOException ignored) {
				} catch (FTPIllegalReplyException ignored) {
				}
	    	}
		}
	}
	
/*
	private class CommunicationsListener implements FTPCommunicationListener {

		@Override
		public void received(String msg) {
			Log.d(TAG, msg);
		}

		@Override
		public void sent(String msg) {
			Log.d(TAG, msg);
		}
		
	}
*/
	public void connect(String serverUrl, String username, String password) throws IllegalStateException, IOException, FTPIllegalReplyException, FTPException, URISyntaxException, SyncLoginDeniedError, FTPDataTransferException, FTPAbortedException, FTPListParseException
	{
		try
		{

			// ftp client can still be connected if call to connect() succeeds but call to login fails
			// in which case we need to disconnect before setting security and connecting again
			if (m_ftpClient.isConnected()) {
				m_ftpClient.disconnect(false);
			}

			URI uri = new URI(serverUrl);

			if (uri.getScheme() == null || uri.getScheme().equals("ftp")) {
				// For regular FTP URL, try FTPES first and fall back to regular FTP if that doesn't work
				m_ftpClient.setSecurity(FTPClient.SECURITY_FTPES);
			} else if (uri.getScheme().equals("ftpes")) {
				m_ftpClient.setSecurity(FTPClient.SECURITY_FTPES);
			} else if (uri.getScheme().equals("ftps")) {
				m_ftpClient.setSecurity(FTPClient.SECURITY_FTPS);
			} else {
				throw new URISyntaxException(serverUrl, "Invalid url. Must start with ftp://, ftpes:// or ftps://.");
			}

			try {
				connectToServer(uri, username, password);

			} catch (IllegalStateException | IOException | FTPIllegalReplyException | FTPException ex) {
				Timber.d(ex, "Failed to connect to FTP server %s", uri.getHost());
				if (uri.getScheme() == null || uri.getScheme().equals("ftp")) {
					Timber.d("Retrying FTP connection without TLS");
					try {
						m_ftpClient.disconnect(false);
					} catch (Exception ignored)
					{}
					m_ftpClient.setSecurity(FTPClient.SECURITY_FTP);
					connectToServer(uri, username, password);
				} else {
					throw ex;
				}
			}

			// If server supports compression (MODE Z) then use it
			if (m_ftpClient.isCompressionSupported())
				m_ftpClient.setCompressionEnabled(true);

			m_ftpClient.setType(FTPClient.TYPE_BINARY);
		} catch (FTPException ex) {
			if (ex.getCode() == FTPCodes.NOT_LOGGED_IN) {
				throw new SyncLoginDeniedError();
			} else {
				throw ex;
			}
		}
	}

	private void connectToServer(URI uri, String username, String password)
			throws IOException, FTPIllegalReplyException, FTPException, IllegalStateException, FTPDataTransferException, FTPAbortedException, FTPListParseException {
		if (uri.getPort() == -1 )
			m_ftpClient.connect(uri.getHost());
		else
			m_ftpClient.connect(uri.getHost(), uri.getPort());

		m_ftpClient.login(username, password);

		// Login is not enough to know if the connection is good
		// With FTPES on at least one server (Pure FTP-D)
		// you may only get an error when you actually try to read data channel
		// so do a directory listing and see if that throws an exception.
		m_ftpClient.list();
	}

	public void disconnect() throws IllegalStateException, IOException, FTPIllegalReplyException, FTPException
	{
		m_ftpClient.disconnect(true);
	}

	public void download(String remoteFilePath, String localFilePath) throws IllegalStateException, FileNotFoundException, IOException, FTPIllegalReplyException, FTPException, FTPDataTransferException, SyncCancelException
	{
        Timber.i("Start download");

        String dir = Util.removeFilename(remoteFilePath);
		m_ftpClient.changeDirectory(dir);
		String name = Util.removeDirectory(remoteFilePath);
		final long size = m_ftpClient.fileSize(name);
		if (listener.getTotal() <= 0)
		    listener.setTotal(size);
		File localFile = new File(localFilePath);
		try {
			m_ftpClient.download(name, localFile, new TransferListener());
		} catch (FTPAbortedException ex)
		{
            throw new SyncCancelException();
		} catch (Exception ex) {
		    if (listener.isCancelled())
		        throw new SyncCancelException();
		    else
		        throw ex;
        }

		Timber.i("Finished download cancel = %b", listener.isCancelled());
        if (listener.isCancelled())
            throw new SyncCancelException();

		if (localFile.length() < size) {
			throw new IOException("Size of downloaded file " + localFile.length() + " is less than size on server " + size);
		}
	}
	
	public long getLastModifiedTime(String remoteFilePath) throws IllegalStateException, IOException, FTPIllegalReplyException, FTPException
	{
		String dir = Util.removeFilename(remoteFilePath);
		m_ftpClient.changeDirectory(dir);
		String name = Util.removeDirectory(remoteFilePath);
		java.util.Date md = m_ftpClient.modifiedDate(name);
		return md.getTime()/1000;
	}
	
	public void upload(String localFilePath, String remoteFilePath) throws IllegalStateException, FileNotFoundException, IOException, FTPIllegalReplyException, FTPException, FTPDataTransferException, FTPListParseException, SyncCancelException
	{
		final File localFile = new File(localFilePath);
		final long localFileSize = localFile.length();
		FileInputStream localFileStream = new FileInputStream(localFile);
		upload(localFileStream, localFileSize, remoteFilePath);
	}

	public void upload(InputStream localFileStream, long localFileSize, String remoteFilePath) throws IllegalStateException, FileNotFoundException, IOException, FTPIllegalReplyException, FTPException, FTPDataTransferException, FTPListParseException, SyncCancelException
	{
		try {
			String dir = Util.removeFilename(remoteFilePath);
			// Ensure an absolute path so we don't add to existing cwd
			if (!dir.startsWith("/"))
				dir = "/" + dir;
			mkdirs(dir);
			m_ftpClient.changeDirectory(dir);
			
			final String name = Util.removeDirectory(remoteFilePath);
			
			// Upload to temporary file on server so that you don't get half a file
			// if the upload is interrupted
			final String tempName = "." + name + "__uploading.tmp";
			if (listener.getTotal() <= 0)
			    listener.setTotal(localFileSize);
			m_ftpClient.upload(tempName, localFileStream, 0, 0, new TransferListener());

			if (listener.isCancelled())
			    throw new SyncCancelException();

			// Make sure that uploaded file size matches local size
			final long remoteSize = m_ftpClient.fileSize(tempName);
			if (remoteSize < localFileSize) {
				m_ftpClient.deleteFile(tempName);
				throw new IOException("Size of uploaded file " + remoteSize + " is less than local file size " + localFileSize);
			}
			
			try {
				m_ftpClient.rename(tempName, name);
			} catch (FTPException ex) {
				// Some servers will not allow rename if the file
				// already exists so need to delete first
				m_ftpClient.deleteFile(name);
				m_ftpClient.rename(tempName, name);
			}
		} catch (FTPAbortedException ex)
		{
			throw new SyncCancelException();
		}
		catch (Exception ex) {
            if (listener.isCancelled())
                throw new SyncCancelException();
            else
                throw ex;
        }
	}

	public FileInfo[] getDirectoryListing(String remotePath) throws IllegalStateException, IOException, FTPIllegalReplyException, FTPException, FTPDataTransferException, FTPAbortedException, FTPListParseException
	{
		m_ftpClient.changeDirectory(remotePath);
		
		FTPFile[] ftpFiles = m_ftpClient.list();
		FileInfo[] fileInfos = new FileInfo[ftpFiles.length];
		
		for (int i = 0; i < ftpFiles.length; ++i) {
			fileInfos[i] = new FileInfo(ftpFiles[i].getName(), ftpFiles[i].getType() == FTPFile.TYPE_DIRECTORY, ftpFiles[i].getSize(), ftpFiles[i].getModifiedDate());
		}
		
		return fileInfos;
	}

	public void setListener(SyncListenerWrapper listener)
	{
		this.listener = listener;
	}

	private void mkdirs(String dir) throws IllegalStateException, IOException, FTPIllegalReplyException, FTPException, FTPDataTransferException, FTPAbortedException, FTPListParseException
	{
		if (dir.isEmpty() || dir.equals(File.separator)) {
			// Root dir must always exist.
			return;
		}
								
		// Ensure that parent is created before making child.
		String parent = Util.removeFilename(dir);
		
		// Don't recurse on the parent in case where the path is relative and has no root
		// e.g. "foo/", in this case removeFilename("foo/") = "foo/" 
		if (!parent.equals(dir)) {
			mkdirs(parent);
		}
				
		// Make the child.
		if(!directoryExists(dir))
		{
			m_ftpClient.createDirectory(dir);
		}
	}
	
	private boolean directoryExists(String dir) throws IllegalStateException, IOException, FTPIllegalReplyException, FTPException, FTPDataTransferException, FTPAbortedException, FTPListParseException
	{
		// remove any trailing slashes from the directory name
		int newLen = dir.length();

		while( newLen > 0 && dir.charAt(newLen - 1) == '/' )
			newLen--;
		
		if( newLen != dir.length() )
			dir = dir.substring(0,newLen);
		
		// some FTP servers do silly things like STAT-ing a
		// directory that doesn't exist
		// instead, list the previous directory, then see if the supplied dir
		// is part of the list
		int			index		= dir.lastIndexOf('/');
		String		parent		= dir.substring(0, index + 1);
		String		dirName		= dir.substring(index + 1);
		
		// list the parent directory
		FTPFile[] 	ftpFiles	= m_ftpClient.list(parent);
		if(ftpFiles != null)
		{
			for(FTPFile ftpFile : ftpFiles)
			{
				if(ftpFile.getName().equalsIgnoreCase(dirName))
					return true;
			}
		}
		return false;
	}}
