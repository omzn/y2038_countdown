# 2038年問題カウントダウンクロック

https://youtu.be/E-os8XCysBw

2038年1月19日3時14分7秒(UTC)にUNIX時間が符号つき32ビット整数では0x7fffffff秒になり，
次の瞬間オーバーフローしてしまう問題があります．これを2038年問題と呼びます．
この問題は2000年問題よりも深刻になる可能性を秘めており，適切な対策が必要です．

このデバイスでは，2038年問題発生までの秒数をカウントダウンします．
できるだけ正確を期すため，バックアップ電池内蔵のRTC(DS1307)で時刻を保持しつつ，
WiFi経由でNTPサーバに定期的に問い合わせをしながら時刻を補正し続けます．

なお，プログラム内ではUNIX時間を`uint32_t`で扱っていますが，カウントダウンにバグがあり，2038年問題は*発生します*．

```cpp
      if (dispmode == MODE_UNIXTIME) {
        dispDigits(now.unixtime()-SECONDS_UTC_TO_JST);
      } else if (dispmode == MODE_COUNTDOWN) {
        dispDigits((0x7FFFFFFF - (now.unixtime()-SECONDS_UTC_TO_JST))); // y 2038!
      } else if (dispmode == MODE_CLOCK) {
        dispClock(now);
      } else if (dispmode == MODE_DATE) {
        dispDate(now);
      }
```

https://youtu.be/hJBHb5ON73I

問題を発生させたくない場合は，`dispDigits()`の内部を`(int32_t)`でキャストすればOKです．

https://youtu.be/Mgxpzdlathw

## 材料

* マイコン: ESP-WROOM-02
* LED制御IC: HT16K33
* 7セグメントx4: OSL40562-IB x 2 (アノードコモン)
* 7セグメントx1: OSL10561-IB x 2 (アノードコモン)
* RTC: DS1307
* 32kHz Crystal
* タクトスイッチ（縦型）
* CR2032電池ホルダ
* PCA9306 I2Cレベル変換
* 自作PCB

## ハードウェア

単純に7セグメントLEDを10桁表示させるデバイスです．

HT16K33はLEDドライバで，アノード16本，カソード8本を持ち，8x8マトリクス
を2個まで同時に制御できます．このドライバを使って7セグメントLEDを10桁
駆動させるには，アノードコモンのLEDを準備します．数字のどの部品を表示
させるのかを指定するのに`C0`-`C8`を利用し，どの桁を表示させるのかの指
定に`A0`-`A9`を利用します．

10桁出せる7セグLEDは見たことがないので，4桁を2つ，1桁を2つ組み合わせて
使います．
スイッチはモード（カウントダウン，UNIX時刻表示，通常時刻表示）の切り替え用です．

一応時計なので，バッテリーバックアップ付のRTCを使います．使うのは定番
のDS1307です．

## プログラム

プログラムからのLED制御には，AdafruitのHT16K33 Arduinoライブラリ
(`Adafruit_LEDBackpack`)を利用します．ただし，このドライバはカソードコ
モンのLEDを前提に作られていますので，そのままでは利用できません．
そのため，これを継承したクラス(`Anodecommon_7seg`)を作成し，
アノードコモン用と10桁用の改変部分は全てこちらに記述します．

初期起動時は，WiFiネットワークに参加できないので，自身がアクセスポイン
トになってCC2038という名前のAP（アクセスポイント）に接続するように促さ
れます．接続するとCaptive Webページで，参加するWiFiの設定をしますと，
次回起動時はWiFiに接続します．

ntpサーバに接続すると，時計を自動的に合わせて，動作を開始します．
上でも述べたように，意図的に2038年問題に対処していないので，2038年には動作が乱れるところが観察できます．

簡易httpサーバになっていますので，REST APIもどきが使えます．

| API |  値域 |   効果         |
|-----|-------|----------------|
| mode| int   | 動作モード変更  |
| status| --  | 状況を知る     |

```
$ curl http://esp_7seg.local/status

$ curl "http://esp_7seg.local/mode?0"
```

## GitHub

http://github.com/omzn/y2038_countdown
