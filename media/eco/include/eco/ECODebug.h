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

#ifndef ANDROID_MEDIA_ECO_DEBUG_H_
#define ANDROID_MEDIA_ECO_DEBUG_H_

#include <cutils/atomic.h>

#include "ECOData.h"

namespace android {
namespace media {
namespace eco {

// Convenience methods for constructing binder::Status objects for error returns

#define STATUS_ERROR(errorCode, errorString)  \
    binder::Status::fromServiceSpecificError( \
            errorCode, String8::format("%s:%d: %s", __FUNCTION__, __LINE__, errorString))

#define STATUS_ERROR_FMT(errorCode, errorString, ...) \
    binder::Status::fromServiceSpecificError(         \
            errorCode,                                \
            String8::format("%s:%d: " errorString, __FUNCTION__, __LINE__, __VA_ARGS__))

// ----------------------------------------------------------------------------

}  // namespace eco
}  // namespace media
}  // namespace android

#endif  // ANDROID_MEDIA_ECO_DEBUG_H_
