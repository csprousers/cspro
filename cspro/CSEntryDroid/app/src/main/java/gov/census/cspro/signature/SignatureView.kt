package gov.census.cspro.signature


import android.annotation.SuppressLint
import android.content.Context
import android.content.ContextWrapper
import android.graphics.*
import android.graphics.Bitmap.Config.ARGB_8888
import android.graphics.Matrix.ScaleToFit.CENTER
import android.os.Bundle
import android.os.Parcelable
import android.util.AttributeSet
import android.view.MotionEvent
import android.view.View
import android.view.ViewTreeObserver.OnGlobalLayoutListener
import androidx.appcompat.app.AppCompatActivity
import androidx.core.graphics.toRect
import androidx.core.graphics.toRectF
import androidx.lifecycle.ViewModelProvider
import gov.census.cspro.csentry.R.styleable
import gov.census.cspro.signature.util.Bezier
import gov.census.cspro.signature.util.ControlTimedPoints
import gov.census.cspro.signature.util.TimedPoint
import java.util.*
import kotlin.math.ceil
import kotlin.math.max
import kotlin.math.roundToInt
import kotlin.math.sqrt

class SignatureView(context: Context, attrs: AttributeSet?) : View(context, attrs) {

    //View state
    private var mPoints: MutableList<TimedPoint>? = null
    private var mIsEmpty = false
    private var mHasEditState: Boolean? = null
    private var mLastTouchX = 0f
    private var mLastTouchY = 0f
    private var mLastVelocity = 0f
    private var mLastWidth = 0f
    private val mBounds: Rect = Rect()
    private var mBitmapSavedState: Bitmap? = null


    // Cache
    private val mPointsCache: MutableList<TimedPoint> = ArrayList()
    private val mControlTimedPointsCached: ControlTimedPoints = ControlTimedPoints()
    private val mBezierCached: Bezier = Bezier()

    //Configurable parameters
    private var mMinWidth = 0
    private var mMaxWidth = 0
    private var mVelocityFilterWeight = 0f
    private var mOnSignedListener: OnSignedListener? = null
    private var mClearOnDoubleClick = false

    //Click values
    private var mFirstClick: Long = 0
    private var mCountClick = 0

    private val mPaint = Paint()
    private var mSignatureBitmap: Bitmap? = null
    private var mSignatureBitmapCanvas: Canvas? = null

     private fun View.getParentActivity(): AppCompatActivity? {
         var context = this.context
         while (context is ContextWrapper) {
             if (context is AppCompatActivity) {
                 return context
             }
             context = context.baseContext
         }
         return null
     }

     override fun onSaveInstanceState(): Parcelable {
         val bundle = Bundle()
         bundle.putParcelable("superState", super.onSaveInstanceState())
         if (mHasEditState == null || mHasEditState!!)
         {
             mBitmapSavedState = getTransparentSignatureBitmap()
         }

         //using view model
         getParentActivity()?.let {
             val viewModel = ViewModelProvider(it).get(SignatureViewModel::class.java)
             viewModel.bitmap = mBitmapSavedState
             viewModel.bounds = mBounds
         }
         return bundle
     }

     override fun onRestoreInstanceState(state: Parcelable)
     {
         //using view model
         getParentActivity()?.let { activity ->
             val viewModel = ViewModelProvider(activity).get(SignatureViewModel::class.java)
             mBitmapSavedState = viewModel.bitmap
             mBitmapSavedState?.let {
                 setSignatureBitmap(it, viewModel.bounds)
             }
         }
         var superState: Parcelable? = state
         if (superState is Bundle)
         {
             val bundle = superState
             superState = bundle.getParcelable("superState")
         }
         mHasEditState = false
         super.onRestoreInstanceState(superState)
     }

    private fun clearView()
    {
        mPoints = ArrayList()
        mLastVelocity = 0f
        mLastWidth = (mMinWidth + mMaxWidth) / 2.toFloat()

        if (mSignatureBitmap != null) {
            mSignatureBitmap = null
            ensureSignatureBitmap()
        }

        setIsEmpty(true)
        invalidate()

    }

    fun clear()
    {
        clearView()
        mBounds.setEmpty()
        mHasEditState = true
    }

