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

#ifndef C2_COMPONENT_WRAPPER_H_
#define C2_COMPONENT_WRAPPER_H_

#include <stdbool.h>
#include <stdint.h>

namespace android {

/**
 * Creates a Wrapper around the class C2Component and its methods. The wrapper is used to
 * simulate errors in the android media components by fault injection technique.
 * This is done to check how the framework handles the error situation.
 *
 */
class C2ComponentWrapper
        : public C2Component, public std::enable_shared_from_this<C2ComponentWrapper> {
public:
/**
 * Creates a wrapper around the listener class inside C2Component class.
 */
    class Listener : public C2Component::Listener {
    public:
        explicit Listener(const std::shared_ptr<C2Component::Listener> &listener);
        virtual ~Listener() = default;

        enum FaultMode {
            // run the method for infinite amount of time
            IS_INFINITE,
            // the status of the work is changed
            IS_ALTERED
        };

        c2_status_t mAlteredListenerResult;
        FaultMode mWorkDoneMode;
        void setOnWorkDoneMode(Listener::FaultMode mode);
        void setAlteredListenerResult(c2_status_t status);
        void onWorkDone_nb(std::weak_ptr<C2Component> component,
                           std::list<std::unique_ptr<C2Work>> workItems) override;
        void onTripped_nb(std::weak_ptr<C2Component> component,
                          std::vector<std::shared_ptr<C2SettingResult>> settingResult) override;
        void onError_nb(std::weak_ptr<C2Component> component, uint32_t errorCode) override;
 
    private:
        std::shared_ptr<C2Component::Listener> mListener;
    };

    explicit C2ComponentWrapper(const std::shared_ptr<C2Component> &comp);
    virtual ~C2ComponentWrapper() = default;
    virtual c2_status_t setListener_vb(
            const std::shared_ptr<C2Component::Listener> &listener,
            c2_blocking_t mayBlock) override;
    virtual c2_status_t queue_nb(std::list<std::unique_ptr<C2Work>>* const items) override;
    virtual c2_status_t announce_nb(const std::vector<C2WorkOutline> &items) override;
    virtual c2_status_t flush_sm(
            flush_mode_t mode, std::list<std::unique_ptr<C2Work>>* const flushedWork) override;
    virtual c2_status_t drain_nb(drain_mode_t mode) override;
    virtual c2_status_t start() override;
    virtual c2_status_t stop() override;
    virtual c2_status_t reset() override;
    virtual c2_status_t release() override;
    virtual std::shared_ptr<C2ComponentInterface> intf() override;

    enum FaultMode {
        // work fine with no errors
        WORK_OKAY,
        // error with corrupt value
        IS_CORRUPT,
        // error with timed out component
        IS_TIMED_OUT,
        // run the method for infinite amount of time
        IS_INFINITE,
        // error with handling memory
        HAS_NO_MEMORY,
        // bad internal state error
        IS_BAD_STATE
    };

    enum FlushDrainFaultMode {
        // run the method for infinite amount of time
        IS_HANG,
        // the status of the work is changed
        IS_ALTERED
    };

    FlushDrainFaultMode mFlushMode;
    FlushDrainFaultMode mDrainMode;
    FaultMode mStartMode;
    FaultMode mStopMode;
    FaultMode mResetMode;
    FaultMode mReleaseMode;
    c2_status_t mAlteredResult;

    void setStartMode(FaultMode mode);
    void setStopMode(FaultMode mode);
    void setResetMode(FaultMode mode);
    void setReleaseMode(FaultMode mode);
    void setFlushMode(FlushDrainFaultMode mode);
    void setDrainMode(FlushDrainFaultMode mode);
    void setAlteredDrainResult(c2_status_t status);
    void setAlteredFlushResult(c2_status_t status);

private:
    c2_status_t switchMode(FaultMode mode, std::function<c2_status_t()> func);
    std::shared_ptr<Listener> mListener;
    std::shared_ptr<C2Component> mComp;
};

}  // namespace android

#endif // C2_COMPONENT_WRAPPER_H_
