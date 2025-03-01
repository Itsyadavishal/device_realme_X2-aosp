/*
 * Copyright (C) 2021 The LineageOS Project
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
#define LOG_TAG "android.hardware.biometrics.fingerprint@2.3-service.x2"

#include "BiometricsFingerprint.h"

#include <android-base/logging.h>
#include <inttypes.h>
#include <unistd.h>
#include <functional>
#include <fstream>
#include <thread>

#define FOD_STATUS_PATH "/sys/kernel/oppo_display/notify_fppress"
#define DIMLAYER_PATH "/sys/kernel/oppo_display/dimlayer_hbm"
#define STATUS_ON "1"
#define STATUS_OFF "0"
#define BIND(fn) [this](auto&&... args) -> decltype(auto) { return this->fn(std::forward<decltype(args)>(args)...); }

namespace android {
namespace hardware {
namespace biometrics {
namespace fingerprint {
namespace V2_3 {
namespace implementation {

inline static void set(const std::string& path, const std::string& data) {
    std::unique_ptr<FILE, decltype(&fclose)> stream(fopen(path.c_str(), "w"), &fclose);
    if (stream) {
        (void)write(fileno(stream.get()), data.c_str(), 1);
    }
}

BiometricsFingerprint::BiometricsFingerprint() : isEnrolling(false) {
    mOplusBiometricsFingerprint = vendor::oplus::hardware::biometrics::fingerprint::V2_1::IBiometricsFingerprint::getService();
    if (mOplusBiometricsFingerprint == nullptr) {
        LOG (ERROR) << "BiometricsFingerprint(): Oplus Biometrics hal didn't init trying again, Attempting rescue.";
        for (int i = 0; i < 10; i++) {
            mOplusBiometricsFingerprint = vendor::oplus::hardware::biometrics::fingerprint::V2_1::IBiometricsFingerprint::getService();
            if (mOplusBiometricsFingerprint != nullptr) {
                LOG (INFO) << "BiometricsFingerprint(): rescue(): Got service for oplus biometrics hal exiting loop.";
                break;
            }
            LOG (INFO) << "BiometricsFingerprint(): rescue(): Unable to get service in witin: " << i+1 << " attempts.";
            sleep(15);
        }
        if (mOplusBiometricsFingerprint == nullptr) {
            LOG(ERROR) << "BiometricsFingerprint(): oplus biometrics service didn't start fingerprint hardware won't be available.";
            exit(0);
        }
    }
}

class OplusClientCallback : public vendor::oplus::hardware::biometrics::fingerprint::V2_1::IBiometricsFingerprintClientCallback {
public:
    OplusClientCallback(sp<android::hardware::biometrics::fingerprint::V2_1::IBiometricsFingerprintClientCallback> clientCallback) : mClientCallback(clientCallback), onAuthenticationSuccess(nullptr) {}
    Return<void> onEnrollResult(uint64_t deviceId, uint32_t fingerId,
        uint32_t groupId, uint32_t remaining) {
        return mClientCallback->onEnrollResult(deviceId, fingerId, groupId, remaining);
    }

    Return<void> onAcquired(uint64_t deviceId, vendor::oplus::hardware::biometrics::fingerprint::V2_1::FingerprintAcquiredInfo acquiredInfo,
        int32_t vendorCode) {
        return mClientCallback->onAcquired(deviceId, OplusToAOSPFingerprintAcquiredInfo(acquiredInfo), vendorCode);
    }

    Return<void> onAuthenticated(uint64_t deviceId, uint32_t fingerId, uint32_t groupId,
        const hidl_vec<uint8_t>& token) {
        // FingerID = 0 means authentication failure.
        if (fingerId != 0) {
            if (onAuthenticationSuccess != nullptr) {
                onAuthenticationSuccess();
            }
        }
        return mClientCallback->onAuthenticated(deviceId, fingerId, groupId, token);
    }

    Return<void> onError(uint64_t deviceId, vendor::oplus::hardware::biometrics::fingerprint::V2_1::FingerprintError error, int32_t vendorCode) {
        return mClientCallback->onError(deviceId, OplusToAOSPFingerprintError(error), vendorCode);
    }

    Return<void> onRemoved(uint64_t deviceId, uint32_t fingerId, uint32_t groupId,
        uint32_t remaining) {
        return mClientCallback->onRemoved(deviceId, fingerId, groupId, remaining);
    }

    Return<void> onEnumerate(uint64_t deviceId, uint32_t fingerId, uint32_t groupId,
        uint32_t remaining) {
        return mClientCallback->onEnumerate(deviceId, fingerId, groupId, remaining);
    }

    Return<void> onTouchUp(uint64_t deviceId) {
        set(FOD_STATUS_PATH, STATUS_ON);
        return Void();
    }

    Return<void> onTouchDown(uint64_t deviceId) {
        set(FOD_STATUS_PATH, STATUS_OFF);
        return Void();
    }

    Return<void> onSyncTemplates(uint64_t deviceId, const hidl_vec<uint32_t>& fingerId, uint32_t remaining) { return Void(); }
    Return<void> onFingerprintCmd(int32_t deviceId, const hidl_vec<uint32_t>& groupId, uint32_t remaining) { return Void(); }
    Return<void> onImageInfoAcquired(uint32_t type, uint32_t quality, uint32_t match_score) { return Void(); }
    Return<void> onMonitorEventTriggered(uint32_t type, const hidl_string& data) { return Void(); }
    Return<void> onEngineeringInfoUpdated(uint32_t length, const hidl_vec<uint32_t>& keys, const hidl_vec<hidl_string>& values) { return Void(); }
    void setOnAuthenticationSuccess(const std::function<void()>& callback) { onAuthenticationSuccess = callback; }

private:
    sp<android::hardware::biometrics::fingerprint::V2_1::IBiometricsFingerprintClientCallback> mClientCallback;
    std::function<void()> onAuthenticationSuccess;

    Return<android::hardware::biometrics::fingerprint::V2_1::FingerprintAcquiredInfo> OplusToAOSPFingerprintAcquiredInfo(vendor::oplus::hardware::biometrics::fingerprint::V2_1::FingerprintAcquiredInfo info) {
        switch(info) {
            case vendor::oplus::hardware::biometrics::fingerprint::V2_1::FingerprintAcquiredInfo::ACQUIRED_GOOD: return android::hardware::biometrics::fingerprint::V2_1::FingerprintAcquiredInfo::ACQUIRED_GOOD;
            case vendor::oplus::hardware::biometrics::fingerprint::V2_1::FingerprintAcquiredInfo::ACQUIRED_PARTIAL: return android::hardware::biometrics::fingerprint::V2_1::FingerprintAcquiredInfo::ACQUIRED_PARTIAL;
            case vendor::oplus::hardware::biometrics::fingerprint::V2_1::FingerprintAcquiredInfo::ACQUIRED_INSUFFICIENT: return android::hardware::biometrics::fingerprint::V2_1::FingerprintAcquiredInfo::ACQUIRED_INSUFFICIENT;
            case vendor::oplus::hardware::biometrics::fingerprint::V2_1::FingerprintAcquiredInfo::ACQUIRED_IMAGER_DIRTY: return android::hardware::biometrics::fingerprint::V2_1::FingerprintAcquiredInfo::ACQUIRED_IMAGER_DIRTY;
            case vendor::oplus::hardware::biometrics::fingerprint::V2_1::FingerprintAcquiredInfo::ACQUIRED_TOO_SLOW: return android::hardware::biometrics::fingerprint::V2_1::FingerprintAcquiredInfo::ACQUIRED_TOO_SLOW;
            case vendor::oplus::hardware::biometrics::fingerprint::V2_1::FingerprintAcquiredInfo::ACQUIRED_TOO_FAST: return android::hardware::biometrics::fingerprint::V2_1::FingerprintAcquiredInfo::ACQUIRED_TOO_FAST;
            case vendor::oplus::hardware::biometrics::fingerprint::V2_1::FingerprintAcquiredInfo::ACQUIRED_VENDOR: return android::hardware::biometrics::fingerprint::V2_1::FingerprintAcquiredInfo::ACQUIRED_VENDOR;
            default:
                return android::hardware::biometrics::fingerprint::V2_1::FingerprintAcquiredInfo::ACQUIRED_GOOD;
        }
    }

    Return<android::hardware::biometrics::fingerprint::V2_1::FingerprintError> OplusToAOSPFingerprintError(vendor::oplus::hardware::biometrics::fingerprint::V2_1::FingerprintError error) {
        switch(error) {
            case vendor::oplus::hardware::biometrics::fingerprint::V2_1::FingerprintError::ERROR_NO_ERROR: return android::hardware::biometrics::fingerprint::V2_1::FingerprintError::ERROR_NO_ERROR;
            case vendor::oplus::hardware::biometrics::fingerprint::V2_1::FingerprintError::ERROR_HW_UNAVAILABLE: return android::hardware::biometrics::fingerprint::V2_1::FingerprintError::ERROR_HW_UNAVAILABLE;
            case vendor::oplus::hardware::biometrics::fingerprint::V2_1::FingerprintError::ERROR_UNABLE_TO_PROCESS: return android::hardware::biometrics::fingerprint::V2_1::FingerprintError::ERROR_UNABLE_TO_PROCESS;
            case vendor::oplus::hardware::biometrics::fingerprint::V2_1::FingerprintError::ERROR_TIMEOUT: return android::hardware::biometrics::fingerprint::V2_1::FingerprintError::ERROR_TIMEOUT;
            case vendor::oplus::hardware::biometrics::fingerprint::V2_1::FingerprintError::ERROR_NO_SPACE: return android::hardware::biometrics::fingerprint::V2_1::FingerprintError::ERROR_NO_SPACE;
            case vendor::oplus::hardware::biometrics::fingerprint::V2_1::FingerprintError::ERROR_CANCELED: return android::hardware::biometrics::fingerprint::V2_1::FingerprintError::ERROR_CANCELED;
            case vendor::oplus::hardware::biometrics::fingerprint::V2_1::FingerprintError::ERROR_UNABLE_TO_REMOVE: return android::hardware::biometrics::fingerprint::V2_1::FingerprintError::ERROR_UNABLE_TO_REMOVE;
            case vendor::oplus::hardware::biometrics::fingerprint::V2_1::FingerprintError::ERROR_LOCKOUT: return android::hardware::biometrics::fingerprint::V2_1::FingerprintError::ERROR_LOCKOUT;
            case vendor::oplus::hardware::biometrics::fingerprint::V2_1::FingerprintError::ERROR_VENDOR: return android::hardware::biometrics::fingerprint::V2_1::FingerprintError::ERROR_VENDOR;
            default:
                return android::hardware::biometrics::fingerprint::V2_1::FingerprintError::ERROR_NO_ERROR;
        }
    }
};

Return<uint64_t> BiometricsFingerprint::setNotify(const sp<IBiometricsFingerprintClientCallback>& clientCallback) {
    OplusClientCallback* oplusClientCallback = new OplusClientCallback(clientCallback);
    oplusClientCallback->setOnAuthenticationSuccess(BIND(setFingerprintScreenStateOff));
    mOplusClientCallback = oplusClientCallback;
    return mOplusBiometricsFingerprint->setNotify(mOplusClientCallback);
}

Return<RequestStatus> BiometricsFingerprint::OplusToAOSPRequestStatus(vendor::oplus::hardware::biometrics::fingerprint::V2_1::RequestStatus req) {
    switch(req) {
        case vendor::oplus::hardware::biometrics::fingerprint::V2_1::RequestStatus::SYS_UNKNOWN: return RequestStatus::SYS_UNKNOWN;
        case vendor::oplus::hardware::biometrics::fingerprint::V2_1::RequestStatus::SYS_OK: return RequestStatus::SYS_OK;
        case vendor::oplus::hardware::biometrics::fingerprint::V2_1::RequestStatus::SYS_ENOENT: return RequestStatus::SYS_ENOENT;
        case vendor::oplus::hardware::biometrics::fingerprint::V2_1::RequestStatus::SYS_EINTR: return RequestStatus::SYS_EINTR;
        case vendor::oplus::hardware::biometrics::fingerprint::V2_1::RequestStatus::SYS_EIO: return RequestStatus::SYS_EIO;
        case vendor::oplus::hardware::biometrics::fingerprint::V2_1::RequestStatus::SYS_EAGAIN: return RequestStatus::SYS_EAGAIN;
        case vendor::oplus::hardware::biometrics::fingerprint::V2_1::RequestStatus::SYS_ENOMEM: return RequestStatus::SYS_ENOMEM;
        case vendor::oplus::hardware::biometrics::fingerprint::V2_1::RequestStatus::SYS_EACCES: return RequestStatus::SYS_EACCES;
        case vendor::oplus::hardware::biometrics::fingerprint::V2_1::RequestStatus::SYS_EFAULT: return RequestStatus::SYS_EFAULT;
        case vendor::oplus::hardware::biometrics::fingerprint::V2_1::RequestStatus::SYS_EBUSY: return RequestStatus::SYS_EBUSY;
        case vendor::oplus::hardware::biometrics::fingerprint::V2_1::RequestStatus::SYS_EINVAL: return RequestStatus::SYS_EINVAL;
        case vendor::oplus::hardware::biometrics::fingerprint::V2_1::RequestStatus::SYS_ENOSPC: return RequestStatus::SYS_ENOSPC;
        case vendor::oplus::hardware::biometrics::fingerprint::V2_1::RequestStatus::SYS_ETIMEDOUT: return RequestStatus::SYS_ETIMEDOUT;
        default:
            return RequestStatus::SYS_UNKNOWN;
    }
}

void BiometricsFingerprint::setFingerprintScreenState(const bool on) {
    mOplusBiometricsFingerprint->setScreenState(
        on ? vendor::oplus::hardware::biometrics::fingerprint::V2_1::FingerprintScreenState::FINGERPRINT_SCREEN_ON :
            vendor::oplus::hardware::biometrics::fingerprint::V2_1::FingerprintScreenState::FINGERPRINT_SCREEN_OFF
        );

    set(DIMLAYER_PATH, on ? STATUS_ON: STATUS_OFF);
}

void BiometricsFingerprint::setFingerprintScreenStateOff() {
    setFingerprintScreenState(false);
}

Return<uint64_t> BiometricsFingerprint::preEnroll() {
    setFingerprintScreenState(true);
    return mOplusBiometricsFingerprint->preEnroll();
}

Return<RequestStatus> BiometricsFingerprint::enroll(const hidl_array<uint8_t, 69>& hat, uint32_t gid, uint32_t timeoutSec) {
    isEnrolling = true;
    return OplusToAOSPRequestStatus(mOplusBiometricsFingerprint->enroll(hat, gid, timeoutSec));
}

Return<RequestStatus> BiometricsFingerprint::postEnroll() {
    isEnrolling = false;
    setFingerprintScreenState(isEnrolling);
    return OplusToAOSPRequestStatus(mOplusBiometricsFingerprint->postEnroll());
}

Return<uint64_t> BiometricsFingerprint::getAuthenticatorId() {
    return mOplusBiometricsFingerprint->getAuthenticatorId();
}

Return<RequestStatus> BiometricsFingerprint::cancel() {
    if (isEnrolling)
        isEnrolling = false;
    else 
        setFingerprintScreenState(false);
    return OplusToAOSPRequestStatus(mOplusBiometricsFingerprint->cancel());
}

Return<RequestStatus> BiometricsFingerprint::enumerate() {
    return OplusToAOSPRequestStatus(mOplusBiometricsFingerprint->enumerate());
}

Return<RequestStatus> BiometricsFingerprint::remove(uint32_t gid, uint32_t fid) {
    return OplusToAOSPRequestStatus(mOplusBiometricsFingerprint->remove(gid, fid));
}

Return<RequestStatus> BiometricsFingerprint::setActiveGroup(uint32_t gid, const hidl_string& storePath) {
    return OplusToAOSPRequestStatus(mOplusBiometricsFingerprint->setActiveGroup(gid, storePath));
}

Return<RequestStatus> BiometricsFingerprint::authenticate(uint64_t operationId, uint32_t gid) {
    RequestStatus status = OplusToAOSPRequestStatus(mOplusBiometricsFingerprint->authenticate(operationId, gid));
    if (status == RequestStatus::SYS_OK) {
        setFingerprintScreenState(true);
    }
    return OplusToAOSPRequestStatus(mOplusBiometricsFingerprint->authenticate(operationId, gid));
}

Return<bool> BiometricsFingerprint::isUdfps(uint32_t) { return true; }

Return<void> BiometricsFingerprint::onFingerDown(uint32_t, uint32_t, float, float) { return Void(); }

Return<void> BiometricsFingerprint::onFingerUp() { return Void(); }

Return<void> BiometricsFingerprint::onHideUdfpsOverlay() { return Void(); }

Return<void> BiometricsFingerprint::onShowUdfpsOverlay() { return Void(); }

}  // namespace implementation
}  // namespace V2_3
}  // namespace fingerprint
}  // namespace biometrics
}  // namespace hardware
}  // namespace android