    @SuppressLint("ClickableViewAccessibility")
    override fun onTouchEvent(event: MotionEvent): Boolean
    {
        if (!isEnabled) return false
        val eventX = event.x
        val eventY = event.y
        when (event.action)
        {
            MotionEvent.ACTION_DOWN ->
            {
                parent.requestDisallowInterceptTouchEvent(true)
                mPoints!!.clear()
                if (isDoubleClick()) {
                    return false
                }
                mLastTouchX = eventX
                mLastTouchY = eventY
                addPoint(getNewPoint(eventX, eventY))
                setIsEmpty(false)
            }

            MotionEvent.ACTION_MOVE ->
            {
                addPoint(getNewPoint(eventX, eventY))
                setIsEmpty(false)
            }

            MotionEvent.ACTION_UP ->
            {
                addPoint(getNewPoint(eventX, eventY))
                parent.requestDisallowInterceptTouchEvent(true)
            }
            else -> return false
        }

        invalidate()
        return true
    }

    override fun onDraw(canvas: Canvas)
    {
        if (mSignatureBitmap != null) {
            canvas.drawBitmap(mSignatureBitmap!!, 0f, 0f, mPaint)
        }
    }

    fun setOnSignedListener(listener: OnSignedListener?)
    {
        mOnSignedListener = listener
    }

    private fun setIsEmpty(newValue: Boolean)
    {
        mIsEmpty = newValue
        if (mOnSignedListener != null)
        {
            if (mIsEmpty)
            {
                mOnSignedListener!!.onClear()
            } else {
                mOnSignedListener!!.onSigned()
            }
        }
    }

    fun getSignatureBitmap(): Bitmap
    {
        val originalBitmap = getTransparentSignatureBitmap()
        val paddedBounds = mBounds
        paddedBounds.inset(-SIGNATURE_IMAGE_PADDING, -SIGNATURE_IMAGE_PADDING)
        val whiteBgBitmap = Bitmap.createBitmap(paddedBounds.width(), paddedBounds.height(), ARGB_8888)
        val canvas = Canvas(whiteBgBitmap)
        canvas.drawColor(Color.WHITE)
        canvas.drawBitmap(originalBitmap!!, paddedBounds, Rect(0, 0, paddedBounds.width(), paddedBounds.height()), null )
        return whiteBgBitmap
    }

    fun setSignatureBitmap(signature: Bitmap, bounds: Rect)
    {
        if (isLaidOut)
        {
            clearView()
            ensureSignatureBitmap()

            val tempSrc = RectF()
            val tempDst = RectF()
            val dWidth = signature.width
            val dHeight = signature.height
            val vWidth = width
            val vHeight = height

            // Generate the required transform.
            tempSrc[0f, 0f, dWidth.toFloat()] = dHeight.toFloat()
            tempDst[0f, 0f, vWidth.toFloat()] = vHeight.toFloat()
            val oldCanvasToNewCanvasTransform = Matrix()
            oldCanvasToNewCanvasTransform.setRectToRect(tempSrc, tempDst, CENTER)
            val canvas = Canvas(mSignatureBitmap!!)
            canvas.drawBitmap(signature, oldCanvasToNewCanvasTransform, null)
            setIsEmpty(false)
            invalidate()
            val newBounds = bounds.toRectF()
            oldCanvasToNewCanvasTransform.mapRect(newBounds)
            mBounds.set(newBounds.toRect())
        } else {
            viewTreeObserver.addOnGlobalLayoutListener(object : OnGlobalLayoutListener {
                override fun onGlobalLayout() {
                    // Remove layout listener...
                    viewTreeObserver.removeOnGlobalLayoutListener(this)
                    // Signature bitmap...
                    setSignatureBitmap(signature, bounds)
                }
            })
        }
    }

    private fun getTransparentSignatureBitmap(): Bitmap?
    {
        ensureSignatureBitmap()
        return mSignatureBitmap
    }

