########################################################################### ###
#@Title         Root makefile for omap4430 Android.  Builds everything else.
#@Copyright     Copyright (c) Imagination Technologies Ltd. All Rights Reserved
#@License       Dual MIT/GPLv2
# 
# The contents of this file are subject to the MIT license as set out below.
# 
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
# 
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
# 
# Alternatively, the contents of this file may be used under the terms of
# the GNU General Public License Version 2 ("GPL") in which case the provisions
# of GPL are applicable instead of those above.
# 
# If you wish to allow use of your version of this file only under the terms of
# GPL, and not to allow others to use your version of this file under the terms
# of the MIT license, indicate your decision by deleting the provisions above
# and replace them with the notice and other provisions required by GPL as set
# out in the file called "GPL-COPYING" included in this distribution. If you do
# not delete the provisions above, a recipient may use your version of this file
# under the terms of either the MIT license or GPL.
# 
# This License is also included in this distribution in the file called
# "MIT-COPYING".
# 
# EXCEPT AS OTHERWISE STATED IN A NEGOTIATED AGREEMENT: (A) THE SOFTWARE IS
# PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
# BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
# PURPOSE AND NONINFRINGEMENT; AND (B) IN NO EVENT SHALL THE AUTHORS OR
# COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
# IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
# CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
### ###########################################################################
TARGET_PRODUCT := fiber-w02
ANDROID_ROOT = ../../../../../../../../android
DISCIMAGE =$(ANDROID_ROOT)/out/target/product/$(TARGET_PRODUCT)
LICHEE_KDIR = ../../../../../../../../lichee
KERNELDIR = $(LICHEE_KDIR)/linux-3.3/
TOOLCHAIN = $(LICHEE_KDIR)/buildroot/output/external-toolchain/
CROSS_COMPILE := arm-linux-gnueabi-
KERNEL_CROSS_COMPILE :=arm-linux-gnueabi-
ARCH=arm

KERNELDIR = $(LICHEE_KDIR)
TOP = $(KERNELDIR)/modules/eurasia_km/

SGXCORE := 544
SGX_CORE_REV := 115
SGX_FEATURE_MP :=1
SGX_FEATURE_MP_CORE_COUNT :=2
HAL_VARIANT := sun6i
PVR_SYSTEM := sunxi
DISPLAY_CONTROLLER := dc_sunxi
LINUX :=1
__linux__ :=1

BUILD := release
SUPPORT_MEMINFO_IDS :=0
PVRSRV_DUMP_MK_TRACE :=1
PVRSRV_USSE_EDM_STATUS_DEBUG := 1
PVRSRV_NEED_PVR_DPF =: 1
PVRSRV_NEW_PVR_DPF =: 1
#PVR_ANDROID_HAL_FENCE_DEBUG =: 1

PVR_NO_OMAP_TIMER :=1
SYS_OMAP4_HAS_DVFS_FRAMEWORK :=0
SUPPORT_SLC :=1
LDM_PLATFORM :=1
SGX_DYNAMIC_TIMING_INFO := 0
SUPPORT_LINUX_USING_WORKQUEUES := 1
SUPPORT_ION := 1
SUPPORT_ACTIVE_POWER_MANAGEMENT :=1
SUPPORT_SGX_LOW_LATENCY_SCHEDULING :=1
##################

# We have more memory on OMAP platforms, so we can spare to make the
# pool larger, and have higher resolutions which benefit from it.
#
PVR_LINUX_MEM_AREA_POOL_MAX_PAGES ?= 10800

include ../common/android/paths.mk
include ../common/android/armv7-a.mk
include ../common/android/features.mk

ifneq ($(strip $(KERNELDIR)),)
 include ../kernel_version.mk
 ifeq ($(call kernel-version-at-least,2,6,35),true)
  PVR_NO_OMAP_TIMER := 1
 endif
 ifeq ($(call kernel-version-at-least,2,6,39),true)
  ifeq ($(LDM_PLATFORM),1)
   PVR_LDM_PLATFORM_PRE_REGISTERED := 0
   PVR_LDM_PLATFORM_PRE_REGISTERED_DEV := "\"pvrsrvkm\""
  endif
 endif
 ifeq ($(is_at_least_icecream_sandwich),1)
  ifeq ($(call kernel-version-at-least,3,0),true)
   SUPPORT_DC_CMDCOMPLETE_WHEN_NO_LONGER_DISPLAYED := 1
   SYS_OMAP_HAS_DVFS_FRAMEWORK := 1
  endif
 endif
else
 ifeq ($(is_at_least_icecream_sandwich),1)
  $(warning "KERNELDIR is not set, so can't feature check DVFS or dsscomp.")
  $(warning "Assuming we want DVFS and dsscomp support.")
  SUPPORT_DC_CMDCOMPLETE_WHEN_NO_LONGER_DISPLAYED := 1
  SYS_OMAP_HAS_DVFS_FRAMEWORK := 1
 endif
endif

