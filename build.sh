#!/usr/bin/env bash
set -euo pipefail

# ============================================================================
# lianwall-gui 构建脚本
# 用法: ./build.sh [--deploy]
#   --deploy  构建后自动部署到 ~/.local/bin/
# ============================================================================

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
BUILD_DIR="${SCRIPT_DIR}/build"

# 从 CMakeLists.txt 提取版本号
VERSION=$(grep -oP 'project\(lianwall-gui VERSION \K[0-9.]+' "${SCRIPT_DIR}/CMakeLists.txt")

if [[ -z "$VERSION" ]]; then
    echo "❌ 无法从 CMakeLists.txt 提取版本号"
    exit 1
fi

ARTIFACT_DIR="${BUILD_DIR}/${VERSION}"
BINARY_NAME="lianwall-gui"
DEPLOY=false

# 解析参数
for arg in "$@"; do
    case "$arg" in
        --deploy) DEPLOY=true ;;
        *) echo "未知参数: $arg"; exit 1 ;;
    esac
done

echo "🔨 构建 ${BINARY_NAME} v${VERSION}"
echo "   构建目录: ${BUILD_DIR}"
echo "   产物目录: ${ARTIFACT_DIR}"
echo ""

# CMake 配置（仅首次或 CMakeLists.txt 变更时需要）
if [[ ! -f "${BUILD_DIR}/CMakeCache.txt" ]]; then
    echo "📦 CMake 配置..."
    cmake -S "${SCRIPT_DIR}" -B "${BUILD_DIR}" -DCMAKE_BUILD_TYPE=Release
    echo ""
fi

# 编译
echo "⚙️  编译中..."
cmake --build "${BUILD_DIR}" -j"$(nproc)"
echo ""

# 复制产物到版本目录
mkdir -p "${ARTIFACT_DIR}"
cp "${BUILD_DIR}/${BINARY_NAME}" "${ARTIFACT_DIR}/${BINARY_NAME}_${VERSION}_linux_$(uname -m)"

echo "✅ 构建完成: ${ARTIFACT_DIR}/${BINARY_NAME}_${VERSION}_linux_$(uname -m)"

# 生成 checksums
cd "${ARTIFACT_DIR}"
sha256sum "${BINARY_NAME}_${VERSION}_linux_$(uname -m)" > "checksums_${VERSION}.txt"
echo "📋 校验和:   ${ARTIFACT_DIR}/checksums_${VERSION}.txt"

# 部署
if $DEPLOY; then
    DEST="${HOME}/.local/bin/${BINARY_NAME}"
    cp "${BUILD_DIR}/${BINARY_NAME}" "${DEST}"
    echo "🚀 已部署到: ${DEST}"
fi

echo ""
echo "完成! 🎉"
