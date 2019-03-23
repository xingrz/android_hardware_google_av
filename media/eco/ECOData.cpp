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
#define LOG_TAG "ECOData"

#include <utils/Errors.h>
#include <utils/Log.h>
#include <string>

#include <binder/Parcel.h>

#include "eco/ECOData.h"
#include "eco/ECODataKey.h"

namespace android {
namespace media {
namespace eco {

using namespace ::android;

status_t ECOData::readFromParcel(const Parcel* parcel) {
    parcel->readInt32(&mDataType);
    parcel->readInt64(&mDataTimeUs);
    // TODO(hkuang): Add the implenmentation of reading all keys.
    return NO_ERROR;
}

status_t ECOData::writeToParcel(Parcel* parcel) const {
    parcel->writeInt32(mDataType);
    parcel->writeInt64(mDataTimeUs);
    // TODO(hkuang): Add the implenmentation of writing all keys.
    return NO_ERROR;
}

void ECOData::init() {
    mKeyValueStore[ECO_DATA_KEY_TYPE] = mDataType;
    mKeyValueStore[ECO_DATA_KEY_TIME_US] = mDataTimeUs;
}

int32_t ECOData::getDataType() {
    return mDataType;
}

int64_t ECOData::getDataTimeUs() {
    return mDataTimeUs;
}

// Inserts a new key into store if the key does not exist yet. Otherwise, this will override the
// existing key's value.
ECODataStatus ECOData::setString(const std::string& key, const std::string& value) {
    if (key.empty() || value.empty()) {
        return ECODataStatus::INVALID_ARGUMENT;
    }

    mKeyValueStore[key] = value;

    // TODO(hkuang): Check the valueType is valid for the key.
    return ECODataStatus::OK;
}

ECODataStatus ECOData::findString(const std::string& key, std::string* value) const {
    if (key.empty()) {
        return ECODataStatus::INVALID_ARGUMENT;
    }

    // Check if the key exists.
    if (mKeyValueStore.find(key) == mKeyValueStore.end()) {
        return ECODataStatus::KEY_NOT_EXIST;
    }

    // Safely access the value.
    const std::string& entryValue = std::get<std::string>(mKeyValueStore.at(key));
    value->assign(entryValue);

    return ECODataStatus::OK;
}

// Inserts a new key into store if the key does not exist yet. Otherwise, this will override the
// existing key's value.
template <typename T>
ECODataStatus ECOData::setValue(const std::string& key, T value) {
    if (key.empty()) {
        return ECODataStatus::INVALID_ARGUMENT;
    }

    mKeyValueStore[key] = value;
    return ECODataStatus::OK;
}

template <typename T>
ECODataStatus ECOData::findValue(const std::string& key, T* out) const {
    if (key.empty() || out == nullptr) {
        return ECODataStatus::INVALID_ARGUMENT;
    }

    if (mKeyValueStore.find(key) == mKeyValueStore.end()) {
        return ECODataStatus::KEY_NOT_EXIST;
    }

    // Safely access the value.
    *out = std::get<T>(mKeyValueStore.at(key));

    return ECODataStatus::OK;
}

ECODataStatus ECOData::setInt32(const std::string& key, int32_t value) {
    return setValue<int32_t>(key, value);
}

ECODataStatus ECOData::findInt32(const std::string& key, int32_t* out) const {
    return findValue<int32_t>(key, out);
}

ECODataStatus ECOData::setInt64(const std::string& key, int64_t value) {
    return setValue<int64_t>(key, value);
}

ECODataStatus ECOData::findInt64(const std::string& key, int64_t* out) const {
    return findValue<int64_t>(key, out);
}

ECODataStatus ECOData::setDouble(const std::string& key, double value) {
    return setValue<double>(key, value);
}

ECODataStatus ECOData::findDouble(const std::string& key, double* out) const {
    return findValue<double>(key, out);
}

ECODataStatus ECOData::setSize(const std::string& key, size_t value) {
    return setValue<size_t>(key, value);
}

ECODataStatus ECOData::findSize(const std::string& key, size_t* out) const {
    return findValue<size_t>(key, out);
}

ECODataStatus ECOData::setFloat(const std::string& key, float value) {
    return setValue<float>(key, value);
}

ECODataStatus ECOData::findFloat(const std::string& key, float* out) const {
    return findValue<float>(key, out);
}

ECODataStatus ECOData::set(const std::string& key, const ECOData::ECODataValueType& value) {
    if (key.empty()) {
        return ECODataStatus::INVALID_ARGUMENT;
    }
    mKeyValueStore[key] = value;
    return ECODataStatus::OK;
}

ECODataStatus ECOData::find(const std::string& key, ECOData::ECODataValueType* out) const {
    if (key.empty() || out == nullptr) {
        return ECODataStatus::INVALID_ARGUMENT;
    }

    if (mKeyValueStore.find(key) == mKeyValueStore.end()) {
        return ECODataStatus::KEY_NOT_EXIST;
    }

    // Safely access the value.
    *out = mKeyValueStore.at(key);

    return ECODataStatus::OK;
}

}  // namespace eco
}  // namespace media
}  // namespace android