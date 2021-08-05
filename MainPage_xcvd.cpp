//-------------------------------
// MainPage_xcvd.cpp
// �؂���ƃR�s�[, ������̕ҏW�Ȃ�
//-------------------------------
#include "pch.h"
#include "MainPage.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	// �ҏW���j���[�́u�R�s�[�v���I�����ꂽ.
	IAsyncAction MainPage::xcvd_copy_click_async(IInspectable const&, RoutedEventArgs const&)
	{
		using winrt::Windows::ApplicationModel::DataTransfer::Clipboard;
		using winrt::Windows::ApplicationModel::DataTransfer::DataPackage;
		using winrt::Windows::ApplicationModel::DataTransfer::DataPackageOperation;
		using winrt::Windows::Storage::Streams::DataWriter;
		using winrt::Windows::Storage::Streams::InMemoryRandomAccessStream;

		if (m_list_sel_cnt == 0) {
			// �I�����ꂽ�}�`�̐��� 0 �̏ꍇ,
			// �I������.
			return;
		}
		// �I�����ꂽ�}�`�̃��X�g�𓾂�.
		SHAPE_LIST list_selected;
		slist_selected<Shape>(m_list_shapes, list_selected);
		// �R���[�`�����ŏ��ɌĂяo���ꂽ�X���b�h�R���e�L�X�g��ۑ�����.
		winrt::apartment_context context;
		// �o�̓X�g���[�����쐬����, �f�[�^���C�^�[�𓾂�.
		auto mem_stream{ InMemoryRandomAccessStream() };
		auto out_stream{ mem_stream.GetOutputStreamAt(0) };
		auto dt_writer{ DataWriter(out_stream) };
		// �f�[�^���C�^�[�ɑI�����ꂽ�}�`�̃��X�g����������.
		constexpr bool REDUCED = true;
		slist_write<REDUCED>(list_selected, dt_writer);
		// �������񂾂烊�X�g�͔j������.
		list_selected.clear();
		// �������񂾃f�[�^���o�̓X�g���[���Ɋi�[��, �i�[�����o�C�g���𓾂�.
		auto n_byte{ co_await dt_writer.StoreAsync() };
		auto text = slist_selected_all_text(m_list_shapes);
		// �X���b�h�����C���y�[�W�� UI �X���b�h�ɕς���.
		co_await winrt::resume_foreground(Dispatcher());
		if (n_byte > 0) {
			// �i�[�����o�C�g���� 0 �𒴂���ꍇ,
			// �f�[�^�p�b�P�[�W���쐬����.
			auto dt_pkg{ DataPackage() };
			// �X�g���[�����f�[�^�p�b�P�[�W�Ɋi�[����.
			dt_pkg.RequestedOperation(DataPackageOperation::Copy);
			dt_pkg.SetData(CBF_GPD, winrt::box_value(mem_stream));
			if (text.empty() != true) {
				// �����񂪋�łȂ��ꍇ,
				// ��������f�[�^�p�b�P�[�W�Ɋi�[����.
				dt_pkg.SetText(text);
			}
			// �f�[�^�p�b�P�[�W���N���b�v�{�[�h�Ɋi�[����.
			Clipboard::SetContent(dt_pkg);
		}
		// �f�[�^���C�^�[�����.
		dt_writer.Close();
		// �o�̓X�g���[�������.
		out_stream.Close();
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
		if (m_list_sel_cnt == 0) {
			// �I�����ꂽ�}�`�̐��� 0 �̏ꍇ,
			// �I������.
			return;
		}
		// �I�����ꂽ�}�`�̃��X�g�𓾂�.
		SHAPE_LIST list_selected;
		slist_selected<Shape>(m_list_shapes, list_selected);
		m_dx_mutex.lock();
		for (auto s : list_selected) {
			// �ꗗ���\������Ă邩���肷��.
			if (summary_is_visible()) {
				summary_remove(s);
			}
			// �}�`���폜����, ���̑�����X�^�b�N�ɐς�.
			ustack_push_remove(s);
		}
		ustack_push_null();
		m_dx_mutex.unlock();

		// �I�����ꂽ�}�`�̃��X�g����������.
		//list_selected.clear();
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
		using winrt::Windows::ApplicationModel::DataTransfer::Clipboard;
		using winrt::Windows::ApplicationModel::DataTransfer::StandardDataFormats;

		ustack_is_enable();

		uint32_t undeleted_cnt = 0;	// �����t���O���Ȃ��}�`�̐�
		uint32_t selected_cnt = 0;	// �I�����ꂽ�}�`�̐�
		uint32_t selected_group_cnt = 0;	// �I�����ꂽ�O���[�v�}�`�̐�
		uint32_t runlength_cnt = 0;	// �I�����ꂽ�}�`�̃��������O�X�̐�
		uint32_t selected_text_cnt = 0;	// �I�����ꂽ������}�`�̐�
		uint32_t text_cnt = 0;	// ������}�`�̐�
		uint32_t selected_image_cnt = 0;	// �I�����ꂽ�摜�}�`�̐�
		bool fore_selected = false;	// �őO�ʂ̐}�`�̑I���t���O
		bool back_selected = false;	// �Ŕw�ʂ̐}�`�̑I���t���O
		bool prev_selected = false;	// �ЂƂw�ʂ̐}�`�̑I���t���O
		slist_count(
			m_list_shapes,
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
		const auto& dp_view = Clipboard::GetContent();
		mfi_xcvd_paste().IsEnabled(
			dp_view.Contains(CBF_GPD) ||
			dp_view.Contains(StandardDataFormats::Text()) ||
			dp_view.Contains(StandardDataFormats::Bitmap()));
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
		mfi_image_resize_origin().IsEnabled(exists_selected_image);
		m_list_sel_cnt = selected_cnt;
	}

	// �ҏW���j���[�́u�\��t���v���I�����ꂽ.
	IAsyncAction MainPage::xcvd_paste_click_async(IInspectable const&, RoutedEventArgs const&)
	{
		using winrt::Windows::ApplicationModel::DataTransfer::Clipboard;
		using winrt::Windows::ApplicationModel::DataTransfer::DataPackageView;
		using winrt::Windows::ApplicationModel::DataTransfer::StandardDataFormats;
		using winrt::Windows::Storage::Streams::InMemoryRandomAccessStream;
		using winrt::Windows::Storage::Streams::IRandomAccessStream;
		using winrt::Windows::Storage::Streams::DataReader;

		// �R���[�`�����ŏ��ɌĂяo���ꂽ�X���b�h�R���e�L�X�g��ۑ�����.
		winrt::apartment_context context;
		// Clipboard::GetContent() ��, 
		// WinRT originate error 0x80040904
		// �������N�����̂�, try ... catch �����K�v.
		try {
			// �}�`�f�[�^���N���b�v�{�[�h�Ɋ܂܂�Ă��邩���肷��.
			const auto& dp_view = Clipboard::GetContent();
			if (dp_view.Contains(CBF_GPD)) {
				// �N���b�v�{�[�h����ǂݍ��ނ��߂̃f�[�^���[�_�[�𓾂�, �f�[�^��ǂݍ���.
				auto& dt_object{ co_await Clipboard::GetContent().GetDataAsync(CBF_GPD) };
				auto ra_stream{ unbox_value<InMemoryRandomAccessStream>(dt_object) };
				auto in_stream{ ra_stream.GetInputStreamAt(0) };
				auto dt_reader{ DataReader(in_stream) };
				auto ra_size = static_cast<UINT32>(ra_stream.Size());
				auto operation{ co_await dt_reader.LoadAsync(ra_size) };
				// �}�`�̂��߂̃������̊m�ۂ��ʃX���b�h�ōs��ꂽ�ꍇ, D2DERR_WRONG_STATE �������N�������Ƃ�����.
				// �}�`��\��t����O��, �X���b�h�����C���y�[�W�� UI �X���b�h�ɕς���.
				co_await winrt::resume_foreground(Dispatcher());
				// �f�[�^���[�_�[�ɓǂݍ��߂������肷��.
				if (operation == ra_stream.Size()) {
					SHAPE_LIST slist_pasted;	// �\��t�����X�g

					// �f�[�^���[�_�[����\��t�����X�g��ǂݍ���, ���ꂪ��łȂ������肷��.
					if (slist_read(slist_pasted, dt_reader) && !slist_pasted.empty()) {
						m_dx_mutex.lock();
						// ����ꂽ���X�g����łȂ��ꍇ,
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
					}
					else {
						message_show(ICON_ALERT, L"str_err_paste", L"");
					}
				}
				const auto _{ dt_reader.DetachStream() };
				// �f�[�^���[�_�[�����.
				dt_reader.Close();
				in_stream.Close();
			}
			// �N���b�v�{�[�h�Ƀe�L�X�g���܂܂�Ă��邩���肷��.
			else if (dp_view.Contains(StandardDataFormats::Text())) {

				// �N���b�v�{�[�h����ǂݍ��ނ��߂̃f�[�^���[�_�[�𓾂�.
				const auto text{ co_await Clipboard::GetContent().GetTextAsync() };

				// �����񂪋�łȂ������肷��.
				if (!text.empty()) {
					unselect_all();

					// �p�l���̑傫���ŕ�����}�`���쐬����,.
					const float scale = m_sheet_main.m_sheet_scale;
					const float act_w = static_cast<float>(scp_sheet_panel().ActualWidth());
					const float act_h = static_cast<float>(scp_sheet_panel().ActualHeight());
					auto t = new ShapeText(D2D1_POINT_2F{ 0.0f, 0.0f }, D2D1_POINT_2F{ act_w / scale, act_h / scale }, wchar_cpy(text.c_str()), &m_sheet_main);
#if (_DEBUG)
					debug_leak_cnt++;
#endif
					// �g�̑傫���𕶎���ɍ��킹��.
					t->adjust_bbox(m_sheet_main.m_grid_snap ? m_sheet_main.m_grid_base + 1.0f : 0.0f);
					// �p�l���̒����ɂȂ�悤����ʒu�����߂�.
					D2D1_POINT_2F s_min{
						static_cast<FLOAT>((sb_horz().Value() + act_w * 0.5) / scale - t->m_diff[0].x * 0.5),
						static_cast<FLOAT>((sb_vert().Value() + act_h * 0.5) / scale - t->m_diff[0].y * 0.5)
					};
					pt_add(s_min, m_sheet_min, s_min);

					// ����ɍ��킹�邩���肷��.
					D2D1_POINT_2F g_pos{};
					if (m_sheet_main.m_grid_snap) {
						// ����ʒu�����̑傫���Ŋۂ߂�.
						pt_round(s_min, m_sheet_main.m_grid_base + 1.0, g_pos);
						if (m_misc_pile_up < FLT_MIN) {
							s_min = g_pos;
						}
					}

					// ���_���d�˂�臒l���[�����傫�������肷��.
					if (m_misc_pile_up >= FLT_MIN) {
						D2D1_POINT_2F v_pos;
						if (slist_find_vertex_closest(m_list_shapes, s_min, m_misc_pile_up / m_sheet_main.m_sheet_scale, v_pos)) {
							D2D1_POINT_2F v_vec;
							pt_sub(v_pos, s_min, v_vec);
							D2D1_POINT_2F g_vec;
							pt_sub(g_pos, s_min, g_vec);
							if (m_sheet_main.m_grid_snap && pt_abs2(g_vec) < pt_abs2(v_vec)) {
								s_min = g_pos;
							}
							else {
								s_min = v_pos;
							}
						}
						else if (m_sheet_main.m_grid_snap) {
							s_min = g_pos;
						}
					}
					t->set_pos_start(s_min);
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
			}
			else if (dp_view.Contains(StandardDataFormats::Bitmap())) {
				unselect_all();

				// resume_background ����O�� UI ����l�𓾂�.
				const float act_w = static_cast<float>(scp_sheet_panel().ActualWidth());
				const float act_h = static_cast<float>(scp_sheet_panel().ActualHeight());
				const float sb_w = static_cast<float>(sb_horz().Value());
				const float sb_h = static_cast<FLOAT>(sb_vert().Value());
				// resume_background ���Ȃ��� GetBitmapAsync �����s���邱�Ƃ�����.
				co_await winrt::resume_background();
				auto& bitmap{ co_await Clipboard::GetContent().GetBitmapAsync() };
				auto& ra_stream{ co_await bitmap.OpenReadAsync() };
				auto in_stream{ ra_stream.GetInputStreamAt(0) };
				auto dt_reader{ DataReader(in_stream) };
				auto ra_size = static_cast<UINT32>(ra_stream.Size());
				if (co_await dt_reader.LoadAsync(ra_size) == ra_size) {
					// �p���̕\�����ꂽ�����̒��S�̈ʒu�����߂�.
					const float scale = m_sheet_main.m_sheet_scale;
					ShapeImage* img = new ShapeImage({ static_cast<FLOAT>((sb_w + act_w * 0.5) / scale), static_cast<FLOAT>((sb_h + act_h * 0.5) / scale) }, dt_reader);
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
				}
				dt_reader.Close();
				in_stream.Close();
				ra_stream.Close();
				bitmap = nullptr;
			}
		}
		catch (winrt::hresult_error const& e) {
			auto a = e.code();
		}
		//�X���b�h�R���e�L�X�g�𕜌�����.
		co_await context;
	}

}