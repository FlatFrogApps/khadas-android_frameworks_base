/*
 * Copyright (C) 2020 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.android.test.input

import android.graphics.FrameInfo
import android.os.SystemClock
import android.view.ViewFrameInfo
import com.google.common.truth.Truth.assertThat
import org.junit.Before
import org.junit.Test

class ViewFrameInfoTest {
    companion object {
        private const val TAG = "ViewFrameInfoTest"
    }
    private val mViewFrameInfo = ViewFrameInfo()
    private var mTimeStarted: Long = 0

    @Before
    fun setUp() {
        mViewFrameInfo.reset()
        mViewFrameInfo.updateOldestInputEvent(10)
        mViewFrameInfo.updateNewestInputEvent(20)
        mViewFrameInfo.flags = mViewFrameInfo.flags or FrameInfo.FLAG_WINDOW_LAYOUT_CHANGED
        mTimeStarted = SystemClock.uptimeNanos()
        mViewFrameInfo.markDrawStart()
    }

    @Test
    fun testPopulateFields() {
        assertThat(mViewFrameInfo.drawStart).isGreaterThan(mTimeStarted)
        assertThat(mViewFrameInfo.oldestInputEventTime).isEqualTo(10)
        assertThat(mViewFrameInfo.newestInputEventTime).isEqualTo(20)
        assertThat(mViewFrameInfo.flags).isEqualTo(FrameInfo.FLAG_WINDOW_LAYOUT_CHANGED)
    }

    @Test
    fun testReset() {
        mViewFrameInfo.reset()
        // Ensure that the original object is reset correctly
        assertThat(mViewFrameInfo.drawStart).isEqualTo(0)
        assertThat(mViewFrameInfo.oldestInputEventTime).isEqualTo(0)
        assertThat(mViewFrameInfo.newestInputEventTime).isEqualTo(0)
        assertThat(mViewFrameInfo.flags).isEqualTo(0)
    }

    @Test
    fun testUpdateFrameInfoFromViewFrameInfo() {
        val frameInfo = FrameInfo()
        // By default, all values should be zero
        assertThat(frameInfo.frameInfo[FrameInfo.OLDEST_INPUT_EVENT]).isEqualTo(0)
        assertThat(frameInfo.frameInfo[FrameInfo.NEWEST_INPUT_EVENT]).isEqualTo(0)
        assertThat(frameInfo.frameInfo[FrameInfo.FLAGS]).isEqualTo(0)
        assertThat(frameInfo.frameInfo[FrameInfo.DRAW_START]).isEqualTo(0)

        // The values inside FrameInfo should match those from ViewFrameInfo after we update them
        mViewFrameInfo.populateFrameInfo(frameInfo)
        assertThat(frameInfo.frameInfo[FrameInfo.OLDEST_INPUT_EVENT]).isEqualTo(10)
        assertThat(frameInfo.frameInfo[FrameInfo.NEWEST_INPUT_EVENT]).isEqualTo(20)
        assertThat(frameInfo.frameInfo[FrameInfo.FLAGS]).isEqualTo(
                FrameInfo.FLAG_WINDOW_LAYOUT_CHANGED)
        assertThat(frameInfo.frameInfo[FrameInfo.DRAW_START]).isGreaterThan(mTimeStarted)
    }
}