# TaskBarEx プロジェクト

## プロジェクト概要

Windows 11のタスクバーが1段しか表示されなくなり使いにくいため、タスクバーの表示と同期してタスクバーの内容を一覧表示するQtアプリケーションを開発する。

## 技術調査結果

### Windows APIを使ったタスクバー情報取得

#### 主要な方法
1. **EnumWindows API** - 実行中ウィンドウの列挙
2. **GetWindowText** - ウィンドウタイトル取得
3. **GetWindowThreadProcessId** - プロセスID取得
4. **SHGetFileInfo** - アプリケーションアイコン取得
5. **ITaskbarList3** - タスクバー状態取得（オプション）

#### 実装パターン
```cpp
static BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam) {
    if (IsWindowVisible(hwnd)) {
        // ウィンドウタイトル取得
        // プロセス情報取得
        // リストに追加
    }
}
```

### QtでのWindows API統合

#### Qt6での変更点
- QWinTaskbarButton、QWinExtrasは**廃止**
- Qt6には公式代替手段なし
- 代替ソリューション：
  - **Qx Library** (推奨) - QWinTaskbarButtonの代替
  - 直接Windows API実装
  - qprocessinfoライブラリ

#### アイコン取得方法
```cpp
// HICONからQPixmapへの変換（Qt6）
QPixmap pixmap = QPixmap::fromImage(QImage::fromHICON(hIcon));
```

### Windows 10タスクバー仕様

#### アイコンサイズ（DPI対応）
- 100%DPI: 22×22px
- 125%DPI: 28×28px
- 150%DPI: 33×33px
- 200%DPI: 44×44px

#### 視覚仕様
- 間隔: 2-4px
- 背景: 透明またはアクティブ時の薄いハイライト
- 角丸: 2px radius

### 同一アプリケーション複数起動時の挙動

#### Windows 10の3つのグループ化モード
1. **Always combine, hide labels（デフォルト）**
   - 同一アプリは1つのアイコンにまとめられる
   - 複数インスタンスは枠線で表示

2. **Combine when taskbar is full**
   - 通常は個別表示、満杯時にグループ化

3. **Never combine**
   - 各ウィンドウが独立したボタンで表示

#### グループ化の基準
- **AppUserModelID** によって決定
- 同一実行ファイルパス
- プロセス名での判定

## 推奨実装アプローチ

### アーキテクチャ
```
TaskbarViewer/
├── main.cpp
├── TaskbarWindow.h/cpp      // メインウィンドウ
├── TaskbarModel.h/cpp       // データモデル
├── WindowInfo.h/cpp         // ウィンドウ情報クラス
├── TaskbarGroupManager.h/cpp // グループ化管理
└── CMakeLists.txt           // ビルド設定
```

### 主要機能
- リアルタイム更新（1秒間隔）
- ウィンドウアイコン表示
- クリックでウィンドウアクティベート
- 最小化状態の表示
- DPI自動対応
- Windows 10スタイルのUI

### 技術スタック
- **フレームワーク**: Qt6
- **ライブラリ**: Qx Library（タスクバー統合用）
- **API**: Windows API（EnumWindows、SHGetFileInfo）
- **ビルドシステム**: CMake

## 実装ステータス

- [x] 技術調査完了
- [x] アーキテクチャ設計完了
- [x] プロジェクト構成作成
- [x] 基本ウィンドウ列挙機能実装
- [x] アイコン取得機能実装
- [x] グループ化機能実装
- [x] UI実装（Windows風タスクバーデザイン）
- [x] DLL依存関係解決
- [x] Qt6プラットフォームプラグイン対応
- [x] 安全なHICON→QPixmap変換実装
- [x] **新マウス座標ベース表示制御実装**
- [x] アプリバー表示領域修正
- [x] 循環参照問題解決
- [x] テスト・デバッグ完了

### 解決済み問題
- ✅ Qt Creator実行時のDLL不足エラー
- ✅ プラットフォームプラグイン初期化エラー
- ✅ HICON変換時の初期化エラー
- ✅ ウィンドウ表示問題
- ✅ アイコン表示の四角表示問題
- ✅ ターミナル窓表示問題
- ✅ TaskBarEx出しっぱなし問題（循環参照）
- ✅ **複雑なタスクバー連動ロジックの問題（廃止）**
- ✅ **アプリバー表示領域の見切れ問題**
- ✅ **表示維持範囲の問題**

