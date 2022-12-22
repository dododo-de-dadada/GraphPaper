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

	// 文字列図形からフォントの情報を得る.
	static void pdf_get_font(const ShapeText* s,
		DWRITE_FONT_WEIGHT& weight,
		DWRITE_FONT_STRETCH& stretch,
		DWRITE_FONT_STYLE& style,
		DWRITE_FONT_METRICS1& fmet,
		std::vector<char>& ps_name, 
		std::vector<DWRITE_GLYPH_METRICS>& gmet)
	{
		s->get_font_weight(weight);
		s->get_font_stretch(stretch);
		s->get_font_style(style);

		IDWriteFontCollection* coll = nullptr;
		if (s->m_dw_text_layout->GetFontCollection(&coll) == S_OK) {
			IDWriteFontFamily* family = nullptr;
			UINT32 index;
			BOOL exists;
			if (coll->FindFamilyName(s->m_font_family, &index, &exists) == S_OK &&
				exists &&
				coll->GetFontFamily(index, &family) == S_OK) {
				IDWriteFont* font = nullptr;
				if (family->GetFirstMatchingFont(weight, stretch, style, &font) == S_OK) {
					// フォント情報を得る,
					static_cast<IDWriteFont1*>(font)->GetMetrics(&fmet);
					IDWriteFontFaceReference* ref = nullptr;
					// 各グリフの文字幅を得る.
					if (static_cast<IDWriteFont3*>(font)->GetFontFaceReference(&ref) == S_OK) {
						IDWriteFontFace3* face = nullptr;
						if (ref->CreateFontFace(&face) == S_OK) {
							static constexpr uint32_t U32[]{
								32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47,
								48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63,
								64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79,
								80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95,
								96, 97, 98, 99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111,
								112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126
							};
							static constexpr auto U32_SIZE = std::size(U32);
							static uint16_t gid[U32_SIZE];
							winrt::check_hresult(face->GetGlyphIndices(std::data(U32), static_cast<UINT32>(U32_SIZE), std::data(gid)));
							gmet.resize(U32_SIZE);
							face->GetDesignGlyphMetrics(std::data(gid), static_cast<UINT32>(U32_SIZE), std::data(gmet));

							face->Release();
						}
						ref->Release();
					}
					// 書体のポストスクリプト名を得る.
					IDWriteLocalizedStrings* str = nullptr;
					BOOL exists = false;
					if (font->GetInformationalStrings(DWRITE_INFORMATIONAL_STRING_POSTSCRIPT_NAME, &str, &exists) == S_OK) {
						if (exists) {
							const UINT32 str_cnt = str->GetCount();
							for (uint32_t j = 0; j < str_cnt; j++) {
								UINT32 wstr_len = 0;	// 終端ヌルを除く文字列長
								if (str->GetStringLength(j, &wstr_len) == S_OK && wstr_len > 0) {
									std::vector<wchar_t> wstr(wstr_len + 1);
									if (str->GetString(j, std::data(wstr), wstr_len + 1) == S_OK) {
										int ps_name_len = WideCharToMultiByte(CP_ACP, 0, std::data(wstr), wstr_len + 1, NULL, 0, NULL, NULL);
										ps_name.resize(ps_name_len);
										WideCharToMultiByte(CP_ACP, 0, std::data(wstr), wstr_len + 1, std::data(ps_name), ps_name_len, NULL, NULL);
										break;
									}
								}
							}
						}
						// 存在しないなら書体名から空白文字を取り除いた文字列を, ポストスクリプト名とする.
						else {
							const auto k = wchar_len(s->m_font_family);
							std::vector<wchar_t> wstr(k + 1);
							int wstr_len = 0;
							for (int i = 0; i < k; i++) {
								if (!iswspace(s->m_font_family[i])) {
									wstr[wstr_len++] = s->m_font_family[i];
								}
							}
							wstr[wstr_len] = L'\0';
							int ps_name_len = WideCharToMultiByte(CP_ACP, 0, std::data(wstr), wstr_len + 1, NULL, 0, NULL, NULL);
							ps_name.resize(ps_name_len);
							WideCharToMultiByte(CP_ACP, 0, std::data(wstr), wstr_len + 1, std::data(ps_name), ps_name_len, NULL, NULL);
						}
						str->Release();
					}
					font->Release();
				}
				family->Release();
			}
			coll->Release();
		}
	}


	// 図形をデータライターに PDF として書き込む.
	IAsyncOperation<winrt::hresult> MainPage::pdf_write_async(StorageFile pdf_file)
	{
		HRESULT hr = S_OK;
		try {
			char buf[1024];	// PDF

			// 用紙の幅と高さを, D2D の固定 DPI (96dpi) から PDF の 72dpi に,
			// 変換する (モニターに応じて変化する論理 DPI は用いない). 
			const float w_pt = m_main_sheet.m_sheet_size.width * 72.0f / 96.0f;	// 変換された幅
			const float h_pt = m_main_sheet.m_sheet_size.height * 72.0f / 96.0f;	// 変換された高さ

			// ファイル更新の遅延を設定する.
			CachedFileManager::DeferUpdates(pdf_file);
			// ストレージファイルを開いてランダムアクセスストリームを得る.
			const IRandomAccessStream& pdf_stream{
				co_await pdf_file.OpenAsync(FileAccessMode::ReadWrite)
			};
			// ランダムアクセスストリームの先頭からデータライターを作成する.
			// 必ず先頭を指定する.
			DataWriter dt_writer{
				DataWriter(pdf_stream.GetOutputStreamAt(0))
			};

			// PDF ヘッダー
			// ファイルにバイナリデータが含まれる場合は,
			// ヘッダと改行の直後に, 4 つ以上のバイナリ文字を含むコメント行を置くことが推奨.
			size_t len = dt_write(
				"%PDF-1.7\n"
				"%\xff\xff\xff\xff\n",
				dt_writer);
			std::vector<size_t> obj_len{};
			obj_len.push_back(len);

			// カタログ辞書.
			// トレーラーから参照され,
			// ページツリーを参照する.
			len = dt_write(
				"% Catalog Dictionary\n"
				"1 0 obj <<\n"
				"/Type /Catalog\n"
				"/Pages 2 0 R\n"
				">>\n"
				"endobj\n", dt_writer);
			obj_len.push_back(obj_len.back() + len);

			// ページツリー辞書
			// カタログから参照され,
			// ページを参照する.
			len = dt_write(
				"% Page Tree Dictionary\n"
				"2 0 obj <<\n"
				"/Type /Pages\n"
				"/Count 1\n"
				"/Kids[3 0 R]\n"
				">>\n"
				"endobj\n",
				dt_writer
			);
			obj_len.push_back(obj_len.back() + len);

			// ページオブジェクト
			// ページツリーから参照され,
			// リソースとコンテンツを参照する.
			sprintf_s(buf,
				"%% Page Object\n"
				"3 0 obj <<\n"
				"/Type /Page\n"
				"/MediaBox [0 0 %f %f]\n"
				"/Parent 2 0 R\n"
				"/Resources 4 0 R\n"
				"/Contents [5 0 R]\n"
				">>\n"
				"endobj\n",
				w_pt, h_pt);
			len = dt_write(buf, dt_writer);
			obj_len.push_back(obj_len.back() + len);

			// リソース辞書
			// ページとコンテンツから参照され, 辞書を参照する.
			len = dt_write(
				"% Resouces Dictionary\n"
				"4 0 obj <<\n",
				dt_writer);
			// フォント
			int font_cnt = 0;
			std::vector<std::vector<char>> base_font;	// ベースフォント名
			for (const auto s : m_main_sheet.m_shape_list) {
				if (s->is_deleted()) {
					continue;
				}
				if (typeid(*s) == typeid(ShapeText)) {
					if (font_cnt == 0) {
						len += dt_write(
							"/Font <<\n",
							dt_writer);
					}
					sprintf_s(buf,
						"/F%d %d 0 R\n",
						font_cnt, 6 + 3 * font_cnt
					);
					len += dt_write(buf, dt_writer);
					static_cast<ShapeText*>(s)->m_pdf_font_num = font_cnt++;
				}
			}
			if (font_cnt > 0) {
				len += dt_write(">>\n", dt_writer);
			}
			// 画像
			int image_cnt = 0;
			for (const auto s : m_main_sheet.m_shape_list) {
				if (s->is_deleted()) {
					continue;
				}
				if (typeid(*s) == typeid(ShapeImage)) {
					if (image_cnt == 0) {
						len += dt_write(
							"/XObject <<\n",
							dt_writer);
					}
					sprintf_s(buf,
						"/I%d %d 0 R\n",
						image_cnt, 6 + 3 * font_cnt + image_cnt
					);
					len += dt_write(buf, dt_writer);
					static_cast<ShapeImage*>(s)->m_pdf_obj = image_cnt++;
				}
			}
			if (image_cnt > 0) {
				len += dt_write(">>\n", dt_writer);
			}
			len += dt_write(
				">>\n"
				"endobj\n",
				dt_writer);
			obj_len.push_back(obj_len.back() + len);

			// コンテントオブジェクト (ストリーム)
			// 変換行列に 72dpi / 96dpi (=0.75) を指定する.   
			sprintf_s(buf,
				"%% Content Object\n"
				"5 0 obj <<\n"
				">>\n"
				"stream\n"
				"%f 0 0 %f 0 0 cm\n"
				"q\n",
				72.0f / 96.0f, 72.0f / 96.0f
			);
			len = dt_write(buf, dt_writer);

			// 背景
			sprintf_s(buf,
				"%f %f %f rg\n"
				"0 0 %f %f re\n"
				"b\n",
				m_main_sheet.m_sheet_color.r,
				m_main_sheet.m_sheet_color.g,
				m_main_sheet.m_sheet_color.b,
				m_main_sheet.m_sheet_size.width,
				m_main_sheet.m_sheet_size.height
			);
			len += dt_write(buf, dt_writer);

			// 図形を出力
			const D2D1_SIZE_F sheet_size = m_main_sheet.m_sheet_size;
			for (const auto s : m_main_sheet.m_shape_list) {
				if (s->is_deleted()) {
					continue;
				}
				len += s->pdf_write(sheet_size, dt_writer);
			}
			len += dt_write(
				"Q\n"
				"endstream\n"
				"endobj\n", dt_writer
			);
			obj_len.push_back(obj_len.back() + len);

			// 画像 XObject (ストリームオブジェクト) とフォント辞書
			for (const auto s : m_main_sheet.m_shape_list) {
				if (s->is_deleted()) {
					continue;
				}

				if (typeid(*s) == typeid(ShapeImage)) {
					// 画像 XObject (ストリームオブジェクト)
					const ShapeImage* t = static_cast<const ShapeImage*>(s);
					std::vector<uint8_t> z_buf;
					std::vector<uint8_t> in_buf(3ull * t->m_orig.width * t->m_orig.height);
					for (size_t i = 0; i < t->m_orig.width * t->m_orig.height; i++) {
						in_buf[3 * i + 2] = t->m_data[4 * i + 0];	// B
						in_buf[3 * i + 1] = t->m_data[4 * i + 1];	// G
						in_buf[3 * i + 0] = t->m_data[4 * i + 2];	// R
					}
					z_compress(z_buf, /*<---*/std::data(in_buf), std::size(in_buf));
					in_buf.clear();
					in_buf.shrink_to_fit();

					sprintf_s(buf,
						"%% XObject\n"
						"%d 0 obj <<\n"
						"/Type /XObject\n"
						"/Subtype /Image\n"
						"/Width %u\n"
						"/Height %u\n"
						"/Length %zu\n"
						"/ColorSpace /DeviceRGB\n"
						"/BitsPerComponent 8\n"
						//"/Filter /ASCIIHexDecode\n"
						"/Filter /FlateDecode\n"
						">>\n",
						6 + 3 * font_cnt + t->m_pdf_obj,
						t->m_orig.width,
						t->m_orig.height,
						z_buf.size()
					);
					len += dt_write(buf, dt_writer);
					len += dt_write("stream\n", dt_writer);
					dt_writer.WriteBytes(z_buf);
					len += z_buf.size();
					z_buf.clear();
					z_buf.shrink_to_fit();
					/*
					for (uint32_t y = 0; y < t->m_orig.height; y++) {
						if (y > 0) {
							len += dt_write("\n", dt_writer);
						}
						for (uint32_t x = 0; x < t->m_orig.width; x++) {
							// BGRA -> RGB
							sprintf_s(buf,
								"%02x%02x%02x",
								t->m_data[y * 4 * t->m_orig.width + 4 * x + 2],
								t->m_data[y * 4 * t->m_orig.width + 4 * x + 1],
								t->m_data[y * 4 * t->m_orig.width + 4 * x + 0]
							);
							len += dt_write(buf, dt_writer);
						}
					}
					*/
					// ストリームの最後は '>'.
					len += dt_write(
						">\n"
						"endstream\n"
						"endobj\n",
						dt_writer);
					obj_len.push_back(obj_len.back() + len);
				}
				else if (typeid(*s) == typeid(ShapeText)) {
					// フォント辞書 (Type0), CID フォント辞書, フォント詳細辞書
					const auto t = static_cast<ShapeText*>(s);
					int n = t->m_pdf_font_num;

					DWRITE_FONT_WEIGHT weight = DWRITE_FONT_WEIGHT_NORMAL;
					DWRITE_FONT_STRETCH stretch = DWRITE_FONT_STRETCH_NORMAL;
					DWRITE_FONT_STYLE style = DWRITE_FONT_STYLE_NORMAL;
					std::vector<char> psn{};
					std::vector<DWRITE_GLYPH_METRICS> gmet{};
					DWRITE_FONT_METRICS1 fmet{};
					pdf_get_font(t, weight, stretch, style, fmet, psn, gmet);
					const int upem = fmet.designUnitsPerEm;

					// フォント辞書
					sprintf_s(buf,
						"%% Font Dictionary\n"
						"%d 0 obj <<\n"
						"/Type /Font\n"
						"/BaseFont /%s\n"
						"/Subtype /Type0\n"
						//"/Subtype /TrueType\n"
						//"/Encoding /90msp-RKSJ-H\n"
						"/Encoding /UniJIS-UTF16-H\n"
						"/DescendantFonts [%d 0 R]\n"
						">>\n",
						6 + 3 * n,
						std::data(psn),
						6 + 3 * n + 1
					);
					len += dt_write(buf, dt_writer);

					// CID フォント辞書 (Type 0 の子孫となるフォント)
					// フォント辞書から参照され,
					// フォント詳細辞書を参照する.
					sprintf_s(buf,
						"%% Descendant Font Dictionary\n"
						"%d 0 obj <<\n"
						"/Type /Font\n"
						"/Subtype /CIDFontType2\n"
						"/BaseFont /%s\n"
						"/CIDSystemInfo <<\n"
						"/Registry (Adobe)\n"
						"/Ordering (Japan1)\n"
						"/Supplement 7\n"
						">>\n"
						"/FontDescriptor %d 0 R\n",	// 間接参照で必須.
						6 + 3 * n + 1,
						std::data(psn),
						6 + 3 * n + 2
					);
					len = dt_write(buf, dt_writer);
					// 半角のグリフ幅を設定する
					// 設定しなければ全て全角幅になってしまう.
					len += dt_write("/W [1 [", dt_writer);	// CID 開始番号は 1 (半角空白)
					for (int i = 1; i <= gmet.size(); i++) {
						sprintf_s(buf, "%u ", 1000 * gmet[i - 1].advanceWidth / upem);
						len += dt_write(buf, dt_writer);
					}
					len += dt_write("]]\n", dt_writer);
					len += dt_write(
						">>\n"
						"endobj\n",
						dt_writer);
					obj_len.push_back(obj_len.back() + len);

					// フォント詳細辞書
					// CID フォント辞書から参照される.
					constexpr const char* FONT_STRETCH_NAME[] = {
						"Normal",
						"UltraCondensed",
						"ExtraCondensed",
						"Condensed",
						"SemiCondensed",
						"Normal",
						"SemiExpanded",
						"Expanded",
						"ExtraExpanded",
						"UltraExpanded"
					};
					sprintf_s(buf,
						"%% Font Descriptor Dictionary\n"
						"%d 0 obj <<\n"
						"/Type /FontDescriptor\n"
						"/FontName /%s\n"
						"/FontStretch /%s\n"
						"/FontWeight /%d\n"
						"/Flags 4\n"
						"/FontBBox [%d %d %d %d]\n"
						"/ItalicAngle 0\n"
						"/Ascent %u\n"
						"/Descent %u\n"
						"/CapHeight %u\n"
						"/StemV 0\n"
						">>\n"
						"endobj\n",
						6 + 3 * n + 2,
						std::data(psn),
						FONT_STRETCH_NAME[stretch < 10 ? stretch : 0],
						weight <= 900 ? weight : 900,
						1000 * fmet.glyphBoxLeft / upem,
						(1000 * fmet.glyphBoxBottom / upem),
						1000 * fmet.glyphBoxRight / upem,
						(1000 * fmet.glyphBoxTop / upem),
						1000 * fmet.ascent / upem,
						1000 * fmet.descent / upem,
						1000 * fmet.capHeight / upem
					);
					len = dt_write(buf, dt_writer);
					obj_len.push_back(obj_len.back() + len);
				}
			}

			// 相互参照 (クロスリファレンス)
			sprintf_s(
				buf,
				"%% Cross-reference Table\n"
				"xref\n"
				"0 %zu\n"
				"0000000000 65535 f\n",
				obj_len.size()
			);
			dt_write(buf, dt_writer);
			for (int i = 0; i < obj_len.size() - 1; i++) {
				sprintf_s(buf,
					"%010zu 00000 n\n",
					obj_len[i]
				);
				dt_write(buf, dt_writer);
			}

			// トレイラーと EOF
			sprintf_s(
				buf,
				"%% Trailer\n"
				"trailer <<\n"
				"/Size %zu\n"
				"/Root 1 0 R\n"
				">>\n"
				"startxref\n"
				"%zu\n"
				"%%%%EOF\n",
				obj_len.size(),
				obj_len.back()
			);
			dt_write(buf, dt_writer);

			// ストリームの現在位置をストリームの大きさに格納する.
			pdf_stream.Size(pdf_stream.Position());
			// バッファ内のデータをストリームに出力する.
			co_await dt_writer.StoreAsync();
			// ストリームをフラッシュする.
			co_await pdf_stream.FlushAsync();
			// 遅延させたファイル更新を完了し, 結果を判定する.
			if (co_await CachedFileManager::CompleteUpdatesAsync(pdf_file) == FileUpdateStatus::Complete) {
				// 完了した場合, S_OK を結果に格納する.
				hr = S_OK;
			}
		}
		catch (winrt::hresult_error const& e) {
			hr = e.code();
		}
		co_return hr;
	}

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