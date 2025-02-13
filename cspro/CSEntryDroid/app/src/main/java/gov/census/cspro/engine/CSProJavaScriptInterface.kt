package gov.census.cspro.engine

import android.webkit.JavascriptInterface
import android.webkit.WebView
import org.json.JSONObject

class CSProJavaScriptInterface(private val actionInvoker: ActionInvoker) {

    var lastAsyncResult: String? = null

    private fun createActionInvokerMessage(action: String, argumentsJson: JSONObject?): String {
        // look at CSProHostObject::GetJavaScriptClassText for details on this
        val messageJson = JSONObject()
        messageJson.put("action", 11276) // 11276 is the code for "execute"
        messageJson.put("accessToken", EngineInterface.getInstance().oldCSProJavaScriptInterfaceGetAccessToken())

        val messageArgumentsJson = JSONObject()
        messageArgumentsJson.put("action", action)

        if( argumentsJson != null ) {
            messageArgumentsJson.put("arguments", argumentsJson)
        }

        messageJson.put("arguments", messageArgumentsJson)

        return messageJson.toString()
    }

    private fun getStringResultFromResponse(response: String): String? {
        return try {
            val responseJson = JSONObject(response)
            if(responseJson.getString("type") == "exception") {
                null
            }
            else {
                responseJson.getString("value")
            }
        }
        catch(e: Exception) {
            null
        }
    }

    private fun runSyncAndGetResultAsString(message: String): String? {
        return try {
            val response = actionInvoker.run(message)
            getStringResultFromResponse(response)
        }
        catch(e: Exception) {
            null
        }
    }

    private fun runLogicRelatedAsyncAndProcessCallback(message: String, callback: String?) {
        val handler = object : ActionInvoker.OldCSProObjectRunAsyncHandler() {
            override fun process(webView: WebView, response: String) {
                lastAsyncResult = getStringResultFromResponse(response)

                if( callback != null ) {
                    webView.post {
                        webView.evaluateJavascript(callback, null)
                    }
                }
            }
        }
        actionInvoker.runAsync(message, handler)
    }

    private fun getMaxDisplayDimensions(width: Boolean): Int {
        try {
            val message = createActionInvokerMessage("UI.getMaxDisplayDimensions", null)
            val result = runSyncAndGetResultAsString(message)
            if( result != null ) {
                val resultJson = JSONObject(result)
                return resultJson.getInt(if( width ) "width" else "height")
            }
        }
        catch(e: Exception) {
        }

        return getMaxDisplaySize(width)
    }

    @JavascriptInterface
    fun getMaxDisplayWidth(): Int {
        return getMaxDisplayDimensions(true)
    }

    @JavascriptInterface
    fun getMaxDisplayHeight(): Int {
        return getMaxDisplayDimensions(false)
    }


    @JavascriptInterface
    fun getInputData(): String {
        try {
            val message = createActionInvokerMessage("UI.getInputData", null)
            val result = runSyncAndGetResultAsString(message)
            if( result != null ) {
                return result
            }
        }
        catch(e: Exception) {
        }
        return ""
    }


    @JavascriptInterface
    fun setDisplayOptions(jsonText: String) {
        val argumentsJson = JSONObject(jsonText)
        val message = createActionInvokerMessage("UI.setDisplayOptions", argumentsJson)
        actionInvoker.run(message)
    }


    @JavascriptInterface
    fun returnData(jsonText: String) {
        val argumentsJson = JSONObject()
        argumentsJson.put("result", jsonText)
        val message = createActionInvokerMessage("UI.closeDialog", argumentsJson)
        actionInvoker.run(message)
    }


    // CSPro.getAsyncResult()
    @JavascriptInterface
    fun getAsyncResult(): String? {
        return lastAsyncResult
    }


    // CSPro.do(action)
    @JavascriptInterface
    fun `do`(action: String): String? {
        return if( action == "close" ) {
            val message = createActionInvokerMessage("UI.closeDialog", null)
            actionInvoker.run(message)
        } else {
             null
        }
    }

    // CSPro.do(action, input)
    @JavascriptInterface
    fun `do`(action: String, input: String): String? {
        return null
    }


    private fun createRunLogicMessage(logic: String): String {
        val argumentsJson = JSONObject()
        argumentsJson.put("logic", logic)
        return createActionInvokerMessage("Logic.eval", argumentsJson)
    }

    // CSPro.runLogic(logic)
    @JavascriptInterface
    fun runLogic(logic: String): String? {
        return try {
            val message = createRunLogicMessage(logic)
            runSyncAndGetResultAsString(message)
        }
        catch(e: Exception) {
            null
        }
    }

    // CSPro.runLogicAsync(logic)
    @JavascriptInterface
    fun runLogicAsync(logic: String) {
        val message = createRunLogicMessage(logic)
        runLogicRelatedAsyncAndProcessCallback(message, null)
    }

    // CSPro.runLogicAsync(logic, callback)
    @JavascriptInterface
    fun runLogicAsync(logic: String, callback: String) {
        val message = createRunLogicMessage(logic)
        runLogicRelatedAsyncAndProcessCallback(message, callback)
    }


    private fun createInvokeMessage(functionName: String, arguments: String?): String {
        val argumentsJson = JSONObject()
        argumentsJson.put("function", functionName)
        if( arguments != null && arguments != "undefined" ) {
            argumentsJson.put("arguments", JSONObject(arguments))
        }
        return createActionInvokerMessage("Logic.invoke", argumentsJson)
    }

    private fun runInvokeSync(functionName: String, arguments: String?): String? {
        return try {
            val message = createInvokeMessage(functionName, arguments)
            runSyncAndGetResultAsString(message)
        }
        catch(e: Exception) {
            null
        }
    }

    private fun runInvokeAsync(functionName: String, arguments: String?, callback: String?) {
        val message = createInvokeMessage(functionName, arguments)
        runLogicRelatedAsyncAndProcessCallback(message, callback)
    }

    // CSPro.invoke(functionName)
    @JavascriptInterface
    fun invoke(functionName: String): String? {
        return runInvokeSync(functionName, null)
    }

    // CSPro.invoke(functionName, arguments)
    @JavascriptInterface
    fun invoke(functionName: String, arguments: String): String? {
        return runInvokeSync(functionName, arguments)
    }

    // CSPro.invokeAsync(functionName)
    @JavascriptInterface
    fun invokeAsync(functionName: String) {
        runInvokeAsync(functionName, null, null)
    }

    // CSPro.invokeAsync(functionName, arguments)
    @JavascriptInterface
    fun invokeAsync(functionName: String, arguments: String) {
        runInvokeAsync(functionName, arguments, null)
    }

    // CSPro.invokeAsync(functionName, arguments, callback)
    @JavascriptInterface
    fun invokeAsync(functionName: String, arguments: String, callback: String) {
        runInvokeAsync(functionName, arguments, callback)
    }
}
