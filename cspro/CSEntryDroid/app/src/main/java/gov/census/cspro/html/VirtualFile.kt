package gov.census.cspro.html

class VirtualFile {
    var content: ByteArray
    var contentType: String?

    constructor(content: ByteArray, contentType: String?) {
        this.content = content
        this.contentType = contentType
    }
}