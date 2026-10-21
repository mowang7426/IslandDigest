set -eu
cd "$(dirname "$0")/.."
: "${CA_THEOS_SDKROOT:?CA_THEOS_SDKROOT must be injected}"
package="${1:?provide the DEB to test}"
mode="${2:-}"
python3 - "$package" <<'PY'
import io
from pathlib import Path
import sys
import tarfile

archive = Path(sys.argv[1]).read_bytes()
assert archive[:8] == b"!<arch>\n"
position = 8
found = False
while position < len(archive):
    header = archive[position:position + 60]
    size = int(header[48:58])
    name = header[:16].decode().strip().rstrip("/")
    if name.startswith("data.tar"):
        with tarfile.open(fileobj=io.BytesIO(archive[position + 60:position + 60 + size])) as payload:
            members = [member for member in payload.getmembers()
                       if member.isfile() and member.name.endswith("/IslandDigest.dylib")]
            assert len(members) == 1
            target = Path(".theos/artifact/PackagedIslandDigest.node")
            target.parent.mkdir(parents=True, exist_ok=True)
            target.write_bytes(payload.extractfile(members[0]).read())
            found = True
    position += 60 + size + size % 2
assert found
PY
clang++ -arch "${ID_TEST_ARCH:-arm64}" -isysroot "$CA_THEOS_SDKROOT" \
    -miphoneos-version-min=15.0 -fobjc-arc -dynamiclib -framework UIKit \
    -framework Foundation -framework CoreFoundation -Wl,-no_fixup_chains \
    -Wl,-undefined,dynamic_lookup tests/ArtifactLoad.mm -x c tests/Runner.c \
    -o .theos/artifact/ArtifactLoad.node
node tests/run_artifact.js .theos/artifact/PackagedIslandDigest.node \
    .theos/artifact/ArtifactLoad.node "$mode"
