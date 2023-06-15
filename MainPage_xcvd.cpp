//-------------------------------
// MainPage_xcvd.cpp
// �؂��� (x) �ƃR�s�[ (c), �\��t�� (v), �폜 (d)
//-------------------------------
#include "pch.h"
#include "MainPage.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	using winrt::Windows::Graphics::Imaging::BitmapAlphaMode;
	using winrt::Windows::Graphics::Imaging::BitmapDecoder;
	using winrt::Windows::Graphics::Imaging::BitmapPixelFormat;
	using winrt::Windows::ApplicationModel::DataTransfer::DataPackage;
	using winrt::Windows::ApplicationModel::DataTransfer::DataPackageOperation;
	using winrt::Windows::ApplicationModel::DataTransfer::DataPackageView;
	using winrt::Windows::Storage::Streams::IInputStream;
	using winrt::Windows::Storage::Streams::InMemoryRandomAccessStream;
	using winrt::Windows::Storage::Streams::IOutputStream;
	using winrt::Windows::Storage::Streams::IRandomAccessStreamWithContentType;
	using winrt::Windows::Storage::Streams::RandomAccessStreamReference;
	using winrt::Windows::System::VirtualKey;
	using winrt::Windows::UI::Core::CoreVirtualKeyStates;
	using winrt::Windows::UI::Xaml::FocusState;

	using winrt::Windows::Graphics::Imaging::BitmapEncoder;

	const winrt::param::hstring CLIPBOARD_FORMAT_SHAPES{ L"graph_paper_shape_data" };	// �}�`�f�[�^�̃N���b�v�{�[�h����

	// �\��t����ʒu�����߂�.
	static void xcvd_get_paste_pos(D2D1_POINT_2F& paste_pt, const D2D1_POINT_2F q, const SHAPE_LIST& slist, const double grid_len, const float interval);

	//------------------------------
	// �ҏW���j���[�́u�R�s�[�v���I�����ꂽ.
	//------------------------------
	IAsyncAction MainPage::copy_click_async(IInspectable const& sender, RoutedEventArgs const& args)
	{
		if (! m_main_sheet_focused) {
			co_return;
		}

		// �ҏW���̕����񂪂�����, �I��͈͂�����Ȃ�, �I��͈͂̕�������N���b�v�{�[�h�Ɋi�[����.
		if (m_core_text_focused != nullptr && core_text_selected_len() > 0) {
			winrt::hstring selected_text{ core_text_substr() };
			DataPackage dt_pack{ DataPackage() };
			dt_pack.RequestedOperation(DataPackageOperation::Copy);
			dt_pack.SetText(selected_text);
			Clipboard::SetContent(dt_pack);
		}
		// �����łȂ����, �}�`�̓��e�̕������摜, �����đI�����ꂽ�}�`���N���b�v�{�[�h�Ɋi�[��
		else {
			winrt::apartment_context context;

			// �I�����ꂽ�}�`�̃��X�g�𓾂�.
			SHAPE_LIST selected_list;
			slist_get_selected<Shape>(m_main_sheet.m_shape_list, selected_list);

			// ���X�g����~����, �ŏ��Ɍ�������������ƃr�b�g�}�b�v�𓾂�.
			wchar_t* text_ptr = nullptr;
			RandomAccessStreamReference image_ref = nullptr;
			for (auto it = selected_list.rbegin(); it != selected_list.rend(); it++) {
				// �ŏ��Ɍ���������������|�C���^�[�Ɋi�[����.
				if (text_ptr == nullptr) {
					(*it)->get_text_content(text_ptr);
				}
				// �ŏ��Ɍ��������r�b�g�}�b�v�������������_���A�N�Z�X�X�g���[���Ɋi�[��, ���̎Q�Ƃ𓾂�.
				if (image_ref == nullptr && typeid(*it) == typeid(ShapeImage)) {
					InMemoryRandomAccessStream image_stream{
						InMemoryRandomAccessStream()
					};
					if (co_await static_cast<ShapeImage*>(*it)->copy<false>(BitmapEncoder::BmpEncoderId(), image_stream) && image_stream.Size() > 0) {
						image_ref = RandomAccessStreamReference::CreateFromStream(image_stream);
					}
				}
				// ������ƃr�b�g�}�b�v, ����������������, ����ȏ�͕K�v�Ȃ��̂Œ��f����.
				if (text_ptr != nullptr && image_ref != nullptr) {
					break;
				}
			}

			// �������X�g���[�����쐬����, ���̃f�[�^���C�^�[���쐬����.
			InMemoryRandomAccessStream mem_stream{
				InMemoryRandomAccessStream()
			};
			IOutputStream out_stream{
				mem_stream.GetOutputStreamAt(0)
			};
			DataWriter dt_writer{ DataWriter(out_stream) };

			// �f�[�^���C�^�[�ɑI�����ꂽ�}�`�̃��X�g����������.
			constexpr bool REDUCED = true;
			slist_write<REDUCED>(selected_list, /*--->*/dt_writer);

			// �I�����ꂽ�}�`�̃��X�g��j������.
			selected_list.clear();

			// �������X�g���[���Ƀf�[�^���C�^�[�̓��e���i�[��, �i�[�����o�C�g���𓾂�.
			uint32_t n_byte{
				co_await dt_writer.StoreAsync()
			};
			if (n_byte > 0) {

				// ���C���y�[�W�� UI �X���b�h�ɕς���.
				co_await winrt::resume_foreground(Dispatcher());

				// �f�[�^�p�b�P�[�W���쐬��, �f�[�^�p�b�P�[�W�Ƀ������X�g���[�����i�[����.
				DataPackage content{
					DataPackage()
				};
				content.RequestedOperation(DataPackageOperation::Copy);
				content.SetData(CLIPBOARD_FORMAT_SHAPES, winrt::box_value(mem_stream));

				// �����񂪂���Ȃ�f�[�^�p�b�P�[�W�Ƀe�L�X�g���i�[����.
				if (text_ptr != nullptr) {
					content.SetText(text_ptr);
				}

				// �r�b�g�}�b�v������Ȃ�f�[�^�p�b�P�[�W�Ƀe�L�X�g���i�[����.
				if (image_ref != nullptr) {
					content.SetBitmap(image_ref);
				}

				// �N���b�v�{�[�h�Ƀf�[�^�p�b�P�[�W���i�[����.
				Clipboard::SetContent(content);
				content = nullptr;
			}

			// �f�[�^���C�^�[�����.
			dt_writer.Close();
			dt_writer = nullptr;

			// �o�̓X�g���[�������.
			// �������X�g���[���͕�����_��.
			out_stream.Close();
			out_stream = nullptr;

			// �X���b�h�R���e�L�X�g�𕜌�����.
			co_await context;
		}
		status_bar_set_pos();
	}

	//------------------------------
	// �ҏW���j���[�́u�؂���v���I�����ꂽ.
	//------------------------------
	IAsyncAction MainPage::cut_click_async(IInspectable const& sender, RoutedEventArgs const&)
	{
		if (!m_main_sheet_focused) {
			co_return;
		}

		co_await copy_click_async(nullptr, nullptr);

		if (m_core_text_focused != nullptr) {
			core_text_delete_selection();
		}
		else {
			delete_click(nullptr, nullptr);
		}
	}

	//------------------------------
	// �ҏW���j���[�́u�폜�v���I�����ꂽ.
	//------------------------------
	void MainPage::delete_click(IInspectable const&, RoutedEventArgs const&)
	{
		if (!m_main_sheet_focused) {
			return;
		}
		// XAML �̃L�[�{�[�h�A�N�Z�����[�^�[�ɍ폜�L�[�͎w�肳��Ă���,
		// CoreWWindow �� KeyDow �łȂ�, ���̃n���h���[�ŏ��������.
		if (m_core_text_focused != nullptr) {
			const auto shift_key = ((CoreWindow::GetForCurrentThread().GetKeyState(VirtualKey::Shift) & CoreVirtualKeyStates::Down) == CoreVirtualKeyStates::Down);
			if (shift_key) {
				core_text_delete_key<true>();
			}
			else {
				core_text_delete_key<false>();
			}
		}
		else {
			// �I�����ꂽ�}�`�̐����[�������肷��.
			if (m_undo_select_cnt > 0) {
				undo_push_null();
				// �I�����ꂽ�}�`�̃��X�g�𓾂�.
				SHAPE_LIST selected_list;
				slist_get_selected<Shape>(m_main_sheet.m_shape_list, selected_list);
				// ���X�g�̊e�}�`�ɂ��Ĉȉ����J��Ԃ�.
				m_mutex_draw.lock();
				for (auto s : selected_list) {
					// �ꗗ���\������Ă邩���肷��.
					if (summary_is_visible()) {
						summary_remove(s);
					}
					// �}�`����苎��, ���̑�����X�^�b�N�ɐς�.
					undo_push_remove(s);
				}
				m_mutex_draw.unlock();
				main_bbox_update();
				main_panel_size();
				menu_is_enable();
				main_draw();
				selected_list.clear();
			}
			status_bar_set_pos();
		}
	}

	//------------------------------
	// �ҏW���j���[�́u�\��t���v���I�����ꂽ.
	//------------------------------
	void MainPage::paste_click(IInspectable const&, RoutedEventArgs const&)
	{
		if (!m_main_sheet_focused) {
			return;
		}
		if (tx_find_text_what().FocusState() != FocusState::Unfocused ||
			tx_find_replace_with().FocusState() != FocusState::Unfocused) {
			return;
		}
		// Clipboard::GetContent() ��, 
		// WinRT originate error 0x80040904
		// �������N�����̂�, try ... catch �����K�v.
		try {
			// �N���b�v�{�[�h�ɐ}�`���܂܂�Ă��邩���肷��.
			const auto& dp_view = Clipboard::GetContent();
			if (dp_view.Contains(CLIPBOARD_FORMAT_SHAPES)) {
				xcvd_paste_shape();
			}
			// �N���b�v�{�[�h�ɕ����񂪊܂܂�Ă��邩���肷��.
			else if (dp_view.Contains(StandardDataFormats::Text())) {
				xcvd_paste_text();
			}
			// �N���b�v�{�[�h�Ƀr�b�g�}�b�v���܂܂�Ă��邩���肷��.
			else if (dp_view.Contains(StandardDataFormats::Bitmap())) {
				xcvd_paste_image();
			}
			else {
				throw winrt::hresult_error();
			}
		}
		catch (winrt::hresult_error const&) {
			message_show(ICON_ALERT, L"str_err_paste", L"");
		}
		status_bar_set_pos();
	}

	//------------------------------
	// �摜��\��t����.
	//------------------------------
	IAsyncAction MainPage::xcvd_paste_image(void)
	{
		unselect_shape_all();

		// resume_background ����O�� UI �v�f����l�𓾂�.
		const float win_w = static_cast<float>(scp_main_panel().ActualWidth());
		const float win_h = static_cast<float>(scp_main_panel().ActualHeight());
		const float win_x = static_cast<float>(sb_horz().Value());
		const float win_y = static_cast<float>(sb_vert().Value());
		const float lt_x = m_main_bbox_lt.x;
		const float lt_y = m_main_bbox_lt.y;

		// resume_background ���Ȃ��� GetBitmapAsync �����s���邱�Ƃ�����.
		winrt::apartment_context context;
		co_await winrt::resume_background();

		// �N���b�v�{�[�h����r�b�g�}�b�v SoftwareBitmap �����o��.
		RandomAccessStreamReference reference{
			co_await Clipboard::GetContent().GetBitmapAsync()
		};
		IRandomAccessStreamWithContentType stream{
			co_await reference.OpenReadAsync()
		};
		BitmapDecoder bmp_dec{
			co_await BitmapDecoder::CreateAsync(stream)
		};
		SoftwareBitmap bmp{
			co_await bmp_dec.GetSoftwareBitmapAsync(
				BitmapPixelFormat::Bgra8, BitmapAlphaMode::Straight)
		};

		// �E�B���h�E�̐^�񒆂ɕ\�������悤�ʒu�����߂�.
		// �}�`�̑傫���͌��摜�Ɠ����ɂ���.
		const float img_w = static_cast<float>(bmp.PixelWidth());
		const float img_h = static_cast<float>(bmp.PixelHeight());
		const float scale = m_main_scale;
		D2D1_POINT_2F lt{
			static_cast<FLOAT>(lt_x + (win_x + win_w * 0.5) / scale - img_w * 0.5),
			static_cast<FLOAT>(lt_y + (win_y + win_h * 0.5) / scale - img_h * 0.5)
		};

		// �r�b�g�}�b�v����}�`���쐬����.
		ShapeImage* s = new ShapeImage(lt, D2D1_SIZE_F{ img_w, img_h }, bmp, 1.0f);
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

		const double g_len = (m_snap_grid ? m_main_sheet.m_grid_base + 1.0 : 0.0);
		xcvd_get_paste_pos(lt , /*<---*/lt, m_main_sheet.m_shape_list, g_len, m_snap_point / m_main_scale);
		s->set_pos_start(lt);
		{
			m_mutex_draw.lock();
			undo_push_null();
			undo_push_append(s);
			undo_push_select(s);
			m_mutex_draw.unlock();
		}

		co_await winrt::resume_foreground(Dispatcher());
		// �ꗗ���\������Ă邩���肷��.
		if (summary_is_visible()) {
			summary_append(s);
			summary_select(s);
		}
		main_bbox_update(s);
		main_panel_size();
		menu_is_enable();
		main_draw();

		//�X���b�h�R���e�L�X�g�𕜌�����.
		co_await context;
	}

	//------------------------------
	// �}�`��\��t����.
	//------------------------------
	IAsyncAction MainPage::xcvd_paste_shape(void)
	{
		bool ok = false;	// �\��t���̐����𔻒�

		// �R���[�`�����Ăяo���ꂽ�X���b�h�R���e�L�X�g��ۑ�����.
		winrt::apartment_context context;

		// �N���b�v�{�[�h����ǂݍ��ނ��߂̃f�[�^���[�_�[�𓾂�, �f�[�^��ǂݍ���.
		IInspectable dt_object{
			co_await Clipboard::GetContent().GetDataAsync(CLIPBOARD_FORMAT_SHAPES)
		};
		InMemoryRandomAccessStream ra_stream{
			unbox_value<InMemoryRandomAccessStream>(dt_object)
		};
		if (ra_stream.Size() <= static_cast<uint64_t>(UINT32_MAX)) {
			IInputStream in_stream{
				ra_stream.GetInputStreamAt(0)
			};
			DataReader dt_reader{
				DataReader(in_stream)
			};
			uint32_t ra_size = static_cast<uint32_t>(ra_stream.Size());
			uint32_t operation{
				co_await dt_reader.LoadAsync(ra_size)
			};
			// �f�[�^���[�_�[�ɓǂݍ��߂������肷��.
			if (operation == ra_size) {
				// �}�`�̂��߂̃������̊m�ۂ��ʃX���b�h�ōs��ꂽ�ꍇ, 
				// D2DERR_WRONG_STATE �������N�������Ƃ�����.
				// �}�`��\��t����O��, �X���b�h�����C���y�[�W�� UI �X���b�h�ɕς���.
				co_await winrt::resume_foreground(Dispatcher());
				// �f�[�^���[�_�[����\��t�����X�g��ǂݍ���, ���ꂪ��łȂ������肷��.
				SHAPE_LIST slist_pasted;	// �\��t�����X�g
				if (slist_read(slist_pasted, dt_reader) && !slist_pasted.empty()) {
					m_mutex_draw.lock();
					// �}�`���X�g�̒��̐}�`�̑I�������ׂĉ�������.
					undo_push_null();
					unselect_shape_all();
					// ����ꂽ���X�g�̊e�}�`�ɂ��Ĉȉ����J��Ԃ�.
					for (auto s : slist_pasted) {
						// �ꗗ���\������Ă邩���肷��.
						if (summary_is_visible()) {
							summary_append(s);
						}
						undo_push_append(s);
						main_bbox_update(s);
					}
					m_mutex_draw.unlock();
					main_panel_size();
					menu_is_enable();
					main_draw();
					slist_pasted.clear();
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

	//------------------------------
	// �������\��t����
	//------------------------------
	IAsyncAction MainPage::xcvd_paste_text(void)
	{
		// �R���[�`�����ŏ��ɌĂяo���ꂽ�X���b�h�R���e�L�X�g��ۑ�����.
		winrt::apartment_context context;
		// �N���b�v�{�[�h����ǂݍ��ނ��߂̃f�[�^���[�_�[�𓾂�.
		const winrt::hstring text{ co_await Clipboard::GetContent().GetTextAsync() };

		if (!text.empty()) {
			if (m_core_text_focused != nullptr) {
				core_text_insert(text.data(), static_cast<uint32_t>(text.size()));
			}
			// �����񌟍��p�l���̃e�L�X�g�{�b�N�X�Ƀt�H�[�J�X��������Ε�����}�`�Ƃ��Ă͂����.
			else {

				undo_push_null();
				unselect_shape_all();

				// �p�l���̑傫���ŕ�����}�`���쐬����,.
				const float scale = m_main_scale;
				//const float scale = m_main_sheet.m_sheet_scale;
				const float win_x = static_cast<float>(sb_horz().Value()) / scale;	// �p���̕\������Ă��鍶�ʒu
				const float win_y = static_cast<float>(sb_vert().Value()) / scale;	// �p���̕\������Ă����ʒu
				const float win_w = min(static_cast<float>(scp_main_panel().ActualWidth()) / scale, m_main_sheet.m_sheet_size.width); // �p���̕\������Ă��镝
				const float win_h = min(static_cast<float>(scp_main_panel().ActualHeight()) / scale, m_main_sheet.m_sheet_size.height); // �p���̕\������Ă��鍂��
				const float lt_x = m_main_bbox_lt.x;
				const float lt_y = m_main_bbox_lt.y;
				ShapeText* t = new ShapeText(D2D1_POINT_2F{ 0.0f, 0.0f }, D2D1_POINT_2F{ win_w, win_h }, wchar_cpy(text.c_str()), &m_main_sheet);
#if (_DEBUG)
				debug_leak_cnt++;
#endif
				// �g�𕶎���ɍ��킹��.
				const double g_len = (m_snap_grid ? m_main_sheet.m_grid_base + 1.0 : 0.0);
				t->fit_frame_to_text(static_cast<FLOAT>(g_len));
				// �p�l���̒����ɂȂ�悤����ʒu�����߂�.
				D2D1_POINT_2F lt{
					static_cast<FLOAT>(lt_x + (win_x + win_w * 0.5) - t->m_lineto.x * 0.5),
					static_cast<FLOAT>(lt_y + (win_y + win_h * 0.5) - t->m_lineto.y * 0.5)
				};
				xcvd_get_paste_pos(lt, /*<---*/lt, m_main_sheet.m_shape_list, g_len, m_snap_point / scale);
				t->set_pos_start(lt);
				{
					m_mutex_draw.lock();
					undo_push_append(t);
					undo_push_select(t);
					m_mutex_draw.unlock();
				}

				// �ꗗ���\������Ă邩���肷��.
				if (summary_is_visible()) {
					summary_append(t);
					summary_select(t);
				}
				main_bbox_update(t);
				main_panel_size();
				menu_is_enable();
				main_draw();
			}
		}
		else {
			message_show(ICON_ALERT, L"str_err_paste", L"");
		}
		co_await context;
	}

	// �\��t����ʒu�����߂�.
	// paste_pt	���߂�_
	// q	�\��t�������_
	// g_len	����̑傫��
	// interval	�_�ɓ_����������Ԋu
	static void xcvd_get_paste_pos(D2D1_POINT_2F& paste_pt, const D2D1_POINT_2F src_pt, const SHAPE_LIST& slist, const double g_len, const float interval)
	{
		D2D1_POINT_2F r;	// �ł��߂��_�ւ̈ʒu�x�N�g��
		if (g_len >= 1.0f && interval >= FLT_MIN && slist_find_vertex_closest(slist, src_pt, interval, r)) {
			pt_sub(r, src_pt, r);
			D2D1_POINT_2F g;	// �ł��߂�����ւ̈ʒu�x�N�g��
			pt_round(src_pt, g_len, g);
			pt_sub(g, src_pt, g);
			if (pt_abs2(g) < pt_abs2(r)) {
				paste_pt = g;
			}
			else {
				paste_pt = r;
			}
		}
		else if (g_len >= 1.0f) {
			pt_round(src_pt, g_len, paste_pt);
		}
		else if (interval >= FLT_MIN) {
			slist_find_vertex_closest(slist, src_pt, interval, paste_pt);
		}
		else {
			paste_pt = src_pt;
		}
	}

	// �\��t����_�𓾂�.
	void MainPage::xcvd_paste_pos(D2D1_POINT_2F& past_pt, const D2D1_POINT_2F src_pt) const noexcept
	{
		xcvd_get_paste_pos(past_pt, src_pt, m_main_sheet.m_shape_list, m_snap_grid ? m_main_sheet.m_grid_base + 1.0 : 0.0, m_snap_point / m_main_scale);
	}

}