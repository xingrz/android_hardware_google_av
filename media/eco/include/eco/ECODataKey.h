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
#ifndef ANDROID_MEDIA_ECO_DATA_KEY_H_
#define ANDROID_MEDIA_ECO_DATA_KEY_H_

#include <stdint.h>
#include <sys/mman.h>

#include <android-base/unique_fd.h>
#include <binder/Parcel.h>
#include <binder/Parcelable.h>
#include "ECOService.h"

namespace android {
namespace media {
namespace eco {

// ================================================================================================
// Standard ECOService Stats keys. These keys are used in the ECOData sent from StatsProvider
// to ECOService. Besides these standard keys, StatsProvider could also have their provider
// specific keys.
// ================================================================================================
/* Keys for provider of type STATS_PROVIDER_TYPE_VIDEO_ENCODER. */
constexpr char STATS_KEY_ENCODER_TYPE[] = "stats-encoder-type";
constexpr char STATS_KEY_ENCODER_PROFILE[] = "stats-encoder-profile";
constexpr char STATS_KEY_ENCODER_LEVEL[] = "stats-encoder-level";
constexpr char STATS_KEY_ENCODER_TARGET_BITRATE_BPS[] = "stats-encoder-target-bitrate-bps";
constexpr char STATS_KEY_ENCODER_KFI_FRAMES[] = "stats-encoder-kfi-frames";

constexpr char STATS_KEY_FRAME_PTS_US[] = "stats-frame-pts-us";
constexpr char STATS_KEY_FRAME_AVG_QP[] = "stats-frame-avg-qp";
constexpr char STATS_KEY_FRAME_TYPE[] = "stats-frame-type";
constexpr char STATS_KEY_FRAME_SIZE_BYTES[] = "stats-frame-size-bytes";

// ================================================================================================
// Standard ECOServiceStatsProvider option keys. These keys are used in the ECOData as an option
// when StatsProvider connects with ECOService.
// ================================================================================================
constexpr char PROVIDER_KEY_NAME[] = "provider-name";
constexpr char PROVIDER_KEY_TYPE[] = "provider-type";

// ================================================================================================
// Standard ECOServiceInfoListener option keys. These keys are used in the ECOData as option
// when ECOServiceInfoListener connects with ECOService to specify the informations that the
// listener wants to listen to.
// ================================================================================================
constexpr char LISTENER_KEY_LISTENER_NAME[] = "listener-name";
constexpr char LISTENER_KEY_TYPE[] = "listener-type";
// This key's value will be ECOData.
constexpr char LISTENER_KEY_CRITERION[] = "listener-criterion";
// Listener will receive notification when qp falls below this key's value.
constexpr char LISTENER_KEY_CRITERION_QP_LOW[] = "listener-criterion-qp-low";
// Listener will receive notification when qp goes beyond this key's value.
constexpr char LISTENER_KEY_CRITERION_QP_HIGH[] = "listener-criterion-qp-high";
// Listener will receive notification when bitrate goes beyond this key's value.
constexpr char LISTENER_KEY_CRITERION_BITRATE_OVERSHOOT[] = "listener-criterion-bitrate-overshoot";
// Listener will receive notification when bitrate falls below this key's value.
constexpr char LISTENER_KEY_CRITERION_BITRATE_UNDERSHOOT[] =
        "listener-criterion-bitrate-undershoot";

// ================================================================================================
// Standard ECOService Info keys. These keys are used in the ECOData sent from ECOService
// to ECOServiceInfoListener. The information includes session informations and frame informations.
// ================================================================================================
constexpr char INFO_KEY_ENCODER_TYPE[] = "info-encoder-type";
constexpr char INFO_KEY_ENCODER_PROFILE[] = "info-encoder-profile";
constexpr char INFO_KEY_ENCODER_LEVEL[] = "info-encoder-level";
constexpr char INFO_KEY_ENCODER_TARGET_BITRATE_BPS[] = "info-encoder-target-bitrate-bps";
constexpr char INFO_KEY_ENCODER_KFI_FRAMES[] = "info-encoder-kfi-frames";

constexpr char INFO_KEY_FRAME_AVG_QP[] = "info-frame-avg-qp";
constexpr char INFO_KEY_FRAME_TYPE[] = "info-frame-type";
constexpr char INFO_KEY_FRAME_SIZE_BYTES[] = "info-frame-size-bytes";
constexpr char INFO_KEY_CURRENT_BITRATE_BPS[] = "info-current-bitrate-bps";

// ================================================================================================
// Standard ECOData keys.
// ================================================================================================
constexpr char ECO_DATA_KEY_TYPE[] = "eco-data-type";
constexpr char ECO_DATA_KEY_TIME_US[] = "eco-data-time-us";

}  // namespace eco
}  // namespace media
}  // namespace android

#endif  // ANDROID_MEDIA_ECO_DATA_KEY_H_
