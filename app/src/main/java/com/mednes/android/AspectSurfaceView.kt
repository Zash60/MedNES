package com.mednes.android

import android.content.Context
import android.util.AttributeSet
import android.view.SurfaceView
import android.view.View.MeasureSpec
import kotlin.math.min

class AspectSurfaceView @JvmOverloads constructor(
    context: Context,
    attrs: AttributeSet? = null,
    defStyleAttr: Int = 0
) : SurfaceView(context, attrs, defStyleAttr) {

    private val nesAspectRatio = 240f / 256f  // Height / width

    override fun onMeasure(widthMeasureSpec: Int, heightMeasureSpec: Int) {
        var width = MeasureSpec.getSize(widthMeasureSpec)
        var height = MeasureSpec.getSize(heightMeasureSpec)

        // Fit to width first
        var newHeight = (width * nesAspectRatio).toInt()
        if (newHeight > height) {
            // Too tall; fit to height instead
            val newWidth = (height / nesAspectRatio).toInt()
            setMeasuredDimension(newWidth, height)
        } else {
            setMeasuredDimension(width, newHeight)
        }
    }
}
