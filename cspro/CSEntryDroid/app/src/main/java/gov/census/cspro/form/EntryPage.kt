package gov.census.cspro.form

/**
 * Data entry screen containing one or more fields.
 */
class EntryPage constructor(private val m_nativeReference: Long) {
    fun GetOccurrenceLabel(): String {
        return GetOccurrenceLabel(m_nativeReference)
    }

    fun GetBlockName(): String {
        return GetBlockName(m_nativeReference)
    }

    fun GetBlockLabel(): String {
        return GetBlockLabel(m_nativeReference)
    }

    fun GetBlockQuestionTextUrl(): String? {
        return GetBlockQuestionTextUrl(m_nativeReference)
    }

    fun GetBlockHelpTextUrl(): String? {
        return GetBlockHelpTextUrl(m_nativeReference)
    }

    fun GetPageFields(): Array<CDEField> {
        return GetPageFields(m_nativeReference)
    }

    private external fun GetOccurrenceLabel(nativeReference: Long): String
    private external fun GetBlockName(nativeReference: Long): String
    private external fun GetBlockLabel(nativeReference: Long): String
    private external fun GetBlockQuestionTextUrl(nativeReference: Long): String?
    private external fun GetBlockHelpTextUrl(nativeReference: Long): String?
    private external fun GetPageFields(nativeReference: Long): Array<CDEField>
}