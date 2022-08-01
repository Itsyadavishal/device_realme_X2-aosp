#
# Copyright (C) 2019 The CherishOS Project
#
# SPDX-License-Identifer: Apache-2.0
#

# Inherit some common Cherish stuff.
$(call inherit-product, vendor/cherish/config/common_full_phone.mk)

# Inherit from X2 device.
$(call inherit-product, device/realme/X2/device.mk)

TARGET_BOOT_ANIMATION_RES := 1080

# Device identifier. This must come after all inclusions.
PRODUCT_DEVICE := X2
PRODUCT_NAME := cherish_X2
PRODUCT_BRAND := Realme
PRODUCT_MODEL := Realme X2
PRODUCT_MANUFACTURER := Realme
TARGET_SUPPORTS_QUICK_TAP := true
TARGET_INCLUDE_PIXEL_CHARGER := true
TARGET_FACE_UNLOCK_SUPPORTED := true

# FOD animations
EXTRA_UDFPS_ANIMATIONS := true

# GMS
TARGET_GAPPS_ARCH := arm64
USE_GAPPS := true

PRODUCT_BUILD_PROP_OVERRIDES += \
BUILD_FINGERPRINT="google/raven/raven:12/SQ3A.220605.009.B1/8650216:user/release-keys"

PRODUCT_GMS_CLIENTID_BASE := android-oppo

# Cherish Official
CHERISH_BUILD_TYPE := UNOFFICIAL
PRODUCT_SYSTEM_DEFAULT_PROPERTIES += \
    ro.cherish.maintainer=VISHAL
