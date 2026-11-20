#!/usr/bin/env bash
# =============================================================================
# PostWindow 编译隔离守卫（防倒退棘轮）
#
# 断链批次（GUIFoundation A~E）后 PostWindow 不再依赖 MainWindow：
#   1. src/PostWindow 源文件不得 include 任何 MainWindow 头（同名文件已下沉
#      GUIFoundation/Common/PreData 或经 provider/接口转交）
#   2. src/PostWindow/CMakeLists.txt 不得出现 MainWindow 目标（链接/include）
#
# 用法：bash scripts/check_postwindow_isolation.sh   （CI/提交前运行，0=通过）
# =============================================================================
set -u
cd "$(dirname "$0")/.."

fail=0

# ---- 检查1：PostWindow 源文件对 MainWindow 头的 include ----
# 头名取 basename 精确匹配（允许目录前缀，不允许 Pre/Post 等同后缀改名撞名）；
# 2026-11-12 修正：上游合并曾把 mPostSetPanel/mPostNotesPanel/mPostSlider 的重复
# 副本带进 src/MainWindow（PostWindow 析出前的旧位置残留），PostWindow 源文件
# include 同名头实际解析到本模块自己的副本（本目录在 include 路径上），属误报——
# basename 在 src/PostWindow 本地存在时跳过；真实 MainWindow 依赖（本地无同名
# 文件，如 mTreeView.h）仍然拦截
pattern=$(cd src/MainWindow && ls *.h 2>/dev/null | sed 's/\.h$//' | paste -sd'|')
for f in src/PostWindow/*.cpp src/PostWindow/*.h; do
    [ -e "$f" ] || continue
    hits=$(grep -hE "#include \"([^/\"]*/)?($pattern)\.h\"" "$f" | while read -r line; do
        inc=$(echo "$line" | grep -oE '"[^"]+"' | tr -d '"')
        base=$(basename "$inc" .h)
        [ -e "src/PostWindow/$base.h" ] || echo "$line"
    done)
    if [ -n "$hits" ]; then
        echo "[违规] $f 包含 MainWindow 头："
        echo "$hits" | sed 's/^/       /'
        fail=1
    fi
done

# ---- 检查2：CMake 链接（匹配目标/路径形态，注释中的说明文字不算） ----
if grep -qE '\$\{CurrentModel\}MainWindow|MechMainWindow|\.\./MainWindow' src/PostWindow/CMakeLists.txt; then
    echo "[违规] src/PostWindow/CMakeLists.txt 引用 MainWindow 目标或路径（须保持断链）"
    fail=1
fi

# ---- 检查3：修改记录同步（PostWindow 有改动须同步更新 README.md 修改记录） ----
# 任一源文件比 README.md 新 → 视为未记录，拦截（更新 README 后自然通过）
readme=src/PostWindow/README.md
if [ -f "$readme" ]; then
    changed=$(find src/PostWindow -name '*.cpp' -o -name '*.h' -o -name '*.ui' -o -name 'CMakeLists.txt' 2>/dev/null | while read -r f; do
        [ "$f" -nt "$readme" ] && echo "$f"
    done)
    if [ -n "$changed" ]; then
        echo "[违规] 以下 PostWindow 文件晚于 README.md 修改（须在 src/PostWindow/README.md 文末《修改记录》登记本次改动）："
        echo "$changed" | sed 's/^/       /'
        fail=1
    fi
fi

if [ $fail -eq 0 ]; then
    echo "[PASS] PostWindow 与 MainWindow 保持编译隔离（无头文件引用、无链接）"
fi
exit $fail
