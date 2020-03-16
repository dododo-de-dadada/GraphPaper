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
	IAsyncAction MainPage::mfi_xcvd_copy_click_async(IInspectable const&, RoutedEventArgs const&)
	{
		using winrt::Windows::ApplicationModel::DataTransfer::Clipboard;
		using winrt::Windows::ApplicationModel::DataTransfer::DataPackage;
		using winrt::Windows::ApplicationModel::DataTransfer::DataPackageOperation;
		using winrt::Windows::Storage::Streams::DataWriter;
		using winrt::Windows::Storage::Streams::InMemoryRandomAccessStream;

		if (m_list_selected == 0) {
			// �I�����ꂽ�}�`�̐��� 0 �̏ꍇ,
			// �I������.
			return;
		}
		// �I�����ꂽ�}�`�̃��X�g�𓾂�.
		S_LIST_T list_selected;
		s_list_selected<Shape>(m_list_shapes, list_selected);
		// �R���[�`�����ŏ��ɌĂяo���ꂽ�X���b�h�R���e�L�X�g��ۑ�����.
		winrt::apartment_context context;
		// �o�̓X�g���[�����쐬����, �f�[�^���C�^�[�𓾂�.
		auto ra_stream{ InMemoryRandomAccessStream() };
		auto out_stream{ ra_stream.GetOutputStreamAt(0) };
		auto dt_writer{ DataWriter(out_stream) };
		// �I�����ꂽ�}�`�̃��X�g���f�[�^���C�^�[�ɏ�������.
		constexpr bool REDUCED = true;
		s_list_write<REDUCED>(list_selected, dt_writer);
		// �������񂾂烊�X�g�͔j������.
		list_selected.clear();
		// �������񂾃f�[�^���o�̓X�g���[���Ɋi�[��, �i�[�����o�C�g���𓾂�.
		auto n_byte{ co_await dt_writer.StoreAsync() };
		// �}�`���X�g���當����𓾂�.
		auto text = s_list_text(m_list_shapes);
		// �X���b�h�����C���y�[�W�� UI �X���b�h�ɕς���.
		co_await winrt::resume_foreground(this->Dispatcher());
		if (n_byte > 0) {
			// �i�[�����o�C�g���� 0 �𒴂���ꍇ,
			// �f�[�^�p�b�P�[�W���쐬����.
			auto dt_pkg{ DataPackage() };
			// �X�g���[�����f�[�^�p�b�P�[�W�Ɋi�[����.
			dt_pkg.RequestedOperation(DataPackageOperation::Copy);
			dt_pkg.SetData(CF_GPD, winrt::box_value(ra_stream));
			if (text.empty() == false) {
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
		// �ҏW���j���[���ڂ̎g�p�̉ۂ�ݒ肷��.
		enable_edit_menu();
		// �X���b�h�R���e�L�X�g�𕜌�����.
		co_await context;
	}

	// �ҏW���j���[�́u�؂���v���I�����ꂽ.
	//constexpr uint32_t CUT = 0;
	IAsyncAction MainPage::mfi_xcvd_cut_click_async(IInspectable const&, RoutedEventArgs const&)
	{
		co_await mfi_xcvd_copy_click_async(nullptr, nullptr);
		mfi_xcvd_delete_click(nullptr, nullptr);
	}

	// �ҏW���j���[�́u�폜�v���I�����ꂽ.
	void MainPage::mfi_xcvd_delete_click(IInspectable const&, RoutedEventArgs const&)
	{
		if (m_list_selected == 0) {
			// �I�����ꂽ�}�`�̐��� 0 �̏ꍇ,
			// �I������.
			return;
		}
		// �I�����ꂽ�}�`�̃��X�g�𓾂�.
		S_LIST_T list_selected;
		s_list_selected<Shape>(m_list_shapes, list_selected);
		// ����ꂽ���X�g�̊e�}�`�ɂ��Ĉȉ����J��Ԃ�.
		for (auto s : list_selected) {
			if (m_summary_visible) {
				// �}�`�ꗗ�̕\���t���O�������Ă���ꍇ,
				// �}�`���ꗗ�����������.
				summary_remove(s);
			}
			// �}�`���폜����, ���̑�����X�^�b�N�ɐς�.
			undo_push_remove(s);
		}
		// �I�����ꂽ�}�`�̃��X�g����������.
		list_selected.clear();
		undo_push_null();
		// �ҏW���j���[���ڂ̎g�p�̉ۂ�ݒ肷��.
		enable_edit_menu();
		s_list_bound(m_list_shapes, m_page_layout.m_page_size, m_page_min, m_page_max);
		set_page_panle_size();
		page_draw();
	}

	// �ҏW���j���[�́u�\��t���v���I�����ꂽ.
	IAsyncAction MainPage::mfi_xcvd_paste_click_async(IInspectable const&, RoutedEventArgs const&)
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
			// �}�`�f�[�^���N���b�v�{�[�h�Ɋ܂܂�Ă��邩���ׂ�.
			if (xcvd_contains({ CF_GPD })) {
				// �N���b�v�{�[�h����ǂݍ��ނ��߂̃f�[�^���[�_�[�𓾂�.
				auto dt_object{ co_await Clipboard::GetContent().GetDataAsync(CF_GPD) };
				auto ra_stream{ unbox_value<InMemoryRandomAccessStream>(dt_object) };
				auto in_stream{ ra_stream.GetInputStreamAt(0) };
				auto dt_reader{ DataReader(in_stream) };
				auto dt_size = static_cast<UINT32>(ra_stream.Size());
				auto operation{ co_await dt_reader.LoadAsync(dt_size) };
				// �}�`�̂��߂̃������̊m�ۂ��ʃX���b�h�ōs��ꂽ�ꍇ,
				// D2DERR_WRONG_STATE �������N�������Ƃ�����.
				// �}�`��\��t����O��, �X���b�h�����C���y�[�W�� UI �X���b�h�ɕς���.
				co_await winrt::resume_foreground(this->Dispatcher());
				if (operation == ra_stream.Size()) {
					S_LIST_T list_pasted;	// �\��t�����X�g
					if (s_list_read(list_pasted, dt_reader) == false) {

					}
					else if (list_pasted.empty() == false) {
						// ����ꂽ���X�g����łȂ��ꍇ,
						// �}�`���X�g�̒��̐}�`�̑I�������ׂĉ�������.
						unselect_all();
						// ����ꂽ���X�g�̊e�}�`�ɂ��Ĉȉ����J��Ԃ�.
						for (auto s : list_pasted) {
							if (m_summary_visible) {
								summary_append(s);
							}
							undo_push_append(s);
							s->get_bound(m_page_min, m_page_max);
						}
						undo_push_null();
						list_pasted.clear();
						// �ҏW���j���[���ڂ̎g�p�̉ۂ�ݒ肷��.
						enable_edit_menu();
						set_page_panle_size();
						page_draw();
					}
				}
				const auto _{ dt_reader.DetachStream() };
				// �f�[�^���[�_�[�����.
				dt_reader.Close();
				in_stream.Close();
			}
			else if (xcvd_contains({ StandardDataFormats::Text() })) {
				using winrt::Windows::UI::Xaml::Controls::ContentDialogResult;
				const auto d_result = co_await cd_conf_paste().ShowAsync();
				if (d_result == ContentDialogResult::Primary) {
					// �N���b�v�{�[�h����ǂݍ��ނ��߂̃f�[�^���[�_�[�𓾂�.
					auto text{ co_await Clipboard::GetContent().GetTextAsync() };
					if (text.empty() == false) {
						// �����񂪋�łȂ��ꍇ,
						const double scale = m_page_layout.m_page_scale;
						const auto g_len = m_page_layout.m_grid_base + 1.0;
						D2D1_POINT_2F pos{
							static_cast<FLOAT>(scale * (sb_horz().Value() + scp_page_panel().ActualWidth() * 0.5) - g_len),
							static_cast<FLOAT>(scale * (sb_vert().Value() + scp_page_panel().ActualHeight() * 0.5) - g_len)
						};
						D2D1_POINT_2F diff{
							static_cast<FLOAT>(2.0 * g_len),
							static_cast<FLOAT>(2.0 * g_len)
						};
						if (m_page_layout.m_grid_snap) {
							pt_round(pos, g_len, pos);
						}
						auto t = new ShapeText(pos, diff, wchar_cpy(text.c_str()), &m_page_layout);
#if (_DEBUG)
						debug_leak_cnt++;
#endif
						unselect_all();
						if (m_summary_visible) {
							summary_append(t);
						}
						undo_push_append(t);
						undo_push_null();
						enable_edit_menu();
						t->get_bound(m_page_min, m_page_max);
						set_page_panle_size();
						page_draw();
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

	// �N���b�v�{�[�h�Ƀf�[�^���܂܂�Ă��邩���ׂ�.
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