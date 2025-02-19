
class CSProRT {

    static $Impl = {

        postMessage: function(accessToken, action, data, requestId) {
            try {
                window.chrome.webview.postMessage({
                    accessToken: accessToken,
                    action: action,
                    data: data,
                    requestId: requestId
                });
            }
            catch(e) {
                console.log(e);
            }
        },

        nextRequestId: 1,
        callbacks: { },

        postMessageForResult: function(accessToken, action, data) {
            const requestId = this.nextRequestId++;

            return new Promise((resolve, reject) => {
                this.callbacks[requestId] = {
                    resolve: resolve,
                    reject: reject
                };

                CSProRT.$Impl.postMessage(accessToken, action, data, requestId);
            });
        },

        processMessageResult: function(requestId, resultType, result) {
            const requestCallback = this.callbacks[requestId];
            delete this.callbacks[requestId];

            try {
                if( resultType == "value" ) {
                    requestCallback.resolve(JSON.parse(result));
                }
                else if( resultType == "undefined" ) {
                    requestCallback.resolve(undefined);
                }
                else {
                    requestCallback.reject(new Error(result));
                }
            }
            catch(e) {
                console.log(e);
            }
        }
    }

    static getWindow() {
        return window.chrome.webview;
    }

    constructor(accessToken) {
        this.accessToken = accessToken;
    }

    // posts the message
    postMessage(action, data) {
        CSProRT.$Impl.postMessage(this.accessToken, action, data);
    }

    // posts the message and returns a Promise that can be used for asynchronous processing
    postMessageForResult(action, data) {
        return CSProRT.$Impl.postMessageForResult(this.accessToken, action, data);
    }
}
