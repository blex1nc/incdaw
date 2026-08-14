#!/usr/bin/env bash
#
# Builds INCDAW.app and packages it into a distributable .dmg.
#
# The DMG is AD-HOC SIGNED and NOT NOTARIZED, per docs/DECISIONS.md D-009.
# Ad-hoc signing is not cosmetic: arm64 binaries will not execute at all on
# Apple silicon without at least an ad-hoc signature.
#
# Because it is not notarized, Gatekeeper quarantines the DMG when it is
# downloaded. On first launch each user must either right-click -> Open, or run:
#
#     xattr -dr com.apple.quarantine /Applications/INCDAW.app
#
# If INCDAW is ever distributed publicly, enrolling in the Apple Developer
# Program adds two steps to this script (sign with a Developer ID Application
# certificate, then notarytool + stapler). Nothing else changes.

set -euo pipefail

readonly ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
readonly BUILD_DIR="${ROOT}/build-release"
readonly STAGE_DIR="${ROOT}/build-dmg-stage"
readonly DIST_DIR="${ROOT}/dist"

readonly APP_NAME="INCDAW"
log() { printf '\033[1m==>\033[0m %s\n' "$*"; }

# ── 1. Build ──────────────────────────────────────────────────────────────────
log "Configuring (Release)"
cmake -S "${ROOT}" -B "${BUILD_DIR}" -G Ninja \
      -DCMAKE_BUILD_TYPE=Release \
      -DINCDAW_BUILD_TESTS=ON > /dev/null

# CMake writes this during configure, so the version can never drift from the
# one compiled into the bundle.
VERSION="$(cat "${BUILD_DIR}/VERSION")"
readonly VERSION
readonly DMG_PATH="${DIST_DIR}/${APP_NAME}-${VERSION}.dmg"

log "Building ${APP_NAME} ${VERSION}"
cmake --build "${BUILD_DIR}"

# ── 2. Test ───────────────────────────────────────────────────────────────────
# A release that has not passed its own suite is not a release.
log "Running test suite"
( cd "${BUILD_DIR}" && ctest --output-on-failure )

readonly APP_PATH="${BUILD_DIR}/src/${APP_NAME}.app"
[[ -d "${APP_PATH}" ]] || { echo "error: ${APP_PATH} was not produced" >&2; exit 1; }

# ── 3. Sign ───────────────────────────────────────────────────────────────────
log "Ad-hoc signing ${APP_NAME}.app"
codesign --force --deep --sign - --options runtime --timestamp=none "${APP_PATH}"
codesign --verify --deep --strict --verbose=1 "${APP_PATH}"

# ── 4. Stage ──────────────────────────────────────────────────────────────────
log "Staging disk image contents"
rm -rf "${STAGE_DIR}"
mkdir -p "${STAGE_DIR}"
cp -R "${APP_PATH}" "${STAGE_DIR}/"
ln -s /Applications "${STAGE_DIR}/Applications"

cat > "${STAGE_DIR}/FIRST LAUNCH.txt" <<'NOTE'
INCDAW is signed for local use but is not notarized by Apple.

The first time you open it, macOS will refuse with a warning. This is expected.

To open it:
  1. Drag INCDAW to Applications.
  2. Right-click INCDAW in Applications and choose "Open".
  3. Confirm in the dialog.

Or, from Terminal:
  xattr -dr com.apple.quarantine /Applications/INCDAW.app

You only need to do this once per installed copy.
NOTE

# ── 5. Package ────────────────────────────────────────────────────────────────
log "Creating disk image"
mkdir -p "${DIST_DIR}"
rm -f "${DMG_PATH}"
hdiutil create \
    -volname "${APP_NAME} ${VERSION}" \
    -srcfolder "${STAGE_DIR}" \
    -ov -format UDZO \
    "${DMG_PATH}" > /dev/null

codesign --force --sign - "${DMG_PATH}"

rm -rf "${STAGE_DIR}"

# ── 6. Verify ─────────────────────────────────────────────────────────────────
log "Verifying disk image"
hdiutil verify "${DMG_PATH}" > /dev/null

SIZE="$(du -h "${DMG_PATH}" | cut -f1 | tr -d ' ')"
log "Built ${DMG_PATH} (${SIZE})"

# Gatekeeper will reject this, by design — we report it rather than hide it.
if ! spctl -a -t open --context context:primary-signature "${DMG_PATH}" 2>/dev/null; then
    echo
    echo "Note: Gatekeeper rejects this image because it is not notarized."
    echo "      This is expected (docs/DECISIONS.md D-009). See 'FIRST LAUNCH.txt'"
    echo "      inside the image for the one-time steps each user must take."
fi
