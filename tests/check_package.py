import hashlib
import io
from pathlib import Path
import struct
import sys
import tarfile


def check(package, expected_version="0.1.3", scheme="roothide"):
    assert scheme in ("roothide", "rootless"), scheme
    package_arch = "iphoneos-arm64e" if scheme == "roothide" else "iphoneos-arm64"
    prefix = "" if scheme == "roothide" else "var/jb/"
    expected_install_name = ("@loader_path/.jbroot/Library/MobileSubstrate/DynamicLibraries/IslandDigest.dylib"
                             if scheme == "roothide" else "@rpath/IslandDigest.dylib")
    archive = package.read_bytes()
    assert archive[:8] == b"!<arch>\n", "Not a Debian ar archive"
    position = 8
    members = {}
    while position < len(archive):
        header = archive[position:position + 60]
        size = int(header[48:58])
        name = header[:16].decode().strip().rstrip("/")
        members[name] = archive[position + 60:position + 60 + size]
        position += 60 + size + size % 2
    control_name = next(name for name in members if name.startswith("control.tar"))
    with tarfile.open(fileobj=io.BytesIO(members[control_name])) as control:
        metadata_file = next(member for member in control.getmembers()
                             if member.name.split("/")[-1] == "control")
        metadata = control.extractfile(metadata_file).read().decode()
        for expected in (f"Architecture: {package_arch}", f"Version: {expected_version}",
                         "firmware (>= 15.0)", "firmware (<< 18.0)"):
            assert expected in metadata, expected
        assert all(member.name.split("/")[-1] not in
                   ("preinst", "postinst", "prerm", "postrm")
                   for member in control.getmembers())
    data_name = next(name for name in members if name.startswith("data.tar"))
    with tarfile.open(fileobj=io.BytesIO(members[data_name])) as payload:
        files = [member for member in payload.getmembers() if member.isfile()]
        paths = {member.name.lstrip("./"): member for member in files}
        tweak_path = prefix + "Library/MobileSubstrate/DynamicLibraries/IslandDigest.dylib"
        filter_path = prefix + "Library/MobileSubstrate/DynamicLibraries/IslandDigest.plist"
        prefs_base = prefix + "Library/PreferenceBundles/IslandDigestPrefs.bundle/"
        prefs_path = prefs_base + "IslandDigestPrefs"
        info_path = prefs_base + "Info.plist"
        entry_path = prefix + "Library/PreferenceLoader/Preferences/IslandDigest.plist"
        entry_icon = prefix + "Library/PreferenceLoader/Preferences/IslandDigest.png"
        icon_master = prefs_base + ("IslandDigest-master.png" if expected_version in ("0.3.1", "0.3.2", "0.3.3", "0.3.4") else "IslandDigest-icon.png")
        icon_2x = prefs_base + ("IslandDigest@2x.png" if expected_version in ("0.3.1", "0.3.2", "0.3.3", "0.3.4") else "IslandDigest-icon@2x.png")
        icon_3x = prefs_base + ("IslandDigest@3x.png" if expected_version in ("0.3.1", "0.3.2", "0.3.3", "0.3.4") else "IslandDigest-icon@3x.png")
        icon_bundle = prefs_base + "IslandDigest.png"
        required = {tweak_path, filter_path, prefs_path, info_path, entry_path, entry_icon,
                    icon_master, icon_2x, icon_3x, icon_bundle}
        entry_2x = prefix + "Library/PreferenceLoader/Preferences/IslandDigest@2x.png"
        entry_3x = prefix + "Library/PreferenceLoader/Preferences/IslandDigest@3x.png"
        if expected_version in ("0.3.1", "0.3.2", "0.3.3", "0.3.4"):
            required.update((entry_2x, entry_3x))
        assert required <= paths.keys(), paths.keys()
        assert all(path in required or (path.startswith(prefs_base) and
                   path.endswith(("_CodeSignature/CodeResources",))) for path in paths), paths.keys()
        assert "preferenceloader" in metadata
        injection_filter = payload.extractfile(paths[filter_path]).read()
        import plistlib
        assert plistlib.loads(injection_filter)["Filter"]["Bundles"] == ["com.apple.springboard"]
        info = plistlib.loads(payload.extractfile(paths[info_path]).read())
        assert info["NSPrincipalClass"] == "IDPrefsController"
        assert info["CFBundleVersion"] == expected_version
        entry = plistlib.loads(payload.extractfile(paths[entry_path]).read())["entry"]
        assert entry["bundle"] == "IslandDigestPrefs" and entry["detail"] == "IDPrefsController"
        binary = payload.extractfile(paths[tweak_path]).read()
        prefs_binary = payload.extractfile(paths[prefs_path]).read()
        if expected_version in ("0.3.1", "0.3.2", "0.3.3", "0.3.4"):
            assert entry["icon"] == "IslandDigest.png" and entry["cell"] == "PSLinkCell"
            assert set(entry) == {"bundle", "cell", "detail", "label", "icon", "isController"}
            assert info["CFBundleIconFiles"] == ["IslandDigest"]
            icon_sizes = ((entry_icon,29), (entry_2x,58), (entry_3x,87),
                          (icon_bundle,29), (icon_2x,58), (icon_3x,87), (icon_master,1024))
        else:
            icon_sizes = ((entry_icon,1024), (icon_master,1024), (icon_2x,120), (icon_3x,180), (icon_bundle,1024))
        for icon_path, size in icon_sizes:
            icon = payload.extractfile(paths[icon_path]).read()
            assert icon[:8] == b'\x89PNG\r\n\x1a\n'
            assert struct.unpack('>II', icon[16:24]) == (size, size), (icon_path, size)
            assert len(icon) > 1000, icon_path
    if expected_version in ("0.1.8", "0.1.9", "0.2.0", "0.2.1", "0.2.2", "0.2.3", "0.2.5", "0.2.6", "0.2.7", "0.2.8", "0.2.9", "0.3.0", "0.3.1", "0.3.2", "0.3.3", "0.3.4"):
        for key in (b"ContainedMode", b"HideNativeList", b"containedEnabled", b"hideNativeRequested",
                    b"nativeHideSupported", b"nativeHidden", b"nativeHideReason", b"previewPolicyReason",
                    b"cardsRendered", b"cardsRedacted"):
            assert key in binary and key in prefs_binary, ("missing contained contract", key)
        assert b"contentPreviewSetting" in binary
        assert b"AllowPreviewWhenUnlocked" in binary
        assert "Author: MoWang" in metadata
        assert f"IslandDigest-{expected_version}.startup-pending".encode() in binary
    if expected_version in ("0.1.9", "0.2.0", "0.2.1", "0.2.2", "0.2.3", "0.2.5", "0.2.6", "0.2.7", "0.2.8", "0.2.9", "0.3.0", "0.3.1", "0.3.2", "0.3.3", "0.3.4"):
        for key in (b"PanelTopOffset", b"PanelSideInset", b"PanelMaxHeight", b"cardsLayoutReady"):
            assert key in binary and key in prefs_binary, ("missing layout contract", key)
        for selector in (b"cardsReadyForNativeHide", b"setContentInsetAdjustmentBehavior:", b"builtCardCount"):
            assert selector in binary, ("missing readiness contract", selector)
    if expected_version in ("0.3.3", "0.3.4"):
        for key in (b"GradientText", b"TextPalette", b"SolidText", b"TextShadow", b"PerformanceDiagnostics"):
            assert key in binary and key in prefs_binary, ("missing 0.3.3 contract", key)
        assert b"IDNotificationLabel" in binary
    if expected_version == "0.3.4":
        for key in (b"cyanBluePurplePink", b"yellowOrangePinkPurple", b"whiteCyanBluePurple",
                    b"firstFrameLifecycleSupported", b"firstFramePhase", b"firstRenderMs", b"firstHideMs"):
            assert key in binary and key in prefs_binary, ("missing 0.3.4 contract", key)
        assert b"viewIsAppearing:" in binary and b"prepareFirstFrame" in binary
    check_binary(binary, expected_install_name, scheme, "tweak")
    # Theos bundle.mk deliberately emits MH_DYLIB with its full bundle executable
    # install name (unlike tweak.mk's @loader_path/@rpath identity).
    check_binary(prefs_binary, "/" + prefs_path, scheme, "preferences")
    print("PASS metadata, exact payload whitelist, PreferenceLoader registration, SpringBoard-only filter")
    print("SHA-256", hashlib.sha256(archive).hexdigest())


