# PRJ_PLUS
シューティングゲーム作成実験3 plustaker2 読み物 By m@3

This software includes code generated with the assistance of [Gemini & Grok], an AI developed by [Google & xAI].

スプライトが基本8枚しか出ない機種向けに作成したゲームです。降ってくる四角をひたすら撃ち落として出てくるキャラを回収します。連続回収するとコンボが増えていきます。

・AMIGA版

AMIGA OS GCCでコンパイルしてください。gadfも必要です。

・C64版

llvm-cmosでコンパイルしてください。

・MSX1版

z88dkでコンパイルしてください。

・X68000版

elf2x68kでコンパイルしてください。実行には.SC5ファイルと.PCMファイルが必要です。

・FM TOWNS版

FM TOWNS-gcc クロスコンパイル環境でコンパイルしてください。実行には.SC5ファイルと.SNDファイルが必要です。

・MSX2版

z88dkでコンパイルしてください。実行には.MSXファイルと.SC5ファイルが必要です。DOS版は.SC5ファイルと.BATファイルとBLOAD.COMが必要です。

・PC-88VA版

OpenWatcomでコンパイルしてください。実行には.SC5ファイルが必要です。PCEPAT.SYSを組み込んでください。

・PC-8801mkIISR版/X1版

z88dkでコンパイルしてください。直接.d88を生成します。

・FM77AV版

gcc6809(4.3.6branch)でコンパイルして下さい。直接.d77を生成します。メモリがギリギリなので不具合が出るかもしれません。

実機での動作は今のところ未検証です。

Pyxel版もWebの方にあります。

WebMSX:
https://webmsx.org/cbios/?MACHINE=MSX1J&rom=https://github.com/mazone-ma3/PRJ_PLUS/raw/refs/heads/main/MSX1/BIN/plustakermsx.rom

<img width="2880" height="1920" alt="タイトルなし_" src="https://github.com/user-attachments/assets/a100e02f-6d43-40f3-bba3-9aeeaf21ec40" />





















