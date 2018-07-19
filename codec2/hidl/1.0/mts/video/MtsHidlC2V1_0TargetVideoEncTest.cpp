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

//#define LOG_NDEBUG 0
#define LOG_TAG "codec2_hidl_hal_video_enc_test"

#include <android-base/logging.h>
#include <gtest/gtest.h>
#include <stdio.h>
#include <fstream>

#include <codec2/hidl/client.h>
#include <C2AllocatorIon.h>
#include <C2Config.h>
#include <C2Debug.h>
#include <C2Buffer.h>
#include <C2BufferPriv.h>

using android::C2AllocatorIon;

#include <VtsHalHidlTargetTestBase.h>
#include "media_c2_video_hidl_test_common.h"
#include "media_c2_hidl_test_common.h"

class GraphicBuffer : public C2Buffer {
public:
  explicit GraphicBuffer(const std::shared_ptr<C2GraphicBlock> &block)
      : C2Buffer({block->share(C2Rect(block->width(), block->height()),
                               ::C2Fence())}) {}
};

static ComponentTestEnvironment* gEnv = nullptr;

namespace {

class Codec2VideoDecHidlTest : public ::testing::VtsHalHidlTargetTestBase {
   private:
    typedef ::testing::VtsHalHidlTargetTestBase Super;

   public:
    ::std::string getTestCaseInfo() const override {
        return ::std::string() +
                "Component: " + gEnv->getComponent().c_str() + " | " +
                "Instance: " + gEnv->getInstance().c_str() + " | " +
                "Res: " + gEnv->getRes().c_str();
    }

    // google.codec2 Video test setup
    virtual void SetUp() override {
        Super::SetUp();
        mDisableTest = false;
        ALOGV("Codec2VideoDecHidlTest SetUp");
        mClient = android::Codec2Client::CreateFromService(
            gEnv->getInstance().c_str());
        ASSERT_NE(mClient, nullptr);
        mListener.reset(new CodecListener(
            [this](std::list<std::unique_ptr<C2Work>>& workItems) {
                handleWorkDone(workItems);
            }));
        ASSERT_NE(mListener, nullptr);
        for (int i = 0; i < MAX_INPUT_BUFFERS; ++i) {
            mWorkQueue.emplace_back(new C2Work);
        }
        mClient->createComponent(gEnv->getComponent().c_str(), mListener,
                                 &mComponent);
        ASSERT_NE(mComponent, nullptr);

        std::shared_ptr<C2AllocatorStore> store =
            android::GetCodec2PlatformAllocatorStore();
        CHECK_EQ(store->fetchAllocator(C2AllocatorStore::DEFAULT_GRAPHIC,
                                       &mGraphicAllocator),
                 C2_OK);
        mGraphicPool = std::make_shared<C2PooledBlockPool>(mGraphicAllocator,
                                                           mBlockPoolId++);
        ASSERT_NE(mGraphicPool, nullptr);

        mCompName = unknown_comp;
        struct StringToName {
            const char* Name;
            standardComp CompName;
        };

        const StringToName kStringToName[] = {
            {"h263", h263}, {"avc", avc}, {"mpeg4", mpeg4},
            {"hevc", hevc}, {"vp8", vp8}, {"vp9", vp9},
        };

        const size_t kNumStringToName =
            sizeof(kStringToName) / sizeof(kStringToName[0]);

        std::string substring;
        std::string comp;
        substring = std::string(gEnv->getComponent());
        /* TODO: better approach to find the component */
        /* "c2.android." => 11th position */
        size_t pos = 11;
        size_t len = substring.find(".encoder", pos);
        comp = substring.substr(pos, len - pos);

        for (size_t i = 0; i < kNumStringToName; ++i) {
            if (!strcasecmp(comp.c_str(), kStringToName[i].Name)) {
                mCompName = kStringToName[i].CompName;
                break;
            }
        }
        mEos = false;
        mCsd = false;
        mFramesReceived = 0;
        if (mCompName == unknown_comp) mDisableTest = true;
        if (mDisableTest) std::cout << "[   WARN   ] Test Disabled \n";
    }

