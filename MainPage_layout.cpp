#include "pch.h"
#include "MainPage.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	using winrt::Windows::ApplicationModel::Resources::ResourceLoader;
	using winrt::Windows::Storage::ApplicationData;
	using winrt::Windows::Storage::CreationCollisionOption;
	using winrt::Windows::UI::Xaml::Controls::ContentDialogResult;
	using winrt::Windows::UI::Xaml::Controls::Primitives::SliderSnapsTo;
	using winrt::Windows::UI::Xaml::Controls::TextBlock;
	using winrt::Windows::UI::Xaml::Controls::ToggleMenuFlyoutItem;
	using winrt::Windows::UI::Xaml::Setter;
	using winrt::Windows::Storage::StorageDeleteOption;

	constexpr wchar_t FONT_FAMILY_DEFVAL[] = L"Segoe UI Variable";	// 書体名の規定値 (システムリソースに値が無かった場合)
	constexpr wchar_t FONT_STYLE_DEFVAL[] = L"BodyTextBlockStyle";	// 文字列の規定値を得るシステムリソース

	// 背景パターンの画像ブラシを得る.
	void MainPage::background_get_brush(void) noexcept
	{
		HRESULT hr = S_OK;
		// WIC ファクトリを使って, 画像ファイルを読み込み WIC デコーダーを作成する.
		// WIC ファクトリは ShapeImage が確保しているものを使用する.
		winrt::com_ptr<IWICBitmapDecoder> wic_decoder;
		if (hr == S_OK) {
			hr = ShapeImage::wic_factory->CreateDecoderFromFilename(L"Assets/background.png", nullptr, GENERIC_READ, WICDecodeMetadataCacheOnDemand, wic_decoder.put());
		}
		// 読み込まれた画像のフレーム数を得る (通常は 1 フレーム).
		UINT f_cnt;
		if (hr == S_OK) {
			hr = wic_decoder->GetFrameCount(&f_cnt);
		}
		// 最後のフレームを得る.
		winrt::com_ptr<IWICBitmapFrameDecode> wic_frame;
		if (hr == S_OK) {
			hr = wic_decoder->GetFrame(f_cnt - 1, wic_frame.put());
		}
		wic_decoder = nullptr;
		// WIC ファクトリを使って, WIC フォーマットコンバーターを作成する.
		if (hr == S_OK) {
			hr = ShapeImage::wic_factory->CreateFormatConverter(m_wic_background.put());
		}
		// WIC フォーマットコンバーターに, 得たフレームを格納する.
		if (hr == S_OK) {
			hr = m_wic_background->Initialize(wic_frame.get(), GUID_WICPixelFormat32bppPBGRA, WICBitmapDitherTypeNone, nullptr, 0.0f, WICBitmapPaletteTypeCustom);
		}
		wic_frame = nullptr;
	}

	// 背景色の項目に印をつける
	void MainPage::background_color_is_checked(const bool checker_board, const D2D1_COLOR_F& color)
	{
		if (checker_board) {
			tmfi_menu_background_show().IsChecked(true);
			tmfi_popup_background_show().IsChecked(true);
		}
		else {
			tmfi_menu_background_show().IsChecked(false);
			tmfi_popup_background_show().IsChecked(false);
		}
		if (equal(color, COLOR_BLACK)) {
			rmfi_menu_background_black().IsChecked(true);
			rmfi_popup_background_black().IsChecked(true);
		}
		else {
			rmfi_menu_background_white().IsChecked(true);
			rmfi_popup_background_white().IsChecked(true);
		}
	}

	// 背景パターンがクリックされた.
	void MainPage::background_pattern_click(IInspectable const& sender, RoutedEventArgs const&)
	{
		if (sender == tmfi_menu_background_show() || sender == tmfi_popup_background_show()) {
			const auto toggle = sender.as<ToggleMenuFlyoutItem>();
			if (m_background_show != toggle.IsChecked()) {
				m_background_show = toggle.IsChecked();
				main_draw();
			}
		}
		else if (sender == rmfi_menu_background_white() || sender == rmfi_popup_background_white()) {
			if (!equal(m_background_color, COLOR_WHITE)) {
				m_background_color = COLOR_WHITE;
				main_draw();
			}
		}
		else if (sender == rmfi_menu_background_black() || sender == rmfi_popup_background_black()) {
			if (!equal(m_background_color, COLOR_BLACK)) {
				m_background_color = COLOR_BLACK;
				main_draw();
			}
		}
	}

	// レイアウトメニューの「方眼の強調」が選択された.
	void MainPage::grid_emph_click(IInspectable const& sender, RoutedEventArgs const&)
	{
		GRID_EMPH val;
		if (sender == rmfi_menu_grid_emph_1() || sender == rmfi_popup_grid_emph_1()) {
			val = GRID_EMPH_0;
		}
		else if (sender == rmfi_menu_grid_emph_2() || sender == rmfi_popup_grid_emph_2()) {
			val = GRID_EMPH_2;
		}
		else if (sender == rmfi_menu_grid_emph_3() || sender == rmfi_popup_grid_emph_3()) {
			val = GRID_EMPH_3;
		}
		else {
			throw winrt::hresult_not_implemented();
			return;
		}
		grid_emph_is_checked(val);
		GRID_EMPH g_emph;
		m_main_page.get_grid_emph(g_emph);
		if (!equal(g_emph, val)) {
			undo_push_set<UNDO_T::GRID_EMPH>(&m_main_page, val);
			undo_push_null();
			undo_menu_is_enabled();
			main_draw();
		}
		status_bar_set_pos();
	}

	// レイアウトメニューの「方眼の強調」に印をつける.
	// g_emph	方眼の強調
	void MainPage::grid_emph_is_checked(const GRID_EMPH& g_emph)
	{
		if (g_emph.m_gauge_1 == 0 && g_emph.m_gauge_2 == 0) {
			rmfi_menu_grid_emph_1().IsChecked(true);
			rmfi_popup_grid_emph_1().IsChecked(true);
		}
		else if (g_emph.m_gauge_1 != 0 && g_emph.m_gauge_2 == 0) {
			rmfi_menu_grid_emph_2().IsChecked(true);
			rmfi_popup_grid_emph_2().IsChecked(true);
		}
		else if (g_emph.m_gauge_1 != 0 && g_emph.m_gauge_2 != 0) {
			rmfi_menu_grid_emph_3().IsChecked(true);
			rmfi_popup_grid_emph_3().IsChecked(true);
		}
	}

	// レイアウトメニューの「方眼の大きさ」>「大きさ」が選択された.
	IAsyncAction MainPage::grid_len_click_async(IInspectable const&, RoutedEventArgs const&)
	{
		m_mutex_event.lock();
		constexpr auto MAX_VALUE = 127.5;
		constexpr auto TICK_FREQ = 0.5;
		const auto str_grid_length{
			ResourceLoader::GetForCurrentView().GetString(L"str_grid_length") + L": "
		};
		const auto str_title{
			ResourceLoader::GetForCurrentView().GetString(L"str_grid_length")
		};
		m_prop_page.set_attr_to(&m_main_page);
		const auto dpi = m_prop_d2d.m_logical_dpi;
		const auto g_len = m_prop_page.m_grid_base + 1.0;
		float g_base;
		m_prop_page.get_grid_base(g_base);
		wchar_t buf[32];
		conv_len_to_str<LEN_UNIT_NAME_APPEND>(m_len_unit, g_base + 1.0, dpi, g_len, buf);

		dialog_slider_0().Minimum(0.0);
		dialog_slider_0().Maximum(MAX_VALUE);
		dialog_slider_0().TickFrequency(TICK_FREQ);
		dialog_slider_0().SnapsTo(SliderSnapsTo::Ticks);
		dialog_slider_0().Value(g_base);
		dialog_slider_0().Header(box_value(str_grid_length + buf));
		dialog_slider_0().Visibility(Visibility::Visible);

		dialog_combo_box_0().Items().Append(box_value(rmfi_menu_len_unit_pixel().Text()));
		dialog_combo_box_0().Items().Append(box_value(rmfi_menu_len_unit_inch().Text()));
		dialog_combo_box_0().Items().Append(box_value(rmfi_menu_len_unit_milli().Text()));
		dialog_combo_box_0().Items().Append(box_value(rmfi_menu_len_unit_point().Text()));
		dialog_combo_box_0().Items().Append(box_value(rmfi_menu_len_unit_grid().Text()));
		if (m_len_unit == LEN_UNIT::PIXEL) {
			dialog_combo_box_0().SelectedIndex(0);
		}
		else if (m_len_unit == LEN_UNIT::INCH) {
			dialog_combo_box_0().SelectedIndex(1);
		}
		else if (m_len_unit == LEN_UNIT::MILLI) {
			dialog_combo_box_0().SelectedIndex(2);
		}
		else if (m_len_unit == LEN_UNIT::POINT) {
			dialog_combo_box_0().SelectedIndex(3);
		}
		else if (m_len_unit == LEN_UNIT::GRID) {
			dialog_combo_box_0().SelectedIndex(4);
		}
		dialog_combo_box_0().Visibility(Visibility::Visible);

		cd_dialog_prop().Title(box_value(str_title));
		{
			const auto revoker0{
				dialog_slider_0().ValueChanged(winrt::auto_revoke, [this, str_grid_length](IInspectable const&, RangeBaseValueChangedEventArgs const& args) {
					const auto unit = m_len_unit;
					const auto dpi = m_prop_d2d.m_logical_dpi;
					const auto g_len = m_main_page.m_grid_base + 1.0f;	// <---
					const float val = static_cast<float>(args.NewValue());
					wchar_t buf[32];
					conv_len_to_str<LEN_UNIT_NAME_APPEND>(unit, val + 1.0, dpi, g_len, buf);
					dialog_slider_0().Header(box_value(str_grid_length + buf));
					if (m_prop_page.set_grid_base(val)) {
						dialog_draw();
					}
				})
			};
			const auto revoker1{
				dialog_combo_box_0().SelectionChanged(winrt::auto_revoke, [this, str_grid_length](IInspectable const&, SelectionChangedEventArgs const&) {
					if (dialog_combo_box_0().SelectedIndex() == 0) {
						if (m_len_unit != LEN_UNIT::PIXEL) {
							m_len_unit = LEN_UNIT::PIXEL;
							const auto unit = m_len_unit;
							const auto dpi = m_prop_d2d.m_logical_dpi;
							const auto g_len = m_main_page.m_grid_base + 1.0f;	// <---
							const auto val = m_prop_page.m_grid_base + 1.0f;
							wchar_t buf[32];
							conv_len_to_str<LEN_UNIT_NAME_APPEND>(unit, val, dpi, g_len, buf);
							dialog_slider_0().Header(box_value(str_grid_length + buf));
						}
					}
					else if (dialog_combo_box_0().SelectedIndex() == 1) {
						if (m_len_unit != LEN_UNIT::INCH) {
							m_len_unit = LEN_UNIT::INCH;
							const auto unit = m_len_unit;
							const auto dpi = m_prop_d2d.m_logical_dpi;
							const auto g_len = m_main_page.m_grid_base + 1.0f;	// <---
							const auto val = m_prop_page.m_grid_base + 1.0f;
							wchar_t buf[32];
							conv_len_to_str<LEN_UNIT_NAME_APPEND>(unit, val, dpi, g_len, buf);
							dialog_slider_0().Header(box_value(str_grid_length + buf));
						}
					}
					else if (dialog_combo_box_0().SelectedIndex() == 2) {
						if (m_len_unit != LEN_UNIT::MILLI) {
							m_len_unit = LEN_UNIT::MILLI;
							const auto unit = m_len_unit;
							const auto dpi = m_prop_d2d.m_logical_dpi;
							const auto g_len = m_main_page.m_grid_base + 1.0f;	// <---
							const auto val = m_prop_page.m_grid_base + 1.0f;
							wchar_t buf[32];
							conv_len_to_str<LEN_UNIT_NAME_APPEND>(unit, val, dpi, g_len, buf);
							dialog_slider_0().Header(box_value(str_grid_length + buf));
						}
					}
					else if (dialog_combo_box_0().SelectedIndex() == 3) {
						if (m_len_unit != LEN_UNIT::POINT) {
							m_len_unit = LEN_UNIT::POINT;
							const auto unit = m_len_unit;
							const auto dpi = m_prop_d2d.m_logical_dpi;
							const auto g_len = m_main_page.m_grid_base + 1.0f;	// <---
							const auto val = m_prop_page.m_grid_base + 1.0f;
							wchar_t buf[32];
							conv_len_to_str<LEN_UNIT_NAME_APPEND>(unit, val, dpi, g_len, buf);
							dialog_slider_0().Header(box_value(str_grid_length + buf));
						}
					}
					else if (dialog_combo_box_0().SelectedIndex() == 4) {
						if (m_len_unit != LEN_UNIT::GRID) {
							m_len_unit = LEN_UNIT::GRID;
							const auto unit = m_len_unit;
							const auto dpi = m_prop_d2d.m_logical_dpi;
							const auto g_len = m_main_page.m_grid_base + 1.0f;	// <---
							const auto val = m_prop_page.m_grid_base + 1.0f;
							wchar_t buf[32];
							conv_len_to_str<LEN_UNIT_NAME_APPEND>(unit, val , dpi, g_len, buf);
							dialog_slider_0().Header(box_value(str_grid_length + buf));
						}
					}

				})
			};
			if (co_await cd_dialog_prop().ShowAsync() == ContentDialogResult::Primary) {
				float setting_val;
				float page_val;

				m_main_page.get_grid_base(page_val);
				m_prop_page.get_grid_base(setting_val);
				if (!equal(page_val, setting_val)) {
					undo_push_set<UNDO_T::GRID_BASE>(&m_main_page, setting_val);
					undo_push_null();
					undo_menu_is_enabled();
					main_draw();
				}

			}
		}
		dialog_slider_0().Visibility(Visibility::Collapsed);
		dialog_combo_box_0().Items().Clear();
		dialog_combo_box_0().Visibility(Visibility::Collapsed);
		status_bar_set_pos();
		m_mutex_event.unlock();
	}

	// レイアウトメニューの「方眼の大きさ」>「狭める」が選択された.
	void MainPage::grid_len_con_click(IInspectable const&, RoutedEventArgs const&)
	{
		float g_base;
		m_main_page.get_grid_base(g_base);
		const float val = (g_base + 1.0f) * 0.5f - 1.0f;
		if (val >= 1.0f) {
			undo_push_set<UNDO_T::GRID_BASE>(&m_main_page, val);
			undo_push_null();
			undo_menu_is_enabled();
			main_draw();
		}
		status_bar_set_pos();
	}

	// レイアウトメニューの「方眼の大きさ」>「広げる」が選択された.
	void MainPage::grid_len_exp_click(IInspectable const&, RoutedEventArgs const&)
	{
		float g_base;
		m_main_page.get_grid_base(g_base);
		const float val = (g_base + 1.0f) * 2.0f - 1.0f;
		if (val <= max(m_main_page.m_page_size.width, m_main_page.m_page_size.height)) {
			undo_push_set<UNDO_T::GRID_BASE>(&m_main_page, val);
			undo_push_null();
			undo_menu_is_enabled();
			main_draw();
		}
		status_bar_set_pos();
	}

	// レイアウトメニューの「方眼の表示」>「最背面」が選択された.
	void MainPage::grid_show_click(IInspectable const& sender, RoutedEventArgs const&)
	{
		GRID_SHOW new_val;
		if (sender == rmfi_menu_grid_show_back() || sender == rmfi_popup_grid_show_back()) {
			new_val = GRID_SHOW::BACK;
		}
		else if (sender == rmfi_menu_grid_show_front() || sender == rmfi_popup_grid_show_front()) {
			new_val = GRID_SHOW::FRONT;
		}
		else if (sender == rmfi_menu_grid_show_hide() || sender == rmfi_popup_grid_show_hide()) {
			new_val = GRID_SHOW::HIDE;
		}
		else {
			throw winrt::hresult_not_implemented();
			return;
		}
		grid_show_is_checked(new_val);
		if (m_main_page.m_grid_show != new_val) {
			undo_push_set<UNDO_T::GRID_SHOW>(&m_main_page, new_val);
			undo_push_null();
			undo_menu_is_enabled();
			main_draw();
		}
		status_bar_set_pos();
	}

	// レイアウトメニューの「方眼の表示」に印をつける.
	// g_show	方眼の表示
	void MainPage::grid_show_is_checked(const GRID_SHOW g_show)
	{
		if (g_show == GRID_SHOW::BACK) {
			rmfi_menu_grid_show_back().IsChecked(true);
			rmfi_popup_grid_show_back().IsChecked(true);
		}
		else if (g_show == GRID_SHOW::FRONT) {
			rmfi_menu_grid_show_front().IsChecked(true);
			rmfi_popup_grid_show_front().IsChecked(true);
		}
		else if (g_show == GRID_SHOW::HIDE) {
			rmfi_menu_grid_show_hide().IsChecked(true);
			rmfi_popup_grid_show_hide().IsChecked(true);
		}
	}
	// レイアウトメニューの「レイアウトを既定値に戻す」が選択された.
	// データを保存したファイルがある場合, それを削除する.
	IAsyncAction MainPage::layout_reset_click_async(IInspectable const&, RoutedEventArgs const&)
	{
		winrt::Windows::Storage::IStorageItem setting_item{
			co_await ApplicationData::Current().LocalFolder().TryGetItemAsync(LAYOUT_FILE)
		};
		if (setting_item != nullptr) {
			auto delete_file = setting_item.try_as<StorageFile>();
			if (delete_file != nullptr) {
				HRESULT hres = E_FAIL;
				try {
					co_await delete_file.DeleteAsync(StorageDeleteOption::PermanentDelete);
					mfi_menu_layout_reset().IsEnabled(false);
					hres = S_OK;
				}
				catch (winrt::hresult_error const& e) {
					hres = e.code();
				}
				if (hres != S_OK) {

				}
				delete_file = nullptr;
			}
			setting_item = nullptr;
		}
		// レイアウトを既定値に戻す.
		layout_init();
		main_draw();
		status_bar_set_pos();
	}

	// レイアウトメニューの「 レイアウトを保存」が選択された
	// ローカルフォルダーにファイルを作成し, 設定データを保存する.
	IAsyncAction MainPage::layout_save_click_async(IInspectable const&, RoutedEventArgs const&)
	{
		auto setting_file{
			co_await ApplicationData::Current().LocalFolder().CreateFileAsync(LAYOUT_FILE, CreationCollisionOption::ReplaceExisting)
		};
		if (setting_file != nullptr) {
			co_await file_write_gpf_async<false, true>(setting_file);
			setting_file = nullptr;
			mfi_menu_layout_reset().IsEnabled(true);
		}
		//using winrt::Windows::Storage::AccessCache::StorageApplicationPermissions;
		//auto const& mru_list = StorageApplicationPermissions::MostRecentlyUsedList();
		//mru_list.Clear();
	}

	// レイアウトを既定値に戻す.
	void MainPage::layout_init(void) noexcept
	{
		// 書体の属性を初期化する.
		{
			// リソースの取得に失敗した場合に備えて, 静的な既定値を書体属性に格納する.
			m_main_page.set_font_family(wchar_cpy(FONT_FAMILY_DEFVAL));
			m_main_page.set_font_size(FONT_SIZE_DEFVAL);
			m_main_page.set_font_stretch(DWRITE_FONT_STRETCH::DWRITE_FONT_STRETCH_NORMAL);
			m_main_page.set_font_style(DWRITE_FONT_STYLE::DWRITE_FONT_STYLE_NORMAL);
			m_main_page.set_font_weight(DWRITE_FONT_WEIGHT::DWRITE_FONT_WEIGHT_NORMAL);
			// BodyTextBlockStyle をリソースディクショナリから得る.
			auto resource = Resources().TryLookup(box_value(FONT_STYLE_DEFVAL));
			if (resource != nullptr) {
				auto style = resource.try_as<winrt::Windows::UI::Xaml::Style>();
				std::list<winrt::Windows::UI::Xaml::Style> stack;
				// リソースからスタイルを得る.
				while (style != nullptr) {
					// スタイルが空でない場合.
					// スタイルをスタックに積む.
					stack.push_back(style);
					// スタイルの継承元のスタイルを得る.
					style = style.BasedOn();
				}
				try {
					while (!stack.empty()) {
						// スタックが空でない場合,
						// スタイルをスタックから取り出す.
						style = stack.back();
						stack.pop_back();
						// スタイルの中の各セッターについて.
						auto const& setters = style.Setters();
						for (auto const& base : setters) {
							auto const& setter = base.try_as<Setter>();
							// セッターのプロパティーを得る.
							auto const& prop = setter.Property();
							if (prop == TextBlock::FontFamilyProperty()) {
								// プロパティーが FontFamily の場合,
								// セッターの値から, 書体名を得る.
								const auto val = unbox_value<winrt::Windows::UI::Xaml::Media::FontFamily>(setter.Value());
								m_main_page.set_font_family(wchar_cpy(val.Source().c_str()));
							}
							else if (prop == TextBlock::FontSizeProperty()) {
								// プロパティーが FontSize の場合,
								// セッターの値から, 書体の大きさを得る.
								const auto val = unbox_value<float>(setter.Value());
								m_main_page.m_font_size = val;
							}
							else if (prop == TextBlock::FontStretchProperty()) {
								// プロパティーが FontStretch の場合,
								// セッターの値から, 書体の幅を得る.
								const auto val = unbox_value<int32_t>(setter.Value());
								m_main_page.set_font_stretch(static_cast<DWRITE_FONT_STRETCH>(val));
							}
							else if (prop == TextBlock::FontStyleProperty()) {
								// プロパティーが FontStyle の場合,
								// セッターの値から, 字体を得る.
								const auto val = unbox_value<int32_t>(setter.Value());
								m_main_page.set_font_style(static_cast<DWRITE_FONT_STYLE>(val));
							}
							else if (prop == TextBlock::FontWeightProperty()) {
								// プロパティーが FontWeight の場合,
								// セッターの値から, 書体の太さを得る.
								const auto val = unbox_value<int32_t>(setter.Value());
								m_main_page.set_font_weight(static_cast<DWRITE_FONT_WEIGHT>(val));
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
			ShapeText::is_available_font(m_main_page.m_font_family);
		}

		{
			const IInspectable& accent_color = Resources().TryLookup(
				box_value(L"SystemAccentColor"));
			D2D1_COLOR_F grid_color;
			if (accent_color != nullptr) {
				const auto uwp_color = unbox_value<Color>(accent_color);
				conv_uwp_to_color(uwp_color, grid_color);
				grid_color.a = GRID_COLOR_DEFVAL.a;
			}
			else {
				grid_color = GRID_COLOR_DEFVAL;
			}
			m_main_page.set_arrow_size(ARROW_SIZE_DEFVAL);
			m_main_page.set_arrow_style(ARROW_STYLE::ARROW_NONE);
			m_main_page.set_corner_radius(D2D1_POINT_2F{ GRID_LEN_DEFVAL, GRID_LEN_DEFVAL });
			m_main_page.set_fill_color(COLOR_WHITE);
			m_main_page.set_font_color(COLOR_BLACK);
			m_main_page.set_font_style(DWRITE_FONT_STYLE::DWRITE_FONT_STYLE_NORMAL);
			m_main_page.set_grid_base(GRID_LEN_DEFVAL - 1.0);
			m_main_page.set_grid_color(grid_color);
			m_main_page.set_grid_emph(GRID_EMPH_0);
			m_main_page.set_grid_show(GRID_SHOW::BACK);
			m_main_page.set_page_color(COLOR_WHITE);
			m_main_page.set_page_size(PAGE_SIZE_DEFVAL);
			m_main_page.set_page_margin(D2D1_RECT_F{ 0.0f, 0.0f, 0.0f, 0.0f });
			m_main_page.set_stroke_cap(D2D1_CAP_STYLE::D2D1_CAP_STYLE_FLAT);
			m_main_page.set_stroke_color(COLOR_BLACK);
			//m_main_page.set_dash_cap(D2D1_CAP_STYLE::D2D1_CAP_STYLE_FLAT);
			m_main_page.set_stroke_dash_pat(DASH_PAT_DEFVAL);
			m_main_page.set_stroke_dash(D2D1_DASH_STYLE::D2D1_DASH_STYLE_SOLID);
			m_main_page.set_stroke_join_limit(JOIN_MITER_LIMIT_DEFVAL);
			m_main_page.set_stroke_join(D2D1_LINE_JOIN::D2D1_LINE_JOIN_MITER_OR_BEVEL);
			m_main_page.set_stroke_width(1.0);
			m_main_page.set_text_align_vert(DWRITE_PARAGRAPH_ALIGNMENT::DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
			m_main_page.set_text_align_horz(DWRITE_TEXT_ALIGNMENT::DWRITE_TEXT_ALIGNMENT_LEADING);
			m_main_page.set_text_line_sp(0.0);
			m_main_page.set_text_pad(TEXT_PAD_DEFVAL);
		}
		m_image_keep_aspect = true;
		m_len_unit = LEN_UNIT::PIXEL;
		m_color_code = COLOR_CODE::DEC;
		m_main_scale = 1.0f;
		m_snap_grid = true;
		m_snap_point = SNAP_INTERVAL_DEF_VAL;
		m_status_bar = STATUS_BAR_DEF_VAL;
		m_background_show = false;
		m_background_color = COLOR_WHITE;

		//event_menu_is_checked();

		font_style_is_checked(m_main_page.m_font_style);
		font_stretch_is_checked(m_main_page.m_font_stretch);
		font_weight_is_checked(m_main_page.m_font_weight);
		grid_emph_is_checked(m_main_page.m_grid_emph);
		grid_show_is_checked(m_main_page.m_grid_show);
		stroke_arrow_is_checked(m_main_page.m_arrow_style);
		stroke_cap_is_checked(m_main_page.m_stroke_cap);
		stroke_dash_is_checked(m_main_page.m_stroke_dash);
		stroke_join_is_checked(m_main_page.m_stroke_join);
		stroke_width_is_checked(m_main_page.m_stroke_width);
		text_align_horz_is_checked(m_main_page.m_text_align_horz);
		text_align_vert_is_checked(m_main_page.m_text_align_vert);
		background_color_is_checked(m_background_show, m_background_color);
		page_zoom_is_checked(m_main_scale);

		image_keep_aspect_is_checked(m_image_keep_aspect);
		len_unit_is_checked(m_len_unit);
		color_code_is_checked(m_color_code);
		status_bar_is_checked(m_status_bar);
		tmfi_menu_snap_grid().IsChecked(m_snap_grid);
	}

	//------------------------------
// レイアウトメニューの「ページの大きさ」が選択された
//------------------------------
	IAsyncAction MainPage::page_size_click_async(IInspectable const&, RoutedEventArgs const&)
	{
		m_prop_page.set_attr_to(&m_main_page);

		if (m_len_unit == LEN_UNIT::GRID) {
			cb_len_unit().SelectedItem(box_value(cbi_len_unit_grid()));
		}
		else if (m_len_unit == LEN_UNIT::INCH) {
			cb_len_unit().SelectedItem(box_value(cbi_len_unit_inch()));
		}
		else if (m_len_unit == LEN_UNIT::MILLI) {
			cb_len_unit().SelectedItem(box_value(cbi_len_unit_milli()));
		}
		else if (m_len_unit == LEN_UNIT::POINT) {
			cb_len_unit().SelectedItem(box_value(cbi_len_unit_point()));
		}
		else {
			cb_len_unit().SelectedItem(box_value(cbi_len_unit_pixel()));
		}

		const double g_len = m_main_page.m_grid_base + 1.0;
		const double dpi = m_main_d2d.m_logical_dpi;
		wchar_t buf[32];
		conv_len_to_str<LEN_UNIT_NAME_NOT_APPEND>(m_len_unit, m_main_page.m_page_size.width, dpi, g_len, buf);
		tx_page_size_width().Text(buf);
		conv_len_to_str<LEN_UNIT_NAME_NOT_APPEND>(m_len_unit, m_main_page.m_page_size.height, dpi, g_len, buf);
		tx_page_size_height().Text(buf);
		conv_len_to_str<LEN_UNIT_NAME_NOT_APPEND>(m_len_unit, m_main_page.m_page_margin.left, dpi, g_len, buf);
		tx_page_margin_left().Text(buf);
		conv_len_to_str<LEN_UNIT_NAME_NOT_APPEND>(m_len_unit, m_main_page.m_page_margin.top, dpi, g_len, buf);
		tx_page_margin_top().Text(buf);
		conv_len_to_str<LEN_UNIT_NAME_NOT_APPEND>(m_len_unit, m_main_page.m_page_margin.right, dpi, g_len, buf);
		tx_page_margin_right().Text(buf);
		conv_len_to_str<LEN_UNIT_NAME_NOT_APPEND>(m_len_unit, m_main_page.m_page_margin.bottom, dpi, g_len, buf);
		tx_page_margin_bottom().Text(buf);

		// この時点では, テキストボックスに正しい数値を格納しても, TextChanged は呼ばれない.
		// プライマリーボタンは使用可能にしておく.
		cd_page_size_dialog().IsPrimaryButtonEnabled(true);
		cd_page_size_dialog().IsSecondaryButtonEnabled(m_main_page.m_shape_list.size() > 0);
		const auto d_result = co_await cd_page_size_dialog().ShowAsync();
		if (d_result == ContentDialogResult::Primary) {
			constexpr wchar_t INVALID_NUM[] = L"str_err_number";

			if (cbi_len_unit_grid().IsSelected()) {
				m_len_unit = LEN_UNIT::GRID;
			}
			else if (cbi_len_unit_inch().IsSelected()) {
				m_len_unit = LEN_UNIT::INCH;
			}
			else if (cbi_len_unit_milli().IsSelected()) {
				m_len_unit = LEN_UNIT::MILLI;
			}
			else if (cbi_len_unit_grid().IsSelected()) {
				m_len_unit = LEN_UNIT::GRID;
			}
			else if (cbi_len_unit_point().IsSelected()) {
				m_len_unit = LEN_UNIT::POINT;
			}
			else {
				m_len_unit = LEN_UNIT::PIXEL;
			}
			len_unit_is_checked(m_len_unit);

			float new_left;
			if (swscanf_s(tx_page_margin_left().Text().c_str(), L"%f", &new_left) != 1) {
				// 「無効な数値です」メッセージダイアログを表示する.
				message_show(ICON_ALERT, INVALID_NUM, L"tx_page_margin_left/Header");
				co_return;
			}
			float new_top;
			if (swscanf_s(tx_page_margin_top().Text().c_str(), L"%f", &new_top) != 1) {
				// 「無効な数値です」メッセージダイアログを表示する.
				message_show(ICON_ALERT, INVALID_NUM, L"tx_page_margin_top/Header");
				co_return;
			}
			float new_right;
			if (swscanf_s(tx_page_margin_right().Text().c_str(), L"%f", &new_right) != 1) {
				// 「無効な数値です」メッセージダイアログを表示する.
				message_show(ICON_ALERT, INVALID_NUM, L"tx_page_margin_right/Header");
				co_return;
			}
			float new_bottom;
			if (swscanf_s(tx_page_margin_bottom().Text().c_str(), L"%f", &new_bottom) != 1) {
				// 「無効な数値です」メッセージダイアログを表示する.
				message_show(ICON_ALERT, INVALID_NUM, L"tx_page_margin_bottom/Header");
				co_return;
			}
			// 表示の縦横の長さの値をピクセル単位の値に変換する.
			D2D1_RECT_F p_mar{
				static_cast<FLOAT>(conv_len_to_pixel(m_len_unit, new_left, dpi, g_len)),
				static_cast<FLOAT>(conv_len_to_pixel(m_len_unit, new_top, dpi, g_len)),
				static_cast<FLOAT>(conv_len_to_pixel(m_len_unit, new_right, dpi, g_len)),
				static_cast<FLOAT>(conv_len_to_pixel(m_len_unit, new_bottom, dpi, g_len))
			};

			float new_width;
			if (swscanf_s(tx_page_size_width().Text().c_str(), L"%f", &new_width) != 1) {
				// 「無効な数値です」メッセージダイアログを表示する.
				message_show(ICON_ALERT, INVALID_NUM, L"tx_page_size_width/Header");
				co_return;
			}
			float new_height;
			if (swscanf_s(tx_page_size_height().Text().c_str(), L"%f", &new_height) != 1) {
				// 「無効な数値です」メッセージダイアログを表示する.
				message_show(ICON_ALERT, INVALID_NUM, L"tx_page_size_height/Header");
				co_return;
			}
			D2D1_SIZE_F p_size{
				static_cast<FLOAT>(conv_len_to_pixel(m_len_unit, new_width, dpi, g_len)),
				static_cast<FLOAT>(conv_len_to_pixel(m_len_unit, new_height, dpi, g_len))
			};

			const bool flag_size = !equal(p_size, m_main_page.m_page_size);
			const bool flag_mar = !equal(p_mar, m_main_page.m_page_margin);
			if (flag_size || flag_mar) {
				if (flag_size) {
					undo_push_set<UNDO_T::PAGE_SIZE>(&m_main_page, p_size);
				}
				if (flag_mar) {
					undo_push_set<UNDO_T::PAGE_PAD>(&m_main_page, p_mar);
				}
				undo_push_null();
				undo_menu_is_enabled();
				main_bbox_update();
				main_panel_size();
				main_draw();
				status_bar_set_pos();
				status_bar_set_grid();
				status_bar_set_page();
				status_bar_set_unit();
			}
		}
		else if (d_result == ContentDialogResult::Secondary) {
			constexpr wchar_t INVALID_NUM[] = L"str_err_number";

			if (cbi_len_unit_grid().IsSelected()) {
				m_len_unit = LEN_UNIT::GRID;
			}
			else if (cbi_len_unit_inch().IsSelected()) {
				m_len_unit = LEN_UNIT::INCH;
			}
			else if (cbi_len_unit_milli().IsSelected()) {
				m_len_unit = LEN_UNIT::MILLI;
			}
			else if (cbi_len_unit_grid().IsSelected()) {
				m_len_unit = LEN_UNIT::GRID;
			}
			else if (cbi_len_unit_point().IsSelected()) {
				m_len_unit = LEN_UNIT::POINT;
			}
			else {
				m_len_unit = LEN_UNIT::PIXEL;
			}
			len_unit_is_checked(m_len_unit);

			float new_left;
			if (swscanf_s(tx_page_margin_left().Text().c_str(), L"%f", &new_left) != 1) {
				// 「無効な数値です」メッセージダイアログを表示する.
				message_show(ICON_ALERT, INVALID_NUM, L"tx_page_margin_left/Header");
				co_return;
			}
			float new_top;
			if (swscanf_s(tx_page_margin_top().Text().c_str(), L"%f", &new_top) != 1) {
				// 「無効な数値です」メッセージダイアログを表示する.
				message_show(ICON_ALERT, INVALID_NUM, L"tx_page_margin_top/Header");
				co_return;
			}
			float new_right;
			if (swscanf_s(tx_page_margin_right().Text().c_str(), L"%f", &new_right) != 1) {
				// 「無効な数値です」メッセージダイアログを表示する.
				message_show(ICON_ALERT, INVALID_NUM, L"tx_page_margin_right/Header");
				co_return;
			}
			float new_bottom;
			if (swscanf_s(tx_page_margin_bottom().Text().c_str(), L"%f", &new_bottom) != 1) {
				// 「無効な数値です」メッセージダイアログを表示する.
				message_show(ICON_ALERT, INVALID_NUM, L"tx_page_margin_bottom/Header");
				co_return;
			}
			// 長さの値をピクセル単位の値に変換する.
			D2D1_RECT_F p_mar{	// ページの余白
				static_cast<FLOAT>(conv_len_to_pixel(m_len_unit, new_left, dpi, g_len)),
				static_cast<FLOAT>(conv_len_to_pixel(m_len_unit, new_top, dpi, g_len)),
				static_cast<FLOAT>(conv_len_to_pixel(m_len_unit, new_right, dpi, g_len)),
				static_cast<FLOAT>(conv_len_to_pixel(m_len_unit, new_bottom, dpi, g_len))
			};

			// リスト中の図形を囲む矩形を得る.
			D2D1_POINT_2F b_lt{ FLT_MAX, FLT_MAX };
			D2D1_POINT_2F b_rb{ -FLT_MAX, -FLT_MAX };
			slist_bbox_shape(m_main_page.m_shape_list, b_lt, b_rb);

			// 矩形の大きさがゼロ,なら中断する.
			if (b_rb.x - b_lt.x < 1.0F || b_rb.y - b_lt.y < 1.0F) {
				co_return;
			}

			// 左上点の座標のいずれかが負ならば, 原点となるよう矩形を移動する.
			float dx = 0.0f;	// 矩形を移動した距離
			float dy = 0.0f;	// 矩形を移動した距離
			if (b_lt.x < 0.0F) {
				dx = -b_lt.x;
				b_lt.x = 0.0F;
				b_rb.x += dx;
			}
			if (b_lt.y < 0.0F) {
				dy = -b_lt.y;
				b_lt.y = 0.0F;
				b_rb.y += dy;
			}

			// 左上点の値を, 図形を取り囲むパディングとみなし,
			// 左上点の座標のいずれかが正ならば, その分だけ右下点を右下に移動する.
			if (b_lt.x > 0.0f) {
				b_rb.x += b_lt.x;
			}
			if (b_lt.y > 0.0f) {
				b_rb.y += b_lt.y;
			}

			// 右下点に余白を加えた値がページの大きさとなる.
			D2D1_SIZE_F p_size{	// ページの大きさ.
				new_left + b_rb.x + new_right,
				new_top + b_rb.y + new_bottom
			};

			// ページの大きさ, 余白, いずれかが異なる.
			// あるいは矩形が移動したなら
			const bool size_changed = !equal(p_size, m_main_page.m_page_size);
			const bool mar_chanfed = !equal(p_mar, m_main_page.m_page_margin);
			if (size_changed || mar_chanfed || dx > 0.0f || dy > 0.0f) {
				// 矩形が移動したなら, 図形が矩形に収まるよう, 図形も移動させる.
				if (dx > 0.0f || dy > 0.0f) {
					constexpr auto ANY = true;
					undo_push_move({ dx, dy }, ANY);
				}
				// ページの大きさが異なるなら, 更新する.
				if (size_changed) {
					undo_push_set<UNDO_T::PAGE_SIZE>(&m_main_page, p_size);
				}
				// ページの余白が異なるなら, 更新する.
				if (mar_chanfed) {
					undo_push_set<UNDO_T::PAGE_PAD>(&m_main_page, p_mar);
				}
				undo_push_null();
				undo_menu_is_enabled();
				main_bbox_update();
				main_panel_size();
				main_draw();
				status_bar_set_grid();
				status_bar_set_page();
			}
			status_bar_set_unit();
		}
		status_bar_set_pos();
	}

	// ページの大きさダイアログのテキストボックスの値が変更された.
	void MainPage::page_size_value_changed(IInspectable const&, NumberBoxValueChangedEventArgs const&)
	{
		const double dpi = m_main_d2d.m_logical_dpi;	// DPI
		const auto g_len = m_main_page.m_grid_base + 1.0;
		double w = tx_page_size_width().Value();
		double h = tx_page_size_height().Value();
		double l = tx_page_margin_left().Value();
		double t = tx_page_margin_top().Value();
		double r = tx_page_margin_right().Value();
		double b = tx_page_margin_bottom().Value();
		LEN_UNIT u;
		if (cbi_len_unit_grid().IsSelected()) {
			u = LEN_UNIT::GRID;
		}
		else if (cbi_len_unit_inch().IsSelected()) {
			u = LEN_UNIT::INCH;
		}
		else if (cbi_len_unit_milli().IsSelected()) {
			u = LEN_UNIT::MILLI;
		}
		else if (cbi_len_unit_point().IsSelected()) {
			u = LEN_UNIT::POINT;
		}
		else {
			u = LEN_UNIT::PIXEL;
		}
		if (u != LEN_UNIT::PIXEL) {
			w = conv_len_to_pixel(u, w, dpi, g_len);
			h = conv_len_to_pixel(u, h, dpi, g_len);
			l = conv_len_to_pixel(u, l, dpi, g_len);
			t = conv_len_to_pixel(u, t, dpi, g_len);
			r = conv_len_to_pixel(u, r, dpi, g_len);
			b = conv_len_to_pixel(u, b, dpi, g_len);
		}
		if (w >= 1.0 && w <= PAGE_SIZE_MAX && l + r <= w - 1.0 &&
			h >= 1.0 && h <= PAGE_SIZE_MAX && t + b <= h - 1.0) {
			cd_page_size_dialog().IsPrimaryButtonEnabled(true);
		}
		else {
			cd_page_size_dialog().IsPrimaryButtonEnabled(false);
		}
	}

	// ページの大きさダイアログのコンボボックスの選択が変更された.
	void MainPage::page_size_selection_changed(IInspectable const&, SelectionChangedEventArgs const& args) noexcept
	{
		LEN_UNIT old_unit = LEN_UNIT::PIXEL;
		for (const auto i : args.RemovedItems()) {
			if (i == cbi_len_unit_grid()) {
				old_unit = LEN_UNIT::GRID;
			}
			else if (i == cbi_len_unit_inch()) {
				old_unit = LEN_UNIT::INCH;
			}
			else if (i == cbi_len_unit_milli()) {
				old_unit = LEN_UNIT::MILLI;
			}
			else if (i == cbi_len_unit_pixel()) {
				old_unit = LEN_UNIT::PIXEL;
			}
			else if (i == cbi_len_unit_point()) {
				old_unit = LEN_UNIT::POINT;
			}
			else {
				return;
			}
		}
		LEN_UNIT new_unit = LEN_UNIT::PIXEL;
		for (const auto i : args.AddedItems()) {
			if (i == cbi_len_unit_grid()) {
				new_unit = LEN_UNIT::GRID;
			}
			else if (i == cbi_len_unit_inch()) {
				new_unit = LEN_UNIT::INCH;
			}
			else if (i == cbi_len_unit_milli()) {
				new_unit = LEN_UNIT::MILLI;
			}
			else if (i == cbi_len_unit_pixel()) {
				new_unit = LEN_UNIT::PIXEL;
			}
			else if (i == cbi_len_unit_point()) {
				new_unit = LEN_UNIT::POINT;
			}
			else {
				return;
			}
		}
		if (old_unit != new_unit) {
			const double dpi = m_main_d2d.m_logical_dpi;
			const double g_len = m_main_page.m_grid_base + 1.0;
			double val;
			if (swscanf_s(tx_page_size_width().Text().data(), L"%lf", &val)) {
				wchar_t buf[128];
				val = conv_len_to_pixel(old_unit, val, dpi, g_len);
				conv_len_to_str<false>(new_unit, val, dpi, g_len, buf);
				tx_page_size_width().Text(buf);
			}
			if (swscanf_s(tx_page_size_height().Text().data(), L"%lf", &val)) {
				wchar_t buf[128];
				val = conv_len_to_pixel(old_unit, val, dpi, g_len);
				conv_len_to_str<false>(new_unit, val, dpi, g_len, buf);
				tx_page_size_height().Text(buf);
			}
			if (swscanf_s(tx_page_margin_left().Text().data(), L"%lf", &val)) {
				wchar_t buf[128];
				val = conv_len_to_pixel(old_unit, val, dpi, g_len);
				conv_len_to_str<false>(new_unit, val, dpi, g_len, buf);
				tx_page_margin_left().Text(buf);
			}
			if (swscanf_s(tx_page_margin_top().Text().data(), L"%lf", &val)) {
				wchar_t buf[128];
				val = conv_len_to_pixel(old_unit, val, dpi, g_len);
				conv_len_to_str<false>(new_unit, val, dpi, g_len, buf);
				tx_page_margin_top().Text(buf);
			}
			if (swscanf_s(tx_page_margin_right().Text().data(), L"%lf", &val)) {
				wchar_t buf[128];
				val = conv_len_to_pixel(old_unit, val, dpi, g_len);
				conv_len_to_str<false>(new_unit, val, dpi, g_len, buf);
				tx_page_margin_right().Text(buf);
			}
			if (swscanf_s(tx_page_margin_bottom().Text().data(), L"%lf", &val)) {
				wchar_t buf[128];
				val = conv_len_to_pixel(old_unit, val, dpi, g_len);
				conv_len_to_str<false>(new_unit, val, dpi, g_len, buf);
				tx_page_margin_bottom().Text(buf);
			}
		}
	}

	// レイアウトメニューの「ページのズーム」のサブ項目に印をつける.
	void MainPage::page_zoom_is_checked(float scale)
	{
		rmfi_menu_page_zoom_100().IsChecked(equal(scale, 1.0f));
		rmfi_popup_page_zoom_100().IsChecked(equal(scale, 1.0f));
		rmfi_menu_page_zoom_150().IsChecked(equal(scale, 1.5f));
		rmfi_popup_page_zoom_150().IsChecked(equal(scale, 1.5f));
		rmfi_menu_page_zoom_200().IsChecked(equal(scale, 2.0f));
		rmfi_popup_page_zoom_200().IsChecked(equal(scale, 2.0f));
		rmfi_menu_page_zoom_300().IsChecked(equal(scale, 3.0f));
		rmfi_popup_page_zoom_300().IsChecked(equal(scale, 3.0f));
		rmfi_menu_page_zoom_400().IsChecked(equal(scale, 4.0f));
		rmfi_popup_page_zoom_400().IsChecked(equal(scale, 4.0f));
		rmfi_menu_page_zoom_075().IsChecked(equal(scale, 0.75f));
		rmfi_popup_page_zoom_075().IsChecked(equal(scale, 0.75f));
		rmfi_menu_page_zoom_050().IsChecked(equal(scale, 0.5f));
		rmfi_popup_page_zoom_050().IsChecked(equal(scale, 0.5f));
		rmfi_menu_page_zoom_025().IsChecked(equal(scale, 0.25f));
		rmfi_popup_page_zoom_025().IsChecked(equal(scale, 0.25f));
	}

	// レイアウトメニューの「ページのズーム」が選択された.
	void MainPage::page_zoom_click(IInspectable const& sender, RoutedEventArgs const&)
	{
		float scale;
		if (sender == rmfi_menu_page_zoom_100() || sender == rmfi_popup_page_zoom_100()) {
			scale = 1.0f;
		}
		else if (sender == rmfi_menu_page_zoom_150() || sender == rmfi_popup_page_zoom_150()) {
			scale = 1.5f;
		}
		else if (sender == rmfi_menu_page_zoom_200() || sender == rmfi_popup_page_zoom_200()) {
			scale = 2.0f;
		}
		else if (sender == rmfi_menu_page_zoom_300() || sender == rmfi_popup_page_zoom_300()) {
			scale = 3.0f;
		}
		else if (sender == rmfi_menu_page_zoom_400() || sender == rmfi_popup_page_zoom_400()) {
			scale = 4.0f;
		}
		else if (sender == rmfi_menu_page_zoom_075() || sender == rmfi_popup_page_zoom_075()) {
			scale = 0.75f;
		}
		else if (sender == rmfi_menu_page_zoom_050() || sender == rmfi_popup_page_zoom_050()) {
			scale = 0.5f;
		}
		else if (sender == rmfi_menu_page_zoom_025() || sender == rmfi_popup_page_zoom_025()) {
			scale = 0.25f;
		}
		else {
			return;
		}
		page_zoom_is_checked(scale);
		//if (scale != m_main_page.m_page_scale) {
		//	m_main_page.m_page_scale = scale;
		if (scale != m_main_scale) {
			m_main_scale = scale;
			main_panel_size();
			main_draw();
			status_bar_set_zoom();
		}
		status_bar_set_pos();
	}
}