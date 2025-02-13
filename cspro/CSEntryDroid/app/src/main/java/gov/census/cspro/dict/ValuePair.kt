package gov.census.cspro.dict

data class ValuePair(val code: String,
                     val label: String,
					 val textColor: Int,
                     val imagePath: String?,
                     val isSelectable: Boolean)