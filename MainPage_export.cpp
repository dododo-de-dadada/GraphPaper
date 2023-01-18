//
// MainPage_export.cpp
// PDF または SVG にエクスポートする
//

// PDF にインポート時の制限
// 不透明度が反映しない
// 文字列を GID で出力, 埋め込みフォントはなし (Acrobat のみ文字表示)
// 
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
// cmap — Character to Glyph Index Mapping Table
// https://learn.microsoft.com/en-us/typography/opentype/spec/cmap

#include "pch.h"
#include <shcore.h>
#include "MainPage.h"
#include "zlib.h"
#include "CMap.h"

using namespace winrt;
using namespace::Zlib::implementation;
using namespace::CMap::implementation;

namespace winrt::GraphPaper::implementation
{
	using winrt::Windows::Storage::CachedFileManager;
	using winrt::Windows::Storage::FileAccessMode;
	using winrt::Windows::Storage::Provider::FileUpdateStatus;
	using winrt::Windows::Storage::Streams::InMemoryRandomAccessStream;
	using winrt::Windows::Storage::Streams::Buffer;
	using winrt::Windows::Storage::Streams::InputStreamOptions;

	static size_t export_pdf_font_dict(const int o_num, const DWRITE_FONT_FACE_TYPE f_type, const winrt::hstring& p_name, const DataWriter& dt_writer);
	static size_t export_pdf_descendant_font_dict(int o_num, DWRITE_FONT_FACE_TYPE f_type, const winrt::hstring& p_name, const size_t cid_len, const uint16_t* cid_arr, const DWRITE_GLYPH_METRICS* g_met, const int upem, const DataWriter& dt_writer);
	static size_t export_pdf_font_descriptor(const int obj_num, const winrt::hstring& p_name, const DWRITE_FONT_STRETCH stretch, const DWRITE_FONT_WEIGHT weight, const float angle, const DWRITE_FONT_METRICS1& f_met, const DataWriter& dt_writer);
	static void export_pdf_font_info(IDWriteFontFace3* face, const wchar_t* t, const size_t t_len, const wchar_t* family, DWRITE_FONT_METRICS1& f_met, DWRITE_FONT_FACE_TYPE& f_type, winrt::hstring& p_name, std::vector<uint16_t>& cid, std::vector<uint16_t>& gid, std::vector<DWRITE_GLYPH_METRICS>& g_met, FLOAT& angle);

	//------------------------------
	// PDF のフォント辞書を出力する.
	// o_num	オブジェクト番号
	// f_type	フォントフェイスの種類
	// p_name	ポストスクリプト名
	// dt_writer	出力先
	//------------------------------
	static size_t export_pdf_font_dict(const int o_num, const DWRITE_FONT_FACE_TYPE f_type, const winrt::hstring& p_name, const DataWriter& dt_writer)
	{
		wchar_t buf[1024];
		swprintf_s(buf,
			L"%% Font Dictionary\n"
			L"%d 0 obj <<\n"
			L"/Type /Font\n"
			L"/Subtype /Type0\n"
			L"/BaseFont /%s\n"
			//L"/Encoding /UniJIS-UTF16-H\n"
			L"/Encoding /Identity-H\n"
			L"/DescendantFonts [%d 0 R]\n"
			L">>\n",
			o_num,
			//std::data(p_name),
			// OpenType の場合 ポストスクリプト名+'-'+CMap 名で CIDFontType0
			f_type == DWRITE_FONT_FACE_TYPE_TRUETYPE ?
			//std::data(p_name) : std::data(p_name + L"-UniJIS-UTF16-H"),
			std::data(p_name) : std::data(p_name + L"-Identity-H"),
			o_num + 1
		);
		return dt_writer.WriteString(buf);
	}

	//------------------------------
	// PDF の子孫フォント辞書を出力する.
	// o_num	オブジェクト番号
	// f_type	フォントフェイスの種類
	// p_name	書体のポストスクリプト名
	// cid_len	CID 配列の大きさ
	// cid_arr	CID 配列
	// g_met	字形 (グリフ) の計量
	// g_unit	計量の値の単位
	// dt_writer	出力先
	//------------------------------
	static size_t export_pdf_descendant_font_dict(const int o_num, const DWRITE_FONT_FACE_TYPE f_type, const winrt::hstring& p_name, const size_t cid_len, const uint16_t* cid_arr, const DWRITE_GLYPH_METRICS* g_met, const int g_unit, const DataWriter& dt_writer)
	{
		wchar_t buf[1024];
		swprintf_s(buf,
			L"%% Descendant Font Dictionary\n"
			L"%d 0 obj <<\n"
			L"/Type /Font\n"
			L"/Subtype /%s\n"
			L"/BaseFont /%s\n"
			L"/CIDSystemInfo <<\n"
			L"/Registry (Adobe)\n"
			//L"/Ordering (Japan1)\n"
			//L"/Supplement 7\n"
			L"/Ordering (Identity)\n"
			L"/Supplement 0\n"
			L">>\n"
			L"/FontDescriptor %d 0 R\n",	// 間接参照で必須.
			o_num,
			f_type == DWRITE_FONT_FACE_TYPE_TRUETYPE ?
			L"CIDFontType2" : L"CIDFontType0",
			std::data(p_name),
			o_num + 1
		);
		size_t len = dt_writer.WriteString(buf);
		// 半角のグリフ幅を設定する
		// 設定しなければ全て全角幅になってしまう.
		len += dt_writer.WriteString(L"/W [\n");	// CID 開始番号は 1 (半角空白)
		for (int i = 0; i < cid_len; i++) {
			swprintf_s(buf,
				L"%u %u %u\n",
				cid_arr[i], cid_arr[i],
				1000 * g_met[i].advanceWidth / g_unit);
			len += dt_writer.WriteString(buf);
		}
		len += dt_writer.WriteString(L"]\n");
		len += dt_writer.WriteString(
			L">>\n"
			L"endobj\n");
		return len;
	}

