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

	static size_t export_pdf_font_dict(
		const int o_num, const DWRITE_FONT_FACE_TYPE f_type, const winrt::hstring& p_name,
		const DataWriter& dt_writer);
	static size_t export_pdf_descendant_font_dict(
		int o_num, DWRITE_FONT_FACE_TYPE f_type, const winrt::hstring& p_name,
		const size_t cid_len, const uint16_t* cid_arr, const DWRITE_GLYPH_METRICS* g_met,
		const int upem, const DataWriter& dt_writer);
	static size_t export_pdf_font_descriptor(const int obj_num, const winrt::hstring& p_name,
		const DWRITE_FONT_STRETCH stretch, const DWRITE_FONT_WEIGHT weight, const float angle,
		const DWRITE_FONT_METRICS1& f_met, const DataWriter& dt_writer);
	static void export_pdf_font_info(IDWriteFontFace3* face, const wchar_t* t, const size_t t_len,
		const wchar_t* family, DWRITE_FONT_METRICS1& f_met, DWRITE_FONT_FACE_TYPE& f_type,
		winrt::hstring& p_name, std::vector<uint16_t>& cid, std::vector<uint16_t>& gid,
		std::vector<DWRITE_GLYPH_METRICS>& g_met, FLOAT& angle);

	//------------------------------
	// PDF のフォント辞書を出力する.
	// n_pdf	PDF のオブジェクト番号
	// f_type	字面の種類
	// p_name	ポストスクリプト名
	// dt_writer	出力先
	//------------------------------
	static size_t export_pdf_font_dict(
		const int n_pdf,
		const DWRITE_FONT_FACE_TYPE f_type, const winrt::hstring& p_name,
		const DataWriter& dt_writer)
	{
		wchar_t buf[1024];
		swprintf_s(buf,
			L"%d 0 obj <<\n"
			L"%% Font Dictionary\n"
			L"/Type /Font\n"
			L"/Subtype /Type0\n"
			L"/BaseFont /%s\n"
			//L"/Encoding /UniJIS-UTF16-H\n"
			L"/Encoding /Identity-H\n"
			L"/DescendantFonts [%d 0 R]\n"
			L">>\n",
			n_pdf,
			//std::data(p_name),
			// OpenType の場合 ポストスクリプト名+'-'+CMap 名で CIDFontType0
			f_type == DWRITE_FONT_FACE_TYPE_TRUETYPE ?
			//std::data(p_name) : std::data(p_name + L"-UniJIS-UTF16-H"),
			std::data(p_name) : std::data(p_name + L"-Identity-H"),
			n_pdf + 1
		);
		return dt_writer.WriteString(buf);
	}

	//------------------------------
	// PDF の子孫フォント辞書を出力する.
	// o_num	オブジェクト番号
	// f_type	字面の種類
	// p_name	書体のポストスクリプト名
	// cid_len	CID 配列の大きさ
	// cid_arr	CID 配列
	// g_met	字形 (グリフ) の計量
	// g_unit	計量の値の単位
	// dt_writer	出力先
	//------------------------------
	static size_t export_pdf_descendant_font_dict(
		const int o_num, const DWRITE_FONT_FACE_TYPE f_type, const winrt::hstring& p_name,
		const size_t cid_len, const uint16_t* cid_arr, const DWRITE_GLYPH_METRICS* g_met, 
		const int g_unit, const DataWriter& dt_writer)
	{
		wchar_t buf[1024];
		size_t len = 0;
		swprintf_s(buf,
			L"%d 0 obj <<\n"
			L"%% Descendant Font Dictionary\n"
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
		len += dt_writer.WriteString(buf);

		// 半角のグリフ幅を設定する
		// 設定しなければ全て全角幅になってしまう.
		// CID 配列は, CID の昇順にならんでなければならない.
		len += dt_writer.WriteString(
			L"/W [\n");	
		uint32_t c0 = cid_arr[0];
		uint32_t w0 = 1000 * g_met[0].advanceWidth / g_unit;
		for (int i = 1; i < cid_len; i++) {
			uint32_t w1 = 1000 * g_met[i].advanceWidth / g_unit;
			if (w0 != w1) {
				swprintf_s(buf,
					L"%u %u %u\n",
					c0, cid_arr[i - 1], w0);
				len += dt_writer.WriteString(buf);
				c0 = cid_arr[i];
				w0 = w1;
			}
		}
		swprintf_s(buf,
			L"%u %u %u\n"
			L"]\n"
			L">>\n"
			L"endobj\n",
			c0, cid_arr[cid_len - 1], w0);
		len += dt_writer.WriteString(buf);
		return len;
	}

	static size_t export_pdf_font_descriptor(
		const int obj_num, const winrt::hstring& p_name, const DWRITE_FONT_STRETCH stretch,
		const DWRITE_FONT_WEIGHT weight, const float angle, const DWRITE_FONT_METRICS1& f_met,
		const DataWriter& dt_writer)
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
			L"%d 0 obj <<\n"
			L"%% Font Descriptor Dictionary\n"
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

	// フォントフェイスの情報を得る.
	static void export_pdf_font_info(
		IDWriteFontFace3* face,	// フォントフェイス
		const wchar_t *t,	// 文字列
		const size_t t_len,	// 文字列の長さ
		const wchar_t *family,	// フォントファミリー
		DWRITE_FONT_METRICS1& f_met,	// 書体の計量
		DWRITE_FONT_FACE_TYPE& f_type,	// 字面の形式
		winrt::hstring& p_name,	// 書体のポストスクリプト名
		std::vector<uint16_t>& cid,	// CID の配列
		std::vector<uint16_t>& gid,	// 字形の計量
		std::vector<DWRITE_GLYPH_METRICS>& g_met,	// 字形の計量
		FLOAT& angle	// イタリックの最大角度 (マイナスがいわゆる斜体)
	)
	{
		// 書体の計量を得る.
		face->GetMetrics(&f_met);

		// 字面の形式を得る.
		f_type = face->GetType();

		// 文字列を UTF-32 に変換する.
		std::vector<uint32_t> utf32{
			text_utf16_to_utf32(t, t_len)
		};

		// UTF-32 文字列から重複したコードを削除する.
		std::sort(std::begin(utf32), std::end(utf32));
		const auto last = std::unique(std::begin(utf32), std::end(utf32));
		utf32.erase(last, utf32.end());

		// UTF32 文字列から CID を得る.
		//cid.resize(utf32.size());
		//for (int i = 0; i < utf32.size(); i++) {
		//	cid[i] = cmap_getcid(utf32[i]);
		//}
		HRESULT hr = S_OK;

		// UTF32 文字列からソートされた GID を得る.
		gid.resize(utf32.size());
		if (hr == S_OK) {
			hr = face->GetGlyphIndices(std::data(utf32), static_cast<UINT32>(std::size(utf32)), std::data(gid));
		}
		if (hr == S_OK) {
			std::sort(std::begin(gid), std::end(gid));

			// GID 文字列から字形 (グリフ) の計量を得る.
			g_met.resize(utf32.size());
			hr = face->GetDesignGlyphMetrics(std::data(gid), static_cast<UINT32>(std::size(gid)), std::data(g_met));
		}

		if (hr == S_OK) {
			// 字面からポストスクリプト名を得る.
			IDWriteLocalizedStrings* str = nullptr;
			BOOL exists = false;
			if (face->GetInformationalStrings(DWRITE_INFORMATIONAL_STRING_POSTSCRIPT_NAME, &str, &exists) == S_OK) {
				if (exists) {
					const UINT32 str_cnt = str->GetCount();
					for (uint32_t j = 0; j < str_cnt; j++) {
						UINT32 wstr_len = 0;	// 終端ヌルを除く文字列長
						if (str->GetStringLength(j, &wstr_len) == S_OK && wstr_len > 0) {
							std::vector<wchar_t> wstr(static_cast<size_t>(wstr_len) + 1);
							if (str->GetString(j, std::data(wstr), wstr_len + 1) == S_OK) {
								p_name.clear();
								p_name = std::data(wstr);
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
					const size_t k = wchar_len(family);
					std::vector<wchar_t> wstr(k + 1);
					int wstr_len = 0;
					for (size_t i = 0; i < k; i++) {
						if (!iswspace(family[i])) {
							wstr[wstr_len++] = family[i];
						}
					}
					wstr[wstr_len] = L'\0';
					p_name.clear();
					p_name = std::data(wstr);
				}
			}

			// 字面から傾きを得る.
			// ただし, PDF は, 字面の変形はサポートしてないので, 字体そのものが
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
	}

	// 画像を PDF の XObject として書き出す.
	static size_t export_pdf_image(
		const uint8_t* bgra,	// 画像データ
		const size_t o_width,	// 出力幅
		const size_t o_height,	// 出力高さ
		const D2D1_RECT_F clip,	// クリッピング矩形
		const int obj_num,	// PDF オブジェクト番号
		DataWriter& dt_writer	// 出力先
	)
	{
		// クリッピングしながら画像データをコピーする.
		// PDF はアルファ値をサポートしておらず, 逆に WIC ビットマップは 3 バイトピクセルを
		// サポートしていない.
		const size_t cl = max(static_cast<int>(round(clip.left)), 0);
		const size_t ct = max(static_cast<int>(round(clip.top)), 0);
		const size_t cr = min(static_cast<int>(round(clip.right)), o_width);
		const size_t cb = min(static_cast<int>(round(clip.bottom)), o_height);

		// クリッピング後の画像の大きさがゼロなら何もしない.
		if (cr <= cl || cb <= ct) {
			return 0;
		}

		// 3 バイトピクセルのバッファを用意し, BGRA を RGB にコピー.
		std::vector<uint8_t> in_buf(3ull * (cr - cl) * (cb - ct));
		size_t i = 0;
		for (size_t y = ct; y < cb; y++) {
			for (size_t x = cl; x < cr; x++) {
				in_buf[i++] = bgra[4 * o_width * y + 4 * x + 2];	// R
				in_buf[i++] = bgra[4 * o_width * y + 4 * x + 1];	// G
				in_buf[i++] = bgra[4 * o_width * y + 4 * x + 0];	// B
			}
		}

		// Deflate 圧縮
		std::vector<uint8_t> z_buf;	// 圧縮されたデータのバッファ
		z_compress(z_buf, std::data(in_buf), std::size(in_buf));
		in_buf.clear();
		in_buf.shrink_to_fit();
	
		// XObject として出力する.
		wchar_t buf[1024];
		swprintf_s(buf,
			L"%d 0 obj <<\n"
			L"%% XObject (Image)\n"
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
			cr - cl,
			cb - ct,
			z_buf.size()
		);
		size_t len = 0;
		len += dt_writer.WriteString(buf);
		len += dt_writer.WriteString(L"stream\n");
		dt_writer.WriteBytes(z_buf);
		len += z_buf.size();

		// 圧縮されたデータのバッファを解放.
		z_buf.clear();
		z_buf.shrink_to_fit();

		// ストリームの最後は '>'.
		len += dt_writer.WriteString(
			L">\n"
			L"endstream\n"
			L"endobj\n");
		return len;
	}

	// 図形をデータライターに PDF として書き込む.
	IAsyncOperation<winrt::hresult> MainPage::export_as_pdf_async(
		const StorageFile& pdf_file) noexcept
	{
		HRESULT hres = E_FAIL;
		try {
			wchar_t buf[1024];	// PDF

			// ページの幅と高さを, D2D の固定 DPI (96dpi) から PDF の 72dpi に,
			// 変換する (モニターに応じて変化する論理 DPI は用いない). 
			const float w_pt = m_main_sheet.m_sheet_size.width * 72.0f / 96.0f;	// ポイントに変換された幅
			const float h_pt = m_main_sheet.m_sheet_size.height * 72.0f / 96.0f;	// ポイントに変換された高さ

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
				L"1 0 obj <<\n"
				L"% Catalog Dictionary\n"
				L"/Type /Catalog\n"
				L"/Pages 2 0 R\n"
				L">>\n"
				L"endobj\n");// , dt_writer);
			obj_len.push_back(obj_len.back() + len);

			// ページツリー辞書
			// カタログから参照され,
			// ページを参照する.
			len = dt_writer.WriteString(
				L"2 0 obj <<\n"
				L"% Page Tree Dictionary\n"
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
				L"3 0 obj <<\n"
				L"%% Page Object\n"
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
				L"4 0 obj <<\n"
				L"% Resouces Dictionary\n"
			);
			// フォント
			int text_cnt = 0;	// 文字列オブジェクトの計数
			std::vector<std::vector<char>> base_font;	// ベースフォント名
			for (const auto s : m_main_sheet.m_shape_list) {
				if (s->is_deleted()) {
					continue;
				}
				if (typeid(*s) == typeid(ShapeText)) {
					if (text_cnt == 0) {
						len += dt_writer.WriteString(
							L"/Font <<\n");
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
						len += dt_writer.WriteString(
							L"/Font <<\n");
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
				len += dt_writer.WriteString(
					L">>\n");
			}
			// 画像
			int image_cnt = 0;
			for (const auto s : m_main_sheet.m_shape_list) {
				if (s->is_deleted()) {
					continue;
				}
				if (typeid(*s) == typeid(ShapeImage)) {
					if (image_cnt == 0) {
						len += dt_writer.WriteString(
							L"/XObject <<\n");
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
			// 変換行列 cm に 72dpi / 96dpi (=0.75) を指定する.
			swprintf_s(buf,
				L"5 0 obj <<\n"
				L"%% Content Object\n"
				L">>\n"
				L"stream\n"
				L"%f 0 0 %f 0 0 cm\n"
				L"q\n",
				72.0f / 96.0f, 72.0f / 96.0f
			);
			len = dt_writer.WriteString(buf);

			len += m_main_sheet.export_pdf_sheet(m_background_color, dt_writer);

			if (m_main_sheet.m_grid_show == GRID_SHOW::BACK) {
				len += m_main_sheet.export_pdf_grid(m_background_color, dt_writer);
			}

			// 図形を出力
			const D2D1_SIZE_F sheet_size = m_main_sheet.m_sheet_size;
			for (const auto s : m_main_sheet.m_shape_list) {
				if (s->is_deleted()) {
					continue;
				}
				len += s->export_pdf(sheet_size, dt_writer);
			}
			if (m_main_sheet.m_grid_show == GRID_SHOW::FRONT) {
				len += m_main_sheet.export_pdf_grid(m_background_color, dt_writer);
			}
			len += dt_writer.WriteString(
				L"Q\n"
				L"endstream\n"
				L"endobj\n");
			obj_len.push_back(obj_len.back() + len);

			// フォント辞書
			text_cnt = 0;
			for (const auto s : m_main_sheet.m_shape_list) {
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
					DWRITE_FONT_FACE_TYPE f_type;	// 字面の種類
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
					len = export_pdf_descendant_font_dict(6 + 3 * text_cnt + 1, f_type, p_name, std::size(gid), std::data(gid), std::data(g_met), f_met.designUnitsPerEm, dt_writer);
					obj_len.push_back(obj_len.back() + len);

					// フォント詳細辞書
					// CID フォント辞書から参照される.
					len = export_pdf_font_descriptor(6 + 3 * text_cnt + 2, p_name, stretch, weight, angle, f_met, dt_writer);
					obj_len.push_back(obj_len.back() + len);
					text_cnt++;
				}
				else if (typeid(*s) == typeid(ShapeRuler)) {
					const ShapeRuler* r = static_cast<ShapeRuler*>(s);
					IDWriteFontFace3* face;
					r->get_font_face(face);
					DWRITE_FONT_WEIGHT weight = DWRITE_FONT_WEIGHT_NORMAL;	// 書体の太さ
					DWRITE_FONT_STRETCH stretch = DWRITE_FONT_STRETCH_NORMAL;	// 書体の幅
					DWRITE_FONT_METRICS1 f_met;
					DWRITE_FONT_FACE_TYPE f_type;
					winrt::hstring p_name{};
					std::vector<uint16_t> gid{};	// グリフ ID
					std::vector<uint16_t> cid{};	// 文字 ID
					std::vector<DWRITE_GLYPH_METRICS> g_met{};	// グリフの計量
					float angle;
					export_pdf_font_info(
						face, L"0123456789", 10, r->m_font_family, f_met, f_type, p_name, cid, gid,
						g_met, angle);

					// フォント辞書
					len = export_pdf_font_dict(6 + 3 * text_cnt, f_type, p_name, dt_writer);
					obj_len.push_back(obj_len.back() + len);

					// CID フォント辞書 (Type 0 の子孫となるフォント)
					// フォント辞書から参照され,
					// フォント詳細辞書を参照する.
					len = export_pdf_descendant_font_dict(6 + 3 * text_cnt + 1, f_type, p_name, std::size(gid), std::data(gid), std::data(g_met), f_met.designUnitsPerEm, dt_writer);
					obj_len.push_back(obj_len.back() + len);

					// フォント詳細辞書
					// CID フォント辞書から参照される.
					len = export_pdf_font_descriptor(6 + 3 * text_cnt + 2, p_name, stretch, weight, angle, f_met, dt_writer);
					obj_len.push_back(obj_len.back() + len);
					text_cnt++;
				}
			}

			// 画像 XObject
			image_cnt = 0;
			for (const auto s : m_main_sheet.m_shape_list) {
				if (s->is_deleted()) {
					continue;
				}

				if (typeid(*s) == typeid(ShapeImage)) {
					const uint8_t* bgra = static_cast<ShapeImage*>(s)->m_bgra;
					const size_t w = static_cast<ShapeImage*>(s)->m_orig.width;
					const size_t h = static_cast<ShapeImage*>(s)->m_orig.height;
					const auto c = static_cast<ShapeImage*>(s)->m_clip;
					len = export_pdf_image(bgra, w, h, c, 6 + 3 * text_cnt + image_cnt, dt_writer);
					obj_len.push_back(obj_len.back() + len);
					image_cnt++;
				}
			}

			// 情報辞書
			// 日時は GMT で格納する. Chrome はローカルに変換してくれるが, Edge はダメ.
			const std::time_t now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
			struct tm t;
			gmtime_s(&t, &now);
			swprintf_s(
				buf,
				L"%zu 0 obj <<\n"
				L"%% Info Dictionary\n"
				L"/CreationDate (D:%04u%02u%02u%02u%02u%02u+00)\n"
				L"/ModDate (D:%04u%02u%02u%02u%02u%02u+00)\n"
				L">>\n",
				obj_len.size(),
				1900 + t.tm_year, 1 + t.tm_mon, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec,
				1900 + t.tm_year, 1 + t.tm_mon, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec
			);
			len = dt_writer.WriteString(buf);
			obj_len.push_back(obj_len.back() + len);

			// 相互参照 (クロスリファレンス)
			swprintf_s(buf,
				L"xref\n"
				L"%% Cross-reference Table\n"
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
			swprintf_s(buf,
				L"trailer <<\n"
				L"%% Trailer\n"
				L"/Size %zu\n"
				L"/Root 1 0 R\n"
				L"/Info %zu 0 R\n"
				L">>\n"
				L"startxref\n"
				L"%zu\n"
				L"%%%%EOF\n",
				obj_len.size(),
				obj_len.size() - 1,
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
	// 画像としてストレージファイルに非同期に書き込む.
	// Direct2D コンテンツを画像ファイルに保存する方法
	// 戻り値	書き込めた場合 S_OK
	//-------------------------------
	IAsyncOperation<winrt::hresult> MainPage::export_as_raster_async(
		const StorageFile& image_file	// 書き込み先の画像ファイル
	) noexcept
	{
		HRESULT hr = S_OK;

		// ファイルの MIME タイプをもとに WIC フォーマット の GUID を得る.
		const winrt::hstring mime{ image_file.ContentType() };
		const GUID& wic_fmt = [](const winrt::hstring& mime)
		{
			if (mime == L"image/bmp") {
				return GUID_ContainerFormatBmp;
			}
			else if (mime == L"image/gif") {
				return GUID_ContainerFormatGif;
			}
			else if (mime == L"image/jpeg") {
				return GUID_ContainerFormatJpeg;
			}
			else if (mime == L"image/png") {
				return GUID_ContainerFormatPng;
			}
			else if (mime == L"image/tiff") {
				return GUID_ContainerFormatTiff;
			}
			return GUID_NULL;
		}(mime);

		// WIC フォーマットが空なら, E_FAIL.
		if (wic_fmt == GUID_NULL) {
			hr = E_FAIL;
		}

		// ストレージファイルのランダムアクセスストリームを開く.
		IRandomAccessStream image_stream{};
		if (hr == S_OK) {
			try {
				image_stream = co_await image_file.OpenAsync(FileAccessMode::ReadWrite);
			}
			catch (const winrt::hresult_error& e) {
				hr = e.code();
			}
		}

		// 開いたストリームを元に, エンコーダーのランダムアクセスストリームを作成する.
		winrt::com_ptr<IStream> wic_stream;
		if (hr == S_OK) {
			hr = CreateStreamOverRandomAccessStream(
				winrt::get_unknown(image_stream),
				//IID_PPV_ARGS(&wic_stream)
				__uuidof(IStream),
				wic_stream.put_void()
			);
		}

		//winrt::com_ptr<IWICImagingFactory2> wic_factory;
		//winrt::check_hresult(
		//	CoCreateInstance(
		//		CLSID_WICImagingFactory,
		//		nullptr,
		//		CLSCTX_INPROC_SERVER,
		//		IID_PPV_ARGS(&wic_factory)
		//	)
		//);


		// WIC ファクトリーで, WIC ビットマップエンコーダーを作成する.
		winrt::com_ptr<IWICBitmapEncoder> wic_bmp_enc;
		if (hr == S_OK) {
			hr = ShapeImage::wic_factory->CreateEncoder(wic_fmt, nullptr, wic_bmp_enc.put());
		}
		// WIC ビットマップエンコーダー を初期化する.
		if (hr == S_OK) {
			hr = wic_bmp_enc->Initialize(wic_stream.get(), WICBitmapEncoderNoCache);
		}
		// WIC ビットマップエンコーダー で, WIC フレームエンコーダーを作成する.
		winrt::com_ptr<IWICBitmapFrameEncode> wic_frm_enc;
		if (hr == S_OK) {
			hr = wic_bmp_enc->CreateNewFrame(wic_frm_enc.put(), nullptr);
		}
		// 作成された WIC フレームエンコーダーを初期化する.
		if (hr == S_OK) {
			hr = wic_frm_enc->Initialize(nullptr);
		}

		// デバイスの作成
		/*
		const UINT w = m_main_sheet.m_sheet_size.width;
		const UINT h = m_main_sheet.m_sheet_size.height;
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

		// オフスクリーンの D2D を作成する.
		D2D_UI offscreen;

		// D2D デバイスコンテキストに対応する D2D ビットマップレンダーターゲットの作成する.
		// サポートされているピクセル形式とアルファ モードの
		// https://learn.microsoft.com/ja-jp/windows/win32/direct2d/supported-pixel-formats-and-alpha-modes#supported-formats-for-wic-bitmap-render-target
		// ・WIC ビットマップ レンダー ターゲットでサポートされている形式
		// ・ID2D1DCRenderTarget でサポートされる形式
		// を参照.
		// D2D1_PIXEL_FORMAT には D2D1_ALPHA_MODE_PREMULTIPLIED を指定する.
		// D2D1_ALPHA_MODE_STRAIGHT は使用できない
		const D2D1_SIZE_U p_size{	// 画素単位での大きさ
			static_cast<UINT32>(m_main_sheet.m_sheet_size.width),
			static_cast<UINT32>(m_main_sheet.m_sheet_size.height)
		};
		const D2D1_PIXEL_FORMAT p_format{	// 画素フォーマット
			DXGI_FORMAT::DXGI_FORMAT_B8G8R8A8_UNORM,
			D2D1_ALPHA_MODE::D2D1_ALPHA_MODE_PREMULTIPLIED
		};
		winrt::com_ptr<ID2D1BitmapRenderTarget> target;
		if (hr == S_OK) {
			hr = offscreen.m_d2d_context->CreateCompatibleRenderTarget(
				m_main_sheet.m_sheet_size, p_size, p_format, D2D1_COMPATIBLE_RENDER_TARGET_OPTIONS::D2D1_COMPATIBLE_RENDER_TARGET_OPTIONS_NONE, target.put()
			);
		}

		if (hr == S_OK) {
			m_mutex_draw.lock();

			// ビットマップオブジェクトは, レンダーターゲット依存するため全て消去
			for (const auto s : m_main_sheet.m_shape_list) {
				if (typeid(*s) == typeid(ShapeImage)) {
					static_cast<ShapeImage*>(s)->m_d2d_bitmap = nullptr;
				}
			}

			// ビットマップへの描画
			m_main_sheet.begin_draw(target.get(), false, nullptr, 1.0f);
			target->SaveDrawingState(Shape::m_state_block.get());
			target->BeginDraw();
			m_main_sheet.draw();
			hr = target->EndDraw();
			target->RestoreDrawingState(Shape::m_state_block.get());

			// ビットマップオブジェクトを, レンダーターゲット依存するため全て消去
			for (const auto s : m_main_sheet.m_shape_list) {
				if (typeid(*s) == typeid(ShapeImage)) {
					static_cast<ShapeImage*>(s)->m_d2d_bitmap = nullptr;
				}
			}

			m_mutex_draw.unlock();
		}

		// ビットマップレンダーターゲットからビットマップを得る.
		winrt::com_ptr<ID2D1Bitmap> d2d_bmp;
		if (hr == S_OK) {
			hr = target->GetBitmap(d2d_bmp.put());
		}

		// D2D デバイスコンテキストから D2D デバイスを得て, WIC ファクトリーでその D2D デバイスに対応する WIC イメージエンコーダーを作成する.
		winrt::com_ptr<IWICImageEncoder> wic_img_enc;
		if (hr == S_OK) {
			winrt::com_ptr<ID2D1Device> dev;
			offscreen.m_d2d_context->GetDevice(dev.put());
			hr = ShapeImage::wic_factory->CreateImageEncoder(dev.get(), wic_img_enc.put());
		}

		// WIC イメージエンコーダーで, D2D ビットマップを WIC フレームエンコーダーに書き込む.
		if (hr == S_OK) {
			// nullptr = 通常使用される既定のパラメーター
			hr = wic_img_enc->WriteFrame(d2d_bmp.get(), wic_frm_enc.get(), nullptr);
		}
		if (hr == S_OK) {
			hr = wic_frm_enc->Commit();
		}
		if (hr == S_OK) {
			hr = wic_bmp_enc->Commit();
		}
		// 次のレベルのストレージオブジェクトに渡すため全てのメモリバッファを出力する.
		if (hr == S_OK) {
			hr = wic_stream->Commit(STGC_DEFAULT);
		}

		offscreen.Trim();
		co_return hr;
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

			wchar_t buf[1024];	// 出力バッファ

			// SVG 開始タグを書き込む.
			{
				const auto dpi = m_main_d2d.m_logical_dpi;	// 論理 DPI

				// 単位付きで幅と高さの属性を書き込む.
				double w;	// 単位変換後の幅
				double h;	// 単位変換後の高さ
				wchar_t* u;	// 単位
				if (m_len_unit == LEN_UNIT::INCH) {
					w = m_main_sheet.m_sheet_size.width / dpi;
					h = m_main_sheet.m_sheet_size.height / dpi;
					u = L"in";
				}
				else if (m_len_unit == LEN_UNIT::MILLI) {
					w = m_main_sheet.m_sheet_size.width * MM_PER_INCH / dpi;
					h = m_main_sheet.m_sheet_size.height * MM_PER_INCH / dpi;
					u = L"mm";
				}
				else if (m_len_unit == LEN_UNIT::POINT) {
					w = m_main_sheet.m_sheet_size.width * PT_PER_INCH / dpi;
					h = m_main_sheet.m_sheet_size.height * PT_PER_INCH / dpi;
					u = L"pt";
				}
				// SVG で使用できる上記の単位以外はすべてピクセル.
				else {
					w = m_main_sheet.m_sheet_size.width;
					h = m_main_sheet.m_sheet_size.height;
					u = L"px";
				}

				// svg 開始タグの終了を書き込む.
				// ピクセル単位の幅と高さを viewBox 属性として書き込む.
				swprintf_s(buf,
					L"<svg xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\" "
					L"width=\"%f%s\" height=\"%f%s\" viewBox=\"0 0 %f %f\" >",
					w, u, h, u, m_main_sheet.m_sheet_size.width, m_main_sheet.m_sheet_size.height
				);
				dt_writer.WriteString(buf);
				// 用紙の塗りつぶしと, 余白の平行移動.
				swprintf_s(buf,
					L"<rect width=\"%f\" height=\"%f\" "
					L"fill=\"#%02x%02x%02x\" opacity=\"%f\" />\n"
					L"<g transform=\"translate(%f,%f)\" >\n",
					m_main_sheet.m_sheet_size.width, m_main_sheet.m_sheet_size.height,
					conv_color_comp(m_main_sheet.m_sheet_color.r),
					conv_color_comp(m_main_sheet.m_sheet_color.g),
					conv_color_comp(m_main_sheet.m_sheet_color.b),
					m_main_sheet.m_sheet_color.a,
					m_main_sheet.m_sheet_margin.left,
					m_main_sheet.m_sheet_margin.top
				);
				dt_writer.WriteString(buf);
			}

			if (m_main_sheet.m_grid_show == GRID_SHOW::BACK) {
				m_main_sheet.export_svg(dt_writer);
			}

			// 図形リストの各図形について以下を繰り返す.
			for (auto s : m_main_sheet.m_shape_list) {
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

			if (m_main_sheet.m_grid_show == GRID_SHOW::FRONT) {
				m_main_sheet.export_svg(dt_writer);
			}

			// SVG 終了タグを書き込む.
			dt_writer.WriteString(L"</g>\n</svg>\n");
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