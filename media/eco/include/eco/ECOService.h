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

#ifndef ANDROID_MEDIA_ECO_SERVICE_H_
#define ANDROID_MEDIA_ECO_SERVICE_H_

#include <android/media/eco/BnECOService.h>
#include <binder/BinderService.h>

namespace android {
namespace media {
namespace eco {

/**
 * ECO (Encoder Camera Optimization) service.
 *
 * ECOService creates and manages EcoSession to relay feedback information between one or multiple
 * ECOServiceStatsProvider and ECOServiceInfoListener. The relationship can be many-to-many. In
 * general, ECOServiceStatsProvider extracts information from an encoder for a given encoding
 * session. EcoSession then relays the encoder information to any subscribed
 * ECOServiceInfoListener.
 *
 * Internally, ECOService creates an ECOSession for each encoding session. Upon start, both
 * ECOServiceStatsProvider and ECOServiceInfoListener should call obtainSession to get the
 * ECOSession instance. After that, ECOServiceStatsProvider should push Stats to ECOSession and
 * ECOServiceInfoListener should listen to the info from ECOSession. Upon finish, both
 * ECOServiceStatsProvider and ECOServiceInfoListener should remove themselves from ECOSession.
 * Then ECOService will safely destroy the ECOSession.
 */
class ECOService : public BinderService<ECOService>,
                   public BnECOService,
                   public virtual IBinder::DeathRecipient {
    friend class BinderService<ECOService>;

public:
    ECOService();
    //TODO(hkuang): Add the implementation.

    virtual ~ECOService() {}

    virtual ::android::binder::Status obtainSession(
            int32_t width, int32_t height, bool isCameraRecording,
            ::android::sp<::android::media::eco::IECOSession>* _aidl_return);

    virtual ::android::binder::Status getNumOfSessions(int32_t* _aidl_return);

    virtual ::android::binder::Status getSessions(
            ::std::vector<::android::sp<::android::IBinder>>* _aidl_return);

    // Implementation of BinderService<T>
    static char const* getServiceName() { return "media.ecoservice"; }

    // IBinder::DeathRecipient implementation
    virtual void binderDied(const wp<IBinder>& who);

private:
};

}  // namespace eco
}  // namespace media
}  // namespace android

#endif  // ANDROID_MEDIA_ECO_SERVICE_H_
