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

#ifndef ANDROID_MEDIA_ECO_DATA_H_
#define ANDROID_MEDIA_ECO_DATA_H_

#include <binder/Parcel.h>
#include <binder/Parcelable.h>

namespace android {
namespace media {
namespace eco {

/**
* ECOData is the container for all messages passed between different components in ECOService.
* All messages in ECOServices are represented by a list of key-value pairs.
* For example:
*     "bit-rate" -> 22000000
*     "Provider-Name" -> "QCOM-Video-Encoder".
*     "avg-frame-qp" -> 40
* ECOData follows the same design pattern of AMessage and Metadata in Media Framework.
* TODO(hkuang): Add the implementation and sample usage.
*/
class ECOData : public Parcelable {
public:
    ECOData() : mDataType(0), mDataTimeUs(-1) {}
    ECOData(int32_t type) : mDataType(type), mDataTimeUs(-1) {}
    ECOData(int32_t type, int64_t timeUs) : mDataType(type), mDataTimeUs(timeUs) {}

    // Constants for mDataType.
    enum {
        DATA_TYPE_UNKNOWN = 0,
        /* Data sent from the ECOServiceStatsProvider to ECOService. */
        DATA_TYPE_STATS = 1,
        /* Data sent from the ECOService to ECOServiceInfoListener. */
        DATA_TYPE_INFO = 2,
        /* Configuration data sent by ECOServiceStatsProvider when connects with ECOService. */
        DATA_TYPE_STATS_PROVIDER_OPTON = 3,
        /* Configuration data sent by ECOServiceInfoListener when connects with ECOService. */
        DATA_TYPE_INFO_LISTENER_OPTON = 4,
    };

    /**
    * Serialization over Binder
    */
    status_t readFromParcel(const Parcel* parcel) override;
    status_t writeToParcel(Parcel* parcel) const override;

private:
    /* The type of the data */
    int32_t mDataType;

    // The timestamp time associated with the data in microseconds. The timestamp should be in
    // boottime time base. This is only used when the data type is stats or info. -1 means
    // unavailable.
    int64_t mDataTimeUs;
};

}  // namespace eco
}  // namespace media
}  // namespace android

#endif  // ANDROID_MEDIA_ECO_DATA_H_
