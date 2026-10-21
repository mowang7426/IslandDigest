TARGET := iphone:clang:16.5:15.0
ARCHS := arm64 arm64e

include $(THEOS)/makefiles/common.mk

TWEAK_NAME := IslandDigest
IslandDigest_FILES := Tweak.xm
IslandDigest_CFLAGS := -fobjc-arc
IslandDigest_FRAMEWORKS := UIKit QuartzCore
IslandDigest_LOGOS_DEFAULT_GENERATOR := internal
IslandDigest_LDFLAGS := -Wl,-no_fixup_chains

include $(THEOS_MAKE_PATH)/tweak.mk
