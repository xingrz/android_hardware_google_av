/*
 * Copyright (C) 2019 The Android Open Source Project
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

//#define LOG_NDEBUG 0
#define LOG_TAG "ECOSession"
//#define DEBUG_ECO_SESSION
#include "eco/ECOSession.h"

#include <binder/BinderService.h>
#include <cutils/atomic.h>
#include <inttypes.h>
#include <pthread.h>
#include <stdio.h>
#include <sys/types.h>
#include <utils/Log.h>

#include <algorithm>
#include <climits>
#include <cstring>
#include <ctime>
#include <string>

#include "eco/ECODataKey.h"
#include "eco/ECODebug.h"

namespace android {
namespace media {
namespace eco {

using android::sp;

#define RETURN_IF_ERROR(expr)         \
    {                                 \
        status_t _errorCode = (expr); \
        if (_errorCode != true) {     \
            return _errorCode;        \
        }                             \
    }

// static
sp<ECOSession> ECOSession::createECOSession(int32_t width, int32_t height, bool isCameraRecording) {
    // Only support up to 720P and camera recording use case.
    // TODO(hkuang): Support the same resolution as in EAF. Also relax the isCameraRecording
    // as encoder may not konw it is from camera for some usage cases.
    if (width > 1280 || height > 720 || width == 0 || height == 0 || isCameraRecording == false) {
        ALOGE("Failed to create ECOSession with w: %d, h: %d, isCameraRecording: %d", width, height,
              isCameraRecording);
        return nullptr;
    }
    return new ECOSession(width, height, isCameraRecording);
}

ECOSession::ECOSession(int32_t width, int32_t height, bool isCameraRecording)
      : BnECOSession(),
        mStopThread(false),
        mLastReportedQp(0),
        mListener(nullptr),
        mProvider(nullptr),
        mWidth(width),
        mHeight(height),
        mIsCameraRecording(isCameraRecording) {
    ALOGI("ECOSession created with w: %d, h: %d, isCameraRecording: %d", mWidth, mHeight,
          mIsCameraRecording);
    mThread = std::thread(startThread, this);
}

ECOSession::~ECOSession() {
    mStopThread = true;

    mStatsQueueWaitCV.notify_all();
    if (mThread.joinable()) {
        ALOGD("ECOSession: join the thread");
        mThread.join();
    }
    ALOGI("ECOSession destroyed with w: %d, h: %d, isCameraRecording: %d", mWidth, mHeight,
          mIsCameraRecording);
}

// static
void ECOSession::startThread(ECOSession* session) {
    session->run();
}

void ECOSession::run() {
    ALOGD("ECOSession: starting main thread");

    while (!mStopThread) {
        std::unique_lock<std::mutex> runLock(mStatsQueueLock);

        mStatsQueueWaitCV.wait(runLock,
                               [this] { return mStopThread == true || !mStatsQueue.empty(); });
        if (!mStatsQueue.empty()) {
            ECOData stats = mStatsQueue.front();
            mStatsQueue.pop_front();
            processStats(stats);  // TODO: Handle the error from processStats
        }
    }

    ALOGD("ECOSession: exiting main thread");
}

bool ECOSession::processStats(const ECOData& stats) {
    if (stats.getDataType() != ECOData::DATA_TYPE_STATS) {
        ALOGE("Invalid stats. ECOData with type: %s", stats.getDataTypeString().c_str());
        return false;
    }

    // Get the type of the stats.
    std::string statsType;
    if (stats.findString(KEY_STATS_TYPE, &statsType) != ECODataStatus::OK) {
        ALOGE("Invalid stats ECOData without statsType");
        return false;
    }

    if (statsType.compare(VALUE_STATS_TYPE_SESSION) == 0) {
        RETURN_IF_ERROR(processSessionStats(stats));
    } else if (statsType.compare(VALUE_STATS_TYPE_FRAME) == 0) {
        RETURN_IF_ERROR(processFrameStats(stats));
    } else {
        ALOGE("processStats:: Failed to process stats as ECOData contains unknown stats type");
        return false;
    }

    return true;
}

bool ECOSession::processSessionStats(const ECOData& stats) {
    ALOGV("processSessionStats");

    ECOData info(ECOData::DATA_TYPE_INFO, systemTime(SYSTEM_TIME_BOOTTIME));
    info.setString(KEY_INFO_TYPE, VALUE_INFO_TYPE_SESSION);

    ECODataKeyValueIterator iter(stats);
    while (iter.hasNext()) {
        ECOData::ECODataKeyValuePair entry = iter.next();
        const std::string& key = entry.first;
        const ECOData::ECODataValueType value = entry.second;
        ALOGV("Processing key: %s", key.c_str());
        if (!key.compare(KEY_STATS_TYPE)) {
            // Skip the key KEY_STATS_TYPE as that has been parsed already.
            continue;
        } else if (!key.compare(ENCODER_TYPE)) {
            mCodecType = std::get<int32_t>(value);
            ALOGV("codec type is %d", mCodecType);
        } else if (!key.compare(ENCODER_PROFILE)) {
            mCodecProfile = std::get<int32_t>(value);
            ALOGV("codec profile is %d", mCodecProfile);
        } else if (!key.compare(ENCODER_LEVEL)) {
            mCodecLevel = std::get<int32_t>(value);
            ALOGV("codec level is %d", mCodecLevel);
        } else if (!key.compare(ENCODER_TARGET_BITRATE_BPS)) {
            mBitrateBps = std::get<int32_t>(value);
            ALOGV("codec bitrate is %d", mBitrateBps);
        } else if (!key.compare(ENCODER_KFI_FRAMES)) {
            mKeyFrameIntervalFrames = std::get<int32_t>(value);
            ALOGV("codec kfi is %d", mKeyFrameIntervalFrames);
        } else if (!key.compare(ENCODER_FRAMERATE_FPS)) {
            mFramerateFps = std::get<float>(value);
            ALOGV("codec framerate is %f", mFramerateFps);
        } else if (!key.compare(ENCODER_INPUT_WIDTH)) {
            int32_t width = std::get<int32_t>(value);
            if (width != mWidth) {
                ALOGW("Codec width: %d, expected: %d", width, mWidth);
            }
            ALOGV("codec width is %d", width);
        } else if (!key.compare(ENCODER_INPUT_HEIGHT)) {
            int32_t height = std::get<int32_t>(value);
            if (height != mHeight) {
                ALOGW("Codec height: %d, expected: %d", height, mHeight);
            }
            ALOGV("codec height is %d", height);
        } else {
            ALOGW("Unknown frame stats key %s from provider.", key.c_str());
            continue;
        }
        info.set(key, value);
    }

    if (mListener != nullptr) {
        mListener->onNewInfo(info);
    }

    return true;
}

bool ECOSession::processFrameStats(const ECOData& stats) {
    ALOGD("processFrameStats");

    bool needToNotifyListener = false;
    ECOData info(ECOData::DATA_TYPE_INFO, systemTime(SYSTEM_TIME_BOOTTIME));
    info.setString(KEY_INFO_TYPE, VALUE_INFO_TYPE_FRAME);

    ECODataKeyValueIterator iter(stats);
    while (iter.hasNext()) {
        ECOData::ECODataKeyValuePair entry = iter.next();
        const std::string& key = entry.first;
        const ECOData::ECODataValueType value = entry.second;
        ALOGD("Processing %s key", key.c_str());

        // Only process the keys that are supported by ECOService 1.0.
        if (!key.compare(FRAME_NUM) || !key.compare(FRAME_PTS_US) || !key.compare(FRAME_TYPE) ||
            !key.compare(FRAME_SIZE_BYTES)) {
            info.set(key, value);
        } else if (!key.compare(FRAME_AVG_QP)) {
            // Check the qp to see if need to notify the listener.
            const int32_t currAverageQp = std::get<int32_t>(value);

            // Check if the delta between current QP and last reported QP is larger than the
            // threshold specified by the listener.
            const bool largeQPChangeDetected =
                    abs(currAverageQp - mLastReportedQp) > mListenerQpCondition.mQpChangeThreshold;

            // Check if the qp is going from below threshold to beyond threshold.
            const bool exceedQpBlockinessThreshold =
                    (mLastReportedQp <= mListenerQpCondition.mQpBlocknessThreshold &&
                     currAverageQp > mListenerQpCondition.mQpBlocknessThreshold);

            // Check if the qp is going from beyond threshold to below threshold.
            const bool fallBelowQpBlockinessThreshold =
                    (mLastReportedQp > mListenerQpCondition.mQpBlocknessThreshold &&
                     currAverageQp <= mListenerQpCondition.mQpBlocknessThreshold);

            // Notify the listener if any of the above three conditions met.
            if (largeQPChangeDetected || exceedQpBlockinessThreshold ||
                fallBelowQpBlockinessThreshold) {
                mLastReportedQp = currAverageQp;
                needToNotifyListener = true;
            }

            info.set(key, value);
        } else {
            ALOGW("Unknown frame stats key %s from provider.", key.c_str());
        }
    }

    if (needToNotifyListener && mListener != nullptr) {
        mListener->onNewInfo(info);
    }

    return true;
}

Status ECOSession::addStatsProvider(
        const sp<::android::media::eco::IECOServiceStatsProvider>& provider,
        const ::android::media::eco::ECOData& config, bool* status) {
    ::android::String16 name;
    provider->getName(&name);

    ALOGV("Try to add stats provider name: %s uid: %d pid %d", ::android::String8(name).string(),
          IPCThreadState::self()->getCallingUid(), IPCThreadState::self()->getCallingPid());

    if (provider == nullptr) {
        ALOGE("%s: provider must not be null", __FUNCTION__);
        *status = false;
        return STATUS_ERROR(ERROR_ILLEGAL_ARGUMENT, "Null provider given to addStatsProvider");
    }

    std::scoped_lock<std::mutex> lock(mSessionLock);

    if (mProvider != nullptr) {
        ::android::String16 name;
        mProvider->getName(&name);
        String8 errorMsg = String8::format(
                "ECOService 1.0 only supports one stats provider, current provider: %s",
                ::android::String8(name).string());
        ALOGE("%s", errorMsg.string());
        *status = false;
        return STATUS_ERROR(ERROR_ALREADY_EXISTS, errorMsg.string());
    }

    // TODO: Handle the provider config.
    if (config.getDataType() != ECOData::DATA_TYPE_STATS_PROVIDER_CONFIG) {
        ALOGE("Provider config is invalid");
        *status = false;
        return STATUS_ERROR(ERROR_ILLEGAL_ARGUMENT, "Provider config is invalid");
    }

    mProvider = provider;
    *status = true;
    return binder::Status::ok();
}

Status ECOSession::removeStatsProvider(
        const sp<::android::media::eco::IECOServiceStatsProvider>& provider, bool* status) {
    std::scoped_lock<std::mutex> lock(mSessionLock);
    // Check if the provider is the same as current provider for the session.
    if (provider.get() != mProvider.get()) {
        *status = false;
        return STATUS_ERROR(ERROR_ILLEGAL_ARGUMENT, "Provider does not match");
    }

    mProvider = nullptr;
    *status = true;
    return binder::Status::ok();
}

Status ECOSession::addInfoListener(
        const sp<::android::media::eco::IECOServiceInfoListener>& listener,
        const ::android::media::eco::ECOData& config, bool* status) {
    ALOGV("%s: Add listener %p", __FUNCTION__, listener.get());
    std::scoped_lock<std::mutex> lock(mSessionLock);

    if (mListener != nullptr) {
        ALOGE("ECOService 1.0 only supports one listener");
        *status = false;
        return STATUS_ERROR(ERROR_ALREADY_EXISTS, "ECOService 1.0 only supports one listener");
    }

    if (listener == nullptr) {
        ALOGE("%s: listener must not be null", __FUNCTION__);
        *status = false;
        return STATUS_ERROR(ERROR_ILLEGAL_ARGUMENT, "Null listener given to addInfoListener");
    }

    if (config.getDataType() != ECOData::DATA_TYPE_INFO_LISTENER_CONFIG) {
        *status = false;
        ALOGE("%s: listener config is invalid", __FUNCTION__);
        return STATUS_ERROR(ERROR_ILLEGAL_ARGUMENT, "listener config is invalid");
    }

    if (config.isEmpty()) {
        *status = false;
        ALOGE("Listener must provide listening criterion");
        return STATUS_ERROR(ERROR_ILLEGAL_ARGUMENT, "listener config is empty");
    }

    // For ECOService 1.0, listener must specify the two threshold in order to receive info.
    if (config.findInt32(KEY_LISTENER_QP_BLOCKINESS_THRESHOLD,
                         &mListenerQpCondition.mQpBlocknessThreshold) != ECODataStatus::OK ||
        config.findInt32(KEY_LISTENER_QP_CHANGE_THRESHOLD,
                         &mListenerQpCondition.mQpChangeThreshold) != ECODataStatus::OK ||
        mListenerQpCondition.mQpBlocknessThreshold < ENCODER_MIN_QP ||
        mListenerQpCondition.mQpBlocknessThreshold > ENCODER_MAX_QP) {
        *status = false;
        ALOGE("%s: listener config is invalid", __FUNCTION__);
        return STATUS_ERROR(ERROR_ILLEGAL_ARGUMENT, "listener config is not valid");
    }

    ::android::String16 name;
    listener->getName(&name);

    ALOGD("Info listener name: %s uid: %d pid %d", ::android::String8(name).string(),
          IPCThreadState::self()->getCallingUid(), IPCThreadState::self()->getCallingPid());

    mListener = listener;
    *status = true;
    return binder::Status::ok();
}

Status ECOSession::removeInfoListener(
        const sp<::android::media::eco::IECOServiceInfoListener>& listener, bool* _aidl_return) {
    std::scoped_lock<std::mutex> lock(mSessionLock);
    // Check if the listener is the same as current listener for the session.
    if (listener.get() != mListener.get()) {
        *_aidl_return = false;
        return STATUS_ERROR(ERROR_ILLEGAL_ARGUMENT, "Listener does not match");
    }

    mListener = nullptr;
    *_aidl_return = true;
    return binder::Status::ok();
}

Status ECOSession::pushNewStats(const ::android::media::eco::ECOData& stats, bool*) {
    ALOGV("ECOSession get new stats type: %s", stats.getDataTypeString().c_str());
    std::unique_lock<std::mutex> lock(mStatsQueueLock);
    mStatsQueue.push_back(stats);
    mStatsQueueWaitCV.notify_all();
    return binder::Status::ok();
}

Status ECOSession::getWidth(int32_t* _aidl_return) {
    std::scoped_lock<std::mutex> lock(mSessionLock);
    *_aidl_return = mWidth;
    return binder::Status::ok();
}

Status ECOSession::getHeight(int32_t* _aidl_return) {
    std::scoped_lock<std::mutex> lock(mSessionLock);
    *_aidl_return = mHeight;
    return binder::Status::ok();
}

Status ECOSession::getNumOfListeners(int32_t* _aidl_return) {
    std::scoped_lock<std::mutex> lock(mSessionLock);
    *_aidl_return = (mListener == nullptr ? 0 : 1);
    return binder::Status::ok();
}

Status ECOSession::getNumOfProviders(int32_t* _aidl_return) {
    std::scoped_lock<std::mutex> lock(mSessionLock);
    *_aidl_return = (mProvider == nullptr ? 0 : 1);
    return binder::Status::ok();
}

/*virtual*/ void ECOSession::binderDied(const wp<IBinder>& /*who*/) {
    ALOGV("binderDied");
}

}  // namespace eco
}  // namespace media
}  // namespace android
