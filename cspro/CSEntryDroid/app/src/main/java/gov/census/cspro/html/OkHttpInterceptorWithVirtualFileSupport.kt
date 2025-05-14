package gov.census.cspro.html

import gov.census.cspro.engine.EngineInterface
import gov.census.cspro.html.VirtualFileMappingPathHandler.Companion.URL_PREFIX
import okhttp3.Interceptor
import okhttp3.MediaType.Companion.toMediaTypeOrNull
import okhttp3.ResponseBody.Companion.toResponseBody
import okhttp3.Protocol
import okhttp3.Response
import timber.log.Timber


class OkHttpInterceptorWithVirtualFileSupport : Interceptor {

    override fun intercept(chain: Interceptor.Chain): Response {
        val request = chain.request()
        val url = request.url.toString()

        // intercept URLs that start with "https://appassets.androidplatform.net/lfs/"
        if( VirtualFileMappingPathHandler.isVirtualFileMappingUrl(url) ) {
            try {
                // create a path to match what was designed to process inputs from VirtualFileMappingPathHandler
                val path = url.substring(VirtualFileMappingPathHandler.URL_PREFIX.length)

                val virtualFile = EngineInterface.getInstance().getVirtualFile(path)

                if( virtualFile != null ) {
                    val body = virtualFile.content.toResponseBody(virtualFile.contentType?.toMediaTypeOrNull())

                    return Response.Builder()
                        .request(request)
                        .protocol(Protocol.HTTP_1_1)
                        .code(200)
                        .message("OK")
                        .body(body)
                        .build()
                }
            } catch( e: Exception ) {
                Timber.e(e)
            }
        }

        // proceed normally if not a virtual file or on error
        return chain.proceed(request)
    }

}
