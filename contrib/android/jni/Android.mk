LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE := swe-2.10.03

LOCAL_LDFLAGS   += -ffunction-sections -fdata-sections -Wl,--gc-sections
LOCAL_CFLAGS    += -ffunction-sections -fdata-sections -fvisibility=hidden -Wall -Wno-error=implicit-function-declaration
# sweconfig.c was added by the thread-safe branch and the library now
# requires it -- swi_config_publish(), swi_default_ctx() and the rest.
# Without it this module fails to link, exactly as the MSVC projects did.
LOCAL_SRC_FILES := sweconfig.c swedate.c swehouse.c swejpl.c swemmoon.c swemplan.c sweph.c swephlib.c swecl.c swehel.c swejni.c
include $(BUILD_SHARED_LIBRARY)