    virtual void TearDown() override {
        if (mComponent != nullptr) {
            if (::testing::Test::HasFatalFailure()) return;
            mComponent->release();
            mComponent = nullptr;
        }
        Super::TearDown();
    }

    // callback function to process onWorkDone received by Listener
    void handleWorkDone(std::list<std::unique_ptr<C2Work>>& workItems) {
        for (std::unique_ptr<C2Work>& work : workItems) {
            // handle configuration changes in work done
            if (!work->worklets.empty() &&
                (work->worklets.front()->output.configUpdate.size() != 0)) {
                ALOGV("Config Update");
                std::vector<std::unique_ptr<C2Param>> updates =
                    std::move(work->worklets.front()->output.configUpdate);
                std::vector<C2Param*> configParam;
                std::vector<std::unique_ptr<C2SettingResult>> failures;
                for (size_t i = 0; i < updates.size(); ++i) {
                    C2Param* param = updates[i].get();
                    if (param->index() == C2StreamCsdInfo::output::PARAM_TYPE) {
                        mCsd = true;
                    }
                }
            }
            mFramesReceived++;
            mEos = (work->worklets.front()->output.flags &
                    C2FrameData::FLAG_END_OF_STREAM) != 0;
            ALOGV("WorkDone: frameID received %d",
                (int)work->worklets.front()->output.ordinal.frameIndex.peeku());
            work->input.buffers.clear();
            work->worklets.clear();
            typedef std::unique_lock<std::mutex> ULock;
            ULock l(mQueueLock);
            mWorkQueue.push_back(std::move(work));
            mQueueCondition.notify_all();
        }
    }

    enum standardComp {
        h263,
        avc,
        mpeg4,
        hevc,
        vp8,
        vp9,
        unknown_comp,
    };

    bool mEos;
    bool mCsd;
    bool mDisableTest;
    standardComp mCompName;
    uint32_t mFramesReceived;
    C2BlockPool::local_id_t mBlockPoolId;
    std::shared_ptr<C2BlockPool> mGraphicPool;
    std::shared_ptr<C2Allocator> mGraphicAllocator;

    std::mutex mQueueLock;
    std::condition_variable mQueueCondition;
    std::list<std::unique_ptr<C2Work>> mWorkQueue;

    std::shared_ptr<android::Codec2Client> mClient;
    std::shared_ptr<android::Codec2Client::Listener> mListener;
    std::shared_ptr<android::Codec2Client::Component> mComponent;

