# TaskBarEx 基本設計書

## 1. プロジェクト概要

### 1.1 目的
Windows 11のタスクバーが1段しか表示されなくなった問題を解決するため、タスクバーの内容を一覧表示する拡張ツールを開発する。

### 1.2 主要機能
- 実行中アプリケーションの一覧表示
- Windows 10スタイルのアイコン表示
- 同一アプリケーション複数インスタンスのグループ化
- DPI自動対応
- リアルタイム更新

## 2. システムアーキテクチャ

### 2.1 全体構成図
```
┌─────────────────────────────────────────────────────────────┐
│                        MainWindow                           │
│  ┌─────────────────────────────────────────────────────────┐ │
│  │                 TaskbarContainer                        │ │
│  │  ┌─────────────┐ ┌─────────────┐ ┌─────────────┐       │ │
│  │  │TaskbarGroup │ │TaskbarGroup │ │TaskbarGroup │  ...  │ │
│  │  │   Widget    │ │   Widget    │ │   Widget    │       │ │
│  │  └─────────────┘ └─────────────┘ └─────────────┘       │ │
│  └─────────────────────────────────────────────────────────┘ │
└─────────────────────────────────────────────────────────────┘
                              │
                              │ 管理・制御
                              ▼
┌─────────────────────────────────────────────────────────────┐
│                    TaskbarManager                          │
│  ┌─────────────┐ ┌─────────────┐ ┌─────────────┐            │
│  │WindowDetector│ │WindowInfo  │ │IconExtractor│            │
│  │             │ │   Cache     │ │             │            │
│  └─────────────┘ └─────────────┘ └─────────────┘            │
└─────────────────────────────────────────────────────────────┘
                              │
                              │ システム監視
                              ▼
┌─────────────────────────────────────────────────────────────┐
│                    Windows API Layer                       │
│  EnumWindows | SHGetFileInfo | GetWindowText | ProcessAPI   │
└─────────────────────────────────────────────────────────────┘
```

### 2.2 主要コンポーネント

#### 2.2.1 UI層
- **MainWindow**: アプリケーションのメインウィンドウ
- **TaskbarContainer**: アプリケーショングループを格納するスクロール可能なコンテナ
- **TaskbarGroupWidget**: 個別のアプリケーション（グループ）を表示

#### 2.2.2 管理層
- **TaskbarManager**: 全体の制御とデータ管理
- **TaskbarSettings**: アプリケーション設定の管理
- **ApplicationInfo**: アプリケーション情報のデータモデル

#### 2.2.3 システム層
- **WindowDetector**: Windows APIを使用したウィンドウ検出
- **WindowInfoCache**: ウィンドウ情報の効率的なキャッシング
- **IconExtractor**: アプリケーションアイコンの抽出

## 3. クラス設計

### 3.1 主要クラス関係図
```
ApplicationInfo ←── TaskbarManager ──→ WindowDetector
     │                   │                    │
     │                   ▼                    ▼
     └────→ TaskbarGroupWidget          WindowInfoCache
                   │                         │
                   ▼                         ▼
            TaskbarContainer            IconExtractor
                   │
                   ▼
              MainWindow
```

### 3.2 データフロー
```
Windows System
     │ (EnumWindows)
     ▼
WindowDetector
     │ (ApplicationInfo)
     ▼
TaskbarManager
     │ (Grouped ApplicationInfo)
     ▼
TaskbarContainer
     │ (Individual Groups)
     ▼
TaskbarGroupWidget
     │ (User Interaction)
     ▼
Windows System (Activation)
```

## 4. ファイル構成

