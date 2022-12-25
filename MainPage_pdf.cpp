#include "pch.h"
#include "zlib.h"
#include "MainPage.h"

using namespace::Zlib::implementation;

// PDF フォーマット
// https://aznote.jakou.com/prog/pdf/index.html
// 詳細PDF入門 ー 実装して学ぼう！PDFファイルの構造とその書き方読み方
// https://itchyny.hatenablog.com/entry/2015/09/16/100000
// PDFから「使える」テキストを取り出す（第1回）
// https://golden-lucky.hatenablog.com/entry/2019/12/01/001701
// PDF 構文解説
// https://www.pdf-tools.trustss.co.jp/Syntax/parsePdfProc.html#proc
// 見て作って学ぶ、PDFファイルの基本構造
// https://techracho.bpsinc.jp/west/2018_12_07/65062
// グリフとグリフの実行
// https://learn.microsoft.com/ja-JP/windows/win32/directwrite/glyphs-and-glyph-runs

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	using winrt::Windows::Storage::FileAccessMode;
	using winrt::Windows::Storage::CachedFileManager;
	using winrt::Windows::Storage::Provider::FileUpdateStatus;

}

// グリフにおける座標値や幅を指定する場合、PDF 内では、常に 1 em = 1000 であるものとして、
// 値を設定します。実際のフォントで 1 em = 1024 などとなっている場合は、n / 1024 * 1000 
// というようにして、値を 1 em = 1000 に合わせます.
// 
// PDF の FontBBox (Black Box) のイメージ
//     y
//     ^
//     |
//   +-+--------+
//   | |   /\   |
//   | |  /__\  |
//   | | /    \ |
// --+-+--------+--> x
//   | |        |
//   +-+--------+
//     |