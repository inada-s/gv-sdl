gvsdl 汎用ビジュアライザ SDL ver.
-----

# 概要
colun氏作の汎用ビジュアライザ https://github.com/colun/gvc の API を丸パクリして SDL(OpenGL) で動かせるようにした物.

ビジュアライズを行いたいプログラムに直接組み込み, OpenGLで描画するため高速です.

# 注意
gv.hpp をビジュアライズを行いたいプロジェクトにコピーして使い捨ててください.  
互換性を壊す変更を躊躇なく行う可能性があります. 

# 環境構築
- OpenGL
- [SDL2](https://www.libsdl.org/)
- [SDL_ttf 2](https://www.libsdl.org/projects/SDL_ttf/)

# ビルド
## MacOSX command line
```
g++ -std=c++11 -DENABLE_GV $(sdl2-config --cflags --libs) -lSDL2_ttf -framework OpenGL main.cpp 
```

## MacOSX Xcode
- Add `Other Linker Flags` `-lSDL2`
- Add `Library Search Paths` `/usr/local/lib`
- Add `Header Search Paths` `/usr/local/include/SDL2` and `/usr/X11R6/include`

# リファレンス
## 設定
- `gv.font_path(const char* s)` ttfフォントのパスを設定します.
- `gv.default_alpha(uint8_t a)` デフォルトの透明度を設定します.
- `gv.enabled(bool b)` 有効無効を設定します. オプションでビジュアライズしたい時に使います.

## 実行
- `gv.RunMainThread(std::function<void()> f)` ウインドウをメインスレッドで動かします. fが別スレッドで呼ばれます.
- `gv.RunSubThread()` ウインドウを別スレッドで動かします.

## 描画
- `gv.NewTime()` 新しいページを描きます.
- `gv.Line(double x1, double y1, double x2, double y2, double r, GvColor color)` (x1,y1)から(x2,y2)に線を引きます.
- `gv.Arrow(double x1, double y1, double x2, double y2, double r, GvColor color)` (x1,y1)から(x2,y2)矢印付きの線を引きます.
- `gv.Rect(double x, double y, double w, double h, GvColor color)` (x,y)を左上して 幅w 高さh の四角形を描きます.
- `gv.Circle(double x, double y, double r, bool filled, GvColor color)` (x,y)を中心いして半径rの円を描きます.
- `gv.Text(double x, double y, double r, GvColor color, const char* format = "?", ...)` (x,y)を中心に大きさrの文字を描きます.
