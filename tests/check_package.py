import hashlib
import io
from pathlib import Path
import struct
import sys
import tarfile


def check(package, expected_version="0.1.3"):
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
        for expected in ("Architecture: iphoneos-arm64e", f"Version: {expected_version}",
                         "firmware (>= 15.0)", "firmware (<< 18.0)"):
            assert expected in metadata, expected
        assert all(member.name.split("/")[-1] not in
                   ("preinst", "postinst", "prerm", "postrm")
                   for member in control.getmembers())
    data_name = next(name for name in members if name.startswith("data.tar"))
    with tarfile.open(fileobj=io.BytesIO(members[data_name])) as payload:
        files = [member for member in payload.getmembers() if member.isfile()]
        assert len(files) == 2
        assert all(member.name.lstrip("./").startswith(
            "Library/MobileSubstrate/DynamicLibraries/") for member in files)
        binary = payload.extractfile(next(member for member in files
                                         if member.name.endswith(".dylib"))).read()
        injection_filter = payload.extractfile(next(member for member in files
                                                   if member.name.endswith(".plist"))).read()
        assert b"com.apple.springboard" in injection_filter
    assert struct.unpack_from(">II", binary) == (0xCAFEBABE, 2)
    architectures = set()
    for index in range(2):
        cpu, subtype, offset, size, alignment = struct.unpack_from(">IIIII", binary, 8 + 20 * index)
        assert cpu == 0x100000C
        architectures.add(subtype & 0xFFFFFF)
        image = binary[offset:offset + size]
        assert struct.unpack_from("<I", image)[0] == 0xFEEDFACF
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
        assert install_name == "@loader_path/.jbroot/Library/MobileSubstrate/DynamicLibraries/IslandDigest.dylib"
        assert dependencies and all(value.startswith(("/System/Library/", "/usr/lib/"))
                                    for value in dependencies)
        assert signature_offset is not None
        signature = image[signature_offset:signature_offset + signature_size]
        magic, blob_length, blob_count = struct.unpack_from(">III", signature)
        assert magic == 0xFADE0CC0
        verified = 0
        for blob_index in range(blob_count):
            slot_type, blob_offset = struct.unpack_from(">II", signature, 12 + blob_index * 8)
            if struct.unpack_from(">I", signature, blob_offset)[0] != 0xFADE0C02:
                continue
            directory = signature[blob_offset:]
            hash_offset = struct.unpack_from(">I", directory, 16)[0]
            slot_count, code_limit = struct.unpack_from(">II", directory, 28)
            hash_size, hash_type, platform, page_power = struct.unpack_from("BBBB", directory, 36)
            assert hash_type == 2 and hash_size == 32
            page_size = 1 << page_power
            for page_index in range(slot_count):
                expected = directory[hash_offset + page_index * hash_size:
                                     hash_offset + (page_index + 1) * hash_size]
                page = image[page_index * page_size:min((page_index + 1) * page_size, code_limit)]
                assert hashlib.sha256(page).digest() == expected, (index, page_index)
            verified += slot_count
        assert verified > 0
        architecture = "arm64e" if subtype & 0xFFFFFF == 2 else "arm64"
        print(f"PASS {architecture}: iOS 15.0, RootHide install name, {verified} signed pages")
    assert architectures == {0, 2}
    print("PASS metadata, payload, SpringBoard filter, system dependencies, no install scripts")
    print("SHA-256", hashlib.sha256(archive).hexdigest())


if __name__ == "__main__":
    check(Path(sys.argv[1]), sys.argv[2] if len(sys.argv) > 2 else "0.1.3")
