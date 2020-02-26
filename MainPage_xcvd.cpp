//-------------------------------
// MainPage_xcvd.cpp
// �؂���ƃR�s�[, �\��t��, �폜
//-------------------------------
#include "pch.h"
#include "MainPage.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	// �I�����ꂽ�}�`���N���b�v�{�[�h�ɔ񓯊��ɕۑ�����.
	// X	�؂���t���O. �t���O�������Ă���ꍇ�͐؂���, �Ȃ��ꍇ�̓R�s�[.
	constexpr uint32_t CUT = 0;
	constexpr uint32_t COPY = 1;
	template <uint32_t X>
	IAsyncAction MainPage::clipboard_copy_async(void)
	{
		using winrt::Windows::ApplicationModel::DataTransfer::Clipboard;
		using winrt::Windows::ApplicationModel::DataTransfer::DataPackage;
		using winrt::Windows::ApplicationModel::DataTransfer::DataPackageOperation;
		using winrt::Windows::Storage::Streams::DataWriter;
		using winrt::Windows::Storage::Streams::InMemoryRandomAccessStream;

		//	�I�����ꂽ�}�`�̃��X�g�𓾂�.
		S_LIST_T sel_list;
		s_list_select<Shape>(m_list_shapes, sel_list);
		if (sel_list.empty()) {
			//	����ꂽ���X�g����̏ꍇ,
			//	�I������.
			return;
		}
		if constexpr (X == CUT) {
			// �t���O���؂���̏ꍇ,
			// �I�����ꂽ�}�`��}�`���X�g����폜����.
			for (auto s : sel_list) {
				if (m_summary_visible) {
					summary_remove(s);
				}
				undo_push_remove(s);
			}
			undo_push_null();
			s_list_bound(m_list_shapes, m_page_panel.m_page_size, m_page_min, m_page_max);
			set_page_panle_size();
			draw_page();
		}
		//	�R���[�`�����ŏ��ɌĂяo���ꂽ�X���b�h�R���e�L�X�g��ۑ�����.
		winrt::apartment_context context;
		//	�o�̓X�g���[�����쐬����, �f�[�^���C�^�[�𓾂�.
		auto ra_stream{ InMemoryRandomAccessStream() };
		auto out_stream{ ra_stream.GetOutputStreamAt(0) };
		auto dt_writer{ DataWriter(out_stream) };
		//	�I�����ꂽ�}�`�̃��X�g���f�[�^���C�^�[�ɏ�������.
		constexpr bool REDUCED = true;
		s_list_write<REDUCED>(sel_list, dt_writer);
		//	�������񂾂烊�X�g�͔j������.
		sel_list.clear();
		//	�������񂾃f�[�^���o�̓X�g���[���Ɋi�[��, �i�[�����o�C�g���𓾂�.
		auto n_byte{ co_await dt_writer.StoreAsync() };
		//	�}�`���X�g���當����𓾂�.
		auto text = s_list_text(m_list_shapes);
		//	�X���b�h�����C���y�[�W�� UI �X���b�h�ɕς���.
		co_await winrt::resume_foreground(this->Dispatcher());
		if (n_byte > 0) {
			//	�i�[�����o�C�g���� 0 �𒴂���ꍇ,
			//	�f�[�^�p�b�P�[�W���쐬����.
			auto dt_pkg{ DataPackage() };
			//	�X�g���[�����f�[�^�p�b�P�[�W�Ɋi�[����.
			dt_pkg.RequestedOperation(DataPackageOperation::Copy);
			dt_pkg.SetData(FMT_DATA, winrt::box_value(ra_stream));
			if (text.empty() == false) {
				//	�����񂪋�łȂ��ꍇ,
				//	��������f�[�^�p�b�P�[�W�Ɋi�[����.
				dt_pkg.SetText(text);
			}
			//	�f�[�^�p�b�P�[�W���N���b�v�{�[�h�Ɋi�[����.
			Clipboard::SetContent(dt_pkg);
		}
		//	�f�[�^���C�^�[�����.
		dt_writer.Close();
		//	�o�̓X�g���[�������.
		out_stream.Close();
		//	���ɖ߂�/��蒼�����j���[���ڂ̎g�p�ۂ�ݒ肷��.
		enable_undo_menu();
		//	�ҏW���j���[���ڂ̎g�p�ۂ�ݒ肷��.
		enable_edit_menu();
		//	�X���b�h�R���e�L�X�g�𕜌�����.
		co_await context;
	}

	bool MainPage::clipboard_contains(winrt::hstring const c_formats[], const uint32_t c_count) const
	{
		using winrt::Windows::ApplicationModel::DataTransfer::Clipboard;

		auto const& a_formats = Clipboard::GetContent().AvailableFormats();
		for (auto const& a_format : a_formats) {
			for (uint32_t i = 0; i < c_count; i++) {
				if (a_format == c_formats[i]) {
					return true;
				}
			}
		}
		return false;
	}

	// �N���b�v�{�[�h�ɕۑ����ꂽ�}�`��񓯊��ɓ\��t����
	IAsyncAction MainPage::clipboard_paste_async(void)
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
				auto pkg_view{ Clipboard::GetContent() };
				if (pkg_view.Contains(FMT_DATA)) {
					// �N���b�v�{�[�h����ǂݍ��ނ��߂̃f�[�^���[�_�[�𓾂�.
					auto dt_object{ co_await pkg_view.GetDataAsync(FMT_DATA) };
					auto ra_stream{ unbox_value<InMemoryRandomAccessStream>(dt_object) };
					auto in_stream{ ra_stream.GetInputStreamAt(0) };
					auto dt_reader{ DataReader(in_stream) };
					auto dt_size = static_cast<UINT32>(ra_stream.Size());
					auto operation{ co_await dt_reader.LoadAsync(dt_size) };
					//	�}�`�̂��߂̃������̊m�ۂ��ʃX���b�h�ōs��ꂽ�ꍇ,
					//	D2DERR_WRONG_STATE �������N�������Ƃ�����.
					//	�}�`��\��t����O��, �X���b�h�����C���y�[�W�� UI �X���b�h�ɕς���.
					co_await winrt::resume_foreground(this->Dispatcher());
					if (operation == ra_stream.Size()) {
						S_LIST_T paste_list;	// �\��t�����X�g
						s_list_read(paste_list, dt_reader);
						if (paste_list.empty() == false) {
							// �\��t�����X�g����łȂ��ꍇ,
							// �}�`���X�g�̒��̐}�`�̑I�������ׂĉ�����,
							// �\��t�����X�g�̐}�`��ǉ�����.
							unselect_all();
							for (auto s : paste_list) {
								if (m_summary_visible) {
									summary_append(s);
								}
								undo_push_append(s);
								s->get_bound(m_page_min, m_page_max);
							}
							undo_push_null();
							paste_list.clear();
							enable_undo_menu();
							enable_edit_menu();
							set_page_panle_size();
							draw_page();
						}
					}
					const auto _{ dt_reader.DetachStream() };
					//	�f�[�^���[�_�[�����.
					dt_reader.Close();
					in_stream.Close();
				}
				else if (pkg_view.Contains(StandardDataFormats::Text())) {
					// �N���b�v�{�[�h����ǂݍ��ނ��߂̃f�[�^���[�_�[�𓾂�.
					/*
					�e�L�X�g�𒼐ړ\��t����
					auto text{ co_await pkg_view.GetTextAsync() };
					if (text.empty() == false) {
						wchar_t const* w = text.c_str();
						D2D1_POINT_2F d = { 100, 100 };
						D2D1_POINT_2F p;
						const double sh = sb_horz().Value();
						const double sv = sb_vert().Value();
						const double sc = m_page_panel.m_page_scale;
						const double aw = scp_page_panel().ActualWidth();
						const double ah = scp_page_panel().ActualHeight();
						p.x = static_cast<FLOAT>(sc * (sh + aw * 0.5) - d.x * 0.5);
						p.y = static_cast<FLOAT>(sc * (sv + ah * 0.5) - d.y * 0.5);
						auto t = new ShapeText(p, d, wchar_cpy(w), &m_page_panel);
						unselect_all();
						if (m_summary_visible) {
							summary_append(t);
						}
						undo_push_append(t);
						undo_push_null();
						enable_undo_menu();
						enable_edit_menu();
						t->get_bound(m_page_min, m_page_max);
						set_page_panle_size();
						draw_page();
					}
					*/
				}
			}
			catch (winrt::hresult_error const& e) {
				auto a = e.code();
			}
		//}
		//�X���b�h�R���e�L�X�g�𕜌�����.
		co_await context;
		co_return;
	}

	// �ҏW���j���[�́u�R�s�[�v���I�����ꂽ.
	void MainPage::mfi_copy_click(IInspectable const& /*sender*/, RoutedEventArgs const& /*args*/)
	{
		auto _{ clipboard_copy_async<COPY>() };
	}

	// �ҏW���j���[�́u�؂���v���I�����ꂽ.
	void MainPage::mfi_cut_click(IInspectable const& /*sender*/, RoutedEventArgs const& /*args*/)
	{
		auto _{ clipboard_copy_async<CUT>() };
	}

	void MainPage::delete_selected_shapes(void)
	{
		S_LIST_T sel_list;
		s_list_select<Shape>(m_list_shapes, sel_list);
		if (sel_list.size() == 0) {
			return;
		}
		for (auto s : sel_list) {
			if (m_summary_visible) {
				summary_remove(s);
			}
			undo_push_remove(s);
		}
		sel_list.clear();
		undo_push_null();
		enable_undo_menu();
		enable_edit_menu();
		s_list_bound(m_list_shapes, m_page_panel.m_page_size, m_page_min, m_page_max);
		set_page_panle_size();
		draw_page();
	}

	// �ҏW���j���[�́u�폜�v���I�����ꂽ.
	void MainPage::mfi_delete_click(IInspectable const& /*sender*/, RoutedEventArgs const& /*args*/)
	{
		delete_selected_shapes();
	}

	// �ҏW���j���[�́u�\��t���v���I�����ꂽ.
	void MainPage::mfi_paste_click(IInspectable const& /*sender*/, RoutedEventArgs const& /*args*/)
	{
		auto _{ clipboard_paste_async() };
	}

}