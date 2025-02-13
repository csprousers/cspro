package gov.census.cspro.smartsync.http

import java.io.IOException
import java.io.InputStream
import java.net.HttpURLConnection

class HttpResponse(private val urlConnection: HttpURLConnection) {

    fun getBody(): InputStream? {
        return try {
            urlConnection.inputStream
        } catch (e: IOException) {
            urlConnection.errorStream
        }
    }

    fun getHeaders() = urlConnection.headerFields.map { header -> header.value.map { "${header.key}: $it" } }.flatten()

    fun getHttpStatus() = urlConnection.responseCode

    fun close()
    {
        urlConnection.disconnect()
    }
}