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

	// ���������s�N�Z���P�ʂ̒l�ɕϊ�����.
	static double conv_len_to_val(const LEN_UNIT l_unit, const double value, const double dpi, const double g_len) noexcept;

	// ���������s�N�Z���P�ʂ̒l�ɕϊ�����.
	// �ϊ����ꂽ�l��, 0.5 �s�N�Z���P�ʂɊۂ߂���.
	// l_unit	�����̒P��
	// value	�����̒l
	// dpi	DPI
	// g_len	����̒���
	// �߂�l	�s�N�Z���P�ʂ̒l
	static double conv_len_to_val(const LEN_UNIT l_unit, const double value, const double dpi, const double g_len) noexcept
	{
		double ret;

		if (l_unit == LEN_UNIT::INCH) {
			ret = value * dpi;
		}
		else if (l_unit == LEN_UNIT::MILLI) {
			ret = value * dpi / MM_PER_INCH;
		}
		else if (l_unit == LEN_UNIT::POINT) {
			ret = value * dpi / PT_PER_INCH;
		}
		else if (l_unit == LEN_UNIT::GRID) {
			ret = value * g_len;
		}
		else {
			ret = value;
		}
		return std::round(2.0 * ret) * 0.5;
	}

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
		const auto g_len = m_sample_layout.m_grid_base + 1.0;
		// �y�[�W�̏c���̒����̒l���s�N�Z���P�ʂ̒l�ɕϊ�����.
		D2D1_SIZE_F p_size{
			static_cast<FLOAT>(conv_len_to_val(m_len_unit, pw, dpi, g_len)),
			static_cast<FLOAT>(conv_len_to_val(m_len_unit, ph, dpi, g_len))
		};
		if (equal(p_size, m_page_layout.m_page_size) == false) {
			// �ϊ����ꂽ�l���y�[�W�̑傫���ƈقȂ�ꍇ,
			// �l���y�[�W���C�A�E�g�Ɋi�[����, ���̑�����X�^�b�N�ɐς�.
			undo_push_set<UNDO_OP::PAGE_SIZE>(&m_page_layout, p_size);
			// ��A�̑���̋�؂Ƃ��ăk��������X�^�b�N�ɐς�.
			undo_push_null();
			// ���ɖ߂�/��蒼�����j���[���ڂ̎g�p�̉ۂ�ݒ肷��.
			enable_undo_menu();
		}
		s_list_bound(m_list_shapes, m_page_layout.m_page_size, m_page_min, m_page_max);
		set_page_panle_size();
		page_draw();
		stbar_set_curs();
		stbar_set_grid();
		stbar_set_page();
		stbar_set_unit();
	}

	// �y�[�W�̐��@���̓_�C�A���O�́u�}�`�ɍ��킹��v�{�^���������ꂽ.
	void MainPage::cd_page_size_sec_btn_click(ContentDialog const&, ContentDialogButtonClickEventArgs const&)
	{
		D2D1_POINT_2F b_min = { FLT_MAX, FLT_MAX };
		D2D1_POINT_2F b_max = { -FLT_MAX, -FLT_MAX };
		D2D1_POINT_2F p_size;

		s_list_bound(m_list_shapes, b_min, b_max);
		pt_sub(b_max, b_min, p_size);
		if (p_size.x < 1.0F || p_size.y < 1.0F) {
			return;
		}
		pt_min({ 0.0F, 0.0F }, b_min, m_page_min);
		pt_max(b_max, p_size, m_page_max);
		if (equal(m_page_layout.m_page_size, { p_size.x, p_size.y }) == false) {
			undo_push_set<UNDO_OP::PAGE_SIZE>(&m_page_layout, D2D1_SIZE_F{ p_size.x, p_size.y });
			undo_push_null();
			enable_undo_menu();
		}
		s_list_bound(m_list_shapes, m_page_layout.m_page_size, m_page_min, m_page_max);
		set_page_panle_size();
		page_draw();
		stbar_set_page();
	}

	// �y�[�W�́u�y�[�W�̒P�ʂƐF�̏����v�_�C�A���O�́u�K�p�v�{�^���������ꂽ.
	// �y�[�W���j���[�́u�F�v���I�����ꂽ.
	IAsyncAction MainPage::mfi_page_color_click_async(IInspectable const&, RoutedEventArgs const&)
	{
		using winrt::Windows::ApplicationModel::Resources::ResourceLoader;
		using winrt::Windows::UI::Xaml::Controls::ContentDialogResult;

		m_sample_layout.set_to(&m_page_layout);
		const double val0 = m_sample_layout.m_page_color.r * COLOR_MAX;
		const double val1 = m_sample_layout.m_page_color.g * COLOR_MAX;
		const double val2 = m_sample_layout.m_page_color.b * COLOR_MAX;
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
			D2D1_COLOR_F sample_value;
			m_sample_layout.get_page_color(sample_value);
			D2D1_COLOR_F page_value;
			m_page_layout.get_page_color(page_value);
			if (equal(page_value, sample_value) == false) {
				undo_push_set<UNDO_OP::PAGE_COLOR>(&m_page_layout, sample_value);
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
		m_sample_layout.set_to(&m_page_layout);
		const auto g_len = m_sample_layout.m_grid_base + 1.0;
		pw = m_sample_layout.m_page_size.width;
		ph = m_sample_layout.m_page_size.height;
		wchar_t buf[32];
		conv_val_to_len<!WITH_UNIT_NAME>(m_len_unit, pw, dpi, g_len, buf);
		tx_page_width().Text(buf);
		conv_val_to_len<!WITH_UNIT_NAME>(m_len_unit, ph, dpi, g_len, buf);
		tx_page_height().Text(buf);
		conv_val_to_len<WITH_UNIT_NAME>(m_len_unit, m_page_size_max, dpi, g_len, buf);
		tx_page_size_max().Text(buf);
		// ���̎��_�ł�, �e�L�X�g�{�b�N�X�ɐ��������l���i�[���Ă�, 
		// TextChanged �͌Ă΂�Ȃ�.
		// �v���C�}���[�{�^���͎g�p�\�ɂ��Ă���.
		cd_page_size().IsPrimaryButtonEnabled(true);
		cd_page_size().IsSecondaryButtonEnabled(m_list_shapes.size() > 0);
		const auto _ = cd_page_size().ShowAsync();
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
		const auto scale = max(m_page_layout.m_page_scale, 0.0);
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
		dc->Clear(m_page_layout.m_page_color);
		if (m_page_layout.m_grid_show == GRID_SHOW::BACK) {
			// ������̕\�����Ŕw�ʂɕ\���̏ꍇ,
			// �������\������.
			m_page_layout.draw_grid(m_page_dx, { 0.0f, 0.0f });
		}
		// ���ʂ̐F���u���V�Ɋi�[����.
		//D2D1_COLOR_F anch_color;
		//m_page_layout.get_anchor_color(anch_color);
		//m_page_dx.m_anch_brush->SetColor(anch_color);
		for (auto s : m_list_shapes) {
			if (s->is_deleted()) {
				// �����t���O�������Ă���ꍇ,
				// �p������.
				continue;
			}
			// �}�`��\������.
			s->draw(m_page_dx);
		}
		if (m_page_layout.m_grid_show == GRID_SHOW::FRONT) {
			// ������̕\�����őO�ʂɕ\���̏ꍇ,
			// �������\������.
			m_page_layout.draw_grid(m_page_dx, { 0.0f, 0.0f });
		}
		if (m_pointer_state == STATE_TRAN::PRESS_AREA) {
			// �����ꂽ��Ԃ��͈͂�I�����Ă���ꍇ,
			// �⏕���̐F���u���V�Ɋi�[����.
			//D2D1_COLOR_F aux_color;
			//m_page_layout.get_auxiliary_color(aux_color);
			//m_page_dx.m_aux_brush->SetColor(aux_color);
			if (m_draw_tool == DRAW_TOOL::SELECT
				|| m_draw_tool == DRAW_TOOL::RECT
				|| m_draw_tool == DRAW_TOOL::TEXT
				|| m_draw_tool == DRAW_TOOL::SCALE) {
				// �I���c�[��
				// �܂��͕��`
				// �܂��͕�����̏ꍇ,
				// ���`�̕⏕����\������.
				m_page_layout.draw_auxiliary_rect(m_page_dx, m_pointer_pressed, m_pointer_cur);
			}
			else if (m_draw_tool == DRAW_TOOL::BEZI) {
				// �Ȑ��̏ꍇ,
				// �Ȑ��̕⏕����\������.
				m_page_layout.draw_auxiliary_bezi(m_page_dx, m_pointer_pressed, m_pointer_cur);
			}
			else if (m_draw_tool == DRAW_TOOL::ELLI) {
				// ���~�̏ꍇ,
				// ���~�̕⏕����\������.
				m_page_layout.draw_auxiliary_elli(m_page_dx, m_pointer_pressed, m_pointer_cur);
			}
			else if (m_draw_tool == DRAW_TOOL::LINE) {
				// �����̏ꍇ,
				// �����̕⏕����\������.
				m_page_layout.draw_auxiliary_line(m_page_dx, m_pointer_pressed, m_pointer_cur);
			}
			else if (m_draw_tool == DRAW_TOOL::RRCT) {
				// �p�ە��`�̏ꍇ,
				// �p�ە��`�̕⏕����\������.
				m_page_layout.draw_auxiliary_rrect(m_page_dx, m_pointer_pressed, m_pointer_cur);
			}
			else if (m_draw_tool == DRAW_TOOL::QUAD) {
				// �l�ւ�`�̏ꍇ,
				// �l�ւ�`�̕⏕����\������.
				m_page_layout.draw_auxiliary_quad(m_page_dx, m_pointer_pressed, m_pointer_cur);
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
			stbar_set_curs();
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
	void MainPage::page_set_slider_header(const double value)
	{
		using winrt::Windows::ApplicationModel::Resources::ResourceLoader;

		winrt::hstring hdr;
		if constexpr (U == UNDO_OP::PAGE_COLOR) {
			if constexpr (S == 0) {
				wchar_t buf[32];
				// �F�����̒l�𕶎���ɕϊ�����.
				conv_val_to_col(m_color_code, value, buf);
				auto const& r_loader = ResourceLoader::GetForCurrentView();
				hdr = r_loader.GetString(L"str_col_r") + L": " + buf;
			}
			if constexpr (S == 1) {
				wchar_t buf[32];
				// �F�����̒l�𕶎���ɕϊ�����.
				conv_val_to_col(m_color_code, value, buf);
				auto const& r_loader = ResourceLoader::GetForCurrentView();
				hdr = r_loader.GetString(L"str_col_g") + L": " + buf;
			}
			if constexpr (S == 2) {
				wchar_t buf[32];
				// �F�����̒l�𕶎���ɕϊ�����.
				conv_val_to_col(m_color_code, value, buf);
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
		Shape* s = &m_sample_layout;
		const auto value = args.NewValue();
		page_set_slider_header<U, S>(value);
		if constexpr (U == UNDO_OP::GRID_BASE) {
			s->set_grid_base(value);
		}
		if constexpr (U == UNDO_OP::GRID_GRAY) {
			s->set_grid_gray(value / COLOR_MAX);
		}
		if constexpr (U == UNDO_OP::PAGE_COLOR) {
			D2D1_COLOR_F color;
			s->get_page_color(color);
			if constexpr (S == 0) {
				color.r = static_cast<FLOAT>(value / COLOR_MAX);
			}
			if constexpr (S == 1) {
				color.g = static_cast<FLOAT>(value / COLOR_MAX);
			}
			if constexpr (S == 2) {
				color.b = static_cast<FLOAT>(value / COLOR_MAX);
			}
			s->set_page_color(color);
		}
		if (scp_sample_panel().IsLoaded()) {
			sample_draw();
		}
	}

	// �y�[�W�̃X���b�v�`�F�[���p�l�������[�h���ꂽ.
#if defined(_DEBUG)
	void MainPage::scp_page_panel_loaded(IInspectable const& sender, RoutedEventArgs const&)
#else
	void MainPage::scp_page_panel_loaded(IInspectable const&, RoutedEventArgs const&)
#endif
	{
#if defined(_DEBUG)
		if (sender != scp_page_panel()) {
			return;
		}
#endif // _DEBUG
		m_page_dx.SetSwapChainPanel(scp_page_panel());
		page_draw();
	}

	// �y�[�W�̃X���b�v�`�F�[���p�l���̐��@���ς����.
#if defined(_DEBUG)
	void MainPage::scp_page_panel_size_changed(IInspectable const& sender, SizeChangedEventArgs const& args)
#else
	void MainPage::scp_page_panel_size_changed(IInspectable const&, SizeChangedEventArgs const& args)
#endif	// _DEBUG
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
		double value;
		wchar_t buf[2];
		int cnt;
		// �e�L�X�g�{�b�N�X�̕�����𐔒l�ɕϊ�����.
		cnt = swscanf_s(unbox_value<TextBox>(sender).Text().c_str(), L"%lf%1s", &value, buf, 2);
		if (cnt == 1 && value > 0.0) {
			// �����񂪐��l�ɕϊ��ł����ꍇ,
			value = conv_len_to_val(m_len_unit, value, dpi, m_sample_layout.m_grid_base + 1.0);
		}
		cd_page_size().IsPrimaryButtonEnabled(cnt == 1 && value >= 1.0 && value < m_page_size_max);
	}

}