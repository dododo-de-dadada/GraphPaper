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
	using winrt::Windows::Storage::Streams::InMemoryRandomAccessStream;
	using winrt::Windows::Storage::Streams::Buffer;
	using winrt::Windows::Storage::Streams::InputStreamOptions;

	//------------------------------
	// ������}�`����t�H���g�̏��𓾂�.
	// s	������}�`
	// weight	���̂̑���
	// stretch	���̂̕�
	// style	����
	// f_met	���̂̌v��
	// p_name	���̂̃|�X�g�X�N���v�g��
	// g_met	���̂̌v��
	//------------------------------
	static void pdf_get_font(
		const ShapeText* s,
		DWRITE_FONT_WEIGHT& weight,
		DWRITE_FONT_STRETCH& stretch,
		DWRITE_FONT_STYLE& style,
		DWRITE_FONT_METRICS1& f_met,
		std::vector<wchar_t>& p_name,
		std::vector<DWRITE_GLYPH_METRICS>& g_met
	)
	{
		s->get_font_weight(weight);
		s->get_font_stretch(stretch);
		s->get_font_style(style);

		// �����񃌃C�A�E�g���珑�̃R���N�V�����𓾂�.
		IDWriteFontCollection* coll = nullptr;
		if (s->m_dw_text_layout->GetFontCollection(&coll) == S_OK) {
			// �}�`�ƈ�v���鏑�̃t�@�~���𓾂�.
			IDWriteFontFamily* family = nullptr;
			UINT32 index;
			BOOL exists;
			if (coll->FindFamilyName(s->m_font_family, &index, &exists) == S_OK &&
				exists &&
				coll->GetFontFamily(index, &family) == S_OK) {
				// ���̃t�@�~������, �����ƕ�, ���̂���v���鏑�̂𓾂�.
				IDWriteFont* font = nullptr;
				if (family->GetFirstMatchingFont(weight, stretch, style, &font) == S_OK) {
					// ���̂̌v�ʂ𓾂�,
					static_cast<IDWriteFont1*>(font)->GetMetrics(&f_met);
					IDWriteFontFaceReference* ref = nullptr;
					// �e�O���t�̕������𓾂�.
					if (static_cast<IDWriteFont3*>(font)->GetFontFaceReference(&ref) == S_OK) {
						IDWriteFontFace3* face = nullptr;
						if (ref->CreateFontFace(&face) == S_OK) {
							// CID �̋󔒂Ɛ}�`���� (0-94) �ɑΉ�����
							// �A�X�L�[�R�[�h  (32-126) �z��
							static constexpr uint32_t ASCII[]{
								32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47,
								48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63,
								64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79,
								80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95,
								96, 97, 98, 99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111,
								112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126
							};
							static constexpr auto ASCII_SIZE = std::size(ASCII);
							// �A�X�L�[�R�[�h�ɉ����� GID (�O���t�C���f�b�N�X) �𓾂�.
							static uint16_t gid[ASCII_SIZE];
							winrt::check_hresult(face->GetGlyphIndices(std::data(ASCII), static_cast<UINT32>(ASCII_SIZE), std::data(gid)));
							g_met.resize(ASCII_SIZE);
							face->GetDesignGlyphMetrics(std::data(gid), static_cast<UINT32>(ASCII_SIZE), std::data(g_met));

							face->Release();
						}
						ref->Release();
					}
					// ���̂̃|�X�g�X�N���v�g���𓾂�.
					IDWriteLocalizedStrings* str = nullptr;
					exists = false;
					if (font->GetInformationalStrings(DWRITE_INFORMATIONAL_STRING_POSTSCRIPT_NAME, &str, &exists) == S_OK) {
						if (exists) {
							const UINT32 str_cnt = str->GetCount();
							for (uint32_t j = 0; j < str_cnt; j++) {
								UINT32 wstr_len = 0;	// �I�[�k��������������
								if (str->GetStringLength(j, &wstr_len) == S_OK && wstr_len > 0) {
									std::vector<wchar_t> wstr(wstr_len + 1);
									if (str->GetString(j, std::data(wstr), wstr_len + 1) == S_OK) {
										p_name.clear();
										std::copy(wstr.begin(), wstr.end(), std::back_inserter(p_name));
										//int p_name_len = WideCharToMultiByte(CP_ACP, 0, std::data(wstr), wstr_len + 1, NULL, 0, NULL, NULL);
										//p_name.resize(p_name_len);
										//WideCharToMultiByte(CP_ACP, 0, std::data(wstr), wstr_len + 1, std::data(p_name), p_name_len, NULL, NULL);
										break;
									}
								}
							}
						}
						// ���݂��Ȃ��Ȃ珑�̖�����󔒕�������菜�����������, �|�X�g�X�N���v�g���Ƃ���.
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
							p_name.clear();
							std::copy(wstr.begin(), wstr.end(), std::back_inserter(p_name));
							//for (int i = 0; i < wstr_len + 1; i++) {
							//	p_name[i] = wstr[i];
							//}
							//int p_name_len = WideCharToMultiByte(CP_ACP, 0, std::data(wstr), wstr_len + 1, NULL, 0, NULL, NULL);
							//p_name.resize(p_name_len);
							//WideCharToMultiByte(CP_ACP, 0, std::data(wstr), wstr_len + 1, std::data(p_name), p_name_len, NULL, NULL);
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

	// �}�`���f�[�^���C�^�[�� PDF �Ƃ��ď�������.
	// PDF �t�H�[�}�b�g
	// https://aznote.jakou.com/prog/pdf/index.html
	// �ڍ�PDF���� �[ �������Ċw�ڂ��IPDF�t�@�C���̍\���Ƃ��̏������ǂݕ�
	// https://itchyny.hatenablog.com/entry/2015/09/16/100000
	// PDF����u�g����v�e�L�X�g�����o���i��1��j
	// https://golden-lucky.hatenablog.com/entry/2019/12/01/001701
	// PDF �\�����
	// https://www.pdf-tools.trustss.co.jp/Syntax/parsePdfProc.html#proc
	// ���č���Ċw�ԁAPDF�t�@�C���̊�{�\��
	// https://techracho.bpsinc.jp/west/2018_12_07/65062
	// �O���t�ƃO���t�̎��s
	// https://learn.microsoft.com/ja-JP/windows/win32/directwrite/glyphs-and-glyph-runs
	IAsyncOperation<winrt::hresult> MainPage::export_as_pdf_async(const StorageFile pdf_file) const noexcept
	{
		HRESULT hr = E_FAIL;
		try {
			wchar_t buf[1024];	// PDF

			// �\���̕��ƍ�����, D2D �̌Œ� DPI (96dpi) ���� PDF �� 72dpi ��,
			// �ϊ����� (���j�^�[�ɉ����ĕω�����_�� DPI �͗p���Ȃ�). 
			const float w_pt = m_main_page.m_page_size.width * 72.0f / 96.0f;	// �ϊ����ꂽ��
			const float h_pt = m_main_page.m_page_size.height * 72.0f / 96.0f;	// �ϊ����ꂽ����

			// �X�g���[�W�t�@�C�����J����, �X�g���[���Ƃ��̃f�[�^���C�^�[�𓾂�.
			const IRandomAccessStream& pdf_stream{
				co_await pdf_file.OpenAsync(FileAccessMode::ReadWrite)
			};
			DataWriter dt_writer{
				DataWriter(pdf_stream.GetOutputStreamAt(0))
			};

			// PDF �w�b�_�[
			// �t�@�C���Ƀo�C�i���f�[�^���܂܂��ꍇ��,
			// �w�b�_�Ɖ��s�̒����, 4 �ȏ�̃o�C�i���������܂ރR�����g�s��u�����Ƃ�����.
			size_t len = dt_writer.WriteString(
				L"%PDF-1.7\n"
				L"%\xff\xff\xff\xff\n"
			);
			std::vector<size_t> obj_len{};
			obj_len.push_back(len);

			// �J�^���O����.
			// �g���[���[����Q�Ƃ���,
			// �y�[�W�c���[���Q�Ƃ���.
			len = dt_writer.WriteString(
				L"% Catalog Dictionary\n"
				L"1 0 obj <<\n"
				L"/Type /Catalog\n"
				L"/Pages 2 0 R\n"
				L">>\n"
				L"endobj\n");// , dt_writer);
			obj_len.push_back(obj_len.back() + len);

			// �y�[�W�c���[����
			// �J�^���O����Q�Ƃ���,
			// �y�[�W���Q�Ƃ���.
			len = dt_writer.WriteString(
				L"% Page Tree Dictionary\n"
				L"2 0 obj <<\n"
				L"/Type /Pages\n"
				L"/Count 1\n"
				L"/Kids[3 0 R]\n"
				L">>\n"
				L"endobj\n");
			obj_len.push_back(obj_len.back() + len);

			// �y�[�W�I�u�W�F�N�g
			// �y�[�W�c���[����Q�Ƃ���,
			// ���\�[�X�ƃR���e���c���Q�Ƃ���.
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

			// ���\�[�X����
			// �y�[�W�ƃR���e���c����Q�Ƃ���, �������Q�Ƃ���.
			len = dt_writer.WriteString(
				L"% Resouces Dictionary\n"
				L"4 0 obj <<\n");
			// �t�H���g
			int font_cnt = 0;
			std::vector<std::vector<char>> base_font;	// �x�[�X�t�H���g��
			for (const auto s : m_main_page.m_shape_list) {
				if (s->is_deleted()) {
					continue;
				}
				if (typeid(*s) == typeid(ShapeText)) {
					if (font_cnt == 0) {
						len += dt_writer.WriteString(
							L"/Font <<\n");
					}
					swprintf_s(buf,
						L"/F%d %d 0 R\n",
						font_cnt, 6 + 3 * font_cnt
					);
					len += dt_writer.WriteString(buf);
					static_cast<ShapeText*>(s)->m_pdf_font_num = font_cnt++;
				}
			}
			if (font_cnt > 0) {
				len += dt_writer.WriteString(L">>\n");
			}
			// �摜
			int image_cnt = 0;
			for (const auto s : m_main_page.m_shape_list) {
				if (s->is_deleted()) {
					continue;
				}
				if (typeid(*s) == typeid(ShapeImage)) {
					if (image_cnt == 0) {
						len += dt_writer.WriteString(
							L"/XObject <<\n");
					}
					swprintf_s(buf,
						L"/I%d %d 0 R\n",
						image_cnt, 6 + 3 * font_cnt + image_cnt
					);
					len += dt_writer.WriteString(buf);
					static_cast<ShapeImage*>(s)->m_pdf_obj = image_cnt++;
				}
			}
			if (image_cnt > 0) {
				len += dt_writer.WriteString(L">>\n");
			}
			len += dt_writer.WriteString(
				L">>\n"
				L"endobj\n");
			obj_len.push_back(obj_len.back() + len);

			// �R���e���g�I�u�W�F�N�g (�X�g���[��)
			// �ϊ��s��� 72dpi / 96dpi (=0.75) ���w�肷��.   
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

			// �w�i
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

			// �}�`���o��
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

			// �摜 XObject (�X�g���[���I�u�W�F�N�g) �ƃt�H���g����
			for (const auto s : m_main_page.m_shape_list) {
				if (s->is_deleted()) {
					continue;
				}

				if (typeid(*s) == typeid(ShapeImage)) {
					// �摜 XObject (�X�g���[���I�u�W�F�N�g)
					const ShapeImage* t = static_cast<const ShapeImage*>(s);
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

					// BGRA ���� RGB
					std::vector<uint8_t> z_buf;
					std::vector<uint8_t> in_buf(3ull * t->m_orig.width * t->m_orig.height);
					for (size_t i = 0; i < t->m_orig.width * t->m_orig.height; i++) {
						in_buf[3 * i + 2] = t->m_data[4 * i + 0];	// B
						in_buf[3 * i + 1] = t->m_data[4 * i + 1];	// G
						in_buf[3 * i + 0] = t->m_data[4 * i + 2];	// R
					}
					z_compress(z_buf, std::data(in_buf), std::size(in_buf));
					in_buf.clear();
					in_buf.shrink_to_fit();

					swprintf_s(buf,
						L"%% XObject\n"
						L"%d 0 obj <<\n"
						L"/Type /XObject\n"
						L"/Subtype /Image\n"
						L"/Width %u\n"
						L"/Height %u\n"
						L"/Length %zu\n"
						//L"/Length %u\n"
						L"/ColorSpace /DeviceRGB\n"
						L"/BitsPerComponent 8\n"
						//L"/Filter /ASCIIHexDecode\n"
						L"/Filter /FlateDecode\n"
						//L"/Filter /JPXDecode \n"
						L">>\n",
						6 + 3 * font_cnt + t->m_pdf_obj,
						t->m_orig.width,
						t->m_orig.height,
						z_buf.size()
						//image_len
					);
					len += dt_writer.WriteString(buf);
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
								t->m_data[y * 4 * t->m_orig.width + 4 * x + 2],
								t->m_data[y * 4 * t->m_orig.width + 4 * x + 1],
								t->m_data[y * 4 * t->m_orig.width + 4 * x + 0]
							);
							len += dt_writer.WriteString(buf);
						}
					}
					*/
					// �X�g���[���̍Ō�� '>'.
					len += dt_writer.WriteString(
						L">\n"
						L"endstream\n"
						L"endobj\n");
					obj_len.push_back(obj_len.back() + len);
				}
				else if (typeid(*s) == typeid(ShapeText)) {
					// �t�H���g���� (Type0), CID �t�H���g����, �t�H���g�ڍ׎���
					// �O���t�ɂ�������W�l�╝���w�肷��ꍇ�APDF ���ł́A��� 1 em = 1000 �ł�����̂Ƃ��āA
					// �l��ݒ肵�܂��B���ۂ̃t�H���g�� 1 em = 1024 �ȂǂƂȂ��Ă���ꍇ�́An / 1024 * 1000 
					// �Ƃ����悤�ɂ��āA�l�� 1 em = 1000 �ɍ��킹�܂�.
					// 
					// PDF �� FontBBox (Black Box) �̃C���[�W
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
					std::vector<wchar_t> psn{};
					std::vector<DWRITE_GLYPH_METRICS> g_met{};
					DWRITE_FONT_METRICS1 f_met{};
					pdf_get_font(t, weight, stretch, style, f_met, psn, g_met);
					const int upem = f_met.designUnitsPerEm;

					// �t�H���g����
					swprintf_s(buf,
						L"%% Font Dictionary\n"
						L"%d 0 obj <<\n"
						L"/Type /Font\n"
						L"/BaseFont /%s\n"
						L"/Subtype /Type0\n"
						L"/Encoding /UniJIS-UTF16-H\n"
						L"/DescendantFonts [%d 0 R]\n"
						L">>\n",
						6 + 3 * n,
						std::data(psn),
						6 + 3 * n + 1
					);
					len += dt_writer.WriteString(buf);

					// CID �t�H���g���� (Type 0 �̎q���ƂȂ�t�H���g)
					// �t�H���g��������Q�Ƃ���,
					// �t�H���g�ڍ׎������Q�Ƃ���.
					swprintf_s(buf,
						L"%% Descendant Font Dictionary\n"
						L"%d 0 obj <<\n"
						L"/Type /Font\n"
						L"/Subtype /CIDFontType2\n"
						L"/BaseFont /%s\n"
						L"/CIDSystemInfo <<\n"
						L"/Registry (Adobe)\n"
						L"/Ordering (Japan1)\n"
						L"/Supplement 7\n"
						L">>\n"
						L"/FontDescriptor %d 0 R\n",	// �ԐڎQ�ƂŕK�{.
						6 + 3 * n + 1,
						std::data(psn),
						6 + 3 * n + 2
					);
					len = dt_writer.WriteString(buf);
					// ���p�̃O���t����ݒ肷��
					// �ݒ肵�Ȃ���ΑS�đS�p���ɂȂ��Ă��܂�.
					len += dt_writer.WriteString(L"/W [1 [");	// CID �J�n�ԍ��� 1 (���p��)
					for (int i = 1; i <= g_met.size(); i++) {
						swprintf_s(buf, L"%u ", 1000 * g_met[i - 1].advanceWidth / upem);
						len += dt_writer.WriteString(buf);
					}
					len += dt_writer.WriteString(L"]]\n");
					len += dt_writer.WriteString(
						L">>\n"
						L"endobj\n");
					obj_len.push_back(obj_len.back() + len);

					// �t�H���g�ڍ׎���
					// CID �t�H���g��������Q�Ƃ����.
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
					swprintf_s(buf,
						L"%% Font Descriptor Dictionary\n"
						L"%d 0 obj <<\n"
						L"/Type /FontDescriptor\n"
						L"/FontName /%s\n"
						L"/FontStretch /%s\n"
						L"/FontWeight /%d\n"
						L"/Flags 4\n"
						L"/FontBBox [%d %d %d %d]\n"
						L"/ItalicAngle 0\n"
						L"/Ascent %u\n"
						L"/Descent %u\n"
						L"/CapHeight %u\n"
						L"/StemV 0\n"
						L">>\n"
						L"endobj\n",
						6 + 3 * n + 2,
						std::data(psn),
						FONT_STRETCH_NAME[stretch < 10 ? stretch : 0],
						weight <= 900 ? weight : 900,
						1000 * f_met.glyphBoxLeft / upem,
						(1000 * f_met.glyphBoxBottom / upem),
						1000 * f_met.glyphBoxRight / upem,
						(1000 * f_met.glyphBoxTop / upem),
						1000 * f_met.ascent / upem,
						1000 * f_met.descent / upem,
						1000 * f_met.capHeight / upem
					);
					len = dt_writer.WriteString(buf);
					obj_len.push_back(obj_len.back() + len);
				}
			}

			// ���ݎQ�� (�N���X���t�@�����X)
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

			// �g���C���[�� EOF
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

			// �X�g���[���̌��݈ʒu���X�g���[���̑傫���Ɋi�[����.
			pdf_stream.Size(pdf_stream.Position());
			// �o�b�t�@���̃f�[�^���X�g���[���ɏo�͂���.
			co_await dt_writer.StoreAsync();
			// �X�g���[�����t���b�V������.
			co_await pdf_stream.FlushAsync();
			hr = S_OK;
		}
		catch (winrt::hresult_error const& e) {
			hr = e.code();
		}
		co_return hr;
	}

	//-------------------------------
	// �}�`�f�[�^�� SVG �Ƃ��ăX�g���[�W�t�@�C���ɔ񓯊��ɏ�������.
	// svg_file	�������ݐ�̃t�@�C��
	// �߂�l	�������߂��ꍇ S_OK
	//-------------------------------
	IAsyncOperation<winrt::hresult> MainPage::export_as_raster_async(const StorageFile& image_file) noexcept
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

			// Direct2D �R���e���c���摜�t�@�C���ɕۑ�������@
			try {
				// �t�@�C���̃����_���A�N�Z�X�X�g���[��
				IRandomAccessStream image_stream{
					co_await image_file.OpenAsync(FileAccessMode::ReadWrite)
				};

				// WIC �̃����_���A�N�Z�X�X�g���[��
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

				// �f�o�C�X�̍쐬
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
				Shape::s_factory->CreateWicBitmapRenderTarget(wic_bitmap.get(), prop, target.put());
				*/

				// �f�o�C�X�ƃf�o�C�X�R���e�L�X�g�̍쐬
				D2D_UI d2d;

				// �r�b�g�}�b�v�����_�[�^�[�Q�b�g�̍쐬
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

				// �����_�[�^�[�Q�b�g�ˑ��̃I�u�W�F�N�g������
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
				Shape::s_target = target.get();
				Shape::s_color_brush = cb.get();
				Shape::s_range_brush = rb.get();

				// �r�b�g�}�b�v�ւ̕`��
				m_mutex_draw.lock();
				Shape::s_target->SaveDrawingState(m_main_page.m_state_block.get());
				Shape::s_target->BeginDraw();
				m_main_page.draw();
				winrt::check_hresult(
					Shape::s_target->EndDraw()
				);
				Shape::s_target->RestoreDrawingState(m_main_page.m_state_block.get());
				m_mutex_draw.unlock();

				// �����_�[�^�[�Q�b�g�ˑ��̃I�u�W�F�N�g������
				for (const auto s : m_main_page.m_shape_list) {
					if (typeid(*s) == typeid(ShapeImage)) {
						static_cast<ShapeImage*>(s)->m_d2d_bitmap = nullptr;
					}
				}

				// Retrieve D2D Device.
				winrt::com_ptr<ID2D1Device> dev;
				d2d.m_d2d_context->GetDevice(dev.put());

				// IWICImageEncoder ���g�p���� Direct2D �R���e���c����������
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
	// �}�`�f�[�^�� SVG �Ƃ��ăX�g���[�W�t�@�C���ɔ񓯊��ɏ�������.
	// svg_file	�������ݐ�̃t�@�C��
	// �߂�l	�������߂��ꍇ S_OK
	//-------------------------------
	IAsyncOperation<winrt::hresult> MainPage::export_as_svg_async(const StorageFile& svg_file) const noexcept
	{
		HRESULT hr = E_FAIL;
		try {
			// �X�g���[�W�t�@�C�����J����, �X�g���[���Ƃ��̃f�[�^���C�^�[�𓾂�.
			const IRandomAccessStream& svg_stream{
				co_await svg_file.OpenAsync(FileAccessMode::ReadWrite)
			};
			DataWriter dt_writer{
				DataWriter(svg_stream.GetOutputStreamAt(0))
			};
			// XML �錾�� DOCTYPE ����������.
			dt_writer.WriteString(
				L"<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n"
				L"<!DOCTYPE svg PUBLIC \"-//W3C//DTD SVG 1.1//EN\" "
				L"\"http://www.w3.org/Graphics/SVG/1.1/DTD/svg11.dtd\">\n");
			// SVG �J�n�^�O����������.
			{
				const auto size = m_main_page.m_page_size;	// �\���̑傫��
				const auto unit = m_len_unit;	// �����̒P��
				const auto dpi = m_main_d2d.m_logical_dpi;	// �_�� DPI
				const auto color = m_main_page.m_page_color;	// �w�i�F

				// �P�ʕt���ŕ��ƍ����̑�������������.
				wchar_t buf[1024];	// �o�̓o�b�t�@
				double w;	// �P�ʕϊ���̕�
				double h;	// �P�ʕϊ���̍���
				wchar_t* u;	// �P��
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
				// SVG �Ŏg�p�ł����L�̒P�ʈȊO�͂��ׂăs�N�Z��.
				else {
					w = size.width;
					h = size.height;
					u = L"px";
				}

				// �s�N�Z���P�ʂ̕��ƍ����� viewBox �����Ƃ��ď�������.
				// �w�i�F���X�^�C�������Ƃ��ď�������.
				// svg �J�n�^�O�̏I������������.
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

			// �}�`���X�g�̊e�}�`�ɂ��Ĉȉ����J��Ԃ�.
			for (auto s : m_main_page.m_shape_list) {
				if (s->is_deleted()) {
					continue;
				}
				if (typeid(*s) == typeid(ShapeGroup)) {
					co_await static_cast<const ShapeGroup*>(s)->export_as_svg_async(dt_writer);
				}
				// �}�`���摜�����肷��.
				else if (typeid(*s) == typeid(ShapeImage)) {
					co_await static_cast<const ShapeImage*>(s)->export_as_svg_async(dt_writer);
				}
				else {
					s->export_svg(dt_writer);
				}
			}
			// SVG �I���^�O����������.
			dt_writer.WriteString(L"</svg>\n");
			// �X�g���[���̌��݈ʒu���X�g���[���̑傫���Ɋi�[����.
			svg_stream.Size(svg_stream.Position());
			// �o�b�t�@���̃f�[�^���X�g���[���ɏo�͂���.
			co_await dt_writer.StoreAsync();
			// �X�g���[�����t���b�V������.
			co_await svg_stream.FlushAsync();
			hr = S_OK;
		}
		catch (winrt::hresult_error const& e) {
			// �G���[�����������ꍇ, �G���[�R�[�h�����ʂɊi�[����.
			hr = e.code();
		}
		co_return hr;
	}

}