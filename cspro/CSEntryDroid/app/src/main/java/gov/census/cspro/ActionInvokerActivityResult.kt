package gov.census.cspro

class ActionInvokerActivityResult {
    var result: String
    var refreshToken: String?

    constructor(result: String, refreshToken: String?) {
        this.result = result
        this.refreshToken = refreshToken
    }
}