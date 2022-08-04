/*
   Copyright (c) 2015, The Linux Foundation. All rights reserved.
   Copyright (C) 2016 The CyanogenMod Project.
   Copyright (C) 2019-2020 The LineageOS Project.
   Copyright (C) 2021 ArrowOS.
   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions are
   met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above
      copyright notice, this list of conditions and the following
      disclaimer in the documentation and/or other materials provided
      with the distribution.
    * Neither the name of The Linux Foundation nor the names of its
      contributors may be used to endorse or promote products derived
      from this software without specific prior written permission.
   THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
   WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
   MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
   ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
   BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
   CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
   SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
   BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
   WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
   OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
   IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <cstdlib>
#include <string.h>
#include <fstream>

#define _REALLY_INCLUDE_SYS__SYSTEM_PROPERTIES_H_
#include <sys/sysinfo.h>
#include <sys/_system_properties.h>
#include <android-base/properties.h>

#include "property_service.h"
#include "vendor_init.h"

using android::base::SetProperty;
using android::base::Trim;
using std::string;
static const char *snet_prop_key[] = {
    "ro.boot.vbmeta.device_state",
    "ro.boot.verifiedbootstate",
    "ro.boot.flash.locked",
    "ro.boot.selinux",
    "ro.boot.veritymode",
    "ro.boot.warranty_bit",
    "ro.warranty_bit",
    "ro.debuggable",
    "ro.secure",
    "ro.build.type",
    "ro.build.tags",
    "ro.build.selinux",
    NULL
};
static const char *snet_prop_value[] = {
    "locked",
    "green",
    "1",
    "enforcing",
    "enforcing",
    "0",
    "0",
    "0",
    "1",
    "user",
    "release-keys",
    "0",
    NULL
};

void property_override(char const prop[], char const value[]) {
    auto pi = (prop_info*) __system_property_find(prop);

    if (pi != nullptr)
        __system_property_update(pi, value, strlen(value));
    else
        __system_property_add(prop, strlen(prop), value, strlen(value));
}

static void workaround_snet_properties() {
    for (int i = 0; snet_prop_key[i]; ++i)
        property_override(snet_prop_key[i], snet_prop_value[i]);
}

void vendor_load_properties()
}

workaround_snet_properties();
void set_ro_build_prop(const string &source, const string &prop,
                        const string &value, bool product = true) {
    string prop_name;

    if (product)
        prop_name = "ro.product." + source + prop;
    else
        prop_name = "ro." + source + "build." + prop;

    property_override(prop_name.c_str(), value.c_str());
}

void load_dalvikvm_properties() {
  struct sysinfo sys;
  sysinfo(&sys);
  if (sys.totalram > 6144ull * 1024 * 1024) {
    // from - phone-xhdpi-8192-dalvik-heap.mk
    property_override("dalvik.vm.heapstartsize", "24m");
    property_override("dalvik.vm.heapgrowthlimit", "256m");
    property_override("dalvik.vm.heaptargetutilization", "0.46");
    property_override("dalvik.vm.heapmaxfree", "48m");
    }
  else if (sys.totalram > 4096ull * 1024 * 1024) {
    // from - phone-xhdpi-6144-dalvik-heap.mk
    property_override("dalvik.vm.heapstartsize", "16m");
    property_override("dalvik.vm.heapgrowthlimit", "256m");
    property_override("dalvik.vm.heaptargetutilization", "0.5");
    property_override("dalvik.vm.heapmaxfree", "32m");
    }
  else {
    // from - phone-xhdpi-4096-dalvik-heap.mk
    property_override("dalvik.vm.heapstartsize", "8m");
    property_override("dalvik.vm.heapgrowthlimit", "192m");
    property_override("dalvik.vm.heaptargetutilization", "0.6");
    property_override("dalvik.vm.heapmaxfree", "16m");
  }
  property_override("dalvik.vm.heapsize", "512m");
  property_override("dalvik.vm.heapminfree", "8m");
}

void vendor_load_properties() {
    /*
     * Detect device and configure properties
     */
     workaround_snet_properties();
    std::string model, device, operatorname;

    if (ReadFileToString(operatorname_file, &operatorname)) {
        /* CHINA */
        if ((Trim(operatorname) == "8")) {
        device = "RMX1991CN";
           property_override("ro.build.fingerprint", "google/raven/raven:12/SQ3A.220705.003.A1/8672226:user/release-keys");
           property_override("ro.build.product", "RMX1991CN");
           property_override("ro.boot.verifiedbootstate", "green");
           for (const string &source : source_partitions) {
               set_ro_product_prop(source, "device", "RMX1991CN");
               set_ro_product_prop(source, "model", "RMX1991");
           }

        /* FOREIGN */
        } else if ((Trim(operatorname) == "30")) {
        device = "RMX1993L1";
           property_override("ro.build.fingerprint", "google/raven/raven:12/SQ3A.220705.003.A1/8672226:user/release-keys");
           property_override("ro.build.product", "RMX1993L1");
           property_override("ro.boot.verifiedbootstate", "green");
           for (const string &source : source_partitions) {
               set_ro_product_prop(source, "device", "RMX1993L1");
               set_ro_product_prop(source, "model", "RMX1993");
           }
           SetProperty("ro.com.google.clientidbase.cr", "android-oppo");
           SetProperty("ro.com.google.clientidbase.ms", "android-oppo");
        /* FOREIGN INDIA */
        } else if ((Trim(operatorname) == "31")) {
        device = "RMX1992L1";
           property_override("ro.build.fingerprint", "google/raven/raven:12/SQ3A.220705.003.A1/8672226:user/release-keys");
           property_override("ro.build.product", "RMX1992L1");
           property_override("ro.boot.verifiedbootstate", "green");
           for (const string &source : source_partitions) {
               set_ro_product_prop(source, "device", "RMX1992L1");
               set_ro_product_prop(source, "model", "RMX1992");
           }
           SetProperty("ro.com.google.clientidbase.cr", "android-oppo");
           SetProperty("ro.com.google.clientidbase.ms", "android-oppo");
    }
           SetProperty("ro.separate.soft", "18633");
           SetProperty("ro.com.google.clientidbase.cr", "android-oppo");
           SetProperty("ro.com.google.clientidbase.ms", "android-oppo");
        } else {
    property_override("ro.apex.updatable", "false");
    }

    // list of partitions to override props
    string source_partitions[] = { "", "bootimage.", "odm.", "product.",
                                   "system.", "system_ext.", "vendor." };

    for (const string &source : source_partitions) {
        set_ro_build_prop(source, "device", device);
        set_ro_build_prop(source, "product", model, false);
        set_ro_build_prop(source, "model", model);
        set_ro_build_prop(source, "name", model);
    }

                    {
    // dalvikvm props
    load_dalvikvm_properties();
}
