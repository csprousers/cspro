package gov.census.cspro.engine

import android.webkit.GeolocationPermissions
import android.webkit.PermissionRequest
import android.webkit.WebChromeClient

class ActionInvokerWebChromeClient : WebChromeClient() {
    private val options: MutableSet<String> = mutableSetOf()

    fun addOption(option: String) {
        options.add(option)
    }

    override fun onPermissionRequest(request: PermissionRequest) {
        val allowedResources = request.resources.filter { resource ->
            options.contains(resource)
        }.toTypedArray()

        if (allowedResources.isNotEmpty()) {
            request.grant(allowedResources)
        } else {
            request.deny()
        }
    }

    override fun onGeolocationPermissionsShowPrompt(
        origin: String?,
        callback: GeolocationPermissions.Callback?
    ) {
        val allowed = options.contains("cspro.geolocation")
        callback?.invoke(origin, allowed, false)
    }
}