## 最終実装された動作仕様

### 新マウス座標ベース表示制御

#### 基本動作原理
- **マウスY座標 >= 1070（画面下端から10px以内）** → TaskBarEx即座表示
- **マウスY座標 984-1070（表示維持範囲）** → TaskBarEx表示継続
- **マウスY座標 < 984（アプリバー上端より上）** → TaskBarEx即座非表示

#### 詳細動作仕様
- **画面高さ**: 1080ピクセル
- **表示閾値**: Y=1070（画面下端から10px以内で表示トリガー）
- **非表示閾値**: Y=984（アプリバー上端で非表示トリガー）
- **アプリバー位置**: Y=984～1032（48px高さ）
- **表示維持範囲**: Y=984～1070（86px幅）
- **チェック間隔**: 50ms（高精度マウストラッキング）

#### 実装された主要変更

**1. タスクバー連動機能の完全削除**
```cpp
// 削除された機能:
// - TaskbarVisibilityMonitor
// - Windows フォーカスフック
// - 複雑な状態遷移ベース判定
// - 表示ディレイタイマー
```

**2. シンプルなマウス座標ベース表示制御**
```cpp
void TaskbarWindow::checkMousePosition()
{
    QPoint globalPos = QCursor::pos();
    int mouseY = globalPos.y();
    
    // 表示判定：画面下端付近
    int showThreshold = m_screenHeight - 10;
    bool shouldShow = (mouseY >= showThreshold);
    
    // 非表示判定：アプリバー上端より上
    int hideThreshold = m_screenHeight - m_taskbarHeight - m_appBarHeight;
    bool shouldHide = (mouseY < hideThreshold);
    
    // 表示制御
    if (shouldShow && !isVisible()) {
        show();
    } else if (shouldHide && isVisible()) {
        hide();
    }
}
```

**3. アプリバー表示領域の修正**
```cpp
// メニューバー・ステータスバーの完全無効化
menuBar()->setVisible(false);
statusBar()->setVisible(false);

// centralwidgetのマージン最小化
centralWidget()->layout()->setContentsMargins(0, 0, 0, 0);
```

## 開発コマンド

### ビルド手順（Claude Code用）

```bash
# プロジェクトディレクトリに移動
cd /d/ClaudeCode/project/TaskBarEx

# Qt6とCMakeのパス設定
export PATH="/c/Qt/6.10.0/mingw_64/bin:/c/Qt/Tools/CMake_64/bin:$PATH"

# ビルド実行
/c/Qt/Tools/CMake_64/bin/cmake.exe -B build
/c/Qt/Tools/CMake_64/bin/cmake.exe --build build
```

### 実行パス

```bash
# 実行ファイル（Qt6 DLL自動コピー済み）
./build/TaskBarEx.exe

# 旧パス（Qt Creatorビルド用 - 現在は使用しない）
./build/Desktop_Qt_6_10_0_MinGW_64_bit-Debug/TaskBarEx.exe
```

### ログファイル場所

```bash
# 現在のログ保存場所
./build/log/TaskBarEx_YYYY-MM-DD_HH-MM-SS.log

# 最新ログファイル確認
ls -lt build/log/ | head -3
```

### トラブルシューティング

- **Qt6 DLL不足エラー**: CMakeLists.txtで自動DLLコピーを実装済み
- **プラットフォームプラグイン不足**: platforms/qwindows.dll自動コピー済み
- **Qt Creatorでの実行**: 上記ビルド手順でDLL依存関係解決済み

#### TaskBarEx特有のトラブルシューティング

**1. TaskBarExが出しっぱなしになる**
- **原因**: 循環参照（TaskBarExがフォーカスを取ってタスクバー隠しを阻害）
- **解決済み**: Qt::WA_ShowWithoutActivating属性で解決

**2. タスクバーが隠れているのに検出されない**
- **原因**: 位置検出の閾値設定ミス（screenHeight + 40 vs 実際の隠し位置）
- **解決済み**: `taskbarRect.top < screenHeight - 60` で適切な検出

**3. マウス範囲外でも表示され続ける**
- **原因**: タスクバー表示中の既表示状態で無条件に表示維持
- **解決済み**: マウス範囲外では即座非表示に修正

