TARGET := iphone:clang:16.5:15.0
ARCHS := arm64 arm64e
include $(THEOS)/makefiles/common.mk
BUNDLE_NAME := IslandDigestPrefs
IslandDigestPrefs_FILES := IDPrefs.m
IslandDigestPrefs_CFLAGS := -fobjc-arc
IslandDigestPrefs_FRAMEWORKS := UIKit
IslandDigestPrefs_PRIVATE_FRAMEWORKS := Preferences
IslandDigestPrefs_INSTALL_PATH := /Library/PreferenceBundles
IslandDigestPrefs_LDFLAGS := -Wl,-ld_classic -Wl,-no_fixup_chains
include $(THEOS_MAKE_PATH)/bundle.mk