ifneq ($(LDM_PLATFORM),1)
SUPPORT_LINUX_USING_WORKQUEUES := 0
SUPPORT_LINUX_USING_SHARED_WORKQUEUES := 1
SUPPORT_ACTIVE_POWER_MANAGEMENT := 0
ifneq ($(SUPPORT_PVRSRV_DEVICE_CLASS),0)
DISPLAY_CONTROLLER := pvrlfb
DISPLAY_CONTROLLER_COMPONENT := linux_framebuffer
OMAP_NON_FLIP_DISPLAY := 1
endif
else # LDM_PLATFORM != 1
ifneq ($(SUPPORT_PVRSRV_DEVICE_CLASS),0)
DISPLAY_CONTROLLER_COMPONENT := dc_sunxi
endif
endif # LDM_PLATFORM != 1

ifeq ($(SUPPORT_DRI_DRM),1)
ifeq ($(PVR_LDM_PLATFORM_PRE_REGISTERED),1)
PVR_DRI_DRM_PLATFORM_DEV := 1
PVR_DRI_DRM_STATIC_BUS_ID := 1
PVR_DRI_DRM_DEV_BUS_ID := "\"platform:pvrsrvkm"\"
else
PVR_DRI_DRM_NOT_PCI := 1
KERNEL_COMPONENTS += linux_drm
endif
ifneq ($(SUPPORT_PVRSRV_DEVICE_CLASS),0)
EXTRA_PVRSRVKM_COMPONENTS += $(DISPLAY_CONTROLLER_COMPONENT)
endif
EXTRA_KBUILD_SOURCE := $(KERNELDIR)
# FIXME: Only required for comparison with X's KM
PVR_SECURE_DRM_AUTH_EXPORT := 1
ifneq ($(OMAP_NON_FLIP_DISPLAY),1)
PVR_DISPLAY_CONTROLLER_DRM_IOCTL := 1
endif
else # SUPPORT_DRI_DRM == 1
ifneq ($(SUPPORT_PVRSRV_DEVICE_CLASS),0)
KERNEL_COMPONENTS += $(DISPLAY_CONTROLLER_COMPONENT)
endif
endif # SUPPORT_DRI_DRM == 1

SUPPORT_ANDROID_OMAP_NV12 := 1

PVR_ANDROID_USE_WINDOW_TRANSFORM_HINT := 1

PVR_ANDROID_PLATFORM_HAS_LINUX_FBDEV := 1

# Handle Google's OMAP-based products
#
ifneq ($(filter tuna maguro toro mysid yakju,$(TARGET_DEVICE)),)
# These default on in tuna_defconfig
PVRSRV_USSE_EDM_STATUS_DEBUG ?= 1
PVRSRV_DUMP_MK_TRACE ?= 1
# Go back to the old compiler for tuna kernel modules
KERNEL_CROSS_COMPILE := arm-eabi-
endif

ifeq ($(NO_HARDWARE),1)
ifeq ($(SUPPORT_DC_CMDCOMPLETE_WHEN_NO_LONGER_DISPLAYED),1)
$(info WARNING: SUPPORT_DC_CMDCOMPLETE_WHEN_NO_LONGER_DISPLAYED=1 is incompatible with NO_HARDWARE=1)
$(info WARNING: Setting SUPPORT_DC_CMDCOMPLETE_WHEN_NO_LONGER_DISPLAYED=0 and switching to dc_nohw)
override SUPPORT_DC_CMDCOMPLETE_WHEN_NO_LONGER_DISPLAYED := 0
ifneq ($(SUPPORT_PVRSRV_DEVICE_CLASS),0)
override DISPLAY_CONTROLLER := dcnohw
KERNEL_COMPONENTS += dc_nohw
endif
endif # SUPPORT_DC_CMDCOMPLETE_WHEN_NO_LONGER_DISPLAYED == 1
endif # NO_HARDWARE == 1

FORCE_ENABLE_GROW_SHRINK ?= 1

include ../config/core.mk
include ../common/android/extra_config.mk
include ../common/dridrm.mk
include ../common/opencl.mk
include ../common/omap4.mk

# Not all OMAP kernels have a compatible DVFS framework
#
$(eval $(call TunableKernelConfigC,SYS_OMAP_HAS_DVFS_FRAMEWORK,))

# If set, services allows two flips to enter the processing queue,
# and does not add read dependencies to the set of buffers last
# flipped to. This is necessary for DSS composition on OMAP.
# 
$(eval $(call TunableKernelConfigC,SUPPORT_DC_CMDCOMPLETE_WHEN_NO_LONGER_DISPLAYED,))

.PHONY: clean

clean: clobber
	@echo "clean pvr module..........."

#install: build
#	cp ../../../binary2_sunxi_android_debug/target/pvrsrvkm.ko $(LICHEE_MOD_DIR)/ \
#	cp ../../../binary2_sunxi_android_debug/target/dc_sunxi.ko $(LICHEE_MOD_DIR)/
