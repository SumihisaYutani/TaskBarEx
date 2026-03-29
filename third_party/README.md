# サードパーティライブラリ

このディレクトリには、TaskBarExプロジェクトで使用する外部ライブラリを配置します。

## 推奨ライブラリ構成

### Qx Library（Windows統合）
```
third_party/
└── Qx/
    ├── CMakeLists.txt
    ├── include/
    └── src/
```

**導入方法：**
```bash
git submodule add https://github.com/oblivioncth/Qx.git third_party/Qx
```

**使用目的：**
- QWinTaskbarButtonの代替機能
- Windows 特有の機能統合

### 導入予定ライブラリ

1. **Qx Library** - Windows統合（必須）
2. **spdlog** - ログ出力（オプション）
3. **nlohmann/json** - 設定ファイル（オプション）

## ライセンス

各ライブラリのライセンスについては、個別のライブラリディレクトリ内のLICENSEファイルを参照してください。

## CMake統合

サードパーティライブラリは、メインのCMakeLists.txtで以下のように統合されます：

```cmake
# Qx Library
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/third_party/Qx/CMakeLists.txt")
    add_subdirectory(third_party/Qx)
    target_link_libraries(TaskBarEx Qx)
    target_compile_definitions(TaskBarEx PRIVATE QX_AVAILABLE)
endif()
```