    private fun isDoubleClick(): Boolean
    {
        if (mClearOnDoubleClick)
        {
            if (mFirstClick != 0L && System.currentTimeMillis() - mFirstClick > DOUBLE_CLICK_DELAY_MS)
            {
                mCountClick = 0
            }

            mCountClick++

            if (mCountClick == 1)
            {
                mFirstClick = System.currentTimeMillis()
            } else if (mCountClick == 2)
            {
                val lastClick = System.currentTimeMillis()
                if (lastClick - mFirstClick < DOUBLE_CLICK_DELAY_MS)
                {
                    clearView()
                    return true
                }
            }
        }

        return false
    }

    private fun getNewPoint(x: Float, y: Float): TimedPoint
    {
        val mCacheSize = mPointsCache.size

        val timedPoint: TimedPoint = if (mCacheSize == 0)
        { // Cache is empty, create a new point
            TimedPoint()
        } else { // Get point from cache
            mPointsCache.removeAt(mCacheSize - 1)
        }

        return timedPoint.set(x, y)
    }

    private fun recyclePoint(point: TimedPoint)
    {
        mPointsCache.add(point)
    }

    private fun addPoint(newPoint: TimedPoint)
    {
        mPoints!!.add(newPoint)

        val pointsCount = mPoints!!.size

        if (pointsCount > 3)
        {
            var tmp: ControlTimedPoints =
                calculateCurveControlPoints(mPoints!![0], mPoints!![1], mPoints!![2])
            val c2: TimedPoint = tmp.c2

            recyclePoint(tmp.c1)

            tmp = calculateCurveControlPoints(mPoints!![1], mPoints!![2], mPoints!![3])
            val c3: TimedPoint = tmp.c1
            recyclePoint(tmp.c2)
            val curve: Bezier = mBezierCached.set(mPoints!![1], c2, c3, mPoints!![2])
            val startPoint: TimedPoint = curve.startPoint
            val endPoint: TimedPoint = curve.endPoint
            var velocity: Float = endPoint.velocityFrom(startPoint)
            velocity = if (java.lang.Float.isNaN(velocity)) 0.0f else velocity
            velocity = (mVelocityFilterWeight * velocity
                + (1 - mVelocityFilterWeight) * mLastVelocity)

            // The new width is a function of the velocity. Higher velocities
            // correspond to thinner strokes.
            val newWidth = strokeWidth(velocity)
            addBezier(curve, mLastWidth, newWidth)
            mLastVelocity = velocity
            mLastWidth = newWidth

            // Remove the first element from the list,
            // so that we always have no more than 4 mPoints in mPoints array.
            recyclePoint(mPoints!!.removeAt(0))
            recyclePoint(c2)
            recyclePoint(c3)
        } else if (pointsCount == 1) {
            // To reduce the initial lag make it work with 3 mPoints
            // by duplicating the first point
            val firstPoint: TimedPoint = mPoints!![0]
            mPoints!!.add(getNewPoint(firstPoint.x, firstPoint.y))
        }

        mHasEditState = true

    }

    private fun addBezier(curve: Bezier, startWidth: Float, endWidth: Float)
    {
        ensureSignatureBitmap()

        val originalWidth = mPaint.strokeWidth
        val widthDelta = endWidth - startWidth
        val drawSteps = ceil(curve.length().toDouble()).toFloat()
        var i = 0

        while (i < drawSteps)
        {
            // Calculate the Bezier (x, y) coordinate for this step.
            val t = i.toFloat() / drawSteps
            val tt = t * t
            val ttt = tt * t
            val u = 1 - t
            val uu = u * u
            val uuu = uu * u
            var x: Float = uuu * curve.startPoint.x
            x += 3 * uu * t * curve.control1.x
            x += 3 * u * tt * curve.control2.x
            x += ttt * curve.endPoint.x

            var y: Float = uuu * curve.startPoint.y
            y += 3 * uu * t * curve.control1.y
            y += 3 * u * tt * curve.control2.y
            y += ttt * curve.endPoint.y
            // Set the incremental stroke width and draw.
            mPaint.strokeWidth = startWidth + ttt * widthDelta
            mSignatureBitmapCanvas!!.drawPoint(x, y, mPaint)

            val pointBounds = RectF(x,y, x, y)
            pointBounds.inset(-mPaint.strokeWidth, -mPaint.strokeWidth)
            mBounds.union(pointBounds.toRect())

            i++
        }

        mPaint.strokeWidth = originalWidth
    }

