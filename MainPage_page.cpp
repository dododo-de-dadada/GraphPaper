//-------------------------------
// MainPage_page.cpp
// �y�[�W�̐ݒ�ƕ\��
//-------------------------------
#include "pch.h"
#include "MainPage.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	using winrt::Windows::UI::Xaml::Controls::TextBox;

	constexpr wchar_t TITLE_PAGE[] = L"str_page";

	// �y�[�W���@�_�C�A���O�́u�K�p�v�{�^���������ꂽ.
	void MainPage::cd_page_size_pri_btn_click(ContentDialog const&, ContentDialogButtonClickEventArgs const&)
	{
		constexpr wchar_t INVALID_NUM[] = L"str_err_number";
		const double dpi = m_page_dx.m_logical_dpi;

		// �{��, �����Ȑ��l�����͂���Ă���ꍇ, �u�K�p�v�{�^���͕s�ɂȂ��Ă���̂�
		// �K�v�Ȃ��G���[�`�F�b�N����, �O�̂���.
		double pw;
		if (swscanf_s(tx_page_width().Text().c_str(), L"%lf", &pw) != 1) {
			// �u�����Ȑ��l�ł��v���b�Z�[�W�_�C�A���O��\������.
			cd_message_show(ICON_ALERT, INVALID_NUM, L"tx_page_width/Header");
			return;
		}
		double ph;
		if (swscanf_s(tx_page_height().Text().c_str(), L"%lf", &ph) != 1) {
			// �u�����Ȑ��l�ł��v���b�Z�[�W�_�C�A���O��\������.
			cd_message_show(ICON_ALERT, INVALID_NUM, L"tx_page_height/Header");
			return;
		}
		const auto g_len = m_sample_panel.m_grid_base + 1.0;
		// �y�[�W�̏c���̒����̒l���s�N�Z���P�ʂ̒l�ɕϊ�����.
		D2D1_SIZE_F p_size{
			static_cast<FLOAT>(conv_len_to_val(m_page_unit, pw, dpi, g_len)),
			static_cast<FLOAT>(conv_len_to_val(m_page_unit, ph, dpi, g_len))
		};
		if (equal(p_size, m_page_panel.m_page_size) == false) {
			// �ϊ����ꂽ�l���y�[�W�̑傫���ƈقȂ�ꍇ,
			// �l���y�[�W�p�l���Ɋi�[����, ���̑�����X�^�b�N�ɐς�.
			undo_push_set<UNDO_OP::PAGE_SIZE>(&m_page_panel, p_size);
			// ��A�̑���̋�؂Ƃ��ăk��������X�^�b�N�ɐς�.
			undo_push_null();
			// ���ɖ߂�/��蒼�����j���[���ڂ̎g�p�̉ۂ�ݒ肷��.
			enable_undo_menu();
		}
		s_list_bound(m_list_shapes, m_page_panel.m_page_size, m_page_min, m_page_max);
		set_page_panle_size();
		page_draw();
		// �|�C���^�[�̈ʒu���X�e�[�^�X�o�[�Ɋi�[����.
		status_set_curs();
		// ����̑傫�����X�e�[�^�X�o�[�Ɋi�[����.
		status_set_grid();
		// �y�[�W�̑傫�����X�e�[�^�X�o�[�Ɋi�[����.
		status_set_page();
		status_set_unit();
	}

	// �y�[�W�̐��@���̓_�C�A���O�́u�}�`�ɍ��킹��v�{�^���������ꂽ.
	void MainPage::cd_page_size_sec_btn_click(ContentDialog const&, ContentDialogButtonClickEventArgs const&)
	{
		D2D1_POINT_2F b_min = { FLT_MAX, FLT_MAX };
		D2D1_POINT_2F b_max = { -FLT_MAX, -FLT_MAX };
		D2D1_POINT_2F p_max;

		for (auto s : m_list_shapes) {
			if (s->is_deleted()) {
				continue;
			}
			s->get_bound(b_min, b_max);
		}
		pt_add(b_max, b_min, p_max);
		if (p_max.x < 1.0 || p_max.y < 1.0) {
			return;
		}
		pt_min({ 0.0f, 0.0f }, b_min, m_page_min);
		pt_max(b_max, p_max, m_page_max);
		const D2D1_SIZE_F page = { p_max.x, p_max.y };
		if (equal(m_page_panel.m_page_size, page) == false) {
			undo_push_set<UNDO_OP::PAGE_SIZE>(&m_page_panel, page);
			// ��A�̑���̋�؂Ƃ��ăk��������X�^�b�N�ɐς�.
			undo_push_null();
			// ���ɖ߂�/��蒼�����j���[���ڂ̎g�p�̉ۂ�ݒ肷��.
			enable_undo_menu();
		}
		s_list_bound(m_list_shapes, m_page_panel.m_page_size, m_page_min, m_page_max);
		set_page_panle_size();
		page_draw();
		// �y�[�W�̑傫�����X�e�[�^�X�o�[�Ɋi�[����.
		status_set_page();
	}

	// �y�[�W�́u�P�ʂƏ����v�_�C�A���O�́u�K�p�v�{�^���������ꂽ.
	void MainPage::cd_page_unit_pri_btn_click(ContentDialog const&, ContentDialogButtonClickEventArgs const&)
	{
		const auto p_unit = m_page_unit;
		m_page_unit = static_cast<LEN_UNIT>(cx_page_unit().SelectedIndex());
		m_col_style = static_cast<COL_STYLE>(cx_color_style().SelectedIndex());
		if (p_unit != m_page_unit) {
			// �|�C���^�[�̈ʒu���X�e�[�^�X�o�[�Ɋi�[����.
			status_set_curs();
			// ����̑傫�����X�e�[�^�X�o�[�Ɋi�[����.
			status_set_grid();
			// �y�[�W�̑傫�����X�e�[�^�X�o�[�Ɋi�[����.
			status_set_page();
			status_set_unit();
		}
	}

	// �y�[�W���j���[�́u�F�v���I�����ꂽ.
	IAsyncAction MainPage::mfi_page_color_click(IInspectable const&, RoutedEventArgs const&)
	{
		using winrt::Windows::ApplicationModel::Resources::ResourceLoader;
		using winrt::Windows::UI::Xaml::Controls::ContentDialogResult;

		const double val0 = m_page_panel.m_page_color.r * COLOR_MAX;
		const double val1 = m_page_panel.m_page_color.g * COLOR_MAX;
		const double val2 = m_page_panel.m_page_color.b * COLOR_MAX;
		sample_slider_0().Value(val0);
		sample_slider_1().Value(val1);
		sample_slider_2().Value(val2);
		page_set_slider_header<UNDO_OP::PAGE_COLOR, 0>(val0);
		page_set_slider_header<UNDO_OP::PAGE_COLOR, 1>(val1);
		page_set_slider_header<UNDO_OP::PAGE_COLOR, 2>(val2);
		sample_slider_0().Visibility(VISIBLE);
		sample_slider_1().Visibility(VISIBLE);
		sample_slider_2().Visibility(VISIBLE);
		const auto slider_0_token = sample_slider_0().ValueChanged({ this, &MainPage::page_set_slider<UNDO_OP::PAGE_COLOR, 0> });
		const auto slider_1_token = sample_slider_1().ValueChanged({ this, &MainPage::page_set_slider<UNDO_OP::PAGE_COLOR, 1> });
		const auto slider_2_token = sample_slider_2().ValueChanged({ this, &MainPage::page_set_slider<UNDO_OP::PAGE_COLOR, 2> });
		m_sample_type = SAMP_TYPE::NONE;
		cd_sample().Title(box_value(ResourceLoader::GetForCurrentView().GetString(TITLE_PAGE)));
		const auto d_result = co_await cd_sample().ShowAsync();
		if (d_result == ContentDialogResult::Primary) {
			D2D1_COLOR_F sample_val;
			m_sample_panel.get_page_color(sample_val);
			D2D1_COLOR_F page_val;
			m_page_panel.get_page_color(page_val);
			if (equal(page_val, sample_val) == false) {
				undo_push_set<UNDO_OP::PAGE_COLOR>(&m_page_panel, sample_val);
				// ��A�̑���̋�؂Ƃ��ăk��������X�^�b�N�ɐς�.
				undo_push_null();
				// ���ɖ߂�/��蒼�����j���[���ڂ̎g�p�̉ۂ�ݒ肷��.
				enable_undo_menu();
			}
		}
		sample_slider_0().Visibility(COLLAPSED);
		sample_slider_1().Visibility(COLLAPSED);
		sample_slider_2().Visibility(COLLAPSED);
		sample_slider_0().ValueChanged(slider_0_token);
		sample_slider_1().ValueChanged(slider_1_token);
		sample_slider_2().ValueChanged(slider_2_token);
		page_draw();
	}

	// �y�[�W���j���[�́u�傫���v���I�����ꂽ
	void MainPage::mfi_page_size_click(IInspectable const&, RoutedEventArgs const&)
	{
		const double dpi = m_page_dx.m_logical_dpi;
		//wchar_t const* format = nullptr;
		double pw;
		double ph;
		m_sample_panel.m_grid_base = m_page_panel.m_grid_base;
		const auto g_len = m_sample_panel.m_grid_base + 1.0;
		pw = m_sample_panel.m_page_size.width;
		ph = m_sample_panel.m_page_size.height;
		/*
		wchar_t const* format;
		switch (m_page_unit) {
		default:
		// return;
		//case LEN_UNIT::PIXEL:
			format = FMT_PIXEL;
			break;
		case LEN_UNIT::INCH:
			format = FMT_INCH;
			pw = pw / dpi;
			ph = ph / dpi;
			break;
		case LEN_UNIT::MILLI:
			format = FMT_MILLI;
			pw = pw / dpi * MM_PER_INCH;
			ph = ph / dpi * MM_PER_INCH;
			break;
		case LEN_UNIT::POINT:
			format = FMT_POINT;
			pw = pw / dpi * PT_PER_INCH;
			ph = ph / dpi * PT_PER_INCH;
			break;
		case LEN_UNIT::GRID:
			format = FMT_GRID;
			pw /= m_sample_panel.m_grid_len + 1.0;
			ph /= m_sample_panel.m_grid_len + 1.0;
			break;
		}

		wchar_t buf[32];
		swprintf_s(buf, format, pw);
		*/
		wchar_t buf[32];
		// �s�N�Z���P�ʂ̒����𑼂̒P�ʂ̕�����ɕϊ�����.
		conv_val_to_len<!WITH_UNIT_NAME>(m_page_unit, pw, dpi, g_len, buf);
		tx_page_width().Text(buf);
		// �s�N�Z���P�ʂ̒����𑼂̒P�ʂ̕�����ɕϊ�����.
		conv_val_to_len<!WITH_UNIT_NAME>(m_page_unit, ph, dpi, g_len, buf);
		tx_page_height().Text(buf);
		// �s�N�Z���P�ʂ̒����𑼂̒P�ʂ̕�����ɕϊ�����.
		conv_val_to_len<WITH_UNIT_NAME>(m_page_unit, m_page_size_max, dpi, g_len, buf);
		tx_page_size_max().Text(buf);
		// ���̎��_�ł�, �e�L�X�g�{�b�N�X�ɐ��������l���i�[���Ă�, 
		// TextChanged �͌Ă΂�Ȃ�.
		// �v���C�}���[�{�^���͎g�p�\�ɂ��Ă���.
		cd_page_size().IsPrimaryButtonEnabled(true);
		cd_page_size().IsSecondaryButtonEnabled(m_list_shapes.size() > 0);
		const auto _ = cd_page_size().ShowAsync();
	}

	// �y�[�W���j���[�́u�P�ʂƏ����v���I�����ꂽ
	void MainPage::mfi_page_unit_click(IInspectable const&, RoutedEventArgs const&)
	{
		cx_page_unit().SelectedIndex(static_cast<uint32_t>(m_page_unit));
		cx_color_style().SelectedIndex(static_cast<uint32_t>(m_col_style));
		const auto _ = cd_page_unit().ShowAsync();
	}

	// �y�[�W�Ɛ}�`��\������.
	void MainPage::page_draw(void)
	{
#if defined(_DEBUG)
		if (m_page_dx.m_swapChainPanel.IsLoaded() == false) {
			return;
		}
#endif
		std::lock_guard<std::mutex> lock(m_dx_mutex);

		auto const& dc = m_page_dx.m_d2dContext;
		// �f�o�C�X�R���e�L�X�g�̕`���Ԃ�ۑ��u���b�N�ɕێ�����.
		dc->SaveDrawingState(m_page_dx.m_state_block.get());
		// �f�o�C�X�R���e�L�X�g����ϊ��s��𓾂�.
		D2D1_MATRIX_3X2_F tran;
		dc->GetTransform(&tran);
		// �g�嗦��ϊ��s��̊g��k���̐����Ɋi�[����.
		const auto scale = max(m_page_panel.m_page_scale, 0.0);
		tran.m11 = tran.m22 = static_cast<FLOAT>(scale);
		// �X�N���[���̕ϕ��Ɋg�嗦���|�����l��
		// �ϊ��s��̕��s�ړ��̐����Ɋi�[����.
		D2D1_POINT_2F t_pos;
		pt_add(m_page_min, sb_horz().Value(), sb_vert().Value(), t_pos);
		pt_scale(t_pos, scale, t_pos);
		tran.dx = -t_pos.x;
		tran.dy = -t_pos.y;
		// �ϊ��s����f�o�C�X�R���e�L�X�g�Ɋi�[����.
		dc->SetTransform(&tran);
		// �`����J�n����.
		dc->BeginDraw();
		// �y�[�W�F�œh��Ԃ�.
		dc->Clear(m_page_panel.m_page_color);
		if (m_page_panel.m_grid_show == GRID_SHOW::BACK) {
			// ������̕\�����Ŕw�ʂɕ\���̏ꍇ,
			// �������\������.
			m_page_panel.draw_grid(m_page_dx, { 0.0f, 0.0f });
		}
		// ���ʂ̐F���u���V�Ɋi�[����.
		m_page_dx.m_anch_brush->SetColor(m_page_panel.m_anch_color);
		for (auto s : m_list_shapes) {
			if (s->is_deleted()) {
				// �����t���O�������Ă���ꍇ,
				// �p������.
				continue;
			}
			// �}�`��\������.
			s->draw(m_page_dx);
		}
		if (m_page_panel.m_grid_show == GRID_SHOW::FRONT) {
			// ������̕\�����őO�ʂɕ\���̏ꍇ,
			// �������\������.
			m_page_panel.draw_grid(m_page_dx, { 0.0f, 0.0f });
		}
		if (m_press_state == STATE_TRAN::PRESS_AREA) {
			// �����ꂽ��Ԃ��͈͂�I�����Ă���ꍇ,
			// �⏕���̐F���u���V�Ɋi�[����.
			m_page_dx.m_aux_brush->SetColor(m_page_panel.m_aux_color);
			if (m_draw_tool == DRAW_TOOL::SELECT
				|| m_draw_tool == DRAW_TOOL::RECT
				|| m_draw_tool == DRAW_TOOL::TEXT
				|| m_draw_tool == DRAW_TOOL::SCALE) {
				// �I���c�[��
				// �܂��͕��`
				// �܂��͕�����̏ꍇ,
				// ���`�̕⏕����\������.
				m_page_panel.draw_auxiliary_rect(m_page_dx, m_press_pos, m_curr_pos);
			}
			else if (m_draw_tool == DRAW_TOOL::BEZI) {
				// �Ȑ��̏ꍇ,
				// �Ȑ��̕⏕����\������.
				m_page_panel.draw_auxiliary_bezi(m_page_dx, m_press_pos, m_curr_pos);
			}
			else if (m_draw_tool == DRAW_TOOL::ELLI) {
				// ���~�̏ꍇ,
				// ���~�̕⏕����\������.
				m_page_panel.draw_auxiliary_elli(m_page_dx, m_press_pos, m_curr_pos);
			}
			else if (m_draw_tool == DRAW_TOOL::LINE) {
				// �����̏ꍇ,
				// �����̕⏕����\������.
				m_page_panel.draw_auxiliary_line(m_page_dx, m_press_pos, m_curr_pos);
			}
			else if (m_draw_tool == DRAW_TOOL::RRECT) {
				// �p�ە��`�̏ꍇ,
				// �p�ە��`�̕⏕����\������.
				m_page_panel.draw_auxiliary_rrect(m_page_dx, m_press_pos, m_curr_pos);
			}
			else if (m_draw_tool == DRAW_TOOL::QUAD) {
				// �l�ւ�`�̏ꍇ,
				// �l�ւ�`�̕⏕����\������.
				m_page_panel.draw_auxiliary_quad(m_page_dx, m_press_pos, m_curr_pos);
			}
		}
		// �`����I������.
		HRESULT hr = dc->EndDraw();
		// �ۑ����ꂽ�`��������ɖ߂�.
		dc->RestoreDrawingState(m_page_dx.m_state_block.get());
		if (hr == S_OK) {
			// ���ʂ� S_OK �̏ꍇ,
			// �X���b�v�`�F�[���̓��e����ʂɕ\������.
			m_page_dx.Present();
			// �|�C���^�[�̈ʒu���X�e�[�^�X�o�[�Ɋi�[����.
			status_set_curs();
		}
#if defined(_DEBUG)
		else {
			// ���ʂ� S_OK �łȂ��ꍇ,
			// �u�`��ł��܂���v���b�Z�[�W�_�C�A���O��\������.
			cd_message_show(ICON_ALERT, L"str_err_draw", {});
		}
#endif
	}

	// �l���X���C�_�[�̃w�b�_�[�Ɋi�[����.
	template <UNDO_OP U, int S>
	void MainPage::page_set_slider_header(double val)
	{
		using winrt::Windows::ApplicationModel::Resources::ResourceLoader;

		winrt::hstring hdr;
		if constexpr (U == UNDO_OP::PAGE_COLOR) {
			if constexpr (S == 0) {
				wchar_t buf[32];
				// �F�����̒l�𕶎���ɕϊ�����.
				conv_val_to_col(m_col_style, val, buf);
				auto const& r_loader = ResourceLoader::GetForCurrentView();
				hdr = r_loader.GetString(L"str_col_r") + L": " + buf;
			}
			if constexpr (S == 1) {
				wchar_t buf[32];
				// �F�����̒l�𕶎���ɕϊ�����.
				conv_val_to_col(m_col_style, val, buf);
				auto const& r_loader = ResourceLoader::GetForCurrentView();
				hdr = r_loader.GetString(L"str_col_g") + L": " + buf;
			}
			if constexpr (S == 2) {
				wchar_t buf[32];
				// �F�����̒l�𕶎���ɕϊ�����.
				conv_val_to_col(m_col_style, val, buf);
				auto const& r_loader = ResourceLoader::GetForCurrentView();
				hdr = r_loader.GetString(L"str_col_b") + L": " + buf;
			}
		}
		if constexpr (S == 0) {
			sample_slider_0().Header(box_value(hdr));
		}
		if constexpr (S == 1) {
			sample_slider_1().Header(box_value(hdr));
		}
		if constexpr (S == 2) {
			sample_slider_2().Header(box_value(hdr));
		}
		if constexpr (S == 3) {
			sample_slider_3().Header(box_value(hdr));
		}
	}

	// �l���X���C�_�[�̃w�b�_�[�ƁA���{�̐}�`�Ɋi�[����.
	// U	����̎��
	// S	�X���C�_�[�̔ԍ�
	// args	ValueChanged �œn���ꂽ����
	// �߂�l	�Ȃ�
	template <UNDO_OP U, int S>
	void MainPage::page_set_slider(IInspectable const&, RangeBaseValueChangedEventArgs const& args)
	{
		Shape* s = &m_sample_panel;
		const double val = args.NewValue();
		page_set_slider_header<U, S>(val);
		if constexpr (U == UNDO_OP::GRID_LEN) {
			s->set_grid_size(val);
		}
		if constexpr (U == UNDO_OP::GRID_OPAC) {
			s->set_grid_opac(val / COLOR_MAX);
		}
		if constexpr (U == UNDO_OP::PAGE_COLOR) {
			D2D1_COLOR_F col;
			s->get_page_color(col);
			if constexpr (S == 0) {
				col.r = static_cast<FLOAT>(val / COLOR_MAX);
			}
			if constexpr (S == 1) {
				col.g = static_cast<FLOAT>(val / COLOR_MAX);
			}
			if constexpr (S == 2) {
				col.b = static_cast<FLOAT>(val / COLOR_MAX);
			}
			s->set_page_color(col);
		}
		if (scp_sample_panel().IsLoaded()) {
			sample_draw();
		}
	}

	// �y�[�W�̃p�l�������[�h���ꂽ.
	void MainPage::scp_page_panel_loaded(IInspectable const& sender, RoutedEventArgs const&/*args*/)
	{
#if defined(_DEBUG)
		if (sender != scp_page_panel()) {
			return;
		}
#endif // _DEBUG
		m_page_dx.SetSwapChainPanel(scp_page_panel());
		page_draw();
	}

	// �y�[�W�̃p�l���̐��@���ς����.
	void MainPage::scp_page_panel_size_changed(IInspectable const& sender, SizeChangedEventArgs const& args)
	{
#if defined(_DEBUG)
		if (sender != scp_page_panel()) {
			return;
		}
#endif	// _DEBUG
		const auto z = args.NewSize();
		const auto w = z.Width;
		const auto h = z.Height;
		scroll_set(w, h);
		if (scp_page_panel().IsLoaded() == false) {
			return;
		}
		m_page_dx.SetLogicalSize2({ w, h });
		page_draw();
	}

	// �y�[�W�̑傫����ݒ肷��.
	void MainPage::set_page_panle_size(void)
	{
		const auto w = scp_page_panel().ActualWidth();
		const auto h = scp_page_panel().ActualHeight();
		scroll_set(w, h);
		m_page_dx.SetLogicalSize2({ static_cast<float>(w), static_cast<float>(h) });
	}

	// �e�L�X�g�{�b�N�X�u�y�[�W�̕��v�u�y�[�W�̍����v�̒l���ύX���ꂽ.
	void MainPage::tx_page_size_text_changed(IInspectable const& sender, TextChangedEventArgs const&)
	{
		const double dpi = m_page_dx.m_logical_dpi;
		double val;
		wchar_t ws[2];
		int cnt;
		// �e�L�X�g�{�b�N�X�̕�����𐔒l�ɕϊ�����.
		cnt = swscanf_s(unbox_value<TextBox>(sender).Text().c_str(), L"%lf%1s", &val, ws, 2);
		if (cnt == 1 && val > 0.0) {
			// �l�����s�N�Z���P�ʂ̒l�ɕϊ�����.
			val = conv_len_to_val(m_page_unit, val, dpi, m_sample_panel.m_grid_base + 1.0);
		}
		cd_page_size().IsPrimaryButtonEnabled(cnt == 1 && val >= 1.0 && val < m_page_size_max);
	}

}