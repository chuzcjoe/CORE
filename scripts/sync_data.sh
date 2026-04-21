#! /bin/bash

set -euo pipefail

# =========================
# Config
# =========================
DATA_REPO_URL="git@github.com:chuzcjoe/core_data.git"
DATA_REPO_REF="${1:-master}"   # default to master

TARGET_DIR="$(pwd)"
TMP_DIR="$(mktemp -d)"

echo "[INFO] Target repo: $TARGET_DIR"
echo "[INFO] Temp dir: $TMP_DIR"
echo "[INFO] Using ref: $DATA_REPO_REF"

# =========================
# Cleanup on exit
# =========================
cleanup() {
    if [[ -d "$TMP_DIR" ]]; then
        echo "[INFO] Cleaning up temp directory..."
        rm -rf "$TMP_DIR"
    fi
}
trap cleanup EXIT

# =========================
# Clone repo
# =========================
echo "[INFO] Cloning data repo (ref=$DATA_REPO_REF)..."

git clone \
    --depth=1 \
    --branch "$DATA_REPO_REF" \
    "$DATA_REPO_URL" \
    "$TMP_DIR/data-repo"

cd "$TMP_DIR/data-repo"

# =========================
# Pull LFS data (materialize files)
# =========================
echo "[INFO] Pulling LFS objects..."
git lfs install --local
git lfs pull

# =========================
# Sync files (NO LFS metadata)
# =========================
echo "[INFO] Syncing files into target repo (excluding LFS metadata)..."

rsync -av \
    --exclude=".git" \
    --exclude=".gitignore" \
    --exclude=".gitattributes" \
    --exclude=".lfsconfig" \
    "$TMP_DIR/data-repo/" "$TARGET_DIR/"

# =========================
# Safety cleanup in target repo
# (in case previous runs copied them)
# =========================
echo "[INFO] Ensuring no LFS metadata remains in target repo..."

rm -f "$TARGET_DIR/.gitattributes" || true
rm -f "$TARGET_DIR/.lfsconfig" || true

# If already tracked in git, untrack them
git -C "$TARGET_DIR" rm --cached .gitattributes 2>/dev/null || true
git -C "$TARGET_DIR" rm --cached .lfsconfig 2>/dev/null || true

# =========================
# Done
# =========================
echo "[INFO] Data sync complete (LFS-free)."