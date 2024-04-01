# DIY-keypad64

![photo1](https://github.com/nicotakuya/diy-keypad64/assets/5597377/28d7063e-c62c-4678-b45f-80d20d4d7d06)

Japanese layout USB keyboard.

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

## Bugs

以下のキーが正しく入力されません。

^→&

@→'

:→+

]→[

[→@

\→]

`→無反応

SHIFT and @→"

SHIFT and :→+

SHIFT and ;→+

SHIFT and ^→&

SHIFT and \→}

SHIFT and [→`

SHIFT and ]→{

## Movie

https://www.youtube.com/watch?v=7XjSDxZpHL8
