#include "pch.h"
#include "MainPage.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	using winrt::Windows::Storage::CachedFileManager;
	using winrt::Windows::Storage::FileAccessMode;
	using winrt::Windows::Storage::Provider::FileUpdateStatus;

	//-------------------------------
	// �}�`�f�[�^�� SVG �Ƃ��ăX�g���[�W�t�@�C���ɔ񓯊��ɏ�������.
	// svg_file	�������ݐ�̃t�@�C��
	// �߂�l	�������߂��ꍇ S_OK
	//-------------------------------
	IAsyncOperation<winrt::hresult> MainPage::file_svg_write_async(StorageFile svg_file)
	{
		constexpr char XML_DEC[] = "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>" SVG_NEW_LINE;
		constexpr char DOCTYPE[] = "<!DOCTYPE svg PUBLIC \"-//W3C//DTD SVG 1.1//EN\" \"http://www.w3.org/Graphics/SVG/1.1/DTD/svg11.dtd\">" SVG_NEW_LINE;

		m_mutex_exit.lock();
		// �R���[�`���̊J�n���̃X���b�h�R���e�L�X�g��ۑ�����.
//		winrt::apartment_context context;
		// �X���b�h���o�b�N�O���E���h�ɕς���.
//		co_await winrt::resume_background();
		HRESULT hr = E_FAIL;
		try {
			// �t�@�C���X�V�̒x����ݒ肷��.
			CachedFileManager::DeferUpdates(svg_file);
			// �X�g���[�W�t�@�C�����J���ă����_���A�N�Z�X�X�g���[���𓾂�.
			const IRandomAccessStream& svg_stream{
				co_await svg_file.OpenAsync(FileAccessMode::ReadWrite)
			};
			// �����_���A�N�Z�X�X�g���[���̐擪����f�[�^���C�^�[���쐬����.
			DataWriter dt_writer{
				DataWriter(svg_stream.GetOutputStreamAt(0))
			};
			// XML �錾����������.
			svg_dt_write(XML_DEC, dt_writer);
			// DOCTYPE ����������.
			svg_dt_write(DOCTYPE, dt_writer);
			// SVG �J�n�^�O����������.
			{
				const auto size = m_main_sheet.m_sheet_size;	// �p���̑傫��
				const auto unit = m_len_unit;	// �����̒P��
				const auto dpi = m_main_sheet.m_d2d.m_logical_dpi;	// �_�� DPI
				const auto color = m_main_sheet.m_sheet_color;	// �w�i�F
				constexpr char SVG_TAG[] =
					"<svg xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\" ";
				constexpr char* SVG_UNIT_PX = "px";
				constexpr char* SVG_UNIT_IN = "in";
				constexpr char* SVG_UNIT_MM = "mm";
				constexpr char* SVG_UNIT_PT = "pt";

				// SVG �^�O�̊J�n����������.
				svg_dt_write(SVG_TAG, dt_writer);

				// �P�ʕt���ŕ��ƍ����̑�������������.
				char buf[256];
				double w;	// �P�ʕϊ���̕�
				double h;	// �P�ʕϊ���̍���
				char* u;	// �P��
				if (unit == LEN_UNIT::INCH) {
					w = size.width / dpi;
					h = size.height / dpi;
					u = SVG_UNIT_IN;
				}
				else if (unit == LEN_UNIT::MILLI) {
					w = size.width * MM_PER_INCH / dpi;
					h = size.height * MM_PER_INCH / dpi;
					u = SVG_UNIT_MM;
				}
				else if (unit == LEN_UNIT::POINT) {
					w = size.width * PT_PER_INCH / dpi;
					h = size.height * PT_PER_INCH / dpi;
					u = SVG_UNIT_PT;
				}
				// SVG �Ŏg�p�ł����L�̒P�ʈȊO�͂��ׂăs�N�Z��.
				else {
					w = size.width;
					h = size.height;
					u = SVG_UNIT_PX;
				}
				sprintf_s(buf, "width=\"%lf%s\" height=\"%lf%s\" ", w, u, h, u);
				svg_dt_write(buf, dt_writer);

				// �s�N�Z���P�ʂ̕��ƍ����� viewBox �����Ƃ��ď�������.
				svg_dt_write("viewBox=\"0 0 ", dt_writer);
				svg_dt_write(size.width, dt_writer);
				svg_dt_write(size.height, dt_writer);
				svg_dt_write("\" ", dt_writer);

				// �w�i�F���X�^�C�������Ƃ��ď�������.
				svg_dt_write("style=\"background-color:", dt_writer);
				svg_dt_write(color, dt_writer);

				// svg �J�n�^�O�̏I������������.
				svg_dt_write("\" >" SVG_NEW_LINE, dt_writer);
			}

			// �}�`���X�g�̊e�}�`�ɂ��Ĉȉ����J��Ԃ�.
			for (auto s : m_main_sheet.m_shape_list) {
				if (s->is_deleted()) {
					continue;
				}
				// �}�`���摜�����肷��.
				if (typeid(*s) == typeid(ShapeImage)) {
					// ��Ă����摜�t�@�C�����𓾂�.
					// SVG �t�@�C������ xxxx.svg �Ƃ����,
					// ������t�@�C������ xxxx_yyyymmddhhmmss_999.bmp �ɂȂ�
					const size_t NAME_LEN = 1024;
					wchar_t image_name[NAME_LEN];
					{
						static uint32_t magic_num = 0;	// �~���b�̑���
						const time_t t = time(nullptr);
						struct tm tm;
						localtime_s(&tm, &t);
						swprintf(image_name, NAME_LEN - 20, L"%s", svg_file.Name().data());
						const wchar_t* const dot_ptr = wcsrchr(image_name, L'.');
						const size_t dot_len = (dot_ptr != nullptr ? dot_ptr - image_name : wcslen(image_name));	// �s���I�h�܂ł̒���
						const size_t tail_len = dot_len + wcsftime(image_name + dot_len, NAME_LEN - 8 - dot_len, L"_%Y%m%d%H%M%S_", &tm);
						swprintf(image_name + tail_len, NAME_LEN - tail_len, L"%03d", magic_num++);
					}

					// �摜�p�̃t�@�C���ۑ��s�b�J�[���J����, �X�g���[�W�t�@�C���𓾂�.
					// �s�b�J�[���J���̂� UI �X���b�h�ɕς���.
					ShapeImage* const t = static_cast<ShapeImage*>(s);
					//					co_await winrt::resume_foreground(Dispatcher());
					StorageFile image_file{
						co_await file_pick_save_image_async(image_name)
					};
					//					co_await winrt::resume_background();
					if (image_file != nullptr) {
						CachedFileManager::DeferUpdates(image_file);
						IRandomAccessStream image_stream{
							co_await image_file.OpenAsync(FileAccessMode::ReadWrite)
						};
						const bool ret = co_await t->copy_to(m_enc_id, image_stream);
						// �x���������t�@�C���X�V��������, ���ʂ𔻒肷��.
						if (ret && co_await CachedFileManager::CompleteUpdatesAsync(image_file) == FileUpdateStatus::Complete) {
							wcscpy_s(image_name, NAME_LEN, image_file.Path().c_str());
						}
						image_stream.Close();
						image_stream = nullptr;
						image_file = nullptr;

						// �X���b�h�R���e�L�X�g�𕜌�����.
						//co_await context;
						t->svg_write(image_name, dt_writer);
					}
					else {
						t->svg_write(dt_writer);
					}
				}
				else {
					s->svg_write(dt_writer);
				}
			}
			// SVG �I���^�O����������.
			svg_dt_write("</svg>" SVG_NEW_LINE, dt_writer);
			// �X�g���[���̌��݈ʒu���X�g���[���̑傫���Ɋi�[����.
			svg_stream.Size(svg_stream.Position());
			// �o�b�t�@���̃f�[�^���X�g���[���ɏo�͂���.
			co_await dt_writer.StoreAsync();
			// �X�g���[�����t���b�V������.
			co_await svg_stream.FlushAsync();
			// �x���������t�@�C���X�V��������, ���ʂ𔻒肷��.
			if (co_await CachedFileManager::CompleteUpdatesAsync(svg_file) == FileUpdateStatus::Complete) {
				// ���������ꍇ, S_OK �����ʂɊi�[����.
				hr = S_OK;
			}
		}
		catch (winrt::hresult_error const& e) {
			// �G���[�����������ꍇ, �G���[�R�[�h�����ʂɊi�[����.
			hr = e.code();
		}
		// ���ʂ� S_OK �ȊO�����肷��.
		if (hr != S_OK) {
			// �X���b�h�����C���y�[�W�� UI �X���b�h�ɕς���.
//			co_await winrt::resume_foreground(Dispatcher());
			// �u�t�@�C���ɏ������߂܂���v���b�Z�[�W�_�C�A���O��\������.
			message_show(ICON_ALERT, RES_ERR_WRITE, svg_file.Path());
		}
		// �X���b�h�R���e�L�X�g�𕜌�����.
//		co_await context;
		m_mutex_exit.unlock();
		// ���ʂ�Ԃ��I������.
		co_return hr;
	}

}