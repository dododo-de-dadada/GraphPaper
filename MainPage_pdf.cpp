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

	// 図形をデータライターに PDF として書き込む.
	IAsyncOperation<winrt::hresult> MainPage::pdf_write_async(StorageFile pdf_file)
	{
		// システムフォントコレクションを DWriteFactory から得る.
		winrt::com_ptr<IDWriteFontCollection> collection;
		winrt::check_hresult(m_main_sheet.m_d2d.m_dwrite_factory->GetSystemFontCollection(collection.put()));
		//winrt::com_ptr<IDWriteFactory5> f5{
		//	m_main_sheet.m_d2d.m_dwrite_factory.as<IDWriteFactory5>()
		//};
		//winrt::com_ptr<IDWriteInMemoryFontFileLoader> loa;
		//f5->CreateInMemoryFontFileLoader(loa.put());
		//f5->RegisterFontFileLoader(loa.get());
		//int fcnt = loa->GetFileCount();
		//f5->UnregisterFontFileLoader(loa.get());

		std::vector<wchar_t*> used_font{};
		for (const auto s : m_main_sheet.m_shape_list) {
			wchar_t* x;
			if (s->is_deleted() || !s->get_font_family(x)) {
				continue;
			}
			if (std::find_if(used_font.begin(), used_font.end(),
				[x](wchar_t* y) { return equal(x, y); }) != used_font.end()) {
				used_font.push_back(x);
			}
			// 図形からフォントコレクションを得る
			// フォントコレクションからフォントファミリーを得る
			// フォントファミリーからフォントを得る.
			// フォントからフォントフェイスを得る.
			winrt::com_ptr<IDWriteFontCollection> coll;
			s->get_font_collection(coll.put());
			winrt::com_ptr<IDWriteFontFamily> fam;
			coll->GetFontFamily(0, fam.put());
			winrt::com_ptr<IDWriteFont> font;
			fam->GetFont(0, font.put());
			winrt::com_ptr<IDWriteFontFaceReference> ref;
			font.as<IDWriteFont3>()->GetFontFaceReference(ref.put());
			winrt::com_ptr<IDWriteFontFace3> face;
			ref->CreateFontFace(face.put());
			std::vector<UINT32> u32{};
			wchar_t* t;
			if (s->get_text_content(t)) {
				int i = 0;
				for (; t[i] != L'\0'; i++) {
					u32.push_back(t[i]);
				}
			}
			else {
				u32.push_back(L'0');
				u32.push_back(L'1');
				u32.push_back(L'2');
				u32.push_back(L'3');
				u32.push_back(L'4');
				u32.push_back(L'5');
				u32.push_back(L'6');
				u32.push_back(L'7');
				u32.push_back(L'8');
				u32.push_back(L'9');
			}
			std::vector<UINT16> g16(u32.size());
			winrt::check_hresult(face->GetGlyphIndices(u32.data(), static_cast<UINT32>(u32.size()), g16.data()));
			std::vector<UINT16> gid{};
			for (const auto g : gid) {
				if (std::find(gid.begin(), gid.end(), g) != gid.end()) {
					gid.push_back(g);
				}
			}
			std::vector<DWRITE_GLYPH_METRICS> gmet(gid.size());
			face->GetDesignGlyphMetrics(gid.data(), static_cast<UINT32>(gid.size()), gmet.data());
			/*
			winrt::com_ptr<IDWriteFontFile> file;
			ref->GetFontFile(file.put());

			UINT32 index;
			BOOL exists;
			set->FindFontFaceReference(ref.get(), &index, &exists);

			winrt::com_ptr<IDWriteFontSetBuilder> builder;
			m_main_sheet.m_d2d.m_dwrite_factory.as<IDWriteFactory5>()->CreateFontSetBuilder(builder.put());

			m_main_sheet.m_d2d.m_dwrite_factory->CreateFontFace(DWRITE_FONT_FACE_TYPE::DWRITE_FONT_FACE_TYPE_TRUETYPE, 1, )
			*/
		}

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
			// 逆に, 含まない場合は, 置かなくていいってことだろうか.
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
				"% Document Catalog\n"
				"1 0 obj\n"
				"<<\n"
				"/Type /Catalog\n"
				"/Pages 2 0 R\n"
				">>\n"
				"endobj\n", dt_writer);
			obj_len.push_back(obj_len.back() + len);

			// ページツリーノードの辞書
			// カタログから参照され,
			// ページを参照する.
			len = dt_write(
				"% Page Tree\n"
				"2 0 obj\n"
				"<<\n"
				"/Type /Pages\n"
				"/Count 1\n"
				"/Kids[3 0 R]\n"
				">>\n"
				"endobj\n",
				dt_writer
			);
			obj_len.push_back(obj_len.back() + len);

			// ページオブジェクトの辞書
			// ページツリーから参照され,
			// リソースとコンテンツを参照する.
			sprintf_s(buf,
				"%% Page\n"
				"3 0 obj\n"
				"<<\n"
				"/Type /Page\n"
				"/MediaBox [0 0 %f %f]\n"
				"/Resources 4 0 R\n"
				"/Parent 2 0 R\n"
				"/Contents [5 0 R]\n"
				">>\n"
				"endobj\n",
				w_pt, h_pt);
			len = dt_write(buf, dt_writer);
			obj_len.push_back(obj_len.back() + len);

			// ページのリソース辞書
			// ページとコンテンツから参照され,
			// 辞書を参照する.
			len = dt_write(
				"% Resouces\n"
				"4 0 obj\n"
				"<<\n",
				dt_writer);

			int font_cnt = 0;
			std::vector<std::vector<char>> base_font;	// ベースフォント名
			for (const auto s : m_main_sheet.m_shape_list) {
				if (s->is_deleted()) {
					continue;
				}
				if (typeid(*s) == typeid(ShapeText)) {
					if (font_cnt == 0) {
						len += dt_write(
							"/Font\n",
							dt_writer);
					}
					// フォント
					sprintf_s(buf,
						"<</F%d %d 0 R>>\n",
						font_cnt, 6 + 3 * font_cnt
					);
					len += dt_write(buf, dt_writer);
					static_cast<ShapeText*>(s)->m_pdf_font_num = font_cnt++;
				}
			}
			int image_cnt = 0;
			for (const auto s : m_main_sheet.m_shape_list) {
				if (s->is_deleted()) {
					continue;
				}
				if (typeid(*s) == typeid(ShapeImage)) {
					if (image_cnt == 0) {
						len += dt_write(
							"/XObject\n",
							dt_writer);
					}
					// 画像
					sprintf_s(buf,
						"<</I%d %d 0 R>>\n",
						image_cnt, 6 + 3 * font_cnt + image_cnt
					);
					len += dt_write(buf, dt_writer);
					static_cast<ShapeImage*>(s)->m_pdf_image_num = image_cnt++;
				}
			}
			len += dt_write(
				">>\n"
				"endobj\n",
				dt_writer);
			obj_len.push_back(obj_len.back() + len);

			// コンテントストリーム
			// 座標変換に 72dpi / 96dpi (=0.75) を指定する.   
			sprintf_s(buf,
				"%% Content Stream\n"
				"5 0 obj\n"
				"<<\n"
				">>\n"
				"stream\n"
				"%f 0 0 %f 0 0 cm\n"
				"q\n",
				72.0f / 96.0f, 72.0f / 96.0f
			);
			len = dt_write(buf, dt_writer);

			// 背景
			//const double a = m_main_sheet.m_sheet_color.a;
			//const D2D1_COLOR_F c{
			//	(1.0 - a) + m_main_sheet.m_sheet_color.r * a,
			//	(1.0 - a) + m_main_sheet.m_sheet_color.g * a,
			//	(1.0 - a) + m_main_sheet.m_sheet_color.b * a
			//};
			const D2D1_COLOR_F c{ m_main_sheet.m_sheet_color };
			sprintf_s(buf,
				"%f %f %f rg\n"
				"0 0 %f %f re\n"
				"b\n",
				c.r, c.g, c.b,
				m_main_sheet.m_sheet_size.width, m_main_sheet.m_sheet_size.height
			);
			len += dt_write(buf, dt_writer);

			for (const auto s : m_main_sheet.m_shape_list) {
				if (s->is_deleted()) {
					continue;
				}
				len += s->pdf_write(m_main_sheet, dt_writer);
			}
			len += dt_write(
				"Q\n"
				"endstream\n"
				"endobj\n", dt_writer
			);
			obj_len.push_back(obj_len.back() + len);

			// 辞書
			for (const auto s : m_main_sheet.m_shape_list) {
				if (s->is_deleted()) {
					continue;
				}

				if (typeid(*s) == typeid(ShapeImage)) {
					const ShapeImage* t = static_cast<const ShapeImage*>(s);
					std::vector<uint8_t> z_buf;
					const size_t data_size = 4ull * t->m_orig.width * t->m_orig.height;
					z_compress(z_buf, /*<---*/t->m_data, data_size);

					sprintf_s(buf,
						"%% XObject\n"
						"%d 0 obj\n"
						"<<\n"
						"/Type /XObject\n"
						"/Subtype /Image\n"
						"/Width %u\n"
						"/Height %u\n"
						"/Length %u\n"
						"/ColorSpace /DeviceRGB\n"
						"/BitsPerComponent 8\n"
						//"/Filter /ASCIIHexDecode\n"
						"/Filter /FlateDecode\n"
						">>\n",
						6 + 3 * font_cnt + t->m_pdf_image_num,
						t->m_orig.width,
						t->m_orig.height,
						z_buf.size()
					);
					len += dt_write(buf, dt_writer);
					len += dt_write("stream\n", dt_writer);
					dt_writer.WriteBytes(z_buf);
					len += z_buf.size();
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
					int n = static_cast<ShapeText*>(s)->m_pdf_font_num;

					wchar_t* t;	// 図形の文字列
					s->get_text_content(t);
					wchar_t* u;	// 図形の書体名
					s->get_font_family(u);
					DWRITE_FONT_STRETCH font_stretch;	// 幅の伸縮
					s->get_font_stretch(font_stretch);
					DWRITE_FONT_WEIGHT font_weight;
					s->get_font_weight(font_weight);

					UINT32 index;
					BOOL exists;
					collection->FindFamilyName(u, &index, &exists);
					winrt::com_ptr<IDWriteFontFamily> fam;
					collection->GetFontFamily(index, fam.put());
					winrt::com_ptr<IDWriteFont> font;
					fam->GetFont(0, font.put());
					winrt::com_ptr<IDWriteFontFaceReference> ref;
					font.as<IDWriteFont3>()->GetFontFaceReference(ref.put());
					winrt::com_ptr<IDWriteFontFace3> face;
					ref->CreateFontFace(face.put());

					// ベースフォント名を得る.
					// 文字列図形から得たフォント名を,
					// 含まれる半角空白を取り除いて, マルチバイト文字列に変換する.
					std::vector<wchar_t> v{};	// 空白を除いた書体名
					for (int i = 0; u[i] != '\0'; i++) {
						if (u[i] != L' ') {
							v.push_back(u[i]);
						}
					}
					v.push_back(L'\0');
					int base_font_len = WideCharToMultiByte(CP_ACP, 0, v.data(), static_cast<int>(v.size()), NULL, 0, NULL, NULL);
					base_font.emplace_back(base_font_len);
					WideCharToMultiByte(CP_ACP, 0, v.data(), static_cast<int>(v.size()), base_font[n].data(), base_font_len, NULL, NULL);

					// グリフ幅を得るため, 文字列を, 文字がダブらないように, UINT32 型配列にコピーする.
					//std::vector<UINT32> u32{};
					//for (int i = 0; t[i] != L'\0'; i++) {
					//	if (std::find(u32.begin(), u32.end(), t[i]) == u32.end()) {
					//		u32.push_back(t[i]);
					//	}
					//}

					// 半角文字のみグリフ幅を得る.
					// 全ての文字のグリフ幅を取得したいが,
					// GID を CID に変換する方法がよく分からない.
					// 半角に限れば ' ' (32) から '~' (126) までが, 連続した CID (1 から始まる) に対応する.
					// なので半角のみでごまかす.
					std::vector<UINT32> u32{};
					for (int i = 32; i <= 126; i++) {
						u32.push_back(i);
					}
					std::vector<UINT16> gid(u32.size());
					winrt::check_hresult(face->GetGlyphIndices(u32.data(), static_cast<UINT32>(u32.size()), gid.data()));
					std::vector<DWRITE_GLYPH_METRICS> gmet(gid.size());
					face->GetDesignGlyphMetrics(gid.data(), static_cast<UINT32>(gid.size()), gmet.data());

					// 1 em あたりのデザイン単位を得る.
					DWRITE_FONT_METRICS1 fmet;
					font.as<IDWriteFont1>()->GetMetrics(&fmet);
					const int upe = fmet.designUnitsPerEm;


					// フォント辞書
					sprintf_s(buf,
						"%% Font\n"
						"%d 0 obj\n"
						"<<\n"
						"/Type /Font\n"
						"/BaseFont /%s\n"
						"/Subtype /Type0\n"
						//"/Subtype /TrueType\n"
						//"/Encoding /90msp-RKSJ-H\n"
						"/Encoding /UniJIS-UTF16-H\n"
						"/DescendantFonts [%d 0 R]\n"
						">>\n",
						6 + 3 * n,
						base_font[n].data(),
						6 + 3 * n + 1
					);
					len += dt_write(buf, dt_writer);

					// CID フォント辞書
					// フォント辞書から参照され,
					// フォント詳細辞書を参照する.
					sprintf_s(buf,
						"%% Descendant Font\n"
						"%d 0 obj\n"
						"<<\n"
						"/Type /Font\n"
						"/Subtype /CIDFontType2\n"
						"/BaseFont /%s\n"
						"/CIDSystemInfo <<\n"
						"/Registry (Adobe)\n"
						"/Ordering (Japan1)\n"
						"/Supplement 7\n"
						">>\n"
						"/FontDescriptor %d 0 R\n"	// 間接参照で必須.
						//">>\n"
						//"endobj\n"
						,
						6 + 3 * n + 1,
						base_font[n].data(),
						6 + 3 * n + 2
					);
					len = dt_write(buf, dt_writer);

					// 半角のグリフ幅を設定する
					// CID 開始番号は 1 (= 半角空白)
					len += dt_write("/W [1 [\n", dt_writer);
					for (int i = 1; i <= gid.size(); i++) {
						sprintf_s(buf, "%u ", 1000 * gmet[i - 1].advanceWidth / upe);
						len += dt_write(buf, dt_writer);
					}
					len += dt_write(
						"\n]]\n"
						">>\n"
						"endobj\n",
						dt_writer);
					obj_len.push_back(obj_len.back() + len);

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
						"%% Font Descriptor\n"
						"%d 0 obj\n"
						"<<\n"
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
						base_font[n].data(),
						FONT_STRETCH_NAME[font_stretch < 10 ? font_stretch : 0],
						font_weight <= 900 ? font_weight : 900,
						1000 * fmet.glyphBoxLeft / upe,
						(1000 * fmet.glyphBoxBottom / upe),
						1000 * fmet.glyphBoxRight / upe,
						(1000 * fmet.glyphBoxTop / upe),
						1000 * fmet.ascent / upe,
						1000 * fmet.descent / upe,
						1000 * fmet.capHeight / upe
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
				"trailer\n"
				"<<\n"
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