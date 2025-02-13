package gov.census.cspro.form

data class FieldNote(
    val index: Long,
    val note: String,
    val operatorId: String,
    val isFieldNote: Boolean,
    val groupSymbolIndex: Int,
    val groupLabel: String,
    val label: String)