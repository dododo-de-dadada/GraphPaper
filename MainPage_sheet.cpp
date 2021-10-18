//-------------------------------
// MainPage_sheet.cpp
// �p���̊e�����̐ݒ�
//-------------------------------
#include "pch.h"
#include "MainPage.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	using winrt::Windows::ApplicationModel::Resources::ResourceLoader;
	using winrt::Windows::Foundation::IAsyncAction;
	using winrt::Windows::Graphics::Display::DisplayInformation;
	using winrt::Windows::UI::Color;
	//using winrt::Windows::UI::Text::FontWeight;
	using winrt::Windows::UI::Text::FontStretch;
	using winrt::Windows::UI::Xaml::Controls::ContentDialogResult;
	using winrt::Windows::UI::Xaml::Controls::Primitives::RangeBaseValueChangedEventArgs;
	using winrt::Windows::UI::Xaml::Controls::Primitives::SliderSnapsTo;
	using winrt::Windows::UI::Xaml::Controls::TextBlock;
	using winrt::Windows::UI::Xaml::Controls::TextBox;
	using winrt::Windows::UI::Xaml::Controls::TextChangedEventArgs;
	using winrt::Windows::UI::Xaml::RoutedEventArgs;
	using winrt::Windows::UI::Xaml::Setter;
	using winrt::Windows::UI::Xaml::SizeChangedEventArgs;

	constexpr wchar_t DLG_TITLE[] = L"str_sheet";	// �p���̕\��

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

	// �`�F�b�N�}�[�N��}�`�̑����֘A�̃��j���[���ڂɂ���.
	void MainPage::sheet_attr_is_checked(void) noexcept
	{
		ARROW_STYLE a_style;
		m_main_sheet.get_arrow_style(a_style);
		arrow_style_is_checked(a_style);
		//arrow_style_is_checked(m_main_sheet.m_arrow_style);
		DWRITE_FONT_STYLE f_style;
		m_main_sheet.get_font_style(f_style);
		font_style_is_checked(f_style);
		D2D1_DASH_STYLE s_style;
		m_main_sheet.get_dash_style(s_style);
		dash_style_is_checked(s_style);
		D2D1_LINE_JOIN j_style;
		m_main_sheet.get_join_style(j_style);
		join_style_is_checked(j_style);
		DWRITE_TEXT_ALIGNMENT t_align_t;
		m_main_sheet.get_text_align_t(t_align_t);
		text_align_t_is_checked(t_align_t);
		DWRITE_PARAGRAPH_ALIGNMENT t_align_p;
		m_main_sheet.get_text_align_p(t_align_p);
		text_align_p_is_checked(t_align_p);
		GRID_EMPH g_emph;
		m_main_sheet.get_grid_emph(g_emph);
		grid_emph_is_checked(g_emph);
		GRID_SHOW g_show;
		m_main_sheet.get_grid_show(g_show);
		grid_show_is_checked(g_show);
		bool g_snap;
		m_main_sheet.get_grid_snap(g_snap);
		tmfi_grid_snap().IsChecked(g_snap);
		tmfi_grid_snap_2().IsChecked(g_snap);
	}

	// �p�����j���[�́u�p���̐F�v���I�����ꂽ.
	IAsyncAction MainPage::sheet_color_click_async(IInspectable const&, RoutedEventArgs const&)
	{
		m_sample_sheet.set_attr_to(&m_main_sheet);
		const float val0 = m_sample_sheet.m_sheet_color.r * COLOR_MAX;
		const float val1 = m_sample_sheet.m_sheet_color.g * COLOR_MAX;
		const float val2 = m_sample_sheet.m_sheet_color.b * COLOR_MAX;

		sample_slider_0().Maximum(255.0);
		sample_slider_0().TickFrequency(1.0);
		sample_slider_0().SnapsTo(SliderSnapsTo::Ticks);
		sample_slider_0().Value(val0);
		sheet_slider_set_header<UNDO_OP::SHEET_COLOR, 0>(val0);
		sample_slider_1().Maximum(255.0);
		sample_slider_1().TickFrequency(1.0);
		sample_slider_1().SnapsTo(SliderSnapsTo::Ticks);
		sample_slider_1().Value(val1);
		sheet_slider_set_header<UNDO_OP::SHEET_COLOR, 1>(val1);
		sample_slider_2().Maximum(255.0);
		sample_slider_2().TickFrequency(1.0);
		sample_slider_2().SnapsTo(SliderSnapsTo::Ticks);
		sample_slider_2().Value(val2);
		sheet_slider_set_header<UNDO_OP::SHEET_COLOR, 2>(val2);

		sample_slider_0().Visibility(UI_VISIBLE);
		sample_slider_1().Visibility(UI_VISIBLE);
		sample_slider_2().Visibility(UI_VISIBLE);
		const auto slider_0_token = sample_slider_0().ValueChanged({ this, &MainPage::sheet_slider_value_changed<UNDO_OP::SHEET_COLOR, 0> });
		const auto slider_1_token = sample_slider_1().ValueChanged({ this, &MainPage::sheet_slider_value_changed<UNDO_OP::SHEET_COLOR, 1> });
		const auto slider_2_token = sample_slider_2().ValueChanged({ this, &MainPage::sheet_slider_value_changed<UNDO_OP::SHEET_COLOR, 2> });
		//m_sample_type = SAMPLE_TYPE::NONE;
		//m_sample_dx.SetSwapChainPanel(scp_sample_panel());
		//const auto samp_w = scp_sample_panel().Width();
		//const auto samp_h = scp_sample_panel().Height();
		//m_sample_sheet.m_sheet_size.width = static_cast<FLOAT>(samp_w);
		//m_sample_sheet.m_sheet_size.height = static_cast<FLOAT>(samp_h);

		cd_sample_dialog().Title(box_value(ResourceLoader::GetForCurrentView().GetString(DLG_TITLE)));
		const auto d_result = co_await cd_sample_dialog().ShowAsync();
		if (d_result == ContentDialogResult::Primary) {
			D2D1_COLOR_F sample_value;
			m_sample_sheet.get_sheet_color(sample_value);
			D2D1_COLOR_F sheet_value;
			m_main_sheet.get_sheet_color(sheet_value);
			if (equal(sheet_value, sample_value) != true) {
				ustack_push_set<UNDO_OP::SHEET_COLOR>(&m_main_sheet, sample_value);
				ustack_push_null();
				ustack_is_enable();
				sheet_draw();
			}
		}
		//m_sample_dx.Release();
		sample_slider_0().Visibility(UI_COLLAPSED);
		sample_slider_1().Visibility(UI_COLLAPSED);
		sample_slider_2().Visibility(UI_COLLAPSED);
		sample_slider_0().ValueChanged(slider_0_token);
		sample_slider_1().ValueChanged(slider_1_token);
		sample_slider_2().ValueChanged(slider_2_token);
	}

	// �p����\������.
	void MainPage::sheet_draw(void)
	{
#if defined(_DEBUG)
		if (scp_sheet_panel().IsLoaded() != true) {
			return;
		}
#endif
		if (m_d2d_mutex.try_lock() != true) {
			// ���b�N�ł��Ȃ��ꍇ
			return;
		}

		//auto const& dc = m_main_d2d.m_d2d_context;
		// �f�o�C�X�R���e�L�X�g�̕`���Ԃ�ۑ��u���b�N�ɕێ�����.
		m_main_d2d.m_d2d_context->SaveDrawingState(m_main_d2d.m_state_block.get());
		// �f�o�C�X�R���e�L�X�g����ϊ��s��𓾂�.
		D2D1_MATRIX_3X2_F tran;
		m_main_d2d.m_d2d_context->GetTransform(&tran);
		// �g�嗦��ϊ��s��̊g��k���̐����Ɋi�[����.
		const auto sheet_scale = max(m_main_sheet.m_sheet_scale, 0.0f);
		tran.m11 = tran.m22 = sheet_scale;
		// �X�N���[���̕ϕ��Ɋg�嗦���|�����l��
		// �ϊ��s��̕��s�ړ��̐����Ɋi�[����.
		D2D1_POINT_2F t_pos;
		pt_add(m_main_min, sb_horz().Value(), sb_vert().Value(), t_pos);
		pt_mul(t_pos, sheet_scale, t_pos);
		tran.dx = -t_pos.x;
		tran.dy = -t_pos.y;
		// �ϊ��s����f�o�C�X�R���e�L�X�g�Ɋi�[����.
		m_main_d2d.m_d2d_context->SetTransform(&tran);
		// �`����J�n����.
		m_main_d2d.m_d2d_context->BeginDraw();
		m_main_sheet.draw(m_main_d2d);
		if (m_event_state == EVENT_STATE::PRESS_AREA) {
			const auto t_draw = m_drawing_tool;
			if (t_draw == DRAWING_TOOL::SELECT || t_draw == DRAWING_TOOL::RECT || t_draw == DRAWING_TOOL::TEXT || t_draw == DRAWING_TOOL::RULER) {
				m_main_sheet.draw_auxiliary_rect(m_main_d2d, m_event_pos_pressed, m_event_pos_curr);
			}
			else if (t_draw == DRAWING_TOOL::BEZI) {
				m_main_sheet.draw_auxiliary_bezi(m_main_d2d, m_event_pos_pressed, m_event_pos_curr);
			}
			else if (t_draw == DRAWING_TOOL::ELLI) {
				m_main_sheet.draw_auxiliary_elli(m_main_d2d, m_event_pos_pressed, m_event_pos_curr);
			}
			else if (t_draw == DRAWING_TOOL::LINE) {
				m_main_sheet.draw_auxiliary_line(m_main_d2d, m_event_pos_pressed, m_event_pos_curr);
			}
			else if (t_draw == DRAWING_TOOL::RRECT) {
				m_main_sheet.draw_auxiliary_rrect(m_main_d2d, m_event_pos_pressed, m_event_pos_curr);
			}
			else if (t_draw == DRAWING_TOOL::POLY) {
				m_main_sheet.draw_auxiliary_poly(m_main_d2d, m_event_pos_pressed, m_event_pos_curr, m_drawing_poly_opt);
			}
		}
		// �`����I������.
		HRESULT hr = m_main_d2d.m_d2d_context->EndDraw();
		// �ۑ����ꂽ�`��������ɖ߂�.
		m_main_d2d.m_d2d_context->RestoreDrawingState(m_main_d2d.m_state_block.get());
		if (hr == S_OK) {
			// ���ʂ� S_OK �̏ꍇ,
			// �X���b�v�`�F�[���̓��e����ʂɕ\������.
			m_main_d2d.Present();
			// �|�C���^�[�̈ʒu���X�e�[�^�X�o�[�Ɋi�[����.
			status_set_curs();
		}
#if defined(_DEBUG)
		else {
			// ���ʂ� S_OK �łȂ��ꍇ,
			// �u�`��ł��܂���v���b�Z�[�W�_�C�A���O��\������.
			message_show(ICON_ALERT, L"str_err_draw", {});
		}
#endif
		m_d2d_mutex.unlock();
	}

	// �O�i�F�𓾂�.
	const D2D1_COLOR_F& MainPage::sheet_foreground(void) const noexcept
	{
		return Shape::s_foreground_color;
	}

	// �p���̑���������������.
	void MainPage::sheet_init(void) noexcept
	{
		// ���̂̑���������������.
		{
			// ���\�[�X�̎擾�Ɏ��s�����ꍇ�ɔ�����,
			// �Œ�̊���l�����̑����Ɋi�[����.
			m_main_sheet.set_font_family(wchar_cpy(L"Segoe UI"));
			m_main_sheet.set_font_size(FONT_SIZE_DEFVAL);
			m_main_sheet.set_font_stretch(DWRITE_FONT_STRETCH::DWRITE_FONT_STRETCH_NORMAL);
			m_main_sheet.set_font_style(DWRITE_FONT_STYLE::DWRITE_FONT_STYLE_NORMAL);
			m_main_sheet.set_font_weight(DWRITE_FONT_WEIGHT::DWRITE_FONT_WEIGHT_NORMAL);
			// BodyTextBlockStyle �����\�[�X�f�B�N�V���i�����瓾��.
			auto resource = Resources().TryLookup(box_value(L"BodyTextBlockStyle"));
			if (resource != nullptr) {
				auto style = resource.try_as<winrt::Windows::UI::Xaml::Style>();
				std::list<winrt::Windows::UI::Xaml::Style> stack;
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
								auto value = unbox_value<winrt::Windows::UI::Xaml::Media::FontFamily>(setter.Value());
								m_main_sheet.set_font_family(wchar_cpy(value.Source().c_str()));
							}
							else if (prop == TextBlock::FontSizeProperty()) {
								// �v���p�e�B�[�� FontSize �̏ꍇ,
								// �Z�b�^�[�̒l����, ���̂̑傫���𓾂�.
								const auto value = unbox_value<float>(setter.Value());
								m_main_sheet.m_font_size = value;
							}
							else if (prop == TextBlock::FontStretchProperty()) {
								// �v���p�e�B�[�� FontStretch �̏ꍇ,
								// �Z�b�^�[�̒l����, ���̂̐L�k�𓾂�.
								auto value = unbox_value<int32_t>(setter.Value());
								m_main_sheet.set_font_stretch(static_cast<DWRITE_FONT_STRETCH>(value));
							}
							else if (prop == TextBlock::FontStyleProperty()) {
								// �v���p�e�B�[�� FontStyle �̏ꍇ,
								// �Z�b�^�[�̒l����, ���̂𓾂�.
								auto value = unbox_value<int32_t>(setter.Value());
								m_main_sheet.set_font_style(static_cast<DWRITE_FONT_STYLE>(value));
							}
							else if (prop == TextBlock::FontWeightProperty()) {
								// �v���p�e�B�[�� FontWeight �̏ꍇ,
								// �Z�b�^�[�̒l����, ���̂̑����𓾂�.
								auto value = unbox_value<int32_t>(setter.Value());
								m_main_sheet.set_font_weight(static_cast<DWRITE_FONT_WEIGHT>(value));
								//Determine the type of a boxed value
								//auto prop = setter.Value().try_as<winrt::Windows::Foundation::IPropertyValue>();
								//auto prop_type = prop.Type();
								//if (prop_type == winrt::Windows::Foundation::PropertyType::Inspectable) {
								// ...
								//}
								//else if (prop_type == winrt::Windows::Foundation::PropertyType::Int32) {
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
			ShapeText::is_available_font(m_main_sheet.m_font_family);
		}

		{
			const auto accent_color = Resources().TryLookup(box_value(L"SystemAccentColor"));
			D2D1_COLOR_F grid_color;
			if (accent_color != nullptr) {
				const auto uwp_color = unbox_value<Color>(accent_color);
				conv_uwp_to_color(unbox_value<Color>(accent_color), grid_color);
				grid_color.a = 0.5f;
			}
			else {
				grid_color = GRID_COLOR_DEFVAL;
			}
			m_main_sheet.set_arrow_size(ARROW_SIZE_DEFVAL);
			m_main_sheet.set_arrow_style(ARROW_STYLE::NONE);
			m_main_sheet.set_corner_radius(D2D1_POINT_2F{ GRID_LEN_DEFVAL, GRID_LEN_DEFVAL });
			m_main_sheet.set_fill_color(Shape::s_background_color);
			m_main_sheet.set_font_color(Shape::s_foreground_color);
			m_main_sheet.set_grid_base(GRID_LEN_DEFVAL - 1.0);
			m_main_sheet.set_grid_color(grid_color);
			m_main_sheet.set_grid_emph(GRID_EMPH_0);
			m_main_sheet.set_grid_show(GRID_SHOW::BACK);
			m_main_sheet.set_grid_snap(true);
			m_main_sheet.set_sheet_color(Shape::s_background_color);
			m_main_sheet.set_sheet_scale(1.0);
			const double dpi = DisplayInformation::GetForCurrentView().LogicalDpi();
			m_main_sheet.m_sheet_size = DEF_SHEET_SIZE;
			m_main_sheet.set_stroke_cap(CAP_STYLE{ D2D1_CAP_STYLE::D2D1_CAP_STYLE_FLAT, D2D1_CAP_STYLE::D2D1_CAP_STYLE_FLAT});
			m_main_sheet.set_stroke_color(Shape::s_foreground_color);
			m_main_sheet.set_dash_cap(D2D1_CAP_STYLE::D2D1_CAP_STYLE_FLAT);
			m_main_sheet.set_dash_patt(DASH_PATT_DEFVAL);
			m_main_sheet.set_dash_style(D2D1_DASH_STYLE::D2D1_DASH_STYLE_SOLID);
			m_main_sheet.set_join_limit(MITER_LIMIT_DEFVAL);
			m_main_sheet.set_join_style(D2D1_LINE_JOIN::D2D1_LINE_JOIN_MITER);
			m_main_sheet.set_stroke_width(1.0);
			m_main_sheet.set_text_align_p(DWRITE_PARAGRAPH_ALIGNMENT::DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
			m_main_sheet.set_text_align_t(DWRITE_TEXT_ALIGNMENT::DWRITE_TEXT_ALIGNMENT_LEADING);
			m_main_sheet.set_text_line_sp(0.0);
			m_main_sheet.set_text_padding(TEXT_MARGIN_DEFVAL);
		}
	}

	void MainPage::sheet_xaml_root_changed(winrt::Windows::UI::Xaml::XamlRoot const&, winrt::Windows::UI::Xaml::XamlRootChangedEventArgs const& args)
	{
		auto a_prop = args.as<winrt::Windows::Foundation::IPropertyValue>();
		auto a_type = a_prop.Type();
	}

	void MainPage::sheet_panel_loading(IInspectable const& sender, winrt::Windows::Foundation::IInspectable const&)
	{
		auto xaml_root = scp_sheet_panel().XamlRoot();
		xaml_root.Changed({ this, &MainPage::sheet_xaml_root_changed });

		//m_main_d2d.SetSwapChainPanel(scp_sheet_panel());
	}

	// �p���̃X���b�v�`�F�[���p�l�������[�h���ꂽ.
	void MainPage::sheet_panel_loaded(IInspectable const& sender, RoutedEventArgs const&)
	{
#if defined(_DEBUG)
		if (sender != scp_sheet_panel()) {
			return;
		}
#endif // _DEBUG
		//auto xaml_root = scp_sheet_panel().XamlRoot();
		//xaml_root.Changed({ this, &MainPage::sheet_xaml_root_changed });

		m_main_d2d.SetSwapChainPanel(scp_sheet_panel());
		//sheet_draw();
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
		const float w = z.Width;
		const float h = z.Height;
		scroll_set(w, h);
		if (scp_sheet_panel().IsLoaded()) {
			m_main_d2d.SetLogicalSize2(D2D1_SIZE_F{ w, h });
			sheet_draw();
		}
	}

	void MainPage::sheet_panel_scale_changed(IInspectable const&, IInspectable const&)
	{
		m_main_d2d.SetCompositionScale(scp_sheet_panel().CompositionScaleX(), scp_sheet_panel().CompositionScaleY());
	}

	// �p���̑傫����ݒ肷��.
	void MainPage::sheet_panle_size(void)
	{
		const float w = static_cast<float>(scp_sheet_panel().ActualWidth());
		const float h = static_cast<float>(scp_sheet_panel().ActualHeight());
		if (w > 0.0f && h > 0.0f) {
			scroll_set(w, h);
			m_main_d2d.SetLogicalSize2(D2D1_SIZE_F{ w, h });
		}
	}

	// �p�����j���[�́u�p���̑傫���v���I�����ꂽ
	IAsyncAction MainPage::sheet_size_click_async(IInspectable const&, RoutedEventArgs const&)
	{
		m_sample_sheet.set_attr_to(&m_main_sheet);
		float g_base;
		m_main_sheet.get_grid_base(g_base);
		wchar_t buf[32];
		conv_len_to_str<LEN_UNIT_HIDE>(m_len_unit, m_main_sheet.m_sheet_size.width, m_main_d2d.m_logical_dpi, g_base + 1.0f, buf);
		tx_sheet_width().Text(buf);
		conv_len_to_str<LEN_UNIT_HIDE>(m_len_unit, m_main_sheet.m_sheet_size.height, m_main_d2d.m_logical_dpi, g_base + 1.0f, buf);
		tx_sheet_height().Text(buf);
		conv_len_to_str<LEN_UNIT_SHOW>(m_len_unit, sheet_size_max(), m_main_d2d.m_logical_dpi, g_base + 1.0f, buf);
		tx_sheet_size_max().Text(buf);
		// ���̎��_�ł�, �e�L�X�g�{�b�N�X�ɐ��������l���i�[���Ă�, TextChanged �͌Ă΂�Ȃ�.
		// �v���C�}���[�{�^���͎g�p�\�ɂ��Ă���.
		cd_sheet_size_dialog().IsPrimaryButtonEnabled(true);
		cd_sheet_size_dialog().IsSecondaryButtonEnabled(m_main_sheet.m_shape_list.size() > 0);
		const auto d_result = co_await cd_sheet_size_dialog().ShowAsync();
		if (d_result == ContentDialogResult::None) {
			// �u�L�����Z���v�������ꂽ�ꍇ,
			co_return;
		}
		else if (d_result == ContentDialogResult::Primary) {
			constexpr wchar_t INVALID_NUM[] = L"str_err_number";

			// �{��, �����Ȑ��l�����͂���Ă���ꍇ, �u�K�p�v�{�^���͕s�ɂȂ��Ă���̂�
			// �K�v�Ȃ��G���[�`�F�b�N����, �O�̂���.
			if (swscanf_s(tx_sheet_width().Text().c_str(), L"%f", &m_main_sheet.m_sheet_size.width) != 1) {
				// �u�����Ȑ��l�ł��v���b�Z�[�W�_�C�A���O��\������.
				message_show(ICON_ALERT, INVALID_NUM, L"tx_sheet_width/Header");
				co_return;
			}
			if (swscanf_s(tx_sheet_height().Text().c_str(), L"%f", &m_main_sheet.m_sheet_size.height) != 1) {
				// �u�����Ȑ��l�ł��v���b�Z�[�W�_�C�A���O��\������.
				message_show(ICON_ALERT, INVALID_NUM, L"tx_sheet_height/Header");
				co_return;
			}
			// �p���̏c���̒����̒l���s�N�Z���P�ʂ̒l�ɕϊ�����.
			const float g_len = g_base + 1.0f;
			D2D1_SIZE_F p_size{
				static_cast<FLOAT>(conv_len_to_val(m_len_unit, m_main_sheet.m_sheet_size.width, m_main_d2d.m_logical_dpi, g_len)),
				static_cast<FLOAT>(conv_len_to_val(m_len_unit, m_main_sheet.m_sheet_size.height, m_main_d2d.m_logical_dpi, g_len))
			};
			if (!equal(p_size, m_main_sheet.m_sheet_size)) {
				// �ϊ����ꂽ�l���p���̑傫���ƈقȂ�ꍇ,
				ustack_push_set<UNDO_OP::SHEET_SIZE>(&m_main_sheet, p_size);
				ustack_push_null();
				ustack_is_enable();
				sheet_update_bbox();
				sheet_panle_size();
				sheet_draw();
				status_set_curs();
				status_set_grid();
				status_set_sheet();
				status_set_unit();
			}
		}
		else if (d_result == ContentDialogResult::Secondary) {
			D2D1_POINT_2F b_min = { FLT_MAX, FLT_MAX };
			D2D1_POINT_2F b_max = { -FLT_MAX, -FLT_MAX };
			D2D1_POINT_2F b_size;

			slist_bound_all(m_main_sheet.m_shape_list, b_min, b_max);
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
				ustack_push_move({ dx, dy }, ALL);
				flag = true;
			}
			D2D1_POINT_2F p_min = { 0.0F, 0.0F };
			D2D1_POINT_2F p_max;
			pt_add(b_max, b_min, p_max);
			D2D1_SIZE_F p_size = { p_max.x, p_max.y };
			if (!equal(m_main_sheet.m_sheet_size, p_size)) {
				ustack_push_set<UNDO_OP::SHEET_SIZE>(&m_main_sheet, p_size);
				flag = true;
			}
			if (flag) {
				ustack_push_null();
				ustack_is_enable();
			}
			sheet_update_bbox();
			sheet_panle_size();
			sheet_draw();
			status_set_sheet();
		}
	}

	// �e�L�X�g�{�b�N�X�u�p���̕��v�u�p���̍����v�̒l���ύX���ꂽ.
	void MainPage::sheet_size_text_changed(IInspectable const& sender, TextChangedEventArgs const&)
	{
		const double dpi = m_main_d2d.m_logical_dpi;
		double value;
		wchar_t buf[2];
		int cnt;
		// �e�L�X�g�{�b�N�X�̕�����𐔒l�ɕϊ�����.
		cnt = swscanf_s(unbox_value<TextBox>(sender).Text().c_str(), L"%lf%1s", &value, buf, 2);
		if (cnt == 1 && value > 0.0) {
			// �����񂪐��l�ɕϊ��ł����ꍇ,
			float g_base;
			m_main_sheet.get_grid_base(g_base);
			value = conv_len_to_val(m_len_unit, value, dpi, g_base + 1.0);
		}
		cd_sheet_size_dialog().IsPrimaryButtonEnabled(cnt == 1 && value >= 1.0 && value < sheet_size_max());
	}

	// �l���X���C�_�[�̃w�b�_�[�Ɋi�[����.
	template <UNDO_OP U, int S> void MainPage::sheet_slider_set_header(const float value)
	{
		using winrt::Windows::ApplicationModel::Resources::ResourceLoader;

		winrt::hstring text;
		if constexpr (U == UNDO_OP::SHEET_COLOR) {
			if constexpr (S == 0) {
				wchar_t buf[32];
				// �F�����̒l�𕶎���ɕϊ�����.
				conv_col_to_str(m_color_code, value, buf);
				text = ResourceLoader::GetForCurrentView().GetString(L"str_color_r") + L": " + buf;
			}
			if constexpr (S == 1) {
				wchar_t buf[32];
				// �F�����̒l�𕶎���ɕϊ�����.
				conv_col_to_str(m_color_code, value, buf);
				text = ResourceLoader::GetForCurrentView().GetString(L"str_color_g") + L": " + buf;
			}
			if constexpr (S == 2) {
				wchar_t buf[32];
				// �F�����̒l�𕶎���ɕϊ�����.
				conv_col_to_str(m_color_code, value, buf);
				text = ResourceLoader::GetForCurrentView().GetString(L"str_color_b") + L": " + buf;
			}
		}
		if constexpr (S == 0) {
			sample_slider_0().Header(box_value(text));
		}
		if constexpr (S == 1) {
			sample_slider_1().Header(box_value(text));
		}
		if constexpr (S == 2) {
			sample_slider_2().Header(box_value(text));
		}
		if constexpr (S == 3) {
			sample_slider_3().Header(box_value(text));
		}
	}

	// �X���C�_�[�̒l���ύX���ꂽ.
	// U	����̎��
	// S	�X���C�_�[�̔ԍ�
	// args	ValueChanged �œn���ꂽ����
	// �߂�l	�Ȃ�
	template <UNDO_OP U, int S>
	void MainPage::sheet_slider_value_changed(IInspectable const&, RangeBaseValueChangedEventArgs const& args)
	{
		if constexpr (U == UNDO_OP::SHEET_COLOR) {
			const auto value = static_cast<float>(args.NewValue());
			sheet_slider_set_header<U, S>(value);
			D2D1_COLOR_F s_color;
			m_sample_sheet.get_sheet_color(s_color);
			if constexpr (S == 0) {
				s_color.r = static_cast<FLOAT>(value / COLOR_MAX);
			}
			if constexpr (S == 1) {
				s_color.g = static_cast<FLOAT>(value / COLOR_MAX);
			}
			if constexpr (S == 2) {
				s_color.b = static_cast<FLOAT>(value / COLOR_MAX);
			}
			m_sample_sheet.set_sheet_color(s_color);
		}
		if (scp_sample_panel().IsLoaded()) {
			sample_draw();
		}
	}

	// �}�`���܂܂��悤�p���̍���ʒu�ƉE���ʒu���X�V����.
	// s	�}�`
	void MainPage::sheet_update_bbox(const Shape* s) noexcept
	{
		s->get_bound(m_main_min, m_main_max, m_main_min, m_main_max);
	}

	// �p���̍���ʒu�ƉE���ʒu��ݒ肷��.
	void MainPage::sheet_update_bbox(void) noexcept
	{
		slist_bound_sheet(m_main_sheet.m_shape_list, m_main_sheet.m_sheet_size, m_main_min, m_main_max);
	}

	// �p�����j���[�́u�\���{���v���I�����ꂽ.
	void MainPage::sheet_zoom_click(IInspectable const& sender, RoutedEventArgs const&)
	{
		float scale;
		if (sender == mfi_sheet_zoom_100() ||
			sender == mfi_sheet_zoom_100_2()) {
			scale = 1.0f;
		}
		else if (sender == mfi_sheet_zoom_150() ||
			sender == mfi_sheet_zoom_150_2()) {
			scale = 1.5f;
		}
		else if (sender == mfi_sheet_zoom_200() ||
			sender == mfi_sheet_zoom_200_2()) {
			scale = 2.0f;
		}
		else if (sender == mfi_sheet_zoom_300() ||
			sender == mfi_sheet_zoom_300_2()) {
			scale = 3.0f;
		}
		else if (sender == mfi_sheet_zoom_400() ||
			sender == mfi_sheet_zoom_400_2()) {
			scale = 4.0f;
		}
		else if (sender == mfi_sheet_zoom_075() ||
			sender == mfi_sheet_zoom_075_2()) {
			scale = 0.75f;
		}
		else if (sender == mfi_sheet_zoom_050() ||
			sender == mfi_sheet_zoom_050_2()) {
			scale = 0.5f;
		}
		else if (sender == mfi_sheet_zoom_025() ||
			sender == mfi_sheet_zoom_025_2()) {
			scale = 0.25f;
		}
		else {
			return;
		}
		if (scale != m_main_sheet.m_sheet_scale) {
			m_main_sheet.m_sheet_scale = scale;
			sheet_panle_size();
			sheet_draw();
			status_set_zoom();
		}
	}

	// �\�����g��܂��͏k������.
	void MainPage::sheet_zoom_delta(const int32_t delta) noexcept
	{
		if (delta > 0 &&
			m_main_sheet.m_sheet_scale < 4.f / 1.1f - FLT_MIN) {
			m_main_sheet.m_sheet_scale *= 1.1f;
		}
		else if (delta < 0 &&
			m_main_sheet.m_sheet_scale > 0.25f * 1.1f + FLT_MIN) {
			m_main_sheet.m_sheet_scale /= 1.1f;
		}
		else {
			return;
		}
		sheet_panle_size();
		sheet_draw();
		status_set_zoom();
	}
}