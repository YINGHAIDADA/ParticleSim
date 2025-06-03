#!/bin/bash
# 清理脚本：删除 output/macOS-debug 下除 _deps 外的所有内容
# 用法：./clean_build.sh

TARGET_DIR="output/macOS-debug"
# 检查目标目录是否存在
if [ ! -d "$TARGET_DIR" ]; then
  echo "目录 $TARGET_DIR 不存在，无需清理"
  exit 0
fi
# 进入目标目录
cd "$TARGET_DIR" || exit 1
echo "正在清理 $TARGET_DIR ..."

# 删除除 _deps 外的所有文件和文件夹
for item in * .[!.]*; do
  # 跳过特殊目录和文件
  if [[ "$item" == "_deps" || "$item" == "." || "$item" == ".." ]]; then
    continue
  fi

  # 安全删除检查
  if [ -e "$item" ]; then
    echo "删除: $item"
    rm -rf "$item"
  fi
done

echo "清理完成！保留的 _deps 目录内容："
cd ../.. || exit 1

echo "开始执行cmake配置："

echo "VulkanSDK: $VULKAN_SDK"
/opt/homebrew/bin/cmake -DCMAKE_BUILD_TYPE=Debug \
-DCMAKE_C_COMPILER=/opt/homebrew/opt/llvm/bin/clang \
-DCMAKE_CXX_COMPILER=/opt/homebrew/opt/llvm/bin/clang++ \
-S/Users/yinghai/dev/ParticleSim \
-B/Users/yinghai/dev/ParticleSim/output/macOS-debug \
-G Ninja