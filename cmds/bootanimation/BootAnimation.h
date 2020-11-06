/*
 * Copyright (C) 2007 The Android Open Source Project
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

#ifndef ANDROID_BOOTANIMATION_H
#define ANDROID_BOOTANIMATION_H

#include <vector>
#include <queue>
#include <climits>

#include <stdint.h>
#include <sys/types.h>

#include <androidfw/AssetManager.h>
#include <gui/DisplayEventReceiver.h>
#include <utils/Looper.h>
#include <utils/Thread.h>
#include <binder/IBinder.h>

#include <EGL/egl.h>
#include <GLES/gl.h>

namespace android {

class Surface;
class SurfaceComposerClient;
class SurfaceControl;

// ---------------------------------------------------------------------------

class BootAnimation : public Thread, public IBinder::DeathRecipient
{
public:
    static constexpr int MAX_FADED_FRAMES_COUNT = std::numeric_limits<int>::max();

    struct Texture {
        GLint   w;
        GLint   h;
        GLuint  name;
    };

    struct Font {
        FileMap* map;
        Texture texture;
        int char_width;
        int char_height;
    };

    struct Animation {
        struct Frame {
            String8 name;
            FileMap* map;
            int trimX;
            int trimY;
            int trimWidth;
            int trimHeight;
            mutable GLuint tid;
            bool operator < (const Frame& rhs) const {
                return name < rhs.name;
            }
        };
        struct Part {
            int count;  // The number of times this part should repeat, 0 for infinite
            int pause;  // The number of frames to pause for at the end of this part
            int clockPosX;  // The x position of the clock, in pixels. Positive values offset from
                            // the left of the screen, negative values offset from the right.
            int clockPosY;  // The y position of the clock, in pixels. Positive values offset from
                            // the bottom of the screen, negative values offset from the top.
                            // If either of the above are INT_MIN the clock is disabled, if INT_MAX
                            // the clock is centred on that axis.
            String8 path;
            String8 trimData;
            SortedVector<Frame> frames;
            bool playUntilComplete;
            int framesToFadeCount;
            float backgroundColor[3];
            uint8_t* audioData;
            int audioLength;
            Animation* animation;

            bool hasFadingPhase() const {
                return !playUntilComplete && framesToFadeCount > 0;
            }
        };
        int fps;
        int width;
        int height;
        bool progressEnabled;
        Vector<Part> parts;
        String8 audioConf;
        String8 fileName;
        ZipFileRO* zip;
        Font clockFont;
        Font progressFont;
    };

    // All callbacks will be called from this class's internal thread.
    class Callbacks : public RefBase {
    public:
        // Will be called during initialization after we have loaded
        // the animation and be provided with all parts in animation.
        virtual void init(const Vector<Animation::Part>& /*parts*/) {}

        // Will be called while animation is playing before each part is
        // played. It will be provided with the part and play count for it.
        // It will be provided with the partNumber for the part about to be played,
        // as well as a reference to the part itself. It will also be provided with
        // which play of that part is about to start, some parts are repeated
        // multiple times.
        virtual void playPart(int /*partNumber*/, const Animation::Part& /*part*/,
                              int /*playNumber*/) {}

        // Will be called when animation is done and thread is shutting down.
        virtual void shutdown() {}
    };

    explicit BootAnimation(sp<Callbacks> callbacks, bool shutdown);
    virtual ~BootAnimation();

    sp<SurfaceComposerClient> session() const;

private:
    virtual bool        threadLoop();
    virtual status_t    readyToRun();
    virtual void        onFirstRef();
    virtual void        binderDied(const wp<IBinder>& who);

    bool                updateIsTimeAccurate();

    class TimeCheckThread : public Thread {
    public:
        explicit TimeCheckThread(BootAnimation* bootAnimation);
        virtual ~TimeCheckThread();
    private:
        virtual status_t    readyToRun();
        virtual bool        threadLoop();
        bool                doThreadLoop();
        void                addTimeDirWatch();

        int mInotifyFd;
        int mSystemWd;
        int mTimeWd;
        BootAnimation* mBootAnimation;
    };

    // Display event handling
    class DisplayEventCallback;
    std::unique_ptr<DisplayEventReceiver> mDisplayEventReceiver;
    sp<Looper> mLooper;
    int displayEventCallback(int fd, int events, void* data);
    void processDisplayEvents();

    status_t initTexture(Texture* texture, AssetManager& asset, const char* name);
    status_t initTexture(FileMap* map, int* width, int* height);
    status_t initFont(Font* font, const char* fallback);
    bool android();
    bool movie();
    //add for bootvideo
    bool video();
    void drawText(const char* str, const Font& font, bool bold, int* x, int* y);
    void drawClock(const Font& font, const int xPos, const int yPos);
    void drawProgress(int percent, const Font& font, const int xPos, const int yPos);
    void fadeFrame(int frameLeft, int frameBottom, int frameWidth, int frameHeight,
                   const Animation::Part& part, int fadedFramesCount);
    bool validClock(const Animation::Part& part);
    Animation* loadAnimation(const String8&);
    bool playAnimation(const Animation&);
    void releaseAnimation(Animation*) const;
    bool parseAnimationDesc(Animation&);
    bool preloadZip(Animation &animation);
    void findBootAnimationFile();
    bool findBootAnimationFileInternal(const std::vector<std::string>& files);
    bool preloadAnimation();
    EGLConfig getEglConfig(const EGLDisplay&);
    ui::Size limitSurfaceSize(int width, int height) const;
    void resizeSurface(int newWidth, int newHeight);
    void projectSceneToWindow();

    bool shouldStopPlayingPart(const Animation::Part& part, int fadedFramesCount,
                               int lastDisplayedProgress);
    void checkExit();

    void handleViewport(nsecs_t timestep);

    sp<SurfaceComposerClient>       mSession;
    AssetManager mAssets;
    Texture     mAndroid[2];
    int         mWidth;
    int         mHeight;
    int         mMaxWidth = 0;
    int         mMaxHeight = 0;
    int         mCurrentInset;
    int         mTargetInset;
    bool        mUseNpotTextures = false;
    EGLDisplay  mDisplay;
    EGLDisplay  mContext;
    EGLDisplay  mSurface;
    sp<IBinder> mDisplayToken;
    sp<SurfaceControl> mFlingerSurfaceControl;
    sp<Surface> mFlingerSurface;
    bool        mShutdown;
    bool        mClockEnabled;
    bool        mTimeIsAccurate;
    bool        mTimeFormat12Hour;
    bool        mShuttingDown;
    String8     mZipFileName;
    bool mVideoAnimation;
    char* mVideoFile;
    nsecs_t mStartbootanimaTime;
    int  mBootVideoTime;
    SortedVector<String8> mLoadedFiles;
    sp<TimeCheckThread> mTimeCheckThread = nullptr;
    sp<Callbacks> mCallbacks;
    Animation* mAnimation = nullptr;
};

// ---------------------------------------------------------------------------

}; // namespace android

#endif // ANDROID_BOOTANIMATION_H
