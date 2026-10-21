TARGET := iphone:clang:16.5:15.0
ARCHS := arm64 arm64e

include $(THEOS)/makefiles/common.mk

TWEAK_NAME := IslandDigest
IslandDigest_FILES := Tweak.xm
IslandDigest_CFLAGS := -fobjc-arc
IslandDigest_FRAMEWORKS := UIKit QuartzCore
IslandDigest_LOGOS_DEFAULT_GENERATOR := internal
# Xcode 15's new linker ignores -no_fixup_chains for arm64e; use the
# classic linker to preserve the audited opcode-based fixups on iOS 15+.
IslandDigest_LDFLAGS := -Wl,-ld_classic -Wl,-no_fixup_chains

include $(THEOS_MAKE_PATH)/tweak.mk

SUBPROJECTS += prefs
include $(THEOS_MAKE_PATH)/aggregate.mk
