//-------------------------------
// MainPage_xcvd.cpp
// �؂���ƃR�s�[
//-------------------------------
#include "pch.h"
#include "MainPage.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	using winrt::Windows::ApplicationModel::DataTransfer::Clipboard;
	using winrt::Windows::ApplicationModel::DataTransfer::DataPackage;
	using winrt::Windows::ApplicationModel::DataTransfer::DataPackageOperation;
	using winrt::Windows::ApplicationModel::DataTransfer::DataPackageView;
	using winrt::Windows::ApplicationModel::DataTransfer::StandardDataFormats;
	using winrt::Windows::Graphics::Imaging::BitmapAlphaMode;
	using winrt::Windows::Graphics::Imaging::BitmapDecoder;
	using winrt::Windows::Graphics::Imaging::BitmapEncoder;
	using winrt::Windows::Graphics::Imaging::BitmapPixelFormat;
	using winrt::Windows::Storage::Streams::DataWriter;
	using winrt::Windows::Storage::Streams::IInputStream;
	using winrt::Windows::Storage::Streams::InMemoryRandomAccessStream;
	using winrt::Windows::Storage::Streams::IRandomAccessStreamWithContentType;
	using winrt::Windows::Storage::Streams::IOutputStream;
	using winrt::Windows::Storage::Streams::RandomAccessStreamReference;

	const winrt::param::hstring CLIPBOARD_SHAPES{ L"graph_paper_shapes_data" };	// �}�`�f�[�^�̃N���b�v�{�[�h����
	//const winrt::param::hstring CLIPBOARD_TIFF{ L"TaggedImageFileFormat" };	// TIFF �̃N���b�v�{�[�h���� (Windows10 �ł͂��Ԃ�g���Ȃ�)

	// �\��t����ʒu�����߂�.
	static void xcvd_paste_pos(D2D1_POINT_2F& pos, const SHAPE_LIST& slist, const double grid_len, const float vert_stick);

	// �ҏW���j���[�́u�R�s�[�v���I�����ꂽ.
	// �v���O�����I�������璆�g��������I
	IAsyncAction MainPage::xcvd_copy_click_async(IInspectable const&, RoutedEventArgs const&)
	{
		constexpr bool REDUCED = true;

		if (m_list_sel_cnt == 0) {
			// �I������.
			return;
		}
		// �R���[�`�����Ăяo���ꂽ�X���b�h�R���e�L�X�g��ۑ�����.
		winrt::apartment_context context;
		// �I�����ꂽ�}�`�̃��X�g�𓾂�.
		SHAPE_LIST list_selected;
		slist_selected<Shape>(m_sheet_main.m_shape_list, list_selected);
		// ���X�g����~����, �ŏ��Ɍ�������������}�`�̕�����, ���邢�͉摜�}�`�̉摜�𓾂�.
		wchar_t* txt = nullptr;
		RandomAccessStreamReference img_ref = nullptr;
		for (auto it = list_selected.rbegin(); it != list_selected.rend(); it++) {
			Shape* s = *it;
			if (txt == nullptr) {
				// ��������|�C���^�[�Ɋi�[����.
				s->get_text_content(txt);
			}
			if (img_ref == nullptr && typeid(*s) == typeid(ShapeImage)) {
				// �r�b�g�}�b�v���X�g���[���Q�ƂɊi�[����.
				InMemoryRandomAccessStream img_stream{ InMemoryRandomAccessStream() };
				co_await static_cast<ShapeImage*>(s)->copy_to(BitmapEncoder::BmpEncoderId(), img_stream);
				if (img_stream.Size() > 0) {
					img_ref = RandomAccessStreamReference::CreateFromStream(img_stream);
				}
			}
			if (txt != nullptr && img_ref != nullptr) {
				// ������Ɖ摜�}�`, �����Ƃ����������Ȃ璆�f����.
				break;
			}
		}
		// �������X�g���[�����쐬����, �f�[�^���C�^�[�𓾂�.
		InMemoryRandomAccessStream mem_stream{ InMemoryRandomAccessStream() };
		IOutputStream out_stream{ mem_stream.GetOutputStreamAt(0) };
		DataWriter dt_writer{ DataWriter(out_stream) };
		// �f�[�^���C�^�[�ɑI�����ꂽ�}�`�̃��X�g����������.
		slist_write<REDUCED>(list_selected, /*--->*/dt_writer);
		// ���X�g��j������.
		list_selected.clear();
		// �������X�g���[���Ƀf�[�^���C�^�[�̓��e���i�[��, �i�[�����o�C�g���𓾂�.
		uint32_t n_byte{ co_await dt_writer.StoreAsync() };
		if (n_byte > 0) {
			// ���C���y�[�W�� UI �X���b�h�ɕς���.
			co_await winrt::resume_foreground(Dispatcher());
			// �f�[�^�p�b�P�[�W���쐬��, �f�[�^�p�b�P�[�W�Ƀ������X�g���[�����i�[����.
			DataPackage dt_pkg{ DataPackage() };
			dt_pkg.RequestedOperation(DataPackageOperation::Copy);
			dt_pkg.SetData(CLIPBOARD_SHAPES, winrt::box_value(mem_stream));
			// �����񂪓���ꂽ�����肷��.
			if (txt != nullptr) {
				// �f�[�^�p�b�P�[�W�Ƀe�L�X�g���i�[����.
				dt_pkg.SetText(txt);
			}
			// �摜������ꂽ�����肷��.
			if (img_ref != nullptr) {
				// �f�[�^�p�b�P�[�W�ɉ摜���i�[����.
				dt_pkg.SetBitmap(img_ref);
			}
			// �N���b�v�{�[�h�Ƀf�[�^�p�b�P�[�W���i�[����.
			Clipboard::SetContent(dt_pkg);
			dt_pkg = nullptr;
		}
		// �f�[�^���C�^�[�����.
		dt_writer.Close();
		dt_writer = nullptr;
		// �o�̓X�g���[�������.
		// �������X�g���[���͕�����_��.
		out_stream.Close();
		out_stream = nullptr;
		xcvd_is_enabled();
		// �X���b�h�R���e�L�X�g�𕜌�����.
		co_await context;
	}

	// �ҏW���j���[�́u�؂���v���I�����ꂽ.
	//constexpr uint32_t CUT = 0;
	IAsyncAction MainPage::xcvd_cut_click_async(IInspectable const&, RoutedEventArgs const&)
	{
		co_await xcvd_copy_click_async(nullptr, nullptr);
		xcvd_delete_click(nullptr, nullptr);
	}

	// �ҏW���j���[�́u�폜�v���I�����ꂽ.
	void MainPage::xcvd_delete_click(IInspectable const&, RoutedEventArgs const&)
	{
		// �I�����ꂽ�}�`�̐����[�������肷��.
		if (m_list_sel_cnt == 0) {
			// �I������.
			return;
		}
		// �I�����ꂽ�}�`�̃��X�g�𓾂�.
		SHAPE_LIST list_selected;
		slist_selected<Shape>(m_sheet_main.m_shape_list, list_selected);
		// ���X�g�̊e�}�`�ɂ��Ĉȉ����J��Ԃ�.
		m_dx_mutex.lock();
		for (auto s : list_selected) {
			// �ꗗ���\������Ă邩���肷��.
			if (summary_is_visible()) {
				summary_remove(s);
			}
			// �}�`���폜��, ���̑�����X�^�b�N�ɐς�.
			ustack_push_remove(s);
		}
		ustack_push_null();
		m_dx_mutex.unlock();

		xcvd_is_enabled();
		sheet_update_bbox();
		sheet_panle_size();
		sheet_draw();
	}

	// �ҏW���j���[�̉ۂ�ݒ肷��.
	// �g�𕶎���ɍ��킹��, �ƌ��̑傫���ɖ߂�.
	// �I���̗L����N���X���Ƃɐ}�`�𐔂�, ���j���[���ڂ̉ۂ𔻒肷��.
	void MainPage::xcvd_is_enabled(void)
	{
		ustack_is_enable();

		uint32_t undeleted_cnt = 0;	// �����t���O���Ȃ��}�`�̐�
		uint32_t selected_cnt = 0;	// �I�����ꂽ�}�`�̐�
		uint32_t selected_group_cnt = 0;	// �I�����ꂽ�O���[�v�}�`�̐�
		uint32_t runlength_cnt = 0;	// �I�����ꂽ�}�`�̘A���̐�
		uint32_t selected_text_cnt = 0;	// �I�����ꂽ������}�`�̐�
		uint32_t text_cnt = 0;	// ������}�`�̐�
		uint32_t selected_image_cnt = 0;	// �I�����ꂽ�摜�}�`�̐�
		bool fore_selected = false;	// �őO�ʂ̐}�`�̑I���t���O
		bool back_selected = false;	// �Ŕw�ʂ̐}�`�̑I���t���O
		bool prev_selected = false;	// �ЂƂw�ʂ̐}�`�̑I���t���O
		slist_count(
			m_sheet_main.m_shape_list,
			undeleted_cnt,
			selected_cnt,
			selected_group_cnt,
			runlength_cnt,
			selected_text_cnt,
			text_cnt,
			selected_image_cnt,
			fore_selected,
			back_selected,
			prev_selected
		);

		// ��������Ă��Ȃ��}�`���ЂƂȏ゠��ꍇ.
		const auto exists_undeleted = (undeleted_cnt > 0);
		// �I�����ꂽ�}�`���ЂƂȏ゠��ꍇ.
		const auto exists_selected = (selected_cnt > 0);
		// �I�����ꂽ�����񂪂ЂƂȏ゠��ꍇ.
		const auto exists_selected_text = (selected_text_cnt > 0);
		// �����񂪂ЂƂȏ゠��ꍇ.
		const auto exists_text = (text_cnt > 0);
		// �I�����ꂽ�摜���ЂƂȏ゠��ꍇ.
		const auto exists_selected_image = (selected_image_cnt > 0);
		// �I������ĂȂ��}�`���ЂƂȏ゠��ꍇ.
		const auto exists_unselected = (selected_cnt < undeleted_cnt);
		// �I�����ꂽ�}�`���ӂ��ȏ゠��ꍇ.
		const auto exists_selected_2 = (selected_cnt > 1);
		// �I�����ꂽ�O���[�v���ЂƂȏ゠��ꍇ.
		const auto exists_selected_group = (selected_group_cnt > 0);
		// �O�ʂɔz�u�\�����肷��.
		// 1. �����̃��������O�X������.
		// 2. �܂���, ���Ȃ��Ƃ� 1 �͑I�����ꂽ�}�`������, 
		//    ���őO�ʂ̐}�`�͑I�����ꂢ�Ȃ�.
		const auto enable_forward = (runlength_cnt > 1 || (exists_selected && fore_selected != true));
		// �w�ʂɔz�u�\�����肷��.
		// 1. �����̃��������O�X������.
		// 2. �܂���, ���Ȃ��Ƃ� 1 �͑I�����ꂽ�}�`������, 
		//    ���Ŕw�ʂ̐}�`�͑I�����ꂢ�Ȃ�.
		const auto enable_backward = (runlength_cnt > 1 || (exists_selected && back_selected != true));

		mfi_xcvd_cut().IsEnabled(exists_selected);
		mfi_xcvd_copy().IsEnabled(exists_selected);
		const DataPackageView& dp_view = Clipboard::GetContent();
		mfi_xcvd_paste().IsEnabled(
			dp_view.Contains(CLIPBOARD_SHAPES) ||
			dp_view.Contains(StandardDataFormats::Text()) ||
			dp_view.Contains(StandardDataFormats::Bitmap())); 
			//|| dp_view.Contains(CLIPBOARD_TIFF));
		mfi_xcvd_delete().IsEnabled(exists_selected);
		mfi_select_all().IsEnabled(exists_unselected);
		mfi_group().IsEnabled(exists_selected_2);
		mfi_ungroup().IsEnabled(exists_selected_group);
		mfi_edit_text().IsEnabled(exists_selected_text);
		mfi_find_text().IsEnabled(exists_text);
		mfi_text_fit_frame_to().IsEnabled(exists_selected_text);
		mfi_bring_forward().IsEnabled(enable_forward);
		mfi_bring_to_front().IsEnabled(enable_forward);
		mfi_send_to_back().IsEnabled(enable_backward);
		mfi_send_backward().IsEnabled(enable_backward);
		mfi_summary_list().IsEnabled(exists_undeleted);
		mfi_image_revert_origin().IsEnabled(exists_selected_image);
		m_list_sel_cnt = selected_cnt;
	}

	// �}�`��\��t����.
	IAsyncAction MainPage::xcvd_paste_shape(void)
	{
		bool ok = false;	// �\��t���̐����𔻒�

		// �R���[�`�����Ăяo���ꂽ�X���b�h�R���e�L�X�g��ۑ�����.
		winrt::apartment_context context;

		// �N���b�v�{�[�h����ǂݍ��ނ��߂̃f�[�^���[�_�[�𓾂�, �f�[�^��ǂݍ���.
		IInspectable dt_object{ co_await Clipboard::GetContent().GetDataAsync(CLIPBOARD_SHAPES) };
		InMemoryRandomAccessStream ra_stream{ unbox_value<InMemoryRandomAccessStream>(dt_object) };
		if (ra_stream.Size() <= static_cast<uint64_t>(UINT32_MAX)) {
			IInputStream in_stream{ ra_stream.GetInputStreamAt(0) };
			DataReader dt_reader{ DataReader(in_stream) };
			uint32_t ra_size = static_cast<uint32_t>(ra_stream.Size());
			uint32_t operation{ co_await dt_reader.LoadAsync(ra_size) };
			// �}�`�̂��߂̃������̊m�ۂ��ʃX���b�h�ōs��ꂽ�ꍇ, D2DERR_WRONG_STATE �������N�������Ƃ�����.
			// �}�`��\��t����O��, �X���b�h�����C���y�[�W�� UI �X���b�h�ɕς���.
			co_await winrt::resume_foreground(Dispatcher());
			// �f�[�^���[�_�[�ɓǂݍ��߂������肷��.
			if (operation == ra_size) {
				// �f�[�^���[�_�[����\��t�����X�g��ǂݍ���, ���ꂪ��łȂ������肷��.
				SHAPE_LIST slist_pasted;	// �\��t�����X�g
				if (slist_read(slist_pasted, dt_reader) && !slist_pasted.empty()) {
					m_dx_mutex.lock();
					// �}�`���X�g�̒��̐}�`�̑I�������ׂĉ�������.
					unselect_all();
					// ����ꂽ���X�g�̊e�}�`�ɂ��Ĉȉ����J��Ԃ�.
					for (auto s : slist_pasted) {
						// �ꗗ���\������Ă邩���肷��.
						if (summary_is_visible()) {
							summary_append(s);
						}
						ustack_push_append(s);
						sheet_update_bbox(s);
					}
					ustack_push_null();
					m_dx_mutex.unlock();
					slist_pasted.clear();
					xcvd_is_enabled();
					sheet_panle_size();
					sheet_draw();
					ok = true;
				}
			}
			// �f�[�^���[�_�[�����.
			dt_reader.Close();
			in_stream.Close();
		}
		if (!ok) {
			message_show(ICON_ALERT, L"str_err_paste", L"");
		}
		co_await context;
	}

	IAsyncAction MainPage::xcvd_paste_text(void)
	{
		// �R���[�`�����ŏ��ɌĂяo���ꂽ�X���b�h�R���e�L�X�g��ۑ�����.
		winrt::apartment_context context;
		// �N���b�v�{�[�h����ǂݍ��ނ��߂̃f�[�^���[�_�[�𓾂�.
		const winrt::hstring text{ co_await Clipboard::GetContent().GetTextAsync() };
		if (!text.empty()) {
			unselect_all();

			// �p�l���̑傫���ŕ�����}�`���쐬����,.
			const float scale = m_sheet_main.m_sheet_scale;
			const float win_x = static_cast<float>(sb_horz().Value());
			const float win_y = static_cast<float>(sb_vert().Value());
			const float win_w = static_cast<float>(scp_sheet_panel().ActualWidth());
			const float win_h = static_cast<float>(scp_sheet_panel().ActualHeight());
			const float min_x = m_sheet_min.x;
			const float min_y = m_sheet_min.y;
			auto t = new ShapeText(D2D1_POINT_2F{ 0.0f, 0.0f }, D2D1_POINT_2F{ win_w / scale, win_h / scale }, wchar_cpy(text.c_str()), &m_sheet_main);
#if (_DEBUG)
			debug_leak_cnt++;
#endif
			// �g�̑傫���𕶎���ɍ��킹��.
			t->adjust_bbox(m_sheet_main.m_grid_snap ? m_sheet_main.m_grid_base + 1.0f : 0.0f);
			// �p�l���̒����ɂȂ�悤����ʒu�����߂�.
			D2D1_POINT_2F pos{
				static_cast<FLOAT>(min_x + (win_x + win_w * 0.5) / scale - t->m_vec[0].x * 0.5),
				static_cast<FLOAT>(min_y + (win_y + win_h * 0.5) / scale - t->m_vec[0].y * 0.5)
			};
			const double grid_len = (m_sheet_main.m_grid_snap ? m_sheet_main.m_grid_base + 1.0 : 0.0);
			const float vert_stick = m_misc_vert_stick / m_sheet_main.m_sheet_scale;
			xcvd_paste_pos(pos, /*<---*/m_sheet_main.m_shape_list, grid_len, vert_stick);
			t->set_pos_start(pos);
			m_dx_mutex.lock();
			ustack_push_append(t);
			ustack_push_select(t);
			ustack_push_null();
			m_dx_mutex.unlock();
			// �ꗗ���\������Ă邩���肷��.
			if (summary_is_visible()) {
				summary_append(t);
				summary_select(t);
			}
			xcvd_is_enabled();
			sheet_update_bbox(t);
			sheet_panle_size();
			sheet_draw();
		}
		else {
			message_show(ICON_ALERT, L"str_err_paste", L"");
		}
		co_await context;
	}

	// �\��t����ʒu�����߂�.
	// grid_len	����̑傫��
	// vert_stick	���_���������鋗��
	static void xcvd_paste_pos(D2D1_POINT_2F& pos, const SHAPE_LIST& slist, const double grid_len, const float vert_stick)
	{
		D2D1_POINT_2F v_pos;
		if (grid_len >= 1.0f && vert_stick >= FLT_MIN &&
			slist_find_vertex_closest(slist, pos, vert_stick, v_pos)) {
			// �}�`�̍���ʒu�����̑傫���Ŋۂ�, ���̒l�Ƃ̋��� (�̎���) �����߂�.
			D2D1_POINT_2F g_pos;
			pt_round(pos, grid_len, g_pos);
			D2D1_POINT_2F g_vec;
			pt_sub(g_pos, pos, g_vec);
			const double g_abs = pt_abs2(g_vec);
			// �ߖT�̒��_�Ƃ̋��� (�̎���) �����߂�.
			D2D1_POINT_2F v_vec;
			pt_sub(v_pos, pos, v_vec);
			const double v_abs = pt_abs2(v_vec);
			if (g_abs < v_abs) {
				pos = g_pos;
			}
			else {
				pos = v_pos;
			}
		}
		else if (grid_len >= 1.0f) {
			pt_round(pos, grid_len, pos);
		}
		else if (vert_stick >= FLT_MIN) {
			slist_find_vertex_closest(slist, pos, vert_stick, pos);
		}
	}

	IAsyncAction MainPage::xcvd_paste_image(void)
	{
		// �R���[�`�����ŏ��ɌĂяo���ꂽ�X���b�h�R���e�L�X�g��ۑ�����.
		winrt::apartment_context context;

		unselect_all();

		// resume_background ����O�� UI ����l�𓾂�.
		const float win_w = static_cast<float>(scp_sheet_panel().ActualWidth());
		const float win_h = static_cast<float>(scp_sheet_panel().ActualHeight());
		const float win_x = static_cast<float>(sb_horz().Value());
		const float win_y = static_cast<float>(sb_vert().Value());
		const float min_x = m_sheet_min.x;
		const float min_y = m_sheet_min.y;

		// resume_background ���Ȃ��� GetBitmapAsync �����s���邱�Ƃ�����.
		co_await winrt::resume_background();

		// �N���b�v�{�[�h����r�b�g�}�b�v SoftwareBitmap �����o��.
		RandomAccessStreamReference reference{ co_await Clipboard::GetContent().GetBitmapAsync() };
		IRandomAccessStreamWithContentType stream{ co_await reference.OpenReadAsync() };
		BitmapDecoder bmp_dec{ co_await BitmapDecoder::CreateAsync(stream) };
		SoftwareBitmap bmp{ co_await bmp_dec.GetSoftwareBitmapAsync(BitmapPixelFormat::Bgra8, BitmapAlphaMode::Straight) };

		// �E�B���h�E�̐^�񒆂ɕ\�������悤�ʒu�����߂�.
		// �}�`�̑傫���͌��摜�Ɠ����ɂ���.
		const float img_w = static_cast<float>(bmp.PixelWidth());
		const float img_h = static_cast<float>(bmp.PixelHeight());
		const float scale = m_sheet_main.m_sheet_scale;
		D2D1_POINT_2F pos{
			static_cast<FLOAT>(min_x + (win_x + win_w * 0.5) / scale - img_w * 0.5),
			static_cast<FLOAT>(min_y + (win_y + win_h * 0.5) / scale - img_h * 0.5)
		};
		const D2D1_SIZE_F view_size{ img_w, img_h };

		// �r�b�g�}�b�v����}�`���쐬����.
		ShapeImage* img = new ShapeImage(pos, view_size, bmp, 1.0f);
#if (_DEBUG)
		debug_leak_cnt++;
#endif
		// �r�b�g�}�b�v���, �r�b�g�}�b�v�ƃf�R�[�_�[���������.
		bmp.Close();
		bmp = nullptr;
		bmp_dec = nullptr;
		stream.Close();
		stream = nullptr;
		reference = nullptr;

		const double grid_len = (m_sheet_main.m_grid_snap ? m_sheet_main.m_grid_base + 1.0 : 0.0);
		const float vert_stick = m_misc_vert_stick / m_sheet_main.m_sheet_scale;
		xcvd_paste_pos(pos, /*<---*/m_sheet_main.m_shape_list, grid_len, vert_stick);
		img->set_pos_start(pos);

		m_dx_mutex.lock();
		ustack_push_append(img);
		ustack_push_select(img);
		ustack_push_null();
		m_dx_mutex.unlock();
		co_await winrt::resume_foreground(Dispatcher());
		ustack_is_enable();
		// �ꗗ���\������Ă邩���肷��.
		if (summary_is_visible()) {
			summary_append(img);
			summary_select(img);
		}
		xcvd_is_enabled();
		sheet_update_bbox(img);
		sheet_panle_size();
		sheet_draw();

		//�X���b�h�R���e�L�X�g�𕜌�����.
		co_await context;
	}

	// �ҏW���j���[�́u�\��t���v���I�����ꂽ.
	void MainPage::xcvd_paste_click(IInspectable const&, RoutedEventArgs const&)
	{
		// Clipboard::GetContent() ��, 
		// WinRT originate error 0x80040904
		// �������N�����̂�, try ... catch �����K�v.
		try {
			// �N���b�v�{�[�h�ɐ}�`���܂܂�Ă��邩���肷��.
			const DataPackageView& dp_view = Clipboard::GetContent();
			if (dp_view.Contains(CLIPBOARD_SHAPES)) {
				xcvd_paste_shape();
				return;
			}
			// �N���b�v�{�[�h�Ƀe�L�X�g���܂܂�Ă��邩���肷��.
			else if (dp_view.Contains(StandardDataFormats::Text())) {
				xcvd_paste_text();
				return;
			}
			// �N���b�v�{�[�h�ɉ摜���܂܂�Ă��邩���肷��.
			else if (dp_view.Contains(StandardDataFormats::Bitmap())) {// || dp_view.Contains(CLIPBOARD_TIFF)) {
				xcvd_paste_image();
				return;
			}
		}
		catch (winrt::hresult_error const&) {
		}
		message_show(ICON_ALERT, L"str_err_paste", L"");
	}

}