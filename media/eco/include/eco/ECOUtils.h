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

#ifndef ANDROID_MEDIA_ECO_UTILS_H_
#define ANDROID_MEDIA_ECO_UTILS_H_

#include <utils/Errors.h>

namespace android {
namespace media {
namespace eco {

#define RETURN_STATUS_IF_ERROR(expr)  \
    {                                 \
        status_t _errorCode = (expr); \
        if (_errorCode != NO_ERROR) { \
            return _errorCode;        \
        }                             \
    }

}  // namespace eco
}  // namespace media
}  // namespace android

#endif  // ANDROID_MEDIA_ECO_UTILS_H_
