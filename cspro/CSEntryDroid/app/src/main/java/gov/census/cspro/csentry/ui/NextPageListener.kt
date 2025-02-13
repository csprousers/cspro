package gov.census.cspro.csentry.ui

/**
 * Listener for next page event initiated by a QuestionWidget
 * e.g. hitting next on keyboard of last field on page or
 * choosing radio button with auto-advance on.
 */
interface NextPageListener {
    fun OnNextPage()
}