   protected:
    static void description(const std::string& description) {
        RecordProperty("description", description);
    }
};

void validateComponent(
    const std::shared_ptr<android::Codec2Client::Component>& component,
    Codec2VideoDecHidlTest::standardComp compName, bool& disableTest) {
    // Validate its a C2 Component
    if (component->getName().find("c2") == std::string::npos) {
        ALOGE("Not a c2 component");
        disableTest = true;
        return;
    }

    // Validate its not an encoder and the component to be tested is video
    if (component->getName().find("decoder") != std::string::npos) {
        ALOGE("Expected Encoder, given Decoder");
        disableTest = true;
        return;
    }
    std::vector<std::unique_ptr<C2Param>> queried;
    c2_status_t c2err =
        component->query({}, {C2PortMediaTypeSetting::input::PARAM_TYPE},
                         C2_DONT_BLOCK, &queried);
    if (c2err != C2_OK && queried.size() == 0) {
        ALOGE("Query media type failed => %d", c2err);
    } else {
        std::string inputDomain =
            ((C2StreamMediaTypeSetting::input*)queried[0].get())->m.value;
        if (inputDomain.find("video/") == std::string::npos) {
            ALOGE("Expected Video Component");
            disableTest = true;
            return;
        }
    }

    // Validates component name
    if (compName == Codec2VideoDecHidlTest::unknown_comp) {
        ALOGE("Component InValid");
        disableTest = true;
        return;
    }
    ALOGV("Component Valid");
}

// LookUpTable of clips for component testing
void GetURLForComponent(char* URL) {
    strcat(URL, "bbb_352x288_420p_30fps_32frames.yuv");
}

void encodeNFrames(const std::shared_ptr<android::Codec2Client::Component> &component,
                   std::mutex &queueLock, std::condition_variable &queueCondition,
                   std::list<std::unique_ptr<C2Work>> &workQueue,
                   std::shared_ptr<C2Allocator> &graphicAllocator,
                   std::shared_ptr<C2BlockPool> &graphicPool,
                   C2BlockPool::local_id_t blockPoolId,
                   std::ifstream& eleStream, uint32_t nFrames,
                   uint32_t nWidth, int32_t nHeight,
                   bool signalEOS = true) {
    typedef std::unique_lock<std::mutex> ULock;
    graphicPool =
        std::make_shared<C2PooledBlockPool>(graphicAllocator, blockPoolId++);

    uint32_t frameID = 0;
    uint32_t maxRetry = 0;
    int bytesCount = nWidth * nHeight * 3 >> 1;
    int32_t timestampIncr = ENCODER_TIMESTAMP_INCREMENT;
    uint64_t timestamp = 0;
    while (1) {
        if (nFrames == 0) break;
        uint32_t flags = 0;
        std::unique_ptr<C2Work> work;
        // Prepare C2Work
        while (!work && (maxRetry < MAX_RETRY)) {
            ULock l(queueLock);
            if (!workQueue.empty()) {
                work.swap(workQueue.front());
                workQueue.pop_front();
            } else {
                queueCondition.wait_for(l, TIME_OUT);
                maxRetry++;
            }
        }
        if (!work && (maxRetry >= MAX_RETRY)) {
            ASSERT_TRUE(false) << "Wait for generating C2Work exceeded timeout";
        }
        if (signalEOS && (nFrames == 1))
            flags |= C2FrameData::FLAG_END_OF_STREAM;

        work->input.flags = (C2FrameData::flags_t)flags;
        work->input.ordinal.timestamp = timestamp;
        work->input.ordinal.frameIndex = frameID;
        char* data = (char*)malloc(bytesCount);
        eleStream.read(data, bytesCount);
        ASSERT_EQ(eleStream.gcount(), bytesCount);
        std::shared_ptr<C2GraphicBlock> block;
        ASSERT_EQ(
            C2_OK,
            graphicPool->fetchGraphicBlock(
                nWidth, nHeight, HAL_PIXEL_FORMAT_YV12,
                {C2MemoryUsage::CPU_READ, C2MemoryUsage::CPU_WRITE}, &block));
        ASSERT_TRUE(block);
        // Graphic View
        C2GraphicView view = block->map().get();
        if (view.error() != C2_OK) {
            fprintf(stderr, "C2GraphicBlock::map() failed : %d", view.error());
            break;
        }

        uint8_t* pY = view.data()[C2PlanarLayout::PLANE_Y];
        uint8_t* pU = view.data()[C2PlanarLayout::PLANE_U];
        uint8_t* pV = view.data()[C2PlanarLayout::PLANE_V];

        memcpy(pY, data, nWidth * nHeight);
        memcpy(pU, data + nWidth * nHeight, (nWidth * nHeight >> 2));
        memcpy(pV, data + (nWidth * nHeight * 5 >> 2), nWidth * nHeight >> 2);

        work->input.buffers.clear();
        work->input.buffers.emplace_back(new GraphicBuffer(block));
        work->worklets.clear();
        work->worklets.emplace_back(new C2Worklet);
        free(data);

        std::list<std::unique_ptr<C2Work>> items;
        items.push_back(std::move(work));

        // DO THE ENCODING
        ASSERT_EQ(component->queue(&items), C2_OK);
        ALOGV("Frame #%d size = %d queued", frameID, bytesCount);
        nFrames--;
        timestamp += timestampIncr;
        frameID++;
        maxRetry = 0;
    }
}

void waitOnInputConsumption(std::mutex &queueLock,
                            std::condition_variable &queueCondition,
                            std::list<std::unique_ptr<C2Work>> &workQueue) {
    typedef std::unique_lock<std::mutex> ULock;
    uint32_t queueSize;
    int maxRetry = 0;
    {
        ULock l(queueLock);
        queueSize = workQueue.size();
    }
    while ((maxRetry < MAX_RETRY) && (queueSize < MAX_INPUT_BUFFERS)) {
        ULock l(queueLock);
        if (queueSize != workQueue.size()) {
            queueSize = workQueue.size();
            maxRetry = 0;
        } else {
            queueCondition.wait_for(l, TIME_OUT);
            maxRetry++;
        }
    }
}

TEST_F(Codec2VideoDecHidlTest, validateCompName) {
    if (mDisableTest) return;
    ALOGV("Checks if the given component is a valid video component");
    validateComponent(mComponent, mCompName, mDisableTest);
    ASSERT_EQ(mDisableTest, false);
}

TEST_F(Codec2VideoDecHidlTest, EncodeTest) {
    description("Encodes input file");
    if (mDisableTest) return;

    char mURL[512];
    int32_t nWidth = ENC_DEFAULT_FRAME_WIDTH;
    int32_t nHeight = ENC_DEFAULT_FRAME_HEIGHT;

    strcpy(mURL, gEnv->getRes().c_str());
    GetURLForComponent(mURL);

    std::ifstream eleStream;
    eleStream.open(mURL, std::ifstream::binary);
    ASSERT_EQ(eleStream.is_open(), true) << mURL << " file not found";
    ALOGV("mURL : %s", mURL);

    ASSERT_EQ(mComponent->start(), C2_OK);
    ASSERT_NO_FATAL_FAILURE(
        encodeNFrames(mComponent, mQueueLock, mQueueCondition, mWorkQueue,
                      mGraphicAllocator, mGraphicPool, mBlockPoolId, eleStream,
                      ENC_NUM_FRAMES, nWidth, nHeight));

    // blocking call to ensures application to Wait till all the inputs are
    // consumed
    if (!mEos) {
        ALOGD("Waiting for input consumption");
        ASSERT_NO_FATAL_FAILURE(
            waitOnInputConsumption(mQueueLock, mQueueCondition, mWorkQueue));
    }

    eleStream.close();
    if (mFramesReceived != ENC_NUM_FRAMES) {
        ALOGE("Input buffer count and Output buffer count mismatch");
        ALOGE("framesReceived : %d inputFrames : %d", mFramesReceived,
              ENC_NUM_FRAMES);
        ASSERT_TRUE(false);
    }

    if (!mCsd) {
        ASSERT_TRUE(false) << "CSD Buffer not received";
    }

    ASSERT_EQ(mComponent->stop(), C2_OK);
}

}  // anonymous namespace

// TODO : Video specific configuration Test
// TODO : Test EOS
// TODO : Flush Test
// TODO : Invalid size buffer input to Encoder
// TODO : Null Input
// TODO : Encoding of odd frame sizes
int main(int argc, char** argv) {
    gEnv = new ComponentTestEnvironment();
    ::testing::AddGlobalTestEnvironment(gEnv);
    ::testing::InitGoogleTest(&argc, argv);
    gEnv->init(&argc, argv);
    int status = gEnv->initFromOptions(argc, argv);
    if (status == 0) {
        int status = RUN_ALL_TESTS();
        LOG(INFO) << "C2 Test result = " << status;
    }
    return status;
}
