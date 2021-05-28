//-------------------------------
// MainPage_sheet.cpp
// �p���̊e�����̐ݒ�
//-------------------------------
#include "pch.h"
#include "MainPage.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	using winrt::Windows::UI::Xaml::Controls::TextBox;

	constexpr wchar_t SHEET_TITLE[] = L"str_sheet";	// �p���̕\��

	// ���������s�N�Z���P�ʂ̒l�ɕϊ�����.
	static double conv_len_to_val(const LEN_UNIT l_unit, const double value, const double dpi, const double g_len) noexcept;

	// ���������s�N�Z���P�ʂ̒l�ɕϊ�����.
	// �ϊ����ꂽ�l��, 0.5 �s�N�Z���P�ʂɊۂ߂���.
	// l_unit	�����̒P��
	// value	�����̒l
	// dpi	DPI
	// g_len	����̑傫��
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

	// �}�`���܂܂��悤�p���̍���ʒu�ƉE���ʒu���X�V����.
	// s	�}�`
	void MainPage::sheet_update_bbox(const Shape* s) noexcept
	{
		s->get_bound(m_sheet_min, m_sheet_max, m_sheet_min, m_sheet_max);
	}

	// �p���̍���ʒu�ƉE���ʒu��ݒ肷��.
	void MainPage::sheet_update_bbox(void) noexcept
	{
		s_list_bound(m_list_shapes, m_sheet_main.m_sheet_size, m_sheet_min, m_sheet_max);
	}

	// �p�����j���[�́u�p���̐F�v���I�����ꂽ.
	IAsyncAction MainPage::sheet_color_click_async(IInspectable const&, RoutedEventArgs const&)
	{
		using winrt::Windows::ApplicationModel::Resources::ResourceLoader;
		using winrt::Windows::UI::Xaml::Controls::ContentDialogResult;

		m_sample_sheet.set_to(&m_sheet_main);
		const float val0 = m_sample_sheet.m_sheet_color.r * COLOR_MAX;
		const float val1 = m_sample_sheet.m_sheet_color.g * COLOR_MAX;
		const float val2 = m_sample_sheet.m_sheet_color.b * COLOR_MAX;
		sample_slider_0().Value(val0);
		sample_slider_1().Value(val1);
		sample_slider_2().Value(val2);
		sheet_set_slider_header<UNDO_OP::SHEET_COLOR, 0>(val0);
		sheet_set_slider_header<UNDO_OP::SHEET_COLOR, 1>(val1);
		sheet_set_slider_header<UNDO_OP::SHEET_COLOR, 2>(val2);
		sample_slider_0().Visibility(VISIBLE);
		sample_slider_1().Visibility(VISIBLE);
		sample_slider_2().Visibility(VISIBLE);
		const auto slider_0_token = sample_slider_0().ValueChanged({ this, &MainPage::sheet_set_slider<UNDO_OP::SHEET_COLOR, 0> });
		const auto slider_1_token = sample_slider_1().ValueChanged({ this, &MainPage::sheet_set_slider<UNDO_OP::SHEET_COLOR, 1> });
		const auto slider_2_token = sample_slider_2().ValueChanged({ this, &MainPage::sheet_set_slider<UNDO_OP::SHEET_COLOR, 2> });
		m_sample_type = SAMP_TYPE::NONE;
		cd_sample().Title(box_value(ResourceLoader::GetForCurrentView().GetString(SHEET_TITLE)));
		const auto d_result = co_await cd_sample().ShowAsync();
		if (d_result == ContentDialogResult::Primary) {
			D2D1_COLOR_F sample_value;
			m_sample_sheet.get_sheet_color(sample_value);
			D2D1_COLOR_F sheet_value;
			m_sheet_main.get_sheet_color(sheet_value);
			if (equal(sheet_value, sample_value) != true) {
				undo_push_set<UNDO_OP::SHEET_COLOR>(&m_sheet_main, sample_value);
				undo_push_null();
				undo_menu_enable();
				sheet_draw();
			}
		}
		sample_slider_0().Visibility(COLLAPSED);
		sample_slider_1().Visibility(COLLAPSED);
		sample_slider_2().Visibility(COLLAPSED);
		sample_slider_0().ValueChanged(slider_0_token);
		sample_slider_1().ValueChanged(slider_1_token);
		sample_slider_2().ValueChanged(slider_2_token);
	}

	// �p����\������.
	void MainPage::sheet_draw(void)
	{
#if defined(_DEBUG)
		if (m_sheet_dx.m_swapChainPanel.IsLoaded() != true) {
			return;
		}
#endif
		if (m_dx_mutex.try_lock() != true) {
			// ���b�N�ł��Ȃ��ꍇ
			return;
		}

		auto const& dc = m_sheet_dx.m_d2dContext;
		// �f�o�C�X�R���e�L�X�g�̕`���Ԃ�ۑ��u���b�N�ɕێ�����.
		dc->SaveDrawingState(m_sheet_dx.m_state_block.get());
		// �f�o�C�X�R���e�L�X�g����ϊ��s��𓾂�.
		D2D1_MATRIX_3X2_F tran;
		dc->GetTransform(&tran);
		// �g�嗦��ϊ��s��̊g��k���̐����Ɋi�[����.
		const auto sheet_scale = max(m_sheet_main.m_sheet_scale, 0.0f);
		tran.m11 = tran.m22 = sheet_scale;
		// �X�N���[���̕ϕ��Ɋg�嗦���|�����l��
		// �ϊ��s��̕��s�ړ��̐����Ɋi�[����.
		D2D1_POINT_2F t_pos;
		pt_add(m_sheet_min, sb_horz().Value(), sb_vert().Value(), t_pos);
		pt_mul(t_pos, sheet_scale, t_pos);
		tran.dx = -t_pos.x;
		tran.dy = -t_pos.y;
		// �ϊ��s����f�o�C�X�R���e�L�X�g�Ɋi�[����.
		dc->SetTransform(&tran);
		// �`����J�n����.
		dc->BeginDraw();
		// �p���F�œh��Ԃ�.
		dc->Clear(m_sheet_main.m_sheet_color);
		GRID_SHOW g_show;
		m_sample_sheet.get_grid_show(g_show);
		if (equal(g_show, GRID_SHOW::BACK)) {
			// ����̕\�����Ŕw�ʂɕ\���̏ꍇ,
			// �����\������.
			m_sheet_main.draw_grid(m_sheet_dx, { 0.0f, 0.0f });
		}
		// ���ʂ̐F���u���V�Ɋi�[����.
		//D2D1_COLOR_F anch_color;
		//m_sheet_main.get_anchor_color(anch_color);
		//m_sheet_dx.m_anch_brush->SetColor(anch_color);
		for (auto s : m_list_shapes) {
			if (s->is_deleted()) {
				// �����t���O�������Ă���ꍇ,
				// �p������.
				continue;
			}
			// �}�`��\������.
			s->draw(m_sheet_dx);
		}
		if (equal(g_show, GRID_SHOW::FRONT)) {
			// ����̕\�����őO�ʂɕ\���̏ꍇ,
			// �����\������.
			m_sheet_main.draw_grid(m_sheet_dx, { 0.0f, 0.0f });
		}
		if (pointer_state() == PBTN_STATE::PRESS_AREA) {
			const auto t_draw = tool_draw();
			if (t_draw == TOOL_DRAW::SELECT || t_draw == TOOL_DRAW::RECT || t_draw == TOOL_DRAW::TEXT || t_draw == TOOL_DRAW::RULER) {
				m_sheet_main.draw_auxiliary_rect(m_sheet_dx, pointer_pressed(), pointer_cur());
			}
			else if (t_draw == TOOL_DRAW::BEZI) {
				m_sheet_main.draw_auxiliary_bezi(m_sheet_dx, pointer_pressed(), pointer_cur());
			}
			else if (t_draw == TOOL_DRAW::ELLI) {
				m_sheet_main.draw_auxiliary_elli(m_sheet_dx, pointer_pressed(), pointer_cur());
			}
			else if (t_draw == TOOL_DRAW::LINE) {
				m_sheet_main.draw_auxiliary_line(m_sheet_dx, pointer_pressed(), pointer_cur());
			}
			else if (t_draw == TOOL_DRAW::RRECT) {
				m_sheet_main.draw_auxiliary_rrect(m_sheet_dx, pointer_pressed(), pointer_cur());
			}
			else if (t_draw == TOOL_DRAW::POLY) {
				m_sheet_main.draw_auxiliary_poly(m_sheet_dx, pointer_pressed(), pointer_cur(), tool_poly());
			}
		}
		// �`����I������.
		HRESULT hr = dc->EndDraw();
		// �ۑ����ꂽ�`��������ɖ߂�.
		dc->RestoreDrawingState(m_sheet_dx.m_state_block.get());
		if (hr == S_OK) {
			// ���ʂ� S_OK �̏ꍇ,
			// �X���b�v�`�F�[���̓��e����ʂɕ\������.
			m_sheet_dx.Present();
			// �|�C���^�[�̈ʒu���X�e�[�^�X�o�[�Ɋi�[����.
			sbar_set_curs();
		}
#if defined(_DEBUG)
		else {
			// ���ʂ� S_OK �łȂ��ꍇ,
			// �u�`��ł��܂���v���b�Z�[�W�_�C�A���O��\������.
			message_show(ICON_ALERT, L"str_err_draw", {});
		}
#endif
		m_dx_mutex.unlock();
	}

	// �O�i�F�𓾂�.
	const D2D1_COLOR_F& MainPage::sheet_foreground(void) const noexcept
	{
		return m_sheet_dx.m_theme_foreground;
	}

	// �p���Ƃ��̑��̑���������������.
	void MainPage::sheet_init(void) noexcept
	{
		// ���̂̑���������������.
		{
			using winrt::Windows::UI::Xaml::Setter;
			using winrt::Windows::UI::Xaml::Controls::TextBlock;
			using winrt::Windows::UI::Xaml::Media::FontFamily;
			using winrt::Windows::UI::Text::FontWeight;
			using winrt::Windows::UI::Text::FontStretch;
			using winrt::Windows::UI::Xaml::Style;

			// ���\�[�X�̎擾�Ɏ��s�����ꍇ�ɔ�����,
			// �Œ�̊���l�����̑����Ɋi�[����.
			m_sheet_main.set_font_family(wchar_cpy(L"Segoe UI"));
			m_sheet_main.set_font_size(FONT_SIZE_DEF);
			m_sheet_main.set_font_stretch(DWRITE_FONT_STRETCH::DWRITE_FONT_STRETCH_NORMAL);
			m_sheet_main.set_font_style(DWRITE_FONT_STYLE::DWRITE_FONT_STYLE_NORMAL);
			m_sheet_main.set_font_weight(DWRITE_FONT_WEIGHT::DWRITE_FONT_WEIGHT_NORMAL);
			// BodyTextBlockStyle �����\�[�X�f�B�N�V���i�����瓾��.
			auto resource = Resources().TryLookup(box_value(L"BodyTextBlockStyle"));
			if (resource != nullptr) {
				auto style = resource.try_as<Style>();
				std::list<Style> stack;
				// ���\�[�X����X�^�C���𓾂�.
				while (style != nullptr) {
					// �X�^�C������łȂ��ꍇ.
					// �X�^�C�����X�^�b�N�ɐς�.
					stack.push_back(style);
					// �X�^�C���̌p�����̃X�^�C���𓾂�.
					style = style.BasedOn();
				}
				try {
					while (stack.empty() != true) {
						// �X�^�b�N����łȂ��ꍇ,
						// �X�^�C�����X�^�b�N������o��.
						style = stack.back();
						stack.pop_back();
						// �X�^�C���̒��̊e�Z�b�^�[�ɂ���.
						auto const& setters = style.Setters();
						for (auto const& base : setters) {
							auto const& setter = base.try_as<Setter>();
							// �Z�b�^�[�̃v���p�e�B�[�𓾂�.
							auto const& prop = setter.Property();
							if (prop == TextBlock::FontFamilyProperty()) {
								// �v���p�e�B�[�� FontFamily �̏ꍇ,
								// �Z�b�^�[�̒l����, ���̖��𓾂�.
								auto value = unbox_value<FontFamily>(setter.Value());
								m_sheet_main.set_font_family(wchar_cpy(value.Source().c_str()));
							}
							else if (prop == TextBlock::FontSizeProperty()) {
								// �v���p�e�B�[�� FontSize �̏ꍇ,
								// �Z�b�^�[�̒l����, ���̂̑傫���𓾂�.
								const auto value = unbox_value<float>(setter.Value());
								m_sheet_main.m_font_size = value;
							}
							else if (prop == TextBlock::FontStretchProperty()) {
								// �v���p�e�B�[�� FontStretch �̏ꍇ,
								// �Z�b�^�[�̒l����, ���̂̐L�k�𓾂�.
								auto value = unbox_value<int32_t>(setter.Value());
								m_sheet_main.set_font_stretch(static_cast<DWRITE_FONT_STRETCH>(value));
							}
							else if (prop == TextBlock::FontStyleProperty()) {
								// �v���p�e�B�[�� FontStyle �̏ꍇ,
								// �Z�b�^�[�̒l����, ���̂𓾂�.
								auto value = unbox_value<int32_t>(setter.Value());
								m_sheet_main.set_font_style(static_cast<DWRITE_FONT_STYLE>(value));
							}
							else if (prop == TextBlock::FontWeightProperty()) {
								// �v���p�e�B�[�� FontWeight �̏ꍇ,
								// �Z�b�^�[�̒l����, ���̂̑����𓾂�.
								auto value = unbox_value<int32_t>(setter.Value());
								m_sheet_main.set_font_weight(static_cast<DWRITE_FONT_WEIGHT>(value));
								//Determine the type of a boxed value
								//auto prop = setter.Value().try_as<winrt::Windows::Foundation::IPropertyValue>();
								//if (prop.Type() == winrt::Windows::Foundation::PropertyType::Inspectable) {
								// ...
								//}
								//else if (prop.Type() == winrt::Windows::Foundation::PropertyType::int32) {
								// ...
								//}
							}
						}
					}
				}
				catch (winrt::hresult_error const&) {
				}
				stack.clear();
				style = nullptr;
				resource = nullptr;
			}
			ShapeText::is_available_font(m_sheet_main.m_font_family);
		}

		{
			m_sheet_main.set_arrow_size(ARROWHEAD_SIZE_DEF);
			m_sheet_main.set_arrow_style(ARROWHEAD_STYLE::NONE);
			m_sheet_main.set_corner_radius(D2D1_POINT_2F{ GRID_LEN_DEF, GRID_LEN_DEF });
			m_sheet_main.set_fill_color(m_sheet_dx.m_theme_background);
			m_sheet_main.set_font_color(sheet_foreground());
			m_sheet_main.set_grid_base(GRID_LEN_DEF - 1.0);
			m_sheet_main.set_grid_gray(GRID_GRAY_DEF);
			m_sheet_main.set_grid_emph(GRID_EMPH_0);
			m_sheet_main.set_grid_show(GRID_SHOW::BACK);
			m_sheet_main.set_grid_snap(true);
			m_sheet_main.set_sheet_color(m_sheet_dx.m_theme_background);
			m_sheet_main.set_sheet_scale(1.0);
			const double dpi = DisplayInformation::GetForCurrentView().LogicalDpi();
			m_sheet_main.m_sheet_size = SHEET_SIZE_DEF;
			m_sheet_main.set_stroke_color(sheet_foreground());
			m_sheet_main.set_stroke_join_limit(MITER_LIMIT_DEF);
			m_sheet_main.set_stroke_join_style(D2D1_LINE_JOIN::D2D1_LINE_JOIN_MITER);
			m_sheet_main.set_stroke_dash_patt(STROKE_DASH_PATT_DEF);
			m_sheet_main.set_stroke_dash_style(D2D1_DASH_STYLE::D2D1_DASH_STYLE_SOLID);
			m_sheet_main.set_stroke_width(1.0);
			m_sheet_main.set_text_align_p(DWRITE_PARAGRAPH_ALIGNMENT::DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
			m_sheet_main.set_text_align_t(DWRITE_TEXT_ALIGNMENT::DWRITE_TEXT_ALIGNMENT_LEADING);
			m_sheet_main.set_text_line(0.0);
			m_sheet_main.set_text_margin(TEXT_MARGIN_DEF);
		}
		len_unit(LEN_UNIT::PIXEL);
		color_code(COLOR_CODE::DEC);
		status_bar(sbar_or(SBAR_FLAG::CURS, SBAR_FLAG::ZOOM));
	}

	// �l��p���̉E���ʒu�Ɋi�[����.
	void MainPage::sheet_max(const D2D1_POINT_2F p_max) noexcept
	{
		m_sheet_max = p_max;
	}

	// �p���̉E���ʒu�𓾂�.
	const D2D1_POINT_2F MainPage::sheet_max(void) const noexcept
	{
		return m_sheet_max;
	}

	// �l��p���̍���ʒu�Ɋi�[����.
	void MainPage::sheet_min(const D2D1_POINT_2F p_min) noexcept
	{
		m_sheet_min = p_min;
	}

	// �p���̍���ʒu�𓾂�.
	const D2D1_POINT_2F MainPage::sheet_min(void) const noexcept
	{
		return m_sheet_min;
	}

	// �p���̃X���b�v�`�F�[���p�l�������[�h���ꂽ.
#if defined(_DEBUG)
	void MainPage::sheet_panel_loaded(IInspectable const& sender, RoutedEventArgs const&)
#else
	void MainPage::sheet_panel_loaded(IInspectable const&, RoutedEventArgs const&)
#endif
	{
#if defined(_DEBUG)
		if (sender != scp_sheet_panel()) {
			return;
		}
#endif // _DEBUG
		m_sheet_dx.SetSwapChainPanel(scp_sheet_panel());
		sheet_draw();
	}

	// �p���̃X���b�v�`�F�[���p�l���̐��@���ς����.
#if defined(_DEBUG)
	void MainPage::sheet_panel_size_changed(IInspectable const& sender, SizeChangedEventArgs const& args)
#else
	void MainPage::sheet_panel_size_changed(IInspectable const&, SizeChangedEventArgs const& args)
#endif	// _DEBUG
	{
#if defined(_DEBUG)
		if (sender != scp_sheet_panel()) {
			return;
		}
#endif	// _DEBUG
		const auto z = args.NewSize();
		const auto w = z.Width;
		const auto h = z.Height;
		scroll_set(w, h);
		if (scp_sheet_panel().IsLoaded() != true) {
			return;
		}
		m_sheet_dx.SetLogicalSize2({ w, h });
		sheet_draw();
	}

	// �p���̑傫����ݒ肷��.
	void MainPage::sheet_panle_size(void)
	{
		const auto w = scp_sheet_panel().ActualWidth();
		const auto h = scp_sheet_panel().ActualHeight();
		scroll_set(w, h);
		m_sheet_dx.SetLogicalSize2({ static_cast<float>(w), static_cast<float>(h) });
	}

	// �l���X���C�_�[�̃w�b�_�[�Ɋi�[����.
	template <UNDO_OP U, int S> void MainPage::sheet_set_slider_header(const float value)
	{
		using winrt::Windows::ApplicationModel::Resources::ResourceLoader;

		winrt::hstring hdr;
		if constexpr (U == UNDO_OP::SHEET_COLOR) {
			if constexpr (S == 0) {
				wchar_t buf[32];
				// �F�����̒l�𕶎���ɕϊ�����.
				conv_col_to_str(color_code(), value, buf);
				auto const& r_loader = ResourceLoader::GetForCurrentView();
				hdr = r_loader.GetString(L"str_col_r") + L": " + buf;
			}
			if constexpr (S == 1) {
				wchar_t buf[32];
				// �F�����̒l�𕶎���ɕϊ�����.
				conv_col_to_str(color_code(), value, buf);
				auto const& r_loader = ResourceLoader::GetForCurrentView();
				hdr = r_loader.GetString(L"str_col_g") + L": " + buf;
			}
			if constexpr (S == 2) {
				wchar_t buf[32];
				// �F�����̒l�𕶎���ɕϊ�����.
				conv_col_to_str(color_code(), value, buf);
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
	template <UNDO_OP U, int S> void MainPage::sheet_set_slider(IInspectable const&, RangeBaseValueChangedEventArgs const& args)
	{
		Shape* s = &m_sample_sheet;
		const auto value = static_cast<float>(args.NewValue());
		sheet_set_slider_header<U, S>(value);
		if constexpr (U == UNDO_OP::GRID_BASE) {
			s->set_grid_base(value);
		}
		if constexpr (U == UNDO_OP::GRID_GRAY) {
			s->set_grid_gray(value / COLOR_MAX);
		}
		if constexpr (U == UNDO_OP::SHEET_COLOR) {
			D2D1_COLOR_F color;
			s->get_sheet_color(color);
			if constexpr (S == 0) {
				color.r = static_cast<FLOAT>(value / COLOR_MAX);
			}
			if constexpr (S == 1) {
				color.g = static_cast<FLOAT>(value / COLOR_MAX);
			}
			if constexpr (S == 2) {
				color.b = static_cast<FLOAT>(value / COLOR_MAX);
			}
			s->set_sheet_color(color);
		}
		if (scp_sample_panel().IsLoaded()) {
			sample_draw();
		}
	}

	// �p�����j���[�́u�p���̑傫���v���I�����ꂽ
	IAsyncAction MainPage::sheet_size_click_async(IInspectable const&, RoutedEventArgs const&)
	{
		using winrt::Windows::UI::Xaml::Controls::ContentDialogResult;

		m_sample_sheet.set_to(&m_sheet_main);
		double pw = m_sheet_main.m_sheet_size.width;
		double ph = m_sheet_main.m_sheet_size.height;
		const double dpi = m_sheet_dx.m_logical_dpi;
		float g_base;
		m_sheet_main.get_grid_base(g_base);
		const auto g_len = g_base + 1.0;
		wchar_t buf[32];
		conv_len_to_str<LEN_UNIT_HIDE>(len_unit(), pw, dpi, g_len, buf);
		tx_sheet_width().Text(buf);
		conv_len_to_str<LEN_UNIT_HIDE>(len_unit(), ph, dpi, g_len, buf);
		tx_sheet_height().Text(buf);
		conv_len_to_str<LEN_UNIT_SHOW>(len_unit(), sheet_size_max(), dpi, g_len, buf);
		tx_sheet_size_max().Text(buf);
		// ���̎��_�ł�, �e�L�X�g�{�b�N�X�ɐ��������l���i�[���Ă�, 
		// TextChanged �͌Ă΂�Ȃ�.
		// �v���C�}���[�{�^���͎g�p�\�ɂ��Ă���.
		cd_sheet_size().IsPrimaryButtonEnabled(true);
		cd_sheet_size().IsSecondaryButtonEnabled(m_list_shapes.size() > 0);
		const auto d_result = co_await cd_sheet_size().ShowAsync();
		if (d_result == ContentDialogResult::None) {
			// �u�L�����Z���v�������ꂽ�ꍇ,
			co_return;
		}
		else if (d_result == ContentDialogResult::Primary) {
			constexpr wchar_t INVALID_NUM[] = L"str_err_number";

			// �{��, �����Ȑ��l�����͂���Ă���ꍇ, �u�K�p�v�{�^���͕s�ɂȂ��Ă���̂�
			// �K�v�Ȃ��G���[�`�F�b�N����, �O�̂���.
			if (swscanf_s(tx_sheet_width().Text().c_str(), L"%lf", &pw) != 1) {
				// �u�����Ȑ��l�ł��v���b�Z�[�W�_�C�A���O��\������.
				message_show(ICON_ALERT, INVALID_NUM, L"tx_sheet_width/Header");
				co_return;
			}
			if (swscanf_s(tx_sheet_height().Text().c_str(), L"%lf", &ph) != 1) {
				// �u�����Ȑ��l�ł��v���b�Z�[�W�_�C�A���O��\������.
				message_show(ICON_ALERT, INVALID_NUM, L"tx_sheet_height/Header");
				co_return;
			}
			// �p���̏c���̒����̒l���s�N�Z���P�ʂ̒l�ɕϊ�����.
			D2D1_SIZE_F p_size{
				static_cast<FLOAT>(conv_len_to_val(len_unit(), pw, dpi, g_len)),
				static_cast<FLOAT>(conv_len_to_val(len_unit(), ph, dpi, g_len))
			};
			if (!equal(p_size, m_sheet_main.m_sheet_size)) {
				// �ϊ����ꂽ�l���p���̑傫���ƈقȂ�ꍇ,
				undo_push_set<UNDO_OP::SHEET_SIZE>(&m_sheet_main, p_size);
				undo_push_null();
				undo_menu_enable();
				sheet_update_bbox();
				sheet_panle_size();
				sheet_draw();
				sbar_set_curs();
				sbar_set_grid();
				sbar_set_sheet();
				sbar_set_unit();
			}
		}
		else if (d_result == ContentDialogResult::Secondary) {
			D2D1_POINT_2F b_min = { FLT_MAX, FLT_MAX };
			D2D1_POINT_2F b_max = { -FLT_MAX, -FLT_MAX };
			D2D1_POINT_2F b_size;

			s_list_bound(m_list_shapes, b_min, b_max);
			pt_sub(b_max, b_min, b_size);
			if (b_size.x < 1.0F || b_size.y < 1.0F) {
				co_return;
			}
			float dx = 0.0F;
			float dy = 0.0F;
			if (b_min.x < 0.0F) {
				dx = -b_min.x;
				b_min.x = 0.0F;
				b_max.x += dx;
			}
			if (b_min.y < 0.0F) {
				dy = -b_min.y;
				b_min.y = 0.0F;
				b_max.y += dy;
			}
			bool flag = false;
			if (dx > 0.0F || dy > 0.0F) {
				constexpr auto ALL = true;
				undo_push_move({ dx, dy }, ALL);
				flag = true;
			}
			D2D1_POINT_2F p_min = { 0.0F, 0.0F };
			D2D1_POINT_2F p_max;
			pt_add(b_max, b_min, p_max);
			D2D1_SIZE_F p_size = { p_max.x, p_max.y };
			if (equal(m_sheet_main.m_sheet_size, p_size) != true) {
				undo_push_set<UNDO_OP::SHEET_SIZE>(&m_sheet_main, p_size);
				flag = true;
			}
			if (flag) {
				undo_push_null();
				undo_menu_enable();
			}
			sheet_update_bbox();
			sheet_panle_size();
			sheet_draw();
			sbar_set_sheet();
		}
	}

	// �e�L�X�g�{�b�N�X�u�p���̕��v�u�p���̍����v�̒l���ύX���ꂽ.
	void MainPage::sheet_size_text_changed(IInspectable const& sender, TextChangedEventArgs const&)
	{
		const double dpi = m_sheet_dx.m_logical_dpi;
		double value;
		wchar_t buf[2];
		int cnt;
		// �e�L�X�g�{�b�N�X�̕�����𐔒l�ɕϊ�����.
		cnt = swscanf_s(unbox_value<TextBox>(sender).Text().c_str(), L"%lf%1s", &value, buf, 2);
		if (cnt == 1 && value > 0.0) {
			// �����񂪐��l�ɕϊ��ł����ꍇ,
			float g_base;
			m_sheet_main.get_grid_base(g_base);
			value = conv_len_to_val(len_unit(), value, dpi, g_base + 1.0);
		}
		cd_sheet_size().IsPrimaryButtonEnabled(cnt == 1 && value >= 1.0 && value < sheet_size_max());
	}

}