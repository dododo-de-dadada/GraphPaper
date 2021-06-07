//-------------------------------
// MainPage_xcvd.cpp
// �؂���ƃR�s�[, �\��t��, �폜
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

		if (m_cnt_selected == 0) {
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
		auto ra_stream{ InMemoryRandomAccessStream() };
		auto out_stream{ ra_stream.GetOutputStreamAt(0) };
		auto dt_writer{ DataWriter(out_stream) };
		// �I�����ꂽ�}�`�̃��X�g���f�[�^���C�^�[�ɏ�������.
		constexpr bool REDUCED = true;
		slist_write<REDUCED>(list_selected, dt_writer);
		// �������񂾂烊�X�g�͔j������.
		list_selected.clear();
		// �������񂾃f�[�^���o�̓X�g���[���Ɋi�[��, �i�[�����o�C�g���𓾂�.
		auto n_byte{ co_await dt_writer.StoreAsync() };
		auto text = slist_selected_all_text(m_list_shapes);
		// �X���b�h�����C���y�[�W�� UI �X���b�h�ɕς���.
		auto cd = this->Dispatcher();
		co_await winrt::resume_foreground(cd);
		if (n_byte > 0) {
			// �i�[�����o�C�g���� 0 �𒴂���ꍇ,
			// �f�[�^�p�b�P�[�W���쐬����.
			auto dt_pkg{ DataPackage() };
			// �X�g���[�����f�[�^�p�b�P�[�W�Ɋi�[����.
			dt_pkg.RequestedOperation(DataPackageOperation::Copy);
			dt_pkg.SetData(CF_GPD, winrt::box_value(ra_stream));
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
		edit_menu_is_enabled();
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
		if (m_cnt_selected == 0) {
			// �I�����ꂽ�}�`�̐��� 0 �̏ꍇ,
			// �I������.
			return;
		}
		// �I�����ꂽ�}�`�̃��X�g�𓾂�.
		SHAPE_LIST list_selected;
		slist_selected<Shape>(m_list_shapes, list_selected);
		m_dx_mutex.lock();
		for (auto s : list_selected) {
			if (m_smry_atomic.load(std::memory_order_acquire)) {
				// �}�`�ꗗ�̕\���t���O�������Ă���ꍇ,
				// �}�`���ꗗ�����������.
				smry_remove(s);
			}
			// �}�`���폜����, ���̑�����X�^�b�N�ɐς�.
			undo_push_remove(s);
		}
		undo_push_null();
		m_dx_mutex.unlock();

		// �I�����ꂽ�}�`�̃��X�g����������.
		//list_selected.clear();
		// �ҏW���j���[���ڂ̎g�p�̉ۂ�ݒ肷��.
		edit_menu_is_enabled();
		sheet_update_bbox();
		sheet_panle_size();
		sheet_draw();
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
			if (xcvd_contains({ CF_GPD })) {
				// �N���b�v�{�[�h����ǂݍ��ނ��߂̃f�[�^���[�_�[�𓾂�, �f�[�^��ǂݍ���.
				auto dt_object{ co_await Clipboard::GetContent().GetDataAsync(CF_GPD) };
				auto ra_stream{ unbox_value<InMemoryRandomAccessStream>(dt_object) };
				auto in_stream{ ra_stream.GetInputStreamAt(0) };
				auto dt_reader{ DataReader(in_stream) };
				auto dt_size = static_cast<UINT32>(ra_stream.Size());
				auto operation{ co_await dt_reader.LoadAsync(dt_size) };
				// �}�`�̂��߂̃������̊m�ۂ��ʃX���b�h�ōs��ꂽ�ꍇ, D2DERR_WRONG_STATE �������N�������Ƃ�����.
				// �}�`��\��t����O��, �X���b�h�����C���y�[�W�� UI �X���b�h�ɕς���.
				auto cd = this->Dispatcher();
				co_await winrt::resume_foreground(cd);
				// �f�[�^���[�_�[�ɓǂݍ��߂������肷��.
				if (operation == ra_stream.Size()) {
					SHAPE_LIST slist_pasted;	// �\��t�����X�g

					// �\��t�����X�g���f�[�^���[�_�[����ǂݍ���, ���ꂪ��łȂ������肷��.
					if (slist_read(slist_pasted, dt_reader) && !slist_pasted.empty()) {
						m_dx_mutex.lock();
						// ����ꂽ���X�g����łȂ��ꍇ,
						// �}�`���X�g�̒��̐}�`�̑I�������ׂĉ�������.
						unselect_all();
						// ����ꂽ���X�g�̊e�}�`�ɂ��Ĉȉ����J��Ԃ�.
						for (auto s : slist_pasted) {
							if (m_smry_atomic.load(std::memory_order_acquire)) {
								smry_append(s);
							}
							undo_push_append(s);
							sheet_update_bbox(s);
						}
						undo_push_null();
						m_dx_mutex.unlock();
						slist_pasted.clear();
						// �ҏW���j���[���ڂ̎g�p�̉ۂ�ݒ肷��.
						edit_menu_is_enabled();
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
			else if (xcvd_contains({ StandardDataFormats::Text() })) {
				using winrt::Windows::UI::Xaml::Controls::ContentDialogResult;
				const auto d_result = co_await cd_conf_paste().ShowAsync();
				if (d_result == ContentDialogResult::Primary) {

					// �N���b�v�{�[�h����ǂݍ��ނ��߂̃f�[�^���[�_�[�𓾂�.
					auto text{ co_await Clipboard::GetContent().GetTextAsync() };

					// �����񂪋�łȂ������肷��.
					if (!text.empty()) {

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
						pt_add(s_min, sheet_min(), s_min);

						// ����ɐ���t���O�������Ă��邩���肷��.
						if (m_sheet_main.m_grid_snap) {
							float g_base;
							m_sheet_main.get_grid_base(g_base);
							const auto g_len = g_base + 1.0;
							pt_round(s_min, g_len, s_min);
						}

						// ���߂�����ʒu�Ɉړ�����.
						t->set_start_pos(s_min);

						m_dx_mutex.lock();
						unselect_all();
						undo_push_append(t);
						undo_push_select(t);
						undo_push_null();
						m_dx_mutex.unlock();
						if (m_smry_atomic.load(std::memory_order_acquire)) {
							smry_append(t);
							smry_select(t);
						}
						edit_menu_is_enabled();
						sheet_update_bbox(t);
						sheet_panle_size();
						sheet_draw();
					}
					else {
						message_show(ICON_ALERT, L"str_err_paste", L"");
					}
				}
			}
		}
		catch (winrt::hresult_error const& e) {
			auto a = e.code();
		}
		//�X���b�h�R���e�L�X�g�𕜌�����.
		co_await context;
	}

	// �N���b�v�{�[�h�Ƀf�[�^���܂܂�Ă��邩���肷��.
	bool MainPage::xcvd_contains(const winrt::hstring formats[], const size_t f_cnt) const
	{
		// DataPackageView::Contains ���g�p�����, ���̓����G���[����������.
		// WinRT originate error - 0x8004006A : '�w�肳�ꂽ�`���� DataPackage �Ɋ܂܂�Ă��܂���B
		// DataPackageView.Contains �܂��� DataPackageView.AvailableFormats ���g���āA���̌`�������݂��邱�Ƃ��m���߂Ă��������B
		// �O�̂���, DataPackageView::AvailableFormats ���g��.
		using winrt::Windows::ApplicationModel::DataTransfer::Clipboard;

		auto const& a_formats = Clipboard::GetContent().AvailableFormats();
		for (auto const& a_format : a_formats) {
			for (size_t i = 0; i < f_cnt; i++) {
				if (a_format == formats[i]) {
					return true;
				}
			}
		}
		return false;
	}

}