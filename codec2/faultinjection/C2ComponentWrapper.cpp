/*
 * Copyright (C) 2018 The Android Open Source Project
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

#define LOG_NDEBUG 0
#define LOG_TAG "C2ComponentWrapper"

#include <inttypes.h>
#include <unistd.h>
#include <list>

#include <C2Config.h>
#include <C2PlatformSupport.h>

#include <C2ComponentWrapper.h>

#include <functional>

namespace android {

void C2ComponentWrapper::setFlushMode(C2ComponentWrapper::FlushDrainFaultMode mode) {
    mFlushMode = mode;
}

void C2ComponentWrapper::setDrainMode(C2ComponentWrapper::FlushDrainFaultMode mode) {
    mDrainMode = mode;
}

void C2ComponentWrapper::setStartMode(C2ComponentWrapper::FaultMode mode) {
    mStartMode = mode;
}

void C2ComponentWrapper::setStopMode(C2ComponentWrapper::FaultMode mode) {
    mStopMode = mode;
}

void C2ComponentWrapper::setResetMode(C2ComponentWrapper::FaultMode mode) {
    mResetMode = mode;
}

void C2ComponentWrapper::setReleaseMode(C2ComponentWrapper::FaultMode mode) {
    mReleaseMode = mode;
}

void C2ComponentWrapper::Listener::setOnWorkDoneMode(C2ComponentWrapper::Listener::FaultMode mode) {
    mWorkDoneMode = mode;
}

void C2ComponentWrapper::Listener::setAlteredListenerResult(c2_status_t status) {
    mWorkDoneMode = IS_ALTERED;
    mAlteredListenerResult = status;
}

C2ComponentWrapper::Listener::Listener(
        const std::shared_ptr<C2Component::Listener> &listener) : mListener(listener) {}

void C2ComponentWrapper::Listener::onWorkDone_nb(std::weak_ptr<C2Component> component,
        std::list<std::unique_ptr<C2Work>> workItems) {
    switch(mWorkDoneMode) {
        case IS_INFINITE:
            while(true) {
                sleep(1);
            }
            break;
        case IS_ALTERED:
            for (const auto& work : workItems) {
                work->result = mAlteredListenerResult;
             }
            break;
        default:
            break;
    }
    mListener->onWorkDone_nb(component, std::move(workItems));
}

void C2ComponentWrapper::Listener::onTripped_nb(std::weak_ptr<C2Component> component,
        std::vector<std::shared_ptr<C2SettingResult>> settingResult) {
    mListener->onTripped_nb(component,settingResult);
}

void C2ComponentWrapper::Listener::onError_nb(
        std::weak_ptr<C2Component> component, uint32_t errorCode) {
    mListener->onError_nb(component, errorCode);
}

C2ComponentWrapper::C2ComponentWrapper(
        const std::shared_ptr<C2Component> &comp) : mComp(comp) {}

c2_status_t C2ComponentWrapper::setListener_vb(
        const std::shared_ptr<C2Component::Listener> &listener, c2_blocking_t mayBlock) {
    mListener = std::make_shared<Listener>(listener);
    return mComp->setListener_vb(mListener, mayBlock);
}

void C2ComponentWrapper::setAlteredFlushResult(c2_status_t status) {
    mAlteredResult = status;
}

void C2ComponentWrapper::setAlteredDrainResult(c2_status_t status) {
    mAlteredResult = status;
}

c2_status_t C2ComponentWrapper::queue_nb(std::list<std::unique_ptr<C2Work>>* const items) {
    return mComp->queue_nb(items);
}

c2_status_t C2ComponentWrapper::announce_nb(const std::vector<C2WorkOutline> &items) {
    return mComp->announce_nb(items);
}

c2_status_t C2ComponentWrapper::flush_sm(
        C2Component::flush_mode_t mode, std::list<std::unique_ptr<C2Work>>* const flushedWork) {
    switch(mFlushMode) {
        case IS_HANG:
            while(true) {
                sleep(1);
            }
            break;
        case IS_ALTERED:
            return mAlteredResult;
        default:
            break;
    }
    return mComp->flush_sm(mode, flushedWork);
}

c2_status_t C2ComponentWrapper::drain_nb(C2Component::drain_mode_t mode) {
     switch(mDrainMode) {
        case IS_HANG:
            while(true) {
                sleep(1);
            }
            break;
        case IS_ALTERED:
            return mAlteredResult;
        default:
            break;
    }
    return mComp->drain_nb(mode);
}

c2_status_t C2ComponentWrapper::switchMode(FaultMode mode, std::function<c2_status_t()> func) {
     switch (mode) {
        case IS_CORRUPT: return C2_CORRUPTED;
        case IS_TIMED_OUT: sleep(1); return C2_TIMED_OUT;
        case IS_INFINITE:
            while(true) {
                sleep(1);
            }
        case HAS_NO_MEMORY: return C2_NO_MEMORY;
        default:
            return func();
            
    }
}

c2_status_t C2ComponentWrapper::start() {
    return switchMode(mStartMode, [this] { return mComp->start(); });
}

c2_status_t C2ComponentWrapper::stop() {
    return switchMode(mStopMode, [this] { return mComp->stop(); });
}

c2_status_t C2ComponentWrapper::reset() {
    return switchMode(mResetMode, [this] { return mComp->reset(); });
}

c2_status_t C2ComponentWrapper::release() {
    return switchMode(mReleaseMode, [this] { return mComp->release(); });
}

std::shared_ptr<C2ComponentInterface> C2ComponentWrapper::intf(){
    return mComp->intf();
}

}  // namespace android