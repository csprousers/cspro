package gov.census.cspro.csentry

import android.app.Application
import android.graphics.Color
import android.text.TextUtils
import androidx.lifecycle.AndroidViewModel
import androidx.lifecycle.LiveData
import androidx.lifecycle.MediatorLiveData
import androidx.lifecycle.MutableLiveData
import gov.census.cspro.data.CaseSummary
import gov.census.cspro.data.GetTrimmedKeyForDisplay
import gov.census.cspro.engine.*
import gov.census.cspro.maps.MapCameraPosition
import gov.census.cspro.maps.MapData
import gov.census.cspro.maps.MapMarker
import java.util.*
import java.util.regex.Matcher
import java.util.regex.Pattern

/**
 * ViewModel for list of cases to display in case listing
 */
class CasesViewModel constructor(application: Application) : AndroidViewModel(application) {
    private val m_cases: MutableLiveData<List<CaseSummary>> = MutableLiveData()
    private val m_showAlphabetically: MutableLiveData<Boolean> = MutableLiveData()
    private val m_showOnlyPartials: MutableLiveData<Boolean> = MutableLiveData()
    private val m_showCaseLabels: MutableLiveData<Boolean?> = MutableLiveData()
    private val m_searchText: MutableLiveData<String> = MutableLiveData()
    private val m_filteredSortedCases: MediatorLiveData<List<CaseSummary>?> = MediatorLiveData()
    private val m_mapLiveData: MutableLiveData<MapData?> = MutableLiveData()
    private val m_mapData: MapData = MapData()
    val mappingOptions: AppMappingOptions

    val cases: LiveData<List<CaseSummary>?>
        get() {
            return m_filteredSortedCases
        }

    val mapLiveData: LiveData<MapData?>
        get() {
            return m_mapLiveData
        }

    var showAlphabetically: Boolean
        get() {
            return (m_showAlphabetically.value == true)
        }
        set(showAlphabetically) {
            m_showAlphabetically.value = showAlphabetically
        }

    init {
        setupLiveData()
        m_showAlphabetically.value = false
        m_showOnlyPartials.value = false
        m_showCaseLabels.value = true
        mappingOptions = EngineInterface.getInstance().mappingOptions
        m_mapData.setBaseMap(EngineInterface.getInstance().baseMapSelection)
        refreshCases()
    }

    fun setShowOnlyPartials(showOnlyPartials: Boolean) {
        m_showOnlyPartials.value = showOnlyPartials
    }

    fun setSearchText(searchText: String) {
        m_searchText.value = searchText
    }

    var showCaseLabels: Boolean
        get() {
            return (m_showCaseLabels.value == true)
        }
        set(showCaseLabels) {
            m_showCaseLabels.value = showCaseLabels
        }

    fun refreshCases() {
        Messenger.getInstance().sendMessage(
            object : EngineMessage() {
                override fun run() {
                    val keys: ArrayList<CaseSummary> = ArrayList()
                    EngineInterface.getInstance().getSequentialCaseIds(keys)
                    m_cases.postValue(keys)
                }
            })
    }

    private fun setupLiveData() {
        m_filteredSortedCases.addSource(m_cases) { updateFilteredCases() }
        m_filteredSortedCases.addSource(m_showAlphabetically) { updateFilteredCases() }
        m_filteredSortedCases.addSource(m_showOnlyPartials) { updateFilteredCases() }
        m_filteredSortedCases.addSource(m_showCaseLabels) { updateFilteredCases() }
        m_filteredSortedCases.addSource(m_searchText) { updateFilteredCases() }
    }

    private fun updateFilteredCases() {
        val cases: List<CaseSummary>? = m_cases.value
        val showAlphabetically: Boolean? = m_showAlphabetically.value
        val showOnlyPartials: Boolean? = m_showOnlyPartials.value
        val showCaseLabels: Boolean? = m_showCaseLabels.value
        val searchText: String? = m_searchText.value

        // Only do the update once all input values have been set
        if ((cases != null) && (showAlphabetically != null) && (showOnlyPartials != null) && (showCaseLabels != null)) {
            val sortedAndFiltered: List<CaseSummary> = sortAndFilter(cases, showAlphabetically, showOnlyPartials, showCaseLabels, searchText)
            m_filteredSortedCases.value = sortedAndFiltered
            if (mappingOptions.isMappingEnabled) {
                updateMap(sortedAndFiltered)
                m_mapLiveData.value = m_mapData
            }
        }
    }

    private fun updateMap(cases: List<CaseSummary?>) {
        val markers: ArrayList<MapMarker> = ArrayList()
        val showCaseLabels: Boolean = if (m_showCaseLabels.value == null) true else (m_showCaseLabels.value?:false)

            for ((caseIndex, ck: CaseSummary?) in cases.withIndex()) {
            val marker: MapMarker? = ck?.latitude?.let { MapMarker(caseIndex, it, ck.longitude) }
            var description: String? = ck?.GetTrimmedKeyForDisplay(showCaseLabels)
            if (!TextUtils.isEmpty(ck?.caseNote)) description += "<br/><font color='#0060FF'>" + ck?.caseNote + "</font>"
            marker?.setDescription(description)
            marker?.setBackgroundColor(if (ck.isPartial) Color.RED else Color.BLUE)
            if (marker != null) {
                markers.add(marker)
            }
            }
        m_mapData.setMarkers(markers)
    }

    fun setMapCamera(camera: MapCameraPosition?) {
        m_mapData.setCameraPosition(camera)
    }

    companion object {
        private fun sortAndFilter(cases: List<CaseSummary>,
                                  showAlphabetically: Boolean,
                                  showOnlyPartials: Boolean,
                                  showCaseLabels: Boolean,
                                  searchText: String?): List<CaseSummary> {
            val filteredCaseSummaries: ArrayList<CaseSummary> = ArrayList()
            val searchPattern: Pattern? = if (searchText == null || TextUtils.isEmpty(searchText)) null else Pattern.compile(searchText, Pattern.CASE_INSENSITIVE or Pattern.LITERAL)
            for (caseSummary: CaseSummary in cases) {
                if (showOnlyPartials && !caseSummary.isPartial) continue
                if (searchPattern != null && !matchesSearchPattern(caseSummary, searchPattern, showCaseLabels)) continue
                filteredCaseSummaries.add(caseSummary)
            }
            if (showAlphabetically) {
                Collections.sort(filteredCaseSummaries) { objA: CaseSummary, objB: CaseSummary -> if (showCaseLabels) return@sort objA.caseLabel.compareTo(objB.caseLabel, ignoreCase = true) else return@sort objA.key.compareTo(objB.key) }
            } else {
                // Default ordering on Android is reverse of data file order so that most
                // recent cases are shown first
                filteredCaseSummaries.reverse()
            }
            return filteredCaseSummaries
        }

        private fun matchesSearchPattern(caseSummary: CaseSummary,
                                         searchPattern: Pattern,
                                         showCaseLabels: Boolean): Boolean {
            val displayKey: String = caseSummary.GetTrimmedKeyForDisplay(showCaseLabels)
            val matcher: Matcher = searchPattern.matcher(displayKey)
            return matcher.find()
        }
    }

}