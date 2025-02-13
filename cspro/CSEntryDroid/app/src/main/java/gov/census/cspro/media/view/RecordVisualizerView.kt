package gov.census.cspro.media.view

import android.content.Context
import android.content.res.Resources
import android.graphics.Canvas
import android.graphics.Color
import android.graphics.Paint
import android.util.AttributeSet
import android.view.View
import java.util.*


class RecordVisualizerView : View {

    private val DENSITY = Resources.getSystem().displayMetrics.density
    private val MAX_REPORTABLE_AMP = 22760f //effective size,  max fft = 32760
    private val UNINITIALIZED = 0f

    private val chunkPaint = Paint()

    private var lastFFT = 0.toFloat()
    private var usageWidth = 0.toDouble()
    private var chunkHeights = ArrayList<Float>()
    private var chunkWidths = ArrayList<Double>()
    private var topBottomPadding = 10 * DENSITY

    private var chunkColor = Color.RED
    private var chunkWidth = 3 * DENSITY
    private var chunkSpace = 2 * DENSITY
    var chunkMaxHeight = UNINITIALIZED
    private var chunkMinHeight = 3 * DENSITY  // don't recommendation size <= 10 dp

    constructor(context: Context) : super(context) {
        init()
    }

    constructor(context: Context, attrs: AttributeSet) : super(context, attrs) {
        init(attrs)
    }

    constructor(context: Context, attrs: AttributeSet, defStyleAttr: Int) : super(
        context,
        attrs,
        defStyleAttr
    ) {
        init(attrs)
    }

    private fun init() {
        chunkPaint.strokeWidth = chunkWidth
        chunkPaint.color = chunkColor
    }

    private fun init(attrs: AttributeSet) {
        chunkPaint.color = chunkColor

        setWillNotDraw(false)
        chunkPaint.isAntiAlias = true
    }

    fun recreate() {
        lastFFT = 0f
        usageWidth = 0.0
        chunkWidths = ArrayList()
        chunkHeights = ArrayList()
        invalidate()
    }

    fun update(fft: Int) {
        this.lastFFT = fft.toFloat()
        invalidate()
    }

    override fun onDraw(canvas: Canvas) {
        super.onDraw(canvas)

        val chunkHorizontalScale = (chunkWidth + chunkSpace).toDouble()
        val maxLineCount = width / chunkHorizontalScale
        val centerView = (height / 2).toFloat()

        if (chunkWidths.size >= maxLineCount && chunkHeights.size > 0) {
            chunkHeights.removeAt(0)
        } else {
            usageWidth += chunkHorizontalScale
            chunkWidths.add(chunkWidths.size, usageWidth)
        }

        if (chunkMaxHeight == UNINITIALIZED) {
            chunkMaxHeight = height - topBottomPadding * 2
        } else if (chunkMaxHeight > height - topBottomPadding * 2) {
            chunkMaxHeight = height - topBottomPadding * 2
        }

        val verticalDrawScale = chunkMaxHeight - chunkMinHeight
        if (verticalDrawScale == 0f) {
            return
        }

        val point = MAX_REPORTABLE_AMP / verticalDrawScale
        if (point == 0f) {
            return
        }

        if (lastFFT == 0f) {
            return
        }

        var fftPoint = lastFFT / point

        fftPoint += chunkMinHeight

        if (fftPoint > chunkMaxHeight) {
            fftPoint = chunkMaxHeight
        } else if (fftPoint < chunkMinHeight) {
            fftPoint = chunkMinHeight
        }

        chunkHeights.add(chunkHeights.size, fftPoint)

        for (i in 0 until chunkHeights.size - 1) {
            val startX = chunkWidths[i].toFloat()
            val stopX = chunkWidths[i].toFloat()
            val startY = centerView - chunkHeights[i] / 2
            val stopY = centerView + chunkHeights[i] / 2

            canvas.drawLine(startX, startY, stopX, stopY, chunkPaint)
        }
    }
}