    private fun calculateCurveControlPoints(s1: TimedPoint, s2: TimedPoint, s3: TimedPoint): ControlTimedPoints
    {
        val dx1: Float = s1.x - s2.x
        val dy1: Float = s1.y - s2.y
        val dx2: Float = s2.x - s3.x
        val dy2: Float = s2.y - s3.y
        val m1X: Float = (s1.x + s2.x) / 2.0f
        val m1Y: Float = (s1.y + s2.y) / 2.0f
        val m2X: Float = (s2.x + s3.x) / 2.0f
        val m2Y: Float = (s2.y + s3.y) / 2.0f
        val l1 = sqrt(dx1 * dx1 + dy1 * dy1.toDouble()).toFloat()
        val l2 = sqrt(dx2 * dx2 + dy2 * dy2.toDouble()).toFloat()
        val dxm = m1X - m2X
        val dym = m1Y - m2Y

        var k = l2 / (l1 + l2)
        if (java.lang.Float.isNaN(k)) k = 0.0f
        val cmX = m2X + dxm * k
        val cmY = m2Y + dym * k
        val tx: Float = s2.x - cmX
        val ty: Float = s2.y - cmY

        return mControlTimedPointsCached.set(
            getNewPoint(m1X + tx, m1Y + ty),
            getNewPoint(m2X + tx, m2Y + ty)
        )
    }

    private fun strokeWidth(velocity: Float): Float
    {
        return max(mMaxWidth / (velocity + 1), mMinWidth.toFloat())
    }

    private fun ensureSignatureBitmap()
    {
        if (mSignatureBitmap == null && width > 0 && height > 0)
        {
            mSignatureBitmap = Bitmap.createBitmap(width, height, ARGB_8888)
            mSignatureBitmapCanvas = Canvas(mSignatureBitmap!!)
        }
    }

    private fun convertDpToPx(dp: Float): Int
    {
        return (context.resources.displayMetrics.density * dp).roundToInt()
    }

    interface OnSignedListener
    {
        fun onSigned()
        fun onClear()
    }

    companion object {
        private const val DEFAULT_ATTR_PEN_MIN_WIDTH_PX = 2
        private const val DEFAULT_ATTR_PEN_MAX_WIDTH_PX = 3
        private const val DEFAULT_ATTR_PEN_COLOR = Color.BLACK
        private const val DEFAULT_ATTR_VELOCITY_FILTER_WEIGHT = 0.9f
        private const val DEFAULT_ATTR_CLEAR_ON_DOUBLE_CLICK = false
        private const val DOUBLE_CLICK_DELAY_MS = 200
        private const val SIGNATURE_IMAGE_PADDING = 50
    }

    init
    {
        val a = context.theme.obtainStyledAttributes(attrs, styleable.SignatureView, 0, 0)

        //Configurable parameters
        try
        {
            mMinWidth = a.getDimensionPixelSize(
                styleable.SignatureView_penMinWidth, convertDpToPx(DEFAULT_ATTR_PEN_MIN_WIDTH_PX.toFloat()))

            mMaxWidth = a.getDimensionPixelSize(
                styleable.SignatureView_penMaxWidth, convertDpToPx(DEFAULT_ATTR_PEN_MAX_WIDTH_PX.toFloat()))

            mPaint.color = a.getColor(styleable.SignatureView_penColor, DEFAULT_ATTR_PEN_COLOR)

            mVelocityFilterWeight = a.getFloat(
                styleable.SignatureView_velocityFilterWeight, DEFAULT_ATTR_VELOCITY_FILTER_WEIGHT)

            mClearOnDoubleClick = a.getBoolean(styleable.SignatureView_clearOnDoubleClick,
                DEFAULT_ATTR_CLEAR_ON_DOUBLE_CLICK)

        } finally
        {
            a.recycle()
        }

        //Fixed parameters
        mPaint.isAntiAlias = true
        mPaint.style = Paint.Style.STROKE
        mPaint.strokeCap = Paint.Cap.ROUND
        mPaint.strokeJoin = Paint.Join.ROUND

        clearView()

    }

}