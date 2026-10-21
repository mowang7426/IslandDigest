set -eu
cd "$(dirname "$0")/.."
: "${THEOS:?THEOS must be injected}"
: "${CA_THEOS_SDKROOT:?CA_THEOS_SDKROOT must be injected}"
python3 -c 'from pathlib import Path; Path(".theos/regression").mkdir(parents=True, exist_ok=True)'
perl "$THEOS/vendor/logos/bin/logos.pl" -c generator=internal Tweak.xm > .theos/regression/Core.mm
for suite in Regression Integration; do
clang++ -arch "${ID_TEST_ARCH:-arm64}" -isysroot "$CA_THEOS_SDKROOT" -miphoneos-version-min=15.0 \
    -fobjc-arc -dynamiclib -framework UIKit -framework QuartzCore -framework Foundation \
    -framework CoreGraphics -Wl,-no_fixup_chains -Wl,-undefined,dynamic_lookup \
    "tests/$suite.mm" -x c tests/Runner.c -o ".theos/regression/$suite.node"
node tests/run_native.js ".theos/regression/$suite.node"
done