def check_binary(binary, expected_install_name, scheme, label):
    assert struct.unpack_from(">II", binary) == (0xCAFEBABE, 2)
    architectures = set()
    for index in range(2):
        cpu, subtype, offset, size, alignment = struct.unpack_from(">IIIII", binary, 8 + 20 * index)
        assert cpu == 0x100000C
        architectures.add(subtype & 0xFFFFFF)
        image = binary[offset:offset + size]
        assert struct.unpack_from("<I", image)[0] == 0xFEEDFACF
        assert struct.unpack_from("<I", image, 12)[0] == 6, "Expected MH_DYLIB"
        command_count = struct.unpack_from("<I", image, 16)[0]
        command_offset = 32
        signature_offset = None
        minimum = None
        install_name = None
        dependencies = []
        for command_index in range(command_count):
            command, length = struct.unpack_from("<II", image, command_offset)
            assert command != 0x80000034, "Unexpected chained fixups"
            if command == 0x1D:
                signature_offset, signature_size = struct.unpack_from("<II", image, command_offset + 8)
            if command == 0x32:
                minimum = struct.unpack_from("<I", image, command_offset + 12)[0]
            if command == 0x25:
                minimum = struct.unpack_from("<I", image, command_offset + 8)[0]
            if command in (0xC, 0xD, 0x80000018, 0x8000001F):
                string_offset = struct.unpack_from("<I", image, command_offset + 8)[0]
                value = image[command_offset + string_offset:command_offset + length].split(b"\0")[0].decode()
                if command == 0xD:
                    install_name = value
                else:
                    dependencies.append(value)
            command_offset += length
        assert minimum == 0xF0000, "Minimum iOS must be 15.0"
        assert install_name == expected_install_name, install_name
        assert dependencies and all(value.startswith(("/System/Library/", "/usr/lib/"))
                                    for value in dependencies)
        assert signature_offset is not None
        signature = image[signature_offset:signature_offset + signature_size]
        magic, blob_length, blob_count = struct.unpack_from(">III", signature)
        assert magic == 0xFADE0CC0
        verified = 0
        verified_sha256 = 0
        for blob_index in range(blob_count):
            slot_type, blob_offset = struct.unpack_from(">II", signature, 12 + blob_index * 8)
            if struct.unpack_from(">I", signature, blob_offset)[0] != 0xFADE0C02:
                continue
            directory = signature[blob_offset:]
            hash_offset = struct.unpack_from(">I", directory, 16)[0]
            slot_count, code_limit = struct.unpack_from(">II", directory, 28)
            hash_size, hash_type, platform, page_power = struct.unpack_from("BBBB", directory, 36)
            # ldid emits both SHA-1 and SHA-256 CodeDirectories. Verify
            # every directory, while requiring SHA-256 below (never SHA-1 only).
            assert (hash_type, hash_size) in ((1, 20), (2, 32)), (hash_type, hash_size)
            digest = hashlib.sha256 if hash_type == 2 else hashlib.sha1
            page_size = 1 << page_power
            for page_index in range(slot_count):
                expected = directory[hash_offset + page_index * hash_size:
                                     hash_offset + (page_index + 1) * hash_size]
                page = image[page_index * page_size:min((page_index + 1) * page_size, code_limit)]
                assert digest(page).digest() == expected, (index, page_index)
            verified += slot_count
            if hash_type == 2:
                verified_sha256 += slot_count
        assert verified_sha256 > 0, "Missing SHA-256 CodeDirectory"
        architecture = "arm64e" if subtype & 0xFFFFFF == 2 else "arm64"
        print(f"PASS {label} {architecture}: iOS 15.0, {scheme} install name, {verified_sha256} SHA-256 signed pages")
        print(f"  CPU subtype: {subtype:#010x}; install name: {install_name}")
        print(f"  Dependencies: {dependencies}")
    assert architectures == {0, 2}


if __name__ == "__main__":
    check(Path(sys.argv[1]), sys.argv[2] if len(sys.argv) > 2 else "0.1.3",
          sys.argv[3] if len(sys.argv) > 3 else "roothide")
