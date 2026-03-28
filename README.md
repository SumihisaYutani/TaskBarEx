# TaskBarEx - Windows タスクバー拡張ツール

Windows 11のタスクバーが1段しか表示されなくなった問題を解決するため、タスクバーの内容を一覧表示する拡張ツールです。

## 機能

- 実行中アプリケーションの一覧表示
- Windows 10スタイルのアイコン表示
- 同一アプリケーション複数インスタンスのグループ化
- DPI自動対応
- リアルタイム更新

## システム要件

- Windows 10/11
- Qt6
- CMake 3.16以上
- Visual Studio 2019以上 または MinGW

## 技術仕様

### 使用技術
- **フレームワーク**: Qt6
- **Windows API**: EnumWindows, SHGetFileInfo
- **外部ライブラリ**: Qx Library (Windows統合)

### アーキテクチャ
- リアルタイムウィンドウ監視
- AppUserModelIDベースのグループ化
- DPI対応アイコン表示

## ライセンス

MIT License

## 開発者向け情報

詳細な技術情報とプロジェクトステータスは[CLAUDE.md](CLAUDE.md)を参照してください。