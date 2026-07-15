# Qt JJY Simulator

`web-jjy` を基にした、Windows と Android 向けの Qt 6 / C++ 実装です。Qt Quick は画面表示、C++ はJJYフレーム生成と音声PCM生成を担当します。

## 仕様

- ローカル時刻の次の秒境界に送信を開始します。開始秒に対応するJJYフレーム位置から出力するため、受信機は次の分境界まで感度調整できます。
- JJYの各秒を、マーカー `0.2秒`、ビット1 `0.5秒`、ビット0 `0.8秒` の高振幅区間で表します。
- 送信方式は開始前に選択できます。既定の「従来方式」は13.333 kHzの矩形波を断続出力し、「Time Station方式」は13.333 kHzの連続正弦波を、時刻コードに従って高振幅と-10 dBの間で変調します。
- OSの時刻を使います。端末側で自動日時設定（NTP同期）を有効にしてください。
- うるう秒の予告ビットは未実装で、常に「なし」を送信します。

> 実際に電波時計を同期できるかは、端末のDAC、イヤホン出力、ケーブル、アンプおよび時計の受信性能に左右されます。Bluetoothは使わず、有線出力で検証してください。

## Windowsでのビルド

Qt 6.11.1 MinGW の例です。

```powershell
cmake -S . -B build-windows -G Ninja `
  -DCMAKE_PREFIX_PATH=C:/Qt/6.11.1/mingw_64
cmake --build build-windows
windeployqt build-windows/QtJjySimulator.exe
```

Qt Creatorでは `CMakeLists.txt` を開き、Desktop Qt 6.11.1 MinGW 64-bit または MSVC 2022 64-bit Kitを選択します。

## Androidでのビルド

1. Qt Creator で Android SDK、NDK、JDK を設定します。
2. この `CMakeLists.txt` を開き、`Android Qt 6.11.1 arm64-v8a` Kitを選択します。
3. 実機へ実行して音声出力を確認します。
4. 配布物は Qt Creator の「Build Android APK」または「Build Android AAB」で生成します。

通常の音声再生だけならAndroid固有の権限は不要です。独自NTP同期やバックグラウンド送信を追加する場合は、Android固有の実装・権限を別途追加してください。
