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
#define LOG_TAG "ECOService"

#include "eco/ECOService.h"

#include <binder/BinderService.h>
#include <cutils/atomic.h>
#include <inttypes.h>
#include <pthread.h>
#include <stdio.h>
#include <sys/types.h>

#include <algorithm>
#include <climits>
#include <cstring>
#include <ctime>
#include <string>

#include "eco/ECODebug.h"

namespace android {
namespace media {
namespace eco {

// ----------------------------------------------------------------------------
// Logging support -- this is for debugging only
// Use "adb shell dumpsys media.ECOService -v 1" to change it.
volatile int32_t gLogLevel = 0;

#define LOG1(...) ALOGD_IF(gLogLevel >= 1, __VA_ARGS__);
#define LOG2(...) ALOGD_IF(gLogLevel >= 2, __VA_ARGS__);

static void setLogLevel(int level) {
    android_atomic_write(level, &gLogLevel);
}

ECOService::ECOService() : BnECOService() {
    ALOGD("ECOService created");
    setLogLevel(10);
}

/*virtual*/ ::android::binder::Status ECOService::obtainSession(
        int32_t /* width */, int32_t /* height */, bool /* isCameraRecording */,
        ::android::sp<::android::media::eco::IECOSession>* /* _aidl_return */) {
    //TODO(hkuang): Add implementation.
    return STATUS_ERROR(ERROR_UNSUPPORTED, "Not implemented yet");
}

/*virtual*/ ::android::binder::Status ECOService::getNumOfSessions(int32_t* /* _aidl_return */) {
    //TODO(hkuang): Add implementation.
    return STATUS_ERROR(ERROR_UNSUPPORTED, "Not implemented yet");
}

/*virtual*/ ::android::binder::Status ECOService::getSessions(
        ::std::vector<::android::sp<::android::IBinder>>* /* _aidl_return */) {
    //TODO(hkuang): Add implementation.
    return STATUS_ERROR(ERROR_UNSUPPORTED, "Not implemented yet");
}

/*virtual*/ void ECOService::binderDied(const wp<IBinder>& /*who*/) {}

}  // namespace eco
}  // namespace media
}  // namespace android