#include "stdafx.h"
#include "CSProHostObject.h"
#include "HtmlViewCtrl.h"
#include "UseHtmlDialogs.h" // temporarily for OldCSProJavaScriptInterface::GetAccessToken
#include <zToolsO/Serializer.h>


// for whatever reason, passing CCmdTarget::GetIDispatch to WebView2 does not
// seem to support providing real functions with arguments, so the functions defined
// below can instead be called as properties, and sendMessage has to be handled by
// using a property to set the message and then a property to get the response

BEGIN_DISPATCH_MAP(CSProHostObject, CCmdTarget)
    // Action Invoker
    DISP_PROPERTY_EX(CSProHostObject, "ActionInvoker", ActionInvokerSyncGetProcessedMessage, ActionInvokerSyncSetMessageSync, VT_BSTR)
    DISP_PROPERTY_EX(CSProHostObject, "ActionInvokerAsync", ActionInvokerAsyncGetUnused, ActionInvokerAsyncSetMessage, VT_BSTR)
    DISP_PROPERTY_EX(CSProHostObject, "ActionInvokerAsyncCSPro", ActionInvokerOldCSProClassGetRequestId, ActionInvokerOldCSProClassSetMessage, VT_BSTR)
END_DISPATCH_MAP()


CSProHostObject::CSProHostObject(HtmlViewCtrl* source_window)
    :   m_sourceWindow(source_window),
        m_actionInvokerWebController(m_sourceWindow),
        m_actionInvokerSyncMessageId(0),
        m_actionInvokerOldCSProAsyncMessageId(0)
{
    EnableAutomation();

    WebViewSyncOperationMarker::DisplayErrorIfOperationInProcess();
}


CSProHostObject::~CSProHostObject()
{
    m_actionInvokerWebController.CancelAndWaitOnActionsInProgress();
}


std::wstring CSProHostObject::GetJavaScriptClassText()
{
    static_assert(Serializer::GetCurrentVersion() / 10000 == 80, "Start adding deprecation warnings when this is used");
    static_assert(static_cast<int>(ActionInvoker::Action::execute) == 11276); // this value is also used in CSProJavaScriptInterface.kt

    return FormatTextCS2WS(LR"!(

    class CSPro {

        static $hostSync = window.chrome.webview.hostObjects.sync.cspro;
        static $lastAsyncResult = null;
        static $callbacks = { };

        static $createMessage(action, args) {
            return JSON.stringify({
                action: %d,
                accessToken: %s,
                arguments: {
                    action: action,
                    arguments: args
                }
            });
        }

        static $processResponse(responseJson) {
            return JSON.parse(responseJson).value;
        }

	    static $sendMessage(action, args) {
            CSPro.$hostSync.setHostProperty("ActionInvoker", CSPro.$createMessage(action, args));
            return CSPro.$processResponse(CSPro.$hostSync.getHostProperty("ActionInvoker"));
        }

	    static $postMessage(action, args, callback) {
            CSPro.$hostSync.setHostProperty("ActionInvokerAsyncCSPro", CSPro.$createMessage(action, args));
            const requestId = CSPro.$hostSync.getHostProperty("ActionInvokerAsyncCSPro");
            if( callback != undefined ) {
                CSPro.$callbacks[requestId] = callback;
            }
        }

        static $processPostedMessageResponse(requestId, responseJson) {
            CSPro.$lastAsyncResult = CSPro.$processResponse(responseJson);
            const callback = CSPro.$callbacks[requestId];
            delete CSPro.$callbacks[requestId];
            eval(callback);
        }

	    static getMaxDisplayWidth() {
            return CSPro.$sendMessage("UI.getMaxDisplayDimensions").width;
	    }

	    static getMaxDisplayHeight() {
            return CSPro.$sendMessage("UI.getMaxDisplayDimensions").height;
	    }
	
	    static getInputData() {
            return JSON.stringify(CSPro.$sendMessage("UI.getInputData"));
	    }
	
	    static setDisplayOptions(t) {
            CSPro.$postMessage("UI.setDisplayOptions", JSON.parse(t));
	    }
	
	    static returnData(t) {
            CSPro.$postMessage("UI.closeDialog", { result: t });
	    }

	    static getAsyncResult() {
            return CSPro.$lastAsyncResult;
        }
	
	    static do(a, t) {
            if( a == "close" ) {
                CSPro.returnData();
            }
	    }
	
	    static runLogic(t) {
            return CSPro.$sendMessage("Logic.eval", { logic: t });
	    }

	    static runLogicAsync(t, c) {
            CSPro.$postMessage("Logic.eval", { logic: t }, c);
        }

	    static invoke(f, a) {
            return CSPro.$sendMessage("Logic.invoke", { function: f, arguments: ( a == undefined ) ? undefined : JSON.parse(a) });
        }

	    static invokeAsync(f, a, c) {
            CSPro.$postMessage("Logic.invoke", { function: f, arguments: ( a == undefined ) ? undefined : JSON.parse(a) }, c);
	    }
    }

    )!", static_cast<int>(ActionInvoker::Action::execute),
         Encoders::ToLogicString(OldCSProJavaScriptInterface::GetAccessToken()).c_str());
}


void CSProHostObject::ActionInvokerSyncSetMessageSync(BSTR message)
{
    m_actionInvokerSyncMessageId = m_actionInvokerWebController.PushMessage(message);
}


BSTR CSProHostObject::ActionInvokerSyncGetProcessedMessage()
{
    const std::shared_ptr<const std::wstring> response = m_actionInvokerWebController.ProcessMessage(m_actionInvokerSyncMessageId, false);

    return ( response != nullptr ) ? SysAllocString(response->c_str()) :
                                     nullptr;
}


void CSProHostObject::ActionInvokerAsyncSetMessage(BSTR message)
{
    int message_id = m_actionInvokerWebController.PushMessage(message);

    ASSERT(m_sourceWindow != nullptr);
    m_sourceWindow->PostMessage(UWM::Html::ActionInvokerProcessAsyncMessage, message_id);
}


BSTR CSProHostObject::ActionInvokerAsyncGetUnused()
{
    // this shouldn't be called but is provided for the DISP_PROPERTY_EX definition
    return ReturnProgrammingError(nullptr);
}


void CSProHostObject::ActionInvokerOldCSProClassSetMessage(BSTR message)
{
    m_actionInvokerOldCSProAsyncMessageId = m_actionInvokerWebController.PushMessage(message, true);
}


BSTR CSProHostObject::ActionInvokerOldCSProClassGetRequestId()
{
    ASSERT(m_sourceWindow != nullptr);
    m_sourceWindow->PostMessage(UWM::Html::ActionInvokerProcessAsyncMessage, m_actionInvokerOldCSProAsyncMessageId);

    return SysAllocString(IntToString(m_actionInvokerOldCSProAsyncMessageId));
}
