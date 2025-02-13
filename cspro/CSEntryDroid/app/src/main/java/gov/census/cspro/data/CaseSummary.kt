package gov.census.cspro.data

data class CaseSummary(
    val key: String,
    val caseLabel: String,
    val caseNote: String,
    val positionInRepository: Double,
    val isPartial: Boolean,
    val latitude: Double,
    val longitude: Double)
{


}
fun CaseSummary.GetTrimmedKeyForDisplay(useCaseLabel: Boolean): String {
    val key = if (useCaseLabel) caseLabel else key
    return key.trim()
}