**4. 操作時にTaskBarExが邪魔になる**
- **原因**: 表示にディレイがあるが非表示が即座
- **解決済み**: 非表示のみにディレイ適用で操作妨害を防止

#### 動作確認方法

1. **基本動作テスト**
   ```bash
   ./build/TaskBarEx.exe
   # タスクバー自動隠し機能を有効にして以下を確認：
   # - マウス → タスクバー範囲 → TaskBarEx表示
   # - マウス → 範囲外 → TaskBarEx即座非表示
   # - タスクバー隠し → TaskBarEx 300ms後非表示
   ```

2. **ログ確認**
   ```bash
   tail -f build/log/TaskBarEx_*.log
   # 以下のログパターンを確認：
   # 🚀 TASKBAR APPEARED - マウスオーバーのため即座に表示
   # 🔴 HIDING TaskBarEx (taskbar visible but mouse away)
   # ⏱️ TASKBAR HIDDEN - starting hide delay timer
   # 🔴 HIDE CONFIRMED: TaskBarExを非表示
   ```

## Claude Code 対話設定

**重要：Claude Codeは対話を続けていると日本語を忘れて英語のみになる傾向があります。**

### 言語設定
- **常に日本語で対話すること**
- 技術用語は適宜英語併記でも可
- コメントやログメッセージは日本語で記載
- 変数名・関数名は英語でも可（Qt慣例に従う）

### 開発時の注意点
- ビルドパスは絶対パスで指定すること
- ログファイルは日時付きで自動生成される
- デバッグ時は詳細ログを有効活用すること

## 次期実装予定機能

### 2段表示システム（実装予定）

#### 基本仕様
- **上段**: 起動中アプリケーションのアイコン表示・グループ表示
- **下段**: タスクバーにピン留めされているアイコンの表示

#### 動的レイアウト調整
- 各段がアイコン幅いっぱいまで広がると2列表示に変更
- アイコン数に応じてアプリバーの高さを動的調整
- マウス表示維持領域もアプリバー高さに連動して拡大

#### 詳細動作仕様（予定）
```
段数計算:
- 1段目: 起動中アプリ（最大アイコン数まで）
- 2段目: ピン留めアプリ（最大アイコン数まで）
- 各段が満杯 → 2列表示に移行

アプリバー高さ計算:
- 基本高さ: 48px（1段1列）
- 2段1列: 96px（48px × 2）
- 1段2列: 96px（48px × 2）
- 2段2列: 192px（48px × 4）

マウス表示維持領域調整:
- 基本: Y=984～1070（86px幅）
- 2段時: Y=936～1070（134px幅）
- 2列時: Y=888～1070（182px幅）
```

#### 実装要件
1. **ピン留めアプリ検出**
   - レジストリからタスクバーピン留め情報取得
   - `HKEY_CURRENT_USER\Software\Microsoft\Windows\CurrentVersion\Explorer\Taskband`

2. **動的レイアウト管理**
   - アイコン数カウントによる段・列計算
   - アプリバー高さリアルタイム調整
   - マウス座標閾値の動的更新

3. **視覚的区分け**
   - 上段・下段の視覚的境界線
   - アクティブ/非アクティブ状態の区別表示
   - ピン留めアイコンの特別な表示（固定表示）

### 実装優先度
- [x] **Phase 1**: 基本1段表示（完了）
- [x] **Phase 2**: マウス座標ベース表示制御（完了）
- [x] **Phase 3**: 0.5秒非表示ディレイ（完了）
- [ ] **Phase 4**: 2段表示システム（次期実装）
- [ ] **Phase 5**: 動的レイアウト調整（次期実装）
- [ ] **Phase 6**: ピン留めアプリ検出・表示（次期実装）

## 参考リンク

- [Qx Library](https://github.com/oblivioncth/Qx) - Qt6対応のWindows統合ライブラリ
- [Windows API EnumWindows](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-enumwindows)
- [Windows API SHGetFileInfo](https://learn.microsoft.com/en-us/windows/win32/api/shellapi/nf-shellapi-shgetfileinfoa)
- [AppUserModelID Documentation](https://learn.microsoft.com/en-us/windows/win32/shell/appids)
- [Windows Taskbar Pinned Items Registry](https://learn.microsoft.com/en-us/windows/win32/shell/taskbar)