	static size_t export_pdf_font_descriptor(const int obj_num, const winrt::hstring& p_name,
		const DWRITE_FONT_STRETCH stretch, const DWRITE_FONT_WEIGHT weight, const float angle, const DWRITE_FONT_METRICS1& f_met, const DataWriter& dt_writer)
	{
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
		const int f_unit = f_met.designUnitsPerEm;
		constexpr const wchar_t* FONT_STRETCH_NAME[] = {
			L"Normal",
			L"UltraCondensed",
			L"ExtraCondensed",
			L"Condensed",
			L"SemiCondensed",
			L"Normal",
			L"SemiExpanded",
			L"Expanded",
			L"ExtraExpanded",
			L"UltraExpanded"
		};
		wchar_t buf[1024];
		swprintf_s(buf,
			L"%% Font Descriptor Dictionary\n"
			L"%d 0 obj <<\n"
			L"/Type /FontDescriptor\n"
			L"/FontName /%s\n"
			L"/FontStretch /%s\n"
			L"/FontWeight /%d\n"
			L"/Flags 4\n"
			L"/FontBBox [%d %d %d %d]\n"
			L"/ItalicAngle %d\n"
			L"/Ascent %u\n"
			L"/Descent %u\n"
			L"/CapHeight %u\n"
			L"/StemV 0\n"
			L">>\n"
			L"endobj\n",
			obj_num,
			std::data(p_name),
			FONT_STRETCH_NAME[stretch < 10 ? stretch : 0],
			weight <= 900 ? weight : 900,
			1000 * f_met.glyphBoxLeft / f_unit,
			(1000 * f_met.glyphBoxBottom / f_unit),
			1000 * f_met.glyphBoxRight / f_unit,
			(1000 * f_met.glyphBoxTop / f_unit),
			static_cast<int>(std::ceil(angle)),
			1000 * f_met.ascent / f_unit,
			1000 * f_met.descent / f_unit,
			1000 * f_met.capHeight / f_unit
		);
		return dt_writer.WriteString(buf);
	}

	//------------------------------
	// 文字列図形からフォントの情報を得る.
	// s	文字列図形
	// weight	書体の太さ
	// stretch	書体の幅
	// style	字体
	// f_met	書体の計量
	// f_type	フォントフェイスの形式
	// p_name	書体のポストスクリプト名
	// g_met	字形の計量
	// angle	イタリックの最大角度 (ふつうがマイナス)
	//------------------------------
	static void export_pdf_font_info(
		IDWriteFontFace3* face,
		const wchar_t *t,
		const size_t t_len,
		const wchar_t *family,
		//const DWRITE_FONT_WEIGHT weight,
		//const DWRITE_FONT_STRETCH stretch,
		//const DWRITE_FONT_STYLE style,
		DWRITE_FONT_METRICS1& f_met,
		DWRITE_FONT_FACE_TYPE& f_type,
		winrt::hstring& p_name,
		std::vector<uint16_t>& cid,
		std::vector<uint16_t>& gid,
		std::vector<DWRITE_GLYPH_METRICS>& g_met,
		FLOAT& angle
	)
	{
		// 書体の計量を得る.
		face->GetMetrics(&f_met);

		// フォントフェイスの形式を得る.
		f_type = face->GetType();

		// 文字列を UTF-32 に変換する.
		std::vector<uint32_t> utf32{ conv_utf16_to_utf32(t, t_len) };

		// UTF-32 文字列から重複したコードを削除する.
		const auto last = std::unique(std::begin(utf32), std::end(utf32));
		utf32.erase(last, utf32.end());

		// UTF32 文字列から CID を得る.
		//cid.resize(utf32.size());
		//for (int i = 0; i < utf32.size(); i++) {
		//	cid[i] = cmap_getcid(utf32[i]);
		//}

		// UTF32 文字列から GID を得る.
		gid.resize(utf32.size());
		winrt::check_hresult(face->GetGlyphIndices(std::data(utf32), static_cast<UINT32>(std::size(utf32)), std::data(gid)));

		// GID 文字列から字形 (グリフ) の計量を得る.
		g_met.resize(utf32.size());
		face->GetDesignGlyphMetrics(std::data(gid), static_cast<UINT32>(std::size(gid)), std::data(g_met));
		//face->Release();

		// フォントフェイスからポストスクリプト名を得る.
		IDWriteLocalizedStrings* str = nullptr;
		BOOL exists = false;
		//if (font->GetInformationalStrings(DWRITE_INFORMATIONAL_STRING_POSTSCRIPT_CID_NAME, &str, &exists) == S_OK) {
		if (face->GetInformationalStrings(DWRITE_INFORMATIONAL_STRING_POSTSCRIPT_NAME, &str, &exists) == S_OK) {
			if (exists) {
				const UINT32 str_cnt = str->GetCount();
				for (uint32_t j = 0; j < str_cnt; j++) {
					UINT32 wstr_len = 0;	// 終端ヌルを除く文字列長
					if (str->GetStringLength(j, &wstr_len) == S_OK && wstr_len > 0) {
						std::vector<wchar_t> wstr(wstr_len + 1);
						if (str->GetString(j, std::data(wstr), wstr_len + 1) == S_OK) {
							p_name.clear();
							p_name = std::data(wstr);
							//std::copy(wstr.begin(), wstr.end(), std::back_inserter(p_name));
							break;
						}
					}
				}
				str->Release();
			}
			// たとえば 'Windows Himaraya' のとき,
			// ポストスクリプト名を要求したにもかかわらず空白の入った文字列を返す場合がある.
			// ポストスクリプト名が存在しない, あるいは上記のような場合は,
			// 書体名から空白文字を取り除いた文字列を, ポストスクリプト名とする.
			if (!exists || std::find(std::begin(p_name), std::end(p_name), L' ') != std::end(p_name)) {
				const auto k = wchar_len(family);
				std::vector<wchar_t> wstr(k + 1);
				int wstr_len = 0;
				for (uint32_t i = 0; i < k; i++) {
					if (!iswspace(family[i])) {
						wstr[wstr_len++] = family[i];
					}
				}
				wstr[wstr_len] = L'\0';
				p_name.clear();
				p_name = std::data(wstr);
			}
		}

		// フォントフェイスから傾きを得る.
		// ただし, PDF は, フォントフェイスの変形はサポートしてないので, フェイスそのものが
		// イタリックでない限り, 斜体にはならない.
		const auto axis_cnt = static_cast<IDWriteFontFace5*>(face)->GetFontAxisValueCount();
		std::vector<DWRITE_FONT_AXIS_VALUE> axis_val(axis_cnt);
		if (static_cast<IDWriteFontFace5*>(face)->GetFontAxisValues(std::data(axis_val), axis_cnt) == S_OK) {
			for (uint32_t i = 0; i < axis_cnt; i++) {
				if (axis_val[i].axisTag == DWRITE_FONT_AXIS_TAG_SLANT) {
					angle = axis_val[i].value;
				}
			}
		}
	}

