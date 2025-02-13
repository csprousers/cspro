package gov.census.cspro.util

fun Int.formatMillisecondsAsHoursMinutesSeconds(): String
{
    val hours = (this / (1000 * 60 * 60))
    val minutes = (this % (1000 * 60 * 60)) / (1000 * 60)
    val seconds = ((this % (1000 * 60 * 60)) % (1000 * 60) / 1000)
    return if (hours <= 0) {
        String.format(
            "%d:%02d",
            minutes, seconds
        )
    } else {
        String.format(
            "%2d:%2d:%02d",
            hours, minutes, seconds
        )
    }
}