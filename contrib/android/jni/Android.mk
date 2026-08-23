LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

# Produces libswe-$(SWE_VERSION).so, and Java loads it by that name:
#   System.loadLibrary("swe-2.10.03-ts.N")
# Version-stamped so a consumer cannot silently link a build that
# predates the thread-safety work.
#
# Read from SE_VERSION rather than repeated here. This line used to carry
# its own copy, so `make bump` in the root would move swe_version() while
# the .so kept the old stamp -- two versions in one release, and the one
# users type into System.loadLibrary() would be the stale one.
SWE_VERSION := $(shell sed -n 's/^\#define SE_VERSION[[:space:]]*"\([^"]*\)".*/\1/p' $(LOCAL_PATH)/sweph.h)
ifeq ($(SWE_VERSION),)
  $(error could not read SE_VERSION from $(LOCAL_PATH)/sweph.h)
endif
LOCAL_MODULE := swe-$(SWE_VERSION)

LOCAL_LDFLAGS   += -ffunction-sections -fdata-sections -Wl,--gc-sections
LOCAL_CFLAGS    += -ffunction-sections -fdata-sections -fvisibility=hidden -Wall -Wno-error=implicit-function-declaration
# sweconfig.c was added by the thread-safe branch and the library now
# requires it -- swi_config_publish(), swi_default_ctx() and the rest.
# Without it this module fails to link, exactly as the MSVC projects did.
LOCAL_SRC_FILES := sweconfig.c swedate.c swehouse.c swejpl.c swemmoon.c swemplan.c sweph.c swephlib.c swecl.c swehel.c swejni.c
include $(BUILD_SHARED_LIBRARY)