### 4.1 ディレクトリ構造
```
TaskBarEx/
├── CMakeLists.txt
├── README.md
├── CLAUDE.md
├── .gitignore
├── include/                    # ヘッダーファイル
│   ├── Common.h
│   ├── MainWindow.h
│   ├── TaskbarManager.h
│   ├── WindowDetector.h
│   ├── ApplicationInfo.h
│   ├── TaskbarGroupWidget.h
│   ├── WindowInfoCache.h
│   └── IconExtractor.h
├── src/                        # ソースファイル
│   ├── main.cpp
│   ├── MainWindow.cpp
│   ├── TaskbarManager.cpp
│   ├── WindowDetector.cpp
│   ├── ApplicationInfo.cpp
│   ├── TaskbarGroupWidget.cpp
│   ├── WindowInfoCache.cpp
│   └── IconExtractor.cpp
├── resources/                  # リソースファイル
│   ├── icons.qrc
│   ├── styles.qrc
│   ├── icons/
│   └── styles/
├── docs/                       # ドキュメント
│   ├── Basic_Design.md
│   ├── Windows_API_Integration.md
│   └── Application_Grouping.md
├── tests/                      # テストファイル（予定）
└── third_party/               # サードパーティライブラリ
    └── README.md
```

### 4.2 主要ファイルの役割

#### ヘッダーファイル (.h)
- **Common.h**: 共通定数、型定義、ユーティリティ関数
- **MainWindow.h**: メインウィンドウとUI管理
- **TaskbarManager.h**: タスクバー管理とデータ制御
- **WindowDetector.h**: ウィンドウ検出とシステム監視
- **ApplicationInfo.h**: アプリケーション情報モデル
- **TaskbarGroupWidget.h**: UI表示コンポーネント
- **WindowInfoCache.h**: パフォーマンス最適化
- **IconExtractor.h**: アイコン抽出と管理

#### ソースファイル (.cpp)
各ヘッダーファイルに対応する実装ファイル

## 5. 技術仕様

### 5.1 開発環境
- **言語**: C++17
- **フレームワーク**: Qt6
- **ビルドシステム**: CMake 3.16+
- **プラットフォーム**: Windows 10/11
- **コンパイラ**: MSVC 2019+ / MinGW

### 5.2 外部依存関係
- **Qt6 Core**: 基本機能
- **Qt6 Widgets**: UI機能
- **Windows API**: システム統合
- **Qx Library**: Windows固有機能（推奨）

### 5.3 システム要件
- **OS**: Windows 10 1903 以降 / Windows 11
- **メモリ**: 最小 100MB
- **CPU**: 特別な要件なし
- **権限**: 標準ユーザー権限

## 6. 設計方針

### 6.1 パフォーマンス
- **効率的なキャッシング**: 頻繁なAPI呼び出しを避ける
- **差分更新**: 変更があった部分のみ更新
- **非同期処理**: UI blocking を避ける
- **メモリ管理**: リソースリークを防ぐ

### 6.2 保守性
- **モジュール分離**: 責任の明確な分割
- **設定可能**: 動作をカスタマイズ可能
- **エラーハンドリング**: 適切な例外処理
- **ログ出力**: デバッグとトラブルシューティング

### 6.3 拡張性
- **プラグイン対応**: 将来的な機能拡張
- **テーマシステム**: カスタマイズ可能なUI
- **設定システム**: 柔軟な設定管理
- **API設計**: 他のアプリケーションとの連携

## 7. 実装優先度

### Phase 1: 基本機能 (MVP)
1. ウィンドウ検出機能
2. 基本的なアイコン表示
3. シンプルなUI
4. アプリケーション起動機能

### Phase 2: 拡張機能
1. アプリケーショングループ化
2. DPI対応
3. 設定機能
4. システムトレイ統合

### Phase 3: 最適化・改善
1. パフォーマンス最適化
2. UI/UX改善
3. エラーハンドリング強化
4. ドキュメント整備

## 8. テスト戦略

### 8.1 単体テスト
- 各クラスの基本機能
- Windows API ラッパー
- データモデルの整合性

### 8.2 統合テスト
- コンポーネント間の連携
- システムAPI統合
- UI操作のテスト

### 8.3 システムテスト
- 実際の環境での動作確認
- 各種アプリケーションとの互換性
- 長時間動作の安定性

## 9. 今後の課題

### 9.1 技術的課題
- UWPアプリケーションの完全対応
- 高DPI環境での最適化
- Windows 11特有の機能対応

### 9.2 機能的課題
- カスタムグループ化
- ホットキーサポート
- 他のタスクバー拡張との協調

### 9.3 運用的課題
- 自動更新機能
- ユーザーサポート
- 多言語対応