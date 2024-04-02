# DIY-keypad64

![photo1](https://github.com/nicotakuya/diy-keypad64/assets/5597377/28d7063e-c62c-4678-b45f-80d20d4d7d06)

## Overview

USB keyboard.

開発途中。

## Files

・diykeypad64.ino : ソースファイル。開発環境はArduino IDE(Arduino-Pico)です。

・8x8font.h : フォントデータ。https://github.com/nicotakuya/8pixelfont

・diykeypad64_schematics.png : 回路図。

・STL_file : 3Dプリンタ用データ。
![stl_file](https://github.com/nicotakuya/diy-keypad64/assets/5597377/2651c217-1ef1-43aa-9d04-8b4432c38859)

・DIYKEYPAD64.COMP : CADLUS X用プリント基板CADデータ。
![diykeypad64_pcb](https://github.com/nicotakuya/diy-keypad64/assets/5597377/92ef58e3-59f5-44e1-a5af-83902d08caf4)

## Parts

・U1 : Raspberry Pi Pico。

・U2 : 1.77inch TFT LCD(AE-ATM0177B3A)。

・U3 : Logic IC 74HC164。

・D1-D8 : 小信号ダイオード。

・D9 : 整流用ショットキーバリアダイオード。電池駆動させたい場合に使用します。

・SW1-64 : タクトスイッチ。

## How to build

このプログラムではKeyboardライブラリのsendReport関数を使ってキーコードを直接送信することで日本語のレイアウトに対応させています。
そのため、"Keyboard.h"を自前で書き換えて、sendReport関数をprivateからpublicに変更する必要があります。

![change_headerfile](https://github.com/nicotakuya/diy-keypad64/assets/5597377/d71338d0-ba68-4275-a02b-77843d085776)

Windowsの場合、"Keyboard.h"は以下のディレクトリに存在します。

C:\Users\(ユーザー名)\AppData\Local\Arduinoxx\packages\rp2040\hardware\rp2040\x.x.x\libraries\Keyboard\src\Keyboard.h

C:\Users\(ユーザー名)\AppData\Local\Arduinoxx\libraries\Keyboard\src\Keyboard.h

C:\Users\(ユーザー名)\Documents\Arduino\libraries\Keyboard\src\Keyboard.h

![keyboard_layout](https://github.com/nicotakuya/diy-keypad64/assets/5597377/69536995-fca8-4afb-ad20-3e140c0677e8)

ソース内のJAPANESEの値を1にするとJPレイアウト。0にするとUSレイアウトにできます。
現時点で基板のシルク印刷はJPレイアウトだけ存在します。
USレイアウトの場合、紙を貼って修正する必要があります。

## Movie

https://www.youtube.com/watch?v=7XjSDxZpHL8
