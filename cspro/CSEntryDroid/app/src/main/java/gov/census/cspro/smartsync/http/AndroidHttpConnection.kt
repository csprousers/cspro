package gov.census.cspro.smartsync.http

import gov.census.cspro.smartsync.SyncCancelException
import gov.census.cspro.smartsync.SyncListenerWrapper
import timber.log.Timber
import java.io.IOException
import java.io.InputStream
import java.io.OutputStream
import java.net.HttpURLConnection
import java.net.URL

class AndroidHttpConnection {

    private var listener: SyncListenerWrapper? = null
    fun setListener(listener: SyncListenerWrapper?) {
        this.listener = listener
    }

    /**
     * Send an http request
     *
     * @param requestMethod "get", "post"...
     * @param url full url to request
     * @param postData optional data to post (or null)
     * @param postDataSize optional size of postData (or -1 if size unknown)
     * @param requestHeaders list of http headers to send with request
     * @return http response
     * @throws IOException
     * @throws SyncCancelException
     */
    @Throws(IOException::class, SyncCancelException::class)
    fun request(requestMethod: String, url: String, postData: IStreamWrapper?, postDataSize: Int,
                requestHeaders: Array<String>): HttpResponse {
        Timber.d("%s: %s", requestMethod, url)
        val urlConnection = URL(url).openConnection() as HttpURLConnection
        urlConnection.connectTimeout = 15 * 1000 // 15 seconds
        urlConnection.readTimeout = 3 * 60 * 1000 // 3 minutes
        return try {
            addRequestHeaders(requestHeaders, urlConnection)
            urlConnection.requestMethod = requestMethod
            urlConnection.setRequestProperty("Connection", "close")
            if (postData != null) {
                sendPostData(postData, postDataSize, urlConnection)
            }
            Timber.d("Content-length: %s", urlConnection.contentLength)
            Timber.d("Result %d", urlConnection.responseCode)
            HttpResponse(urlConnection)
        } catch (e: IOException) {
            Timber.e(e, "Http error")
            Timber.d("Result %d", urlConnection.responseCode)
            HttpResponse(urlConnection)
        }
    }

    @Throws(IOException::class, SyncCancelException::class)
    private fun sendPostData(postData: IStreamWrapper, postDataSize: Int,
                             urlConnection: HttpURLConnection) {
        urlConnection.doOutput = true
        if (postDataSize > 0) {
            urlConnection.setFixedLengthStreamingMode(postDataSize)
            listener?.let { if (it.total <= 0) it.total = postDataSize.toLong() }
        } else {
            urlConnection.setChunkedStreamingMode(0)
        }
        copyStreamsWithProgress(postData, urlConnection.outputStream)
    }

    private fun addRequestHeaders(headers: Array<String>, urlConnection: HttpURLConnection) {
        headers.forEach {
            val (name, value) = it.split(':', limit = 2)
            urlConnection.setRequestProperty(name, value)
        }
    }

    @Throws(IOException::class, SyncCancelException::class)
    private fun copyStreamsWithProgress(inputStream: InputStream, outputStream: OutputStream) {
        val bufferSize = 1024 * 5
        val buffer = ByteArray(bufferSize)
        var read: Int
        var totalRead: Long = 0
        while (inputStream.read(buffer).also { read = it } != -1) {
            if (listener?.isCancelled == true) throw SyncCancelException()
            totalRead += read.toLong()
            listener?.onProgress(totalRead)
            outputStream.write(buffer, 0, read)
        }
    }
}