	// 画像を PDF の XObject として書き出す.
	static size_t export_pdf_image(const uint8_t* bgra, const size_t width, const size_t height, const int obj_num, DataWriter& dt_writer)
	{
		// 画像 XObject (ストリームオブジェクト)
		/*
		InMemoryRandomAccessStream image_stream{};
		co_await t->copy<true>(BitmapEncoder::JpegEncoderId(), image_stream);
		const auto image_len = static_cast<uint32_t>(image_stream.Size());
		Buffer image_buf(image_len);
		co_await image_stream.ReadAsync(image_buf, image_len, InputStreamOptions::None);
		//co_await image_stream.FlushAsync();
		image_stream.Close();
		image_stream = nullptr;
		*/

		// PDF はアルファ値をサポートしておらず, 逆に WIC ビットマップは 3 バイトピクセルを
		// サポートしていない.
		// BGRA を RGB にコピー.
		std::vector<uint8_t> z_buf;
		std::vector<uint8_t> in_buf(3ull * width * height);
		//std::vector<uint8_t> in_buf(3ull * t->m_orig.width * t->m_orig.height);
		for (size_t i = 0; i < width * height; i++) {
			in_buf[3 * i + 2] = bgra[4 * i + 0];	// B
			in_buf[3 * i + 1] = bgra[4 * i + 1];	// G
			in_buf[3 * i + 0] = bgra[4 * i + 2];	// R
		}
		z_compress(z_buf, std::data(in_buf), std::size(in_buf));
		in_buf.clear();
		in_buf.shrink_to_fit();
	
		wchar_t buf[1024];
		swprintf_s(buf,
			L"%% XObject (Image)\n"
			L"%d 0 obj <<\n"
			L"/Type /XObject\n"
			L"/Subtype /Image\n"
			L"/Width %zu\n"
			L"/Height %zu\n"
			L"/Length %zu\n"
			L"/ColorSpace /DeviceRGB\n"
			L"/BitsPerComponent 8\n"
			L"/Filter /FlateDecode\n"
			L">>\n",
			obj_num,
			width,
			height,
			z_buf.size()
		);
		size_t len = dt_writer.WriteString(buf);
		/*
		len += dt_writer.WriteString(L"stream\n");
		dt_writer.WriteBuffer(image_buf);
		len += image_len;
		image_buf = nullptr;
		*/
		len += dt_writer.WriteString(L"stream\n");
		dt_writer.WriteBytes(z_buf);
		len += z_buf.size();
		z_buf.clear();
		z_buf.shrink_to_fit();
		/*
		for (uint32_t y = 0; y < t->m_orig.height; y++) {
			if (y > 0) {
				len += dt_writer.WriteString(L"\n");
			}
			for (uint32_t x = 0; x < t->m_orig.width; x++) {
				// BGRA -> RGB
				swprintf_s(buf,
					L"%02x%02x%02x",
					t->m_bgra[y * 4 * t->m_orig.width + 4 * x + 2],
					t->m_bgra[y * 4 * t->m_orig.width + 4 * x + 1],
					t->m_bgra[y * 4 * t->m_orig.width + 4 * x + 0]
				);
				len += dt_writer.WriteString(buf);
			}
		}
		*/
		// ストリームの最後は '>'.
		len += dt_writer.WriteString(
			L">\n"
			L"endstream\n"
			L"endobj\n");
		return len;
	}

	// 図形をデータライターに PDF として書き込む.
	IAsyncOperation<winrt::hresult> MainPage::export_as_pdf_async(const StorageFile& pdf_file) noexcept
	{
		HRESULT hres = E_FAIL;
		try {
			wchar_t buf[1024];	// PDF

			// ページの幅と高さを, D2D の固定 DPI (96dpi) から PDF の 72dpi に,
			// 変換する (モニターに応じて変化する論理 DPI は用いない). 
			const float w_pt = m_main_page.m_page_size.width * 72.0f / 96.0f;	// ポイントに変換された幅
			const float h_pt = m_main_page.m_page_size.height * 72.0f / 96.0f;	// ポイントに変換された高さ

			// ストレージファイルを開いて, ストリームとそのデータライターを得る.
			const IRandomAccessStream& pdf_stream{
				co_await pdf_file.OpenAsync(FileAccessMode::ReadWrite)
			};
			DataWriter dt_writer{
				DataWriter(pdf_stream.GetOutputStreamAt(0))
			};

			// PDF ヘッダー
			// ファイルにバイナリデータが含まれる場合は,
			// ヘッダと改行の直後に, 4 つ以上のバイナリ文字を含むコメント行を置くことが推奨.
			size_t len = dt_writer.WriteString(
				L"%PDF-1.7\n"
				L"%ああ\n"
			);
			std::vector<size_t> obj_len{};
			obj_len.push_back(len);

			// カタログ辞書.
			// トレーラーから参照され,
			// ページツリーを参照する.
			len = dt_writer.WriteString(
				L"% Catalog Dictionary\n"
				L"1 0 obj <<\n"
				L"/Type /Catalog\n"
				L"/Pages 2 0 R\n"
				L">>\n"
				L"endobj\n");// , dt_writer);
			obj_len.push_back(obj_len.back() + len);

			// ページツリー辞書
			// カタログから参照され,
			// ページを参照する.
			len = dt_writer.WriteString(
				L"% Page Tree Dictionary\n"
				L"2 0 obj <<\n"
				L"/Type /Pages\n"
				L"/Count 1\n"
				L"/Kids[3 0 R]\n"
				L">>\n"
				L"endobj\n");
			obj_len.push_back(obj_len.back() + len);

			// ページオブジェクト
			// ページツリーから参照され,
			// リソースとコンテンツを参照する.
			swprintf_s(buf,
				L"%% Page Object\n"
				L"3 0 obj <<\n"
				L"/Type /Page\n"
				L"/MediaBox [0 0 %f %f]\n"
				L"/Parent 2 0 R\n"
				L"/Resources 4 0 R\n"
				L"/Contents [5 0 R]\n"
				L">>\n"
				L"endobj\n",
				w_pt, h_pt);
			len = dt_writer.WriteString(buf);
			obj_len.push_back(obj_len.back() + len);

			// リソース辞書
			// ページとコンテンツから参照され, 辞書を参照する.
			len = dt_writer.WriteString(
				L"% Resouces Dictionary\n"
				L"4 0 obj <<\n");
			// フォント
			int text_cnt = 0;	// 文字列オブジェクトの計数
			std::vector<std::vector<char>> base_font;	// ベースフォント名
			for (const auto s : m_main_page.m_shape_list) {
				if (s->is_deleted()) {
					continue;
				}
				if (typeid(*s) == typeid(ShapeText)) {
					if (text_cnt == 0) {
						len += dt_writer.WriteString(L"/Font <<\n");
					}
					swprintf_s(buf,
						L"/Font%d %d 0 R\n",
						text_cnt, 6 + 3 * text_cnt
					);
					len += dt_writer.WriteString(buf);
					static_cast<ShapeText*>(s)->m_pdf_text_cnt = text_cnt++;
				}
				else if (typeid(*s) == typeid(ShapeRuler)) {
					if (text_cnt == 0) {
						len += dt_writer.WriteString(L"/Font <<\n");
					}
					swprintf_s(buf,
						L"/Font%d %d 0 R\n",
						text_cnt, 6 + 3 * text_cnt
					);
					len += dt_writer.WriteString(buf);
					static_cast<ShapeRuler*>(s)->m_pdf_text_cnt = text_cnt++;
				}
			}
			if (text_cnt > 0) {
				len += dt_writer.WriteString(L">>\n");
			}
			// 画像
			int image_cnt = 0;
			for (const auto s : m_main_page.m_shape_list) {
				if (s->is_deleted()) {
					continue;
				}
				if (typeid(*s) == typeid(ShapeImage)) {
					if (image_cnt == 0) {
						len += dt_writer.WriteString(L"/XObject <<\n");
					}
					swprintf_s(buf,
						L"/Image%d %d 0 R\n",
						image_cnt, 6 + 3 * text_cnt + image_cnt
					);
					len += dt_writer.WriteString(buf);
					static_cast<ShapeImage*>(s)->m_pdf_image_cnt = image_cnt++;
				}
			}
			if (image_cnt > 0) {
				len += dt_writer.WriteString(L">>\n");
			}
			len += dt_writer.WriteString(
				L">>\n"
				L"endobj\n");
			obj_len.push_back(obj_len.back() + len);

			// コンテントオブジェクト (ストリーム)
			// 変換行列に 72dpi / 96dpi (=0.75) を指定する.   
			swprintf_s(buf,
				L"%% Content Object\n"
				L"5 0 obj <<\n"
				L">>\n"
				L"stream\n"
				L"%f 0 0 %f 0 0 cm\n"
				L"q\n",
				72.0f / 96.0f, 72.0f / 96.0f
			);
			len = dt_writer.WriteString(buf);

			// 背景
			swprintf_s(buf,
				L"%f %f %f rg\n"
				L"0 0 %f %f re\n"
				L"b\n",
				m_main_page.m_page_color.r,
				m_main_page.m_page_color.g,
				m_main_page.m_page_color.b,
				m_main_page.m_page_size.width,
				m_main_page.m_page_size.height
			);
			len += dt_writer.WriteString(buf);

			// 図形を出力
			const D2D1_SIZE_F page_size = m_main_page.m_page_size;
			for (const auto s : m_main_page.m_shape_list) {
				if (s->is_deleted()) {
					continue;
				}
				len += s->export_pdf(page_size, dt_writer);
			}
			len += dt_writer.WriteString(
				L"Q\n"
				L"endstream\n"
				L"endobj\n");
			obj_len.push_back(obj_len.back() + len);

			// フォント辞書
			text_cnt = 0;
			for (const auto s : m_main_page.m_shape_list) {
				if (s->is_deleted()) {
					continue;
				}

				if (typeid(*s) == typeid(ShapeText)) {
					// フォント辞書 (Type0), CID フォント辞書, フォント詳細辞書
					// グリフにおける座標値や幅を指定する場合、PDF 内では、常に 1 em = 1000 であるものとして、
					// 値を設定します。実際のフォントで 1 em = 1024 などとなっている場合は、n / 1024 * 1000 
					// というようにして、値を 1 em = 1000 に合わせます.
					const auto t = static_cast<ShapeText*>(s);

					DWRITE_FONT_WEIGHT weight = DWRITE_FONT_WEIGHT_NORMAL;	// 書体の太さ
					DWRITE_FONT_STRETCH stretch = DWRITE_FONT_STRETCH_NORMAL;	// 書体の幅
					DWRITE_FONT_STYLE style = DWRITE_FONT_STYLE_NORMAL;	// 字体
					DWRITE_FONT_METRICS1 f_met{};	// 書体の計量
					winrt::hstring p_name{};	// ポストスクリプト名
					std::vector<uint16_t> gid{};	// グリフ ID
					std::vector<uint16_t> cid{};	// 文字 ID
					std::vector<DWRITE_GLYPH_METRICS> g_met{};	// グリフの計量
					DWRITE_FONT_FACE_TYPE f_type;	// フォントフェイスの種類
					float angle;
					s->get_font_weight(weight);
					s->get_font_stretch(stretch);
					s->get_font_style(style);
					IDWriteFontFace3* face;
					t->get_font_face(face);
					export_pdf_font_info(face, t->m_text, wchar_len(t->m_text), t->m_font_family, f_met, f_type, p_name, cid, gid, g_met, angle);

					// フォント辞書
					len = export_pdf_font_dict(6 + 3 * text_cnt, f_type, p_name, dt_writer);
					obj_len.push_back(obj_len.back() + len);

					// CID フォント辞書 (Type 0 の子孫となるフォント)
					// フォント辞書から参照され,
					// フォント詳細辞書を参照する.
					len = export_pdf_descendant_font_dict(6 + 3 * text_cnt + 1,
						//f_type, p_name, std::size(cid), std::data(cid), std::data(g_met), f_met.designUnitsPerEm, dt_writer);
						f_type, p_name, std::size(gid), std::data(gid), std::data(g_met), f_met.designUnitsPerEm, dt_writer);
					obj_len.push_back(obj_len.back() + len);

					// フォント詳細辞書
					// CID フォント辞書から参照される.
					len = export_pdf_font_descriptor(6 + 3 * text_cnt + 2,
						p_name, stretch, weight, angle, f_met, dt_writer);
					obj_len.push_back(obj_len.back() + len);
					text_cnt++;
				}
				if (typeid(*s) == typeid(ShapeRuler)) {
					ShapeRuler* r = static_cast<ShapeRuler*>(s);
					IDWriteFontFace3* face;
					r->get_font_face(face);
					DWRITE_FONT_WEIGHT weight = DWRITE_FONT_WEIGHT_NORMAL;	// 書体の太さ
					DWRITE_FONT_STRETCH stretch = DWRITE_FONT_STRETCH_NORMAL;	// 書体の幅
					//DWRITE_FONT_STYLE style = DWRITE_FONT_STYLE_NORMAL;	// 字体
					DWRITE_FONT_METRICS1 f_met;
					DWRITE_FONT_FACE_TYPE f_type;
					winrt::hstring p_name{};
					std::vector<uint16_t> gid{};	// グリフ ID
					std::vector<uint16_t> cid{};	// 文字 ID
					std::vector<DWRITE_GLYPH_METRICS> g_met{};	// グリフの計量
					float angle;
					export_pdf_font_info(face, L"0123456789", 10, r->m_font_family, f_met, f_type, p_name, cid, gid, g_met, angle);

					// フォント辞書
					len = export_pdf_font_dict(
						6 + 3 * text_cnt, f_type, p_name, dt_writer);
					obj_len.push_back(obj_len.back() + len);

					// CID フォント辞書 (Type 0 の子孫となるフォント)
					// フォント辞書から参照され,
					// フォント詳細辞書を参照する.
					len = export_pdf_descendant_font_dict(
						6 + 3 * text_cnt + 1,
						//f_type, p_name, std::size(cid), std::data(cid), std::data(g_met), f_met.designUnitsPerEm, dt_writer);
						f_type, p_name, std::size(gid), std::data(gid), std::data(g_met), f_met.designUnitsPerEm, dt_writer);
					obj_len.push_back(obj_len.back() + len);

					// フォント詳細辞書
					// CID フォント辞書から参照される.
					len = export_pdf_font_descriptor(
						6 + 3 * text_cnt + 2,
						p_name, stretch, weight, angle, f_met, dt_writer);
					obj_len.push_back(obj_len.back() + len);
					text_cnt++;
				}
			}

			// 画像 XObject
			image_cnt = 0;
			for (const auto s : m_main_page.m_shape_list) {
				if (s->is_deleted()) {
					continue;
				}

				if (typeid(*s) == typeid(ShapeImage)) {
					const uint8_t* bgra = static_cast<ShapeImage*>(s)->m_bgra;
					const size_t w = static_cast<ShapeImage*>(s)->m_orig.width;
					const size_t h = static_cast<ShapeImage*>(s)->m_orig.height;
					len = export_pdf_image(bgra, w, h, 6 + 3 * text_cnt + image_cnt, dt_writer);
					obj_len.push_back(obj_len.back() + len);
					image_cnt++;
				}
			}

			// 相互参照 (クロスリファレンス)
			swprintf_s(
				buf,
				L"%% Cross-reference Table\n"
				L"xref\n"
				L"0 %zu\n"
				L"0000000000 65535 f\n",
				obj_len.size()
			);
			dt_writer.WriteString(buf);
			for (int i = 0; i < obj_len.size() - 1; i++) {
				swprintf_s(buf,
					L"%010zu 00000 n\n",
					obj_len[i]
				);
				dt_writer.WriteString(buf);
			}

			// トレイラーと EOF
			swprintf_s(
				buf,
				L"%% Trailer\n"
				L"trailer <<\n"
				L"/Size %zu\n"
				L"/Root 1 0 R\n"
				L">>\n"
				L"startxref\n"
				L"%zu\n"
				L"%%%%EOF\n",
				obj_len.size(),
				obj_len.back()
			);
			dt_writer.WriteString(buf);

			// ストリームの現在位置をストリームの大きさに格納する.
			pdf_stream.Size(pdf_stream.Position());
			// バッファ内のデータをストリームに出力する.
			co_await dt_writer.StoreAsync();
			// ストリームをフラッシュする.
			co_await pdf_stream.FlushAsync();
			hres = S_OK;
		}
		catch (winrt::hresult_error const& e) {
			hres = e.code();
		}
		co_return hres;
	}

	//-------------------------------
	// 図形データを SVG としてストレージファイルに非同期に書き込む.
	// svg_file	書き込み先のファイル
	// 戻り値	書き込めた場合 S_OK
	//-------------------------------
	IAsyncOperation<winrt::hresult> MainPage::export_as_raster_async(const StorageFile& image_file) noexcept
	{
		HRESULT hres = E_FAIL;

		// ファイルのコンテントの種類をもとに GUID の WIC フォーマットを得る.
		const GUID& wic_fmt = [](const winrt::hstring& c_type)
		{
			if (c_type == L"image/bmp") {
				return GUID_ContainerFormatBmp;
			}
			else if (c_type == L"image/gif") {
				return GUID_ContainerFormatGif;
			}
			else if (c_type == L"image/jpeg") {
				return GUID_ContainerFormatJpeg;
			}
			else if (c_type == L"image/png") {
				return GUID_ContainerFormatPng;
			}
			else if (c_type == L"image/tiff") {
				return GUID_ContainerFormatTiff;
			}
			return GUID_NULL;
		}(image_file.ContentType());

		// WIC フォーマットが空でなければ,
		if (wic_fmt != GUID_NULL) {

			// Direct2D コンテンツを画像ファイルに保存する方法
			try {
				// ファイルのランダムアクセスストリーム
				IRandomAccessStream image_stream{
					co_await image_file.OpenAsync(FileAccessMode::ReadWrite)
				};

				// WIC のランダムアクセスストリーム
				winrt::com_ptr<IStream> wic_stream;
				winrt::hresult(
					CreateStreamOverRandomAccessStream(
						winrt::get_unknown(image_stream),
						IID_PPV_ARGS(&wic_stream))
				);

				//winrt::com_ptr<IWICImagingFactory2> wic_factory;
				//winrt::check_hresult(
				//	CoCreateInstance(
				//		CLSID_WICImagingFactory,
				//		nullptr,
				//		CLSCTX_INPROC_SERVER,
				//		IID_PPV_ARGS(&wic_factory)
				//	)
				//);

				// Create and initialize WIC Bitmap Encoder.
				winrt::com_ptr<IWICBitmapEncoder> wic_enc;
				winrt::check_hresult(
					ShapeImage::wic_factory->CreateEncoder(
						wic_fmt, nullptr, wic_enc.put())
				);
				winrt::check_hresult(
					wic_enc->Initialize(
						wic_stream.get(), WICBitmapEncoderNoCache)
				);

				// Create and initialize WIC Frame Encoder.
				winrt::com_ptr<IWICBitmapFrameEncode> wic_frm;
				winrt::check_hresult(
					wic_enc->CreateNewFrame(wic_frm.put(), nullptr)
				);
				winrt::check_hresult(
					wic_frm->Initialize(nullptr)
				);

				// デバイスの作成
				/*
				const UINT w = m_main_page.m_page_size.width;
				const UINT h = m_main_page.m_page_size.height;
				std::vector<uint8_t> mem(4 * w * h);
				winrt::com_ptr<IWICBitmap> wic_bitmap;
				ShapeImage::wic_factory->CreateBitmapFromMemory(
					w, h,
					GUID_WICPixelFormat32bppBGRA, 4 * w, 4 * w * h, std::data(mem), wic_bitmap.put());
				D2D1_RENDER_TARGET_PROPERTIES prop{
					D2D1_RENDER_TARGET_TYPE::D2D1_RENDER_TARGET_TYPE_SOFTWARE,
					D2D1_PIXEL_FORMAT{
						DXGI_FORMAT_B8G8R8A8_UNORM,
						D2D1_ALPHA_MODE_STRAIGHT
						},
					96.0f,
					96.0f,
					D2D1_RENDER_TARGET_USAGE_FORCE_BITMAP_REMOTING,
					D2D1_FEATURE_LEVEL_DEFAULT
				};
				winrt::com_ptr<ID2D1RenderTarget> target;
				Shape::s_d2d_factory->CreateWicBitmapRenderTarget(wic_bitmap.get(), prop, target.put());
				*/

				// デバイスとデバイスコンテキストの作成
				D2D_UI d2d;

				// ビットマップレンダーターゲットの作成
				// サポートされているピクセル形式とアルファ モードの
				// https://learn.microsoft.com/ja-jp/windows/win32/direct2d/supported-pixel-formats-and-alpha-modes#supported-formats-for-wic-bitmap-render-target
				// ・WIC ビットマップ レンダー ターゲットでサポートされている形式
				// ・ID2D1DCRenderTarget でサポートされる形式
				// を参照.
				const UINT32 page_w = static_cast<UINT32>(m_main_page.m_page_size.width);
				const UINT32 page_h = static_cast<UINT32>(m_main_page.m_page_size.height);
				winrt::com_ptr<ID2D1BitmapRenderTarget> target;
				winrt::check_hresult(
					d2d.m_d2d_context->CreateCompatibleRenderTarget(
						m_main_page.m_page_size,
						D2D_SIZE_U{ page_w, page_h },
						D2D1_PIXEL_FORMAT{
							DXGI_FORMAT::DXGI_FORMAT_B8G8R8A8_UNORM,
							D2D1_ALPHA_MODE::D2D1_ALPHA_MODE_PREMULTIPLIED,
						},
						D2D1_COMPATIBLE_RENDER_TARGET_OPTIONS::D2D1_COMPATIBLE_RENDER_TARGET_OPTIONS_NONE,
						target.put()
						)
				);

				// レンダーターゲット依存のオブジェクトを消去
				for (const auto s : m_main_page.m_shape_list) {
					if (typeid(*s) == typeid(ShapeImage)) {
						static_cast<ShapeImage*>(s)->m_d2d_bitmap = nullptr;
					}
				}

				winrt::com_ptr<ID2D1SolidColorBrush> cb;
				winrt::com_ptr<ID2D1SolidColorBrush> rb;
				winrt::check_hresult(
					target->CreateSolidColorBrush(D2D1_COLOR_F{}, cb.put())
				);
				winrt::check_hresult(
					target->CreateSolidColorBrush(D2D1_COLOR_F{}, rb.put())
				);
				Shape::s_d2d_target = target.get();
				Shape::s_d2d_color_brush = cb.get();
				Shape::s_d2d_range_brush = rb.get();

				// ビットマップへの描画
				m_mutex_draw.lock();
				Shape::s_d2d_target->SaveDrawingState(m_main_page.m_state_block.get());
				Shape::s_d2d_target->BeginDraw();
				m_main_page.draw();
				winrt::check_hresult(
					Shape::s_d2d_target->EndDraw()
				);
				Shape::s_d2d_target->RestoreDrawingState(m_main_page.m_state_block.get());
				m_mutex_draw.unlock();

				// レンダーターゲット依存のオブジェクトを消去
				for (const auto s : m_main_page.m_shape_list) {
					if (typeid(*s) == typeid(ShapeImage)) {
						static_cast<ShapeImage*>(s)->m_d2d_bitmap = nullptr;
					}
				}

				// Retrieve D2D Device.
				winrt::com_ptr<ID2D1Device> dev;
				d2d.m_d2d_context->GetDevice(dev.put());

				// IWICImageEncoder を使用して Direct2D コンテンツを書き込む
				winrt::com_ptr<IWICImageEncoder> image_enc;
				winrt::check_hresult(
					ShapeImage::wic_factory->CreateImageEncoder(
						dev.get(), image_enc.put())
				);
				winrt::com_ptr<ID2D1Bitmap> d2d_image;
				winrt::check_hresult(
					target->GetBitmap(d2d_image.put())
				);
				winrt::check_hresult(
					image_enc->WriteFrame(d2d_image.get(), wic_frm.get(), nullptr)
				);
				winrt::check_hresult(
					wic_frm->Commit()
				);
				winrt::check_hresult(
					wic_enc->Commit()
				);
				// Flush all memory buffers to the next-level storage object.
				winrt::check_hresult(
					wic_stream->Commit(STGC_DEFAULT)
				);

				d2d.Trim();
				hres = S_OK;
			}
			catch (const winrt::hresult_error& e) {
				hres = e.code();
			}
		}
		co_return hres;
	}

	//-------------------------------
	// 図形データを SVG としてストレージファイルに非同期に書き込む.
	// svg_file	書き込み先のファイル
	// 戻り値	書き込めた場合 S_OK
	//-------------------------------
	IAsyncOperation<winrt::hresult> MainPage::export_as_svg_async(const StorageFile& svg_file) noexcept
	{
		HRESULT hres = E_FAIL;
		try {

			// ストレージファイルを開いて, ストリームとそのデータライターを得る.
			const IRandomAccessStream& svg_stream{
				co_await svg_file.OpenAsync(FileAccessMode::ReadWrite)
			};
			DataWriter dt_writer{
				DataWriter(svg_stream.GetOutputStreamAt(0))
			};

			// XML 宣言と DOCTYPE を書き込む.
			dt_writer.WriteString(
				L"<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n"
				L"<!DOCTYPE svg PUBLIC \"-//W3C//DTD SVG 1.1//EN\" "
				L"\"http://www.w3.org/Graphics/SVG/1.1/DTD/svg11.dtd\">\n");

			// SVG 開始タグを書き込む.
			{
				const auto size = m_main_page.m_page_size;	// 表示の大きさ
				const auto unit = m_len_unit;	// 長さの単位
				const auto dpi = m_main_d2d.m_logical_dpi;	// 論理 DPI
				const auto color = m_main_page.m_page_color;	// 背景色

				// 単位付きで幅と高さの属性を書き込む.
				wchar_t buf[1024];	// 出力バッファ
				double w;	// 単位変換後の幅
				double h;	// 単位変換後の高さ
				wchar_t* u;	// 単位
				if (unit == LEN_UNIT::INCH) {
					w = size.width / dpi;
					h = size.height / dpi;
					u = L"in";
				}
				else if (unit == LEN_UNIT::MILLI) {
					w = size.width * MM_PER_INCH / dpi;
					h = size.height * MM_PER_INCH / dpi;
					u = L"mm";
				}
				else if (unit == LEN_UNIT::POINT) {
					w = size.width * PT_PER_INCH / dpi;
					h = size.height * PT_PER_INCH / dpi;
					u = L"pt";
				}
				// SVG で使用できる上記の単位以外はすべてピクセル.
				else {
					w = size.width;
					h = size.height;
					u = L"px";
				}

				// ピクセル単位の幅と高さを viewBox 属性として書き込む.
				// 背景色をスタイル属性として書き込む.
				// svg 開始タグの終了を書き込む.
				swprintf_s(buf,
					L"<svg xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\" "
					L"width=\"%f%s\" height=\"%f%s\" "
					L"viewBox=\"0 0 %f %f\" "
					L"style=\"background-color:#%02x%02x%02x\">\n",
					w, u, h, u,
					size.width, size.height,
					static_cast<uint32_t>(std::round(color.r * 255.0)),
					static_cast<uint32_t>(std::round(color.g * 255.0)),
					static_cast<uint32_t>(std::round(color.b * 255.0))
				);
				dt_writer.WriteString(buf);
			}

			// 図形リストの各図形について以下を繰り返す.
			for (auto s : m_main_page.m_shape_list) {
				if (s->is_deleted()) {
					continue;
				}
				if (typeid(*s) == typeid(ShapeGroup)) {
					co_await static_cast<ShapeGroup*>(s)->export_as_svg_async(dt_writer);
				}
				// 図形が画像か判定する.
				else if (typeid(*s) == typeid(ShapeImage)) {
					co_await static_cast<ShapeImage*>(s)->export_as_svg_async(dt_writer);
				}
				else {
					s->export_svg(dt_writer);
				}
			}
			// SVG 終了タグを書き込む.
			dt_writer.WriteString(L"</svg>\n");
			// ストリームの現在位置をストリームの大きさに格納する.
			svg_stream.Size(svg_stream.Position());
			// バッファ内のデータをストリームに出力する.
			co_await dt_writer.StoreAsync();
			// ストリームをフラッシュする.
			co_await svg_stream.FlushAsync();
			hres = S_OK;
		}
		catch (winrt::hresult_error const& e) {
			// エラーが発生した場合, エラーコードを結果に格納する.
			hres = e.code();
		}
		co_return hres;
	}

}