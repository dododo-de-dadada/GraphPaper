#include "pch.h"
#include <shcore.h>
#include "MainPage.h"
#include "zlib.h"

using namespace winrt;
using namespace::Zlib::implementation;

namespace winrt::GraphPaper::implementation
{
	using winrt::Windows::Storage::CachedFileManager;
	using winrt::Windows::Storage::FileAccessMode;
	using winrt::Windows::Storage::Provider::FileUpdateStatus;

	// 文字列図形からフォントの情報を得る.
	static void pdf_get_font(
		const ShapeText* s,
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
							// アスキー空白と図形文字 (32-126)
							static constexpr uint32_t ASCII[]{
								32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47,
								48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63,
								64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79,
								80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95,
								96, 97, 98, 99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111,
								112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126
							};
							static constexpr auto ASCII_SIZE = std::size(ASCII);
							static uint16_t gid[ASCII_SIZE];
							winrt::check_hresult(face->GetGlyphIndices(std::data(ASCII), static_cast<UINT32>(ASCII_SIZE), std::data(gid)));
							gmet.resize(ASCII_SIZE);
							face->GetDesignGlyphMetrics(std::data(gid), static_cast<UINT32>(ASCII_SIZE), std::data(gmet));

							face->Release();
						}
						ref->Release();
					}
					// 書体のポストスクリプト名を得る.
					IDWriteLocalizedStrings* str = nullptr;
					exists = false;
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
							for (uint32_t i = 0; i < k; i++) {
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
	IAsyncOperation<winrt::hresult> MainPage::export_to_pdf_async(const StorageFile pdf_file) const noexcept
	{
		HRESULT hr = E_FAIL;
		try {
			char buf[1024];	// PDF

			// 用紙の幅と高さを, D2D の固定 DPI (96dpi) から PDF の 72dpi に,
			// 変換する (モニターに応じて変化する論理 DPI は用いない). 
			const float w_pt = m_main_sheet.m_sheet_size.width * 72.0f / 96.0f;	// 変換された幅
			const float h_pt = m_main_sheet.m_sheet_size.height * 72.0f / 96.0f;	// 変換された高さ

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
				len += s->export_pdf(sheet_size, dt_writer);
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
			hr = S_OK;
		}
		catch (winrt::hresult_error const& e) {
			hr = e.code();
		}
		co_return hr;
	}

	//-------------------------------
	// 図形データを SVG としてストレージファイルに非同期に書き込む.
	// svg_file	書き込み先のファイル
	// 戻り値	書き込めた場合 S_OK
	//-------------------------------
	IAsyncOperation<winrt::hresult> MainPage::export_to_image_async(const StorageFile& image_file) noexcept
	{
		HRESULT hr = E_FAIL;

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
				Shape::s_factory->CreateWicBitmapRenderTarget(wic_bitmap.get(), prop, target.put());
				*/

				// デバイスとデバイスコンテキストの作成
				D2D_UI d2d;

				// ビットマップレンダーターゲットの作成
				const UINT32 sheet_w = static_cast<UINT32>(m_main_sheet.m_sheet_size.width);
				const UINT32 sheet_h = static_cast<UINT32>(m_main_sheet.m_sheet_size.height);
				winrt::com_ptr<ID2D1BitmapRenderTarget> target;
				winrt::check_hresult(
					d2d.m_d2d_context->CreateCompatibleRenderTarget(
						m_main_sheet.m_sheet_size,
						D2D_SIZE_U{ sheet_w, sheet_h },
						D2D1_PIXEL_FORMAT{
							DXGI_FORMAT::DXGI_FORMAT_B8G8R8A8_UNORM,
							D2D1_ALPHA_MODE::D2D1_ALPHA_MODE_PREMULTIPLIED,
						},
						D2D1_COMPATIBLE_RENDER_TARGET_OPTIONS::D2D1_COMPATIBLE_RENDER_TARGET_OPTIONS_NONE,
						target.put()
						)
				);

				// レンダーターゲット依存のオブジェクトを消去
				for (const auto s : m_main_sheet.m_shape_list) {
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
				Shape::s_target = target.get();
				Shape::s_color_brush = cb.get();
				Shape::s_range_brush = rb.get();

				// ビットマップへの描画
				m_mutex_draw.lock();
				Shape::s_target->SaveDrawingState(m_main_sheet.m_state_block.get());
				Shape::s_target->BeginDraw();
				m_main_sheet.draw();
				winrt::check_hresult(
					Shape::s_target->EndDraw()
				);
				Shape::s_target->RestoreDrawingState(m_main_sheet.m_state_block.get());
				m_mutex_draw.unlock();

				// レンダーターゲット依存のオブジェクトを消去
				for (const auto s : m_main_sheet.m_shape_list) {
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
				hr = S_OK;
			}
			catch (const winrt::hresult_error& e) {
				hr = e.code();
			}
		}
		co_return hr;
	}

	//-------------------------------
	// 図形データを SVG としてストレージファイルに非同期に書き込む.
	// svg_file	書き込み先のファイル
	// 戻り値	書き込めた場合 S_OK
	//-------------------------------
	IAsyncOperation<winrt::hresult> MainPage::export_to_svg_async(const StorageFile& svg_file) const noexcept
	{
		HRESULT hr = E_FAIL;
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
				const auto size = m_main_sheet.m_sheet_size;	// 用紙の大きさ
				const auto unit = m_len_unit;	// 長さの単位
				const auto dpi = m_main_d2d.m_logical_dpi;	// 論理 DPI
				const auto color = m_main_sheet.m_sheet_color;	// 背景色

				// 単位付きで幅と高さの属性を書き込む.
				wchar_t buf[1024];	// 出力バッファ
				double w;	// 単位変換後の幅
				double h;	// 単位変換後の高さ
				wchar_t* u;	// 単位
				if (unit == LEN_UNIT::INCH) {
					w = size.width / dpi;
					h = size.height / dpi;
					u = L"px";
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
					u = L"in";
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
			for (auto s : m_main_sheet.m_shape_list) {
				if (s->is_deleted()) {
					continue;
				}
				if (typeid(*s) == typeid(ShapeGroup)) {
					co_await static_cast<const ShapeGroup*>(s)->export_to_svg_async(dt_writer);
				}
				// 図形が画像か判定する.
				else if (typeid(*s) == typeid(ShapeImage)) {
					co_await static_cast<const ShapeImage*>(s)->export_to_svg_async(dt_writer);
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
			hr = S_OK;
		}
		catch (winrt::hresult_error const& e) {
			// エラーが発生した場合, エラーコードを結果に格納する.
			hr = e.code();
		}
		co_return hr;
	}

}