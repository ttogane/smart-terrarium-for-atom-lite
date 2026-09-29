# Smart Terrarium For Atom Lite

M5Stack Atom Lite で動くスマートテラリウムのモノレポ。

## ディレクトリ構成

| ディレクトリ | 内容 |
|---|---|
| `firmware/` | Atom Lite 用ファームウェア（PlatformIO / Arduino） |
| `server/` | サーバ側アプリケーション（未実装） |
| `schema/` | デバイスとサーバ間で共有するメッセージスキーマ（未作成） |

## firmware のビルド

```sh
cd firmware
cp src/DefineConfig.h.sample src/DefineConfig.h   # 接続先・証明書を記入する（git 管理外）
pio run
```
