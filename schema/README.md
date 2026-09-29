# schema

firmware と server の間で共有するメッセージスキーマ。ここにある定義が両者の契約の唯一の正とする。

## timeline.schema.json

サーバーが生成し、デバイスが再生する演出タイムライン（JSON Schema draft 2020-12）。

| フィールド | 意味 |
|---|---|
| `version` | スキーマのバージョン。`1` 固定 |
| `id` | サーバーが発行する識別子。デバイスは status でこの値をそのまま返す |
| `source` | 再現する場所（`place`, `lat`, `lon`, `tz`）と期間（`start`, `end`。タイムゾーン付き ISO 8601） |
| `speed` | 実時間に対する再生倍率（1 以上） |
| `loop` | 最後の frame の後に先頭へ戻るか |
| `frames` | 1〜200 件の frame |

各 frame は `t`（`source.start` からの経過秒、整数）と、任意のチャンネルを持つ。
デバイスは実時間で `t / speed` 秒後にその frame に到達する。

| チャンネル | 値 |
|---|---|
| `sky` | 空の色 `[r, g, b]`、各 0〜255 |
| `bri` | 明るさ 0〜100 |
| `sun` | 日差し 0〜100 |
| `mist` | 霧 0〜100 |
| `rain` | 雨 0〜100 |
| `event` | `sunrise` / `sunset` / `lightning` |

## スキーマで表現できない制約

以下は JSON Schema では表現できないため、スキーマ検証に加えて別に検査する（`timeline.test.js` が実装を兼ねる）。

- `frames[0].t` は `0`
- `t` は frame ごとに単調増加する（同じ値も不可）

## 値の保持規則

- タイムライン開始時点（`t=0` の直前）の状態は全チャンネル `0`（`sky` は `[0, 0, 0]`）
- frame で省略されたチャンネルは直前の値を保持する
- 値は次にそのチャンネルを指定した frame まで変化しない（frame 間を補間しない）
- `event` は保持しない点イベント。`frame.t` を初めて通過した瞬間に 1 回だけ発火する
- `event` は値チャンネルの保持状態を変更しない。演出（`lightning` 等）が終わったら、その時点で保持中の値に戻る
- 途中再開・スキップで `frame.t` を飛び越した場合、値チャンネルは保持規則で再構成するが、飛び越した `event` は発火しない

## 検証

```sh
cd schema
npm ci
npm test
```

`fixtures/valid/` の各ファイルが受理され、`fixtures/invalid/` の各ファイルが想定した理由で拒否されることを確認する。CI（`.github/workflows/schema.yml`）でも同じコマンドを実行する。
