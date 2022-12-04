#include "pch.h"
#include "MainPage.h"

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

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	using winrt::Windows::Storage::FileAccessMode;
	using winrt::Windows::Storage::CachedFileManager;
	using winrt::Windows::Storage::Provider::FileUpdateStatus;

	IAsyncOperation<winrt::hresult> MainPage::pdf_write_async(StorageFile pdf_file)
	{
		// �V�X�e���t�H���g�R���N�V������ DWriteFactory ���瓾��.
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
			// �}�`����t�H���g�R���N�V�����𓾂�
			// �t�H���g�R���N�V��������t�H���g�t�@�~���[�𓾂�
			// �t�H���g�t�@�~���[����t�H���g�𓾂�.
			// �t�H���g����t�H���g�t�F�C�X�𓾂�.
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
			// �p���̕��ƍ�����, D2D �̌Œ� DPI (96dpi) ���� PDF �� 72dpi ��,
			// �ϊ����� (���j�^�[�ɉ����ĕω�����_�� DPI �͗p���Ȃ�). 
			const float w_pt = m_main_sheet.m_sheet_size.width * 72.0f / 96.0f;	// �ϊ����ꂽ��
			const float h_pt = m_main_sheet.m_sheet_size.height * 72.0f / 96.0f;	// �ϊ����ꂽ����
			// �t�@�C���X�V�̒x����ݒ肷��.
			CachedFileManager::DeferUpdates(pdf_file);
			// �X�g���[�W�t�@�C�����J���ă����_���A�N�Z�X�X�g���[���𓾂�.
			const IRandomAccessStream& pdf_stream{
				co_await pdf_file.OpenAsync(FileAccessMode::ReadWrite)
			};
			// �����_���A�N�Z�X�X�g���[���̐擪����f�[�^���C�^�[���쐬����.
			// �K���擪���w�肷��.
			DataWriter dt_writer{
				DataWriter(pdf_stream.GetOutputStreamAt(0))
			};

			// PDF �w�b�_�[
			// �t�@�C���Ƀo�C�i���f�[�^���܂܂��ꍇ��,
			// �w�b�_�Ɖ��s�̒����, 4 �ȏ�̃o�C�i���������܂ރR�����g�s��u�����Ƃ�����.
			// �t��, �܂܂Ȃ��ꍇ��, �u���Ȃ��Ă������Ă��Ƃ��낤��.
			size_t len = dt_write(
				"%PDF-1.7\n",
				dt_writer);
			std::vector<size_t> obj_len{};
			obj_len.push_back(len);

			// �J�^���O����.
			// �g���[���[����Q�Ƃ���,
			// �y�[�W�c���[���Q�Ƃ���.
			char buf[1024];
			len = dt_write(
				"% Document Catalog\n"
				"1 0 obj\n"
				"<<\n"
				"/Type /Catalog\n"
				"/Pages 2 0 R\n"
				">>\n"
				"endobj\n", dt_writer);
			obj_len.push_back(obj_len.back() + len);

			// �y�[�W�c���[�m�[�h�̎���
			// �J�^���O����Q�Ƃ���,
			// �y�[�W���Q�Ƃ���.
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

			// �y�[�W�I�u�W�F�N�g�̎���
			// �y�[�W�c���[����Q�Ƃ���,
			// ���\�[�X�ƃR���e���c���Q�Ƃ���.
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

			// ���\�[�X
			// �y�[�W�ƃR���e���c����Q�Ƃ���,
			// �������Q�Ƃ���.
			len = dt_write(
				"% Resouces\n"
				"4 0 obj\n"
				"<<\n",
				dt_writer);

			// ���\�[�X - �t�H���g
			len += dt_write(
				"/Font\n",
				dt_writer);

			int k = 0;
			std::vector<std::vector<char>> base_font;	// �x�[�X�t�H���g��
			for (const auto s : m_main_sheet.m_shape_list) {
				if (s->is_deleted() || typeid(*s) != typeid(ShapeText)) {
					continue;
				}
				// �t�H���g
				sprintf_s(buf,
					"<</F%d %d 0 R>>\n",
					k, 6 + 3 * k
				);
				len += dt_write(buf, dt_writer);

				static_cast<ShapeText*>(s)->m_pdf_font = k++;
			}
			len += dt_write(">>\n", dt_writer);
			len += dt_write(
				">>\n"
				"endobj\n",
				dt_writer);
			obj_len.push_back(obj_len.back() + len);

			// �R���e���g�X�g���[��
			// ���W�ϊ��� 72dpi / 96dpi (=0.75) ���w�肷��.   
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

			// �w�i
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
				//m_main_sheet.m_sheet_size.height, m_main_sheet.m_sheet_size.width
			);

			for (const auto s : m_main_sheet.m_shape_list) {
				len += s->pdf_write(m_main_sheet, dt_writer);
			}
			len += dt_write(
				"Q\n"
				"endstream\n"
				"endobj\n", dt_writer
			);
			obj_len.push_back(obj_len.back() + len);

			// ����
			for (const auto s : m_main_sheet.m_shape_list) {
				if (s->is_deleted() || typeid(*s) != typeid(ShapeText)) {
					continue;
				}

				wchar_t* t;	// �}�`�̕�����
				s->get_text_content(t);
				wchar_t* u;	// �}�`�̏��̖�
				s->get_font_family(u);
				DWRITE_FONT_STRETCH font_stretch;	// ���̐L�k
				s->get_font_stretch(font_stretch);
				DWRITE_FONT_WEIGHT font_weight;
				s->get_font_weight(font_weight);
				int n = static_cast<ShapeText*>(s)->m_pdf_font;

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

				// �x�[�X�t�H���g���𓾂�.
				// ������}�`���瓾���t�H���g����,
				// �܂܂�锼�p�󔒂���菜����, �}���`�o�C�g������ɕϊ�����.
				std::vector<wchar_t> v{};	// �󔒂����������̖�
				for (int i = 0; u[i] != '\0'; i++) {
					if (u[i] != L' ') {
						v.push_back(u[i]);
					}
				}
				v.push_back(L'\0');
				int name_len = WideCharToMultiByte(CP_ACP, 0, v.data(), static_cast<int>(v.size()), NULL, 0, NULL, NULL);
				base_font.emplace_back(name_len);
				WideCharToMultiByte(CP_ACP, 0, v.data(), static_cast<int>(v.size()), base_font[n].data(), name_len, NULL, NULL);

				// �������, �������_�u��Ȃ��悤��, UINT32 �^�z��ɃR�s�[����.
				std::vector<UINT32> u32{};
				for (int i = 0; t[i] != L'\0'; i++) {
					if (std::find(u32.begin(), u32.end(), t[i]) == u32.end()) {
						u32.push_back(t[i]);
					}
				}
				std::vector<UINT16> gid(u32.size());
				winrt::check_hresult(face->GetGlyphIndices(u32.data(), static_cast<UINT32>(u32.size()), gid.data()));
				std::vector<DWRITE_GLYPH_METRICS> gmet(gid.size());
				face->GetDesignGlyphMetrics(gid.data(), static_cast<UINT32>(gid.size()), gmet.data());
				DWRITE_FONT_METRICS1 fmet;
				font.as<IDWriteFont1>()->GetMetrics(&fmet);
				const int per_em = fmet.designUnitsPerEm;


				// �t�H���g����
				sprintf_s(buf,
					"%% Font\n"
					"%d 0 obj\n"
					"<<\n"
					"/Type /Font\n"
					"/BaseFont /%s\n"
					"/Subtype /Type0\n"
					"/Encoding /90msp-RKSJ-H\n"
					//"/Encoding /UniJIS-UTF16-H\n"
					"/DescendantFonts[%d 0 R]\n"
					">>\n",
					6 + 3 * n,
					base_font[n].data(),
					6 + 3 * n + 1
				);
				len += dt_write(buf, dt_writer);

				// CID �t�H���g����
				// �t�H���g��������Q�Ƃ���,
				// �t�H���g�ڍ׎������Q�Ƃ���.
				// W = �e�O���t���̕����܂�.
				sprintf_s(buf,
					"%% Descendant Font\n"
					"%d 0 obj\n"
					"<<\n"
					"/Type /Font\n"
					"/Subtype /CIDFontType0\n"
					"/BaseFont /%s\n"
					"/CIDSystemInfo <<\n"
					"/Registry (Adobe)\n"
					"/Ordering (Japan1)\n"
					"/Supplement 6\n"
					">>\n"
					"/FontDescriptor %d 0 R\n",	// �ԐڎQ�ƂŕK�{.
					6 + 3 * n + 1,
					base_font[n].data(),
					6 + 3 * n + 2
				);
				len = dt_write(buf, dt_writer);
				/*
				len += dt_write("/W [\n", dt_writer);
				for (int i = 0; i < gid.size(); i++) {
					sprintf_s(buf,
						"%u %u %u\n",
						gid[i], gid[i], 1000 * gmet[i].advanceWidth / per_em
					);
					len += dt_write(buf, dt_writer);
				}
				len += dt_write(
					"]\n"
					">>\n"
					"endobj\n",
					dt_writer);
				*/
				obj_len.push_back(obj_len.back() + len);

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
					"/Descent -%u\n"
					"/CapHeight %u\n"
					"/StemV 0\n"
					">>\n"
					"endobj\n",
					6 + 3 * n + 2,
					base_font[n].data(),
					FONT_STRETCH_NAME[font_stretch < 10 ? font_stretch : 0],
					font_weight <= 900 ? font_weight : 900,
					1000 * fmet.glyphBoxLeft / per_em,
					-(1000 * fmet.glyphBoxTop / per_em),
					1000 * fmet.glyphBoxRight / per_em,
					-(1000 * fmet.glyphBoxBottom / per_em),
					1000 * fmet.ascent / per_em,
					1000 * fmet.descent / per_em,
					1000 * fmet.capHeight / per_em
				);
				len = dt_write(buf, dt_writer);
				obj_len.push_back(obj_len.back() + len);
			}

			// ���ݎQ�� (�N���X���t�@�����X)
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

			// �g���C���[�� EOF
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

			// �X�g���[���̌��݈ʒu���X�g���[���̑傫���Ɋi�[����.
			pdf_stream.Size(pdf_stream.Position());
			// �o�b�t�@���̃f�[�^���X�g���[���ɏo�͂���.
			co_await dt_writer.StoreAsync();
			// �X�g���[�����t���b�V������.
			co_await pdf_stream.FlushAsync();
			// �x���������t�@�C���X�V��������, ���ʂ𔻒肷��.
			if (co_await CachedFileManager::CompleteUpdatesAsync(pdf_file) == FileUpdateStatus::Complete) {
				// ���������ꍇ, S_OK �����ʂɊi�[����.
				hr = S_OK;
			}
		}
		catch (winrt::hresult_error const& e) {
			hr = e.code();
		}
		co_return hr;
	}

}