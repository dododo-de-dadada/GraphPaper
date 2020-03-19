//-------------------------------
// MainPage.cpp
// メインページの作成と, ファイルメニューの「新規」と「終了」
//-------------------------------
#include "pch.h"
#include "MainPage.h"
#include "MainPage.g.cpp"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	// 長さををピクセル単位の値に変換する.
	// unit	長さの単位
	// value	長さの値
	// dpi	DPI
	// g_len	方眼の長さ
	// 戻り値	ピクセル単位の値
	double conv_len_to_val(const LEN_UNIT unit, const double value, const double dpi, const double g_len) noexcept
	{
		double ret;

		if (unit == LEN_UNIT::INCH) {
			ret = value * dpi;
		}
		else if (unit == LEN_UNIT::MILLI) {
			ret = value * dpi / MM_PER_INCH;
		}
		else if (unit == LEN_UNIT::POINT) {
			ret = value * dpi / PT_PER_INCH;
		}
		else if (unit == LEN_UNIT::GRID) {
			ret = value * g_len;
		}
		else {
			ret = value;
		}
		return std::round(2.0 * ret) * 0.5;
	}

	// 色成分の値を文字列に変換する.
	// style	色成分の形式
	// value	色成分の値
	// buf	文字列の配列
	// len	文字列の最大長 ('\0' を含む長さ)
	// 戻り値	なし
	void conv_val_to_col(const COLOR_CODE style, const double value, wchar_t* buf, const size_t b_len)
	{
		if (style == COLOR_CODE::DEC) {
			swprintf_s(buf, b_len, L"%.0lf", std::round(value));
		}
		else if (style == COLOR_CODE::HEX) {
			swprintf_s(buf, b_len, L"%02X", static_cast<uint32_t>(std::round(value)));
		}
		else if (style == COLOR_CODE::REAL) {
			swprintf_s(buf, b_len, L"%.4lf", value / COLOR_MAX);
		}
		else if (style == COLOR_CODE::CENT) {
			swprintf_s(buf, b_len, L"%.1lf%%", value / COLOR_MAX * 100.0);
		}
		else {
			throw hresult_not_implemented();
		}
	}

	// ピクセル単位の長さを他の単位の文字列に変換する.
	// B	単位付加フラグ
	// unit	長さの単位
	// val	ピクセル単位の長さ
	// dpi	DPI
	// g_len	方眼の大きさ
	// buf	文字列の配列
	// b_len	文字列の最大長 ('\0' を含む長さ)
	template <bool B>
	void conv_val_to_len(const LEN_UNIT unit, const double value, const double dpi, const double g_len, wchar_t *buf, const uint32_t b_len)
	{
		if (unit == LEN_UNIT::PIXEL) {
			if constexpr (B == WITH_UNIT_NAME) {
				swprintf_s(buf, b_len, FMT_PIXEL_UNIT, value);
			}
			else {
				swprintf_s(buf, b_len, FMT_PIXEL, value);
			}
		}
		else if (unit == LEN_UNIT::INCH) {
			if constexpr (B == WITH_UNIT_NAME) {
				swprintf_s(buf, b_len, FMT_INCH_UNIT, value / dpi);
			}
			else {
				swprintf_s(buf, b_len, FMT_INCH, value / dpi);
			}
		}
		else if (unit == LEN_UNIT::MILLI) {
			if constexpr (B == WITH_UNIT_NAME) {
				swprintf_s(buf, b_len, FMT_MILLI_UNIT, value * MM_PER_INCH / dpi);
			}
			else {
				swprintf_s(buf, b_len, FMT_MILLI, value * MM_PER_INCH / dpi);
			}
		}
		else if (unit == LEN_UNIT::POINT) {
			if constexpr (B == WITH_UNIT_NAME) {
				swprintf_s(buf, b_len, FMT_POINT_UNIT, value * PT_PER_INCH / dpi);
			}
			else {
				swprintf_s(buf, b_len, FMT_POINT, value * PT_PER_INCH / dpi);
			}
		}
		else if (unit == LEN_UNIT::GRID) {
			if constexpr (B == WITH_UNIT_NAME) {
				swprintf_s(buf, b_len, FMT_GRID_UNIT, value / g_len);
			}
			else {
				swprintf_s(buf, b_len, FMT_GRID, value / g_len);
			}
		}
	}
	template void conv_val_to_len<!WITH_UNIT_NAME>(const LEN_UNIT unit, const double value, const double dpi, const double g_len, wchar_t* buf, const uint32_t b_len);
	template void conv_val_to_len<WITH_UNIT_NAME>(const LEN_UNIT unit, const double value, const double dpi, const double g_len, wchar_t* buf, const uint32_t b_len);

	// メッセージダイアログを表示する.
	// glyph_key	フォントアイコンのグリフの静的リソースのキー
	// message_key	メッセージのアプリケーションリソースのキー
	// desc_key		説明文のアプリケーションリソースのキー
	// 戻り値	なし
	void MainPage::cd_message_show(winrt::hstring const& glyph_key, winrt::hstring const& message_key, winrt::hstring const& desc_key)
	{
		using winrt::Windows::UI::Xaml::Controls::ContentDialog;
		using winrt::Windows::ApplicationModel::Resources::ResourceLoader;
		using winrt::Windows::UI::Xaml::Controls::ContentDialogButton;
		const wchar_t QUOT[] = L"\"";	// 引用符
		const wchar_t NL[] = L"\u2028";	// テキストブロック内での改行

		auto const& r_loader = ResourceLoader::GetForCurrentView();
		// メッセージをキーとする文字列をリソースローダーから得る.
		winrt::hstring text;
		try {
			text = r_loader.GetString(message_key);
		}
		catch (winrt::hresult_error const&) {}
		if (text.empty()) {
			// 文字列が空の場合,
			text = message_key;
		}
		// 説明をキーとする, 追加する文字列をリソースローダーから得る.
		winrt::hstring added_text;
		try {
			added_text = r_loader.GetString(desc_key);
		}
		catch (winrt::hresult_error const&) {}
		if (added_text.empty() == false) {
			// 追加する文字列が空でない場合,
			text = text + NL + added_text;
		}
		else if (desc_key.empty() == false) {
			// 説明そのものが空でない場合,
			text = text + NL + QUOT + desc_key + QUOT;
		}
		// グリフをキーとして, フォントアイコンのグリフを静的リソースから得る.
		auto glyph = Resources().TryLookup(box_value(glyph_key));
		fi_message().Glyph(glyph != nullptr ? unbox_value<winrt::hstring>(glyph) : glyph_key);
		tk_message().Text(text);
		auto _{ cd_message().ShowAsync() };
	}

	// 編集メニュー項目の使用の可否を設定する.
	// 選択の有無や型ごとに図形を数え,
	// それらによって, メニュー項目の可否を判定する.
	void MainPage::enable_edit_menu(void)
	{
		using winrt::Windows::ApplicationModel::DataTransfer::StandardDataFormats;

		// 元に戻す/やり直しメニュー項目の使用の可否を設定する.
		enable_undo_menu();

		uint32_t undeleted_cnt = 0;	// 消去フラグがない図形の数
		uint32_t selected_cnt = 0;	// 選択された図形の数
		uint32_t selected_group_cnt = 0;	// 選択されたグループ図形の数
		uint32_t runlength_cnt = 0;	// 選択された図形のランレングスの数
		uint32_t selected_text_cnt = 0;	// 選択された文字列図形の数
		uint32_t text_cnt = 0;	// 文字列図形の数
		//Shape* prev_shape = nullptr;	// ひとつ背面の図形
		bool fore_selected = false;	// 最前面の図形の選択フラグ
		bool back_selected = false;	// 最背面の図形の選択フラグ
		bool prev_selected = false;	// ひとつ背面の図形の選択フラグ

		// 図形リストの各図形について以下を繰り返す.
		for (auto s : m_list_shapes) {
			if (s->is_deleted()) {
				// 図形の消去フラグが立っている場合,
				// 以下を無視する.
				continue;
			}
			// 消去フラグがない図形の数をインクリメントする.
			undeleted_cnt++;
			// 図形の動的な型を得る.
			auto const& s_type = typeid(*s);
			if (s_type == typeid(ShapeText)) {
				// 型が文字列図形の場合,
				// 文字列図形の数をインクリメントする.
				text_cnt++;
			}
			// 図形の選択フラグを最前面の図形の選択フラグに格納する.
			fore_selected = s->is_selected();
			if (fore_selected) {
				// 最前面の図形の選択フラグが立っている場合,
				// 選択された図形の数をインクリメントする.
				selected_cnt++;
				if (undeleted_cnt == 1) {
					// 消去フラグがない図形の数が 1 の場合,
					// 最前面の図形の選択フラグを立てる.
					back_selected = true;
				}
				if (s_type == typeid(ShapeGroup)) {
					// 図形の動的な型がグループ図形の場合,
					// 選択されたグループ図形の数をインクリメントする.
					selected_group_cnt++;
				}
				else if (s_type == typeid(ShapeText)) {
					// 図形の動的な型が文字列図形の場合,
					// 選択された文字列図形の数をインクリメントする.
					selected_text_cnt++;
				}
				if (/*prev_shape == nullptr ||*/ prev_selected == false) {
					// ひとつ背面の図形がヌル
					// またはひとつ背面の図形の選択フラグがない場合,
					// 選択された図形のランレングスの数をインクリメントする.
					runlength_cnt++;
				}
			}
			//prev_shape = s;
			prev_selected = fore_selected;
		}
		// 消去されていない図形がひとつ以上ある場合.
		const auto exists_undeleted = (undeleted_cnt > 0);
		// 選択された図形がひとつ以上ある場合.
		const auto exsits_selected = (selected_cnt > 0);
		// 選択された文字列図形がひとつ以上ある場合.
		const auto exsits_selected_text = (selected_text_cnt > 0);
		// 文字列図形がひとつ以上ある場合.
		const auto exsits_text = (text_cnt > 0);
		// 選択されてない図形がひとつ以上ある場合.
		const auto exits_unselected = (selected_cnt < undeleted_cnt);
		// 選択された図形がふたつ以上ある場合.
		const auto exsits_selected_2 = (selected_cnt > 1);
		// 選択されたグループ図形がひとつ以上ある場合.
		const auto exsits_selected_group = (selected_group_cnt > 0);
		// 前面に配置可能か調べる.
		// 1. 複数のランレングスがある.
		// 2. または, 少なくとも 1 つは選択された図形があり, 
		//    かつ最前面の図形は選択されいない.
		const auto enable_forward = (runlength_cnt > 1 || (exsits_selected && fore_selected == false));
		// 背面に配置可能か調べる.
		// 1. 複数のランレングスがある.
		// 2. または, 少なくとも 1 つは選択された図形があり, 
		//    かつ最背面の図形は選択されいない.
		const auto enable_backward = (runlength_cnt > 1 || (exsits_selected && back_selected == false));

		mfi_xcvd_cut().IsEnabled(exsits_selected);
		mfi_xcvd_copy().IsEnabled(exsits_selected);
		mfi_xcvd_paste().IsEnabled(xcvd_contains({ CF_GPD, StandardDataFormats::Text() }));
		mfi_xcvd_delete().IsEnabled(exsits_selected);
		mfi_select_all().IsEnabled(exits_unselected);
		mfi_group().IsEnabled(exsits_selected_2);
		mfi_ungroup().IsEnabled(exsits_selected_group);
		mfi_text_edit().IsEnabled(exsits_selected_text);
		mfi_text_find().IsEnabled(exsits_text);
		mfi_bring_forward().IsEnabled(enable_forward);
		mfi_bring_to_front().IsEnabled(enable_forward);
		mfi_send_to_back().IsEnabled(enable_backward);
		mfi_send_backward().IsEnabled(enable_backward);
		mfi_summary().IsEnabled(exists_undeleted);
		m_list_selected = selected_cnt;
	}

	// メインページを作成する.
	MainPage::MainPage(void)
	{
		// お約束.
		InitializeComponent();

		// コンテキストメニューを静的リソースから読み込む.
		// ポップアップは静的なリソースとして定義して、複数の要素で使用することができる.
		{
			m_menu_stroke = unbox_value<MenuFlyout>(Resources().Lookup(box_value(L"mf_stroke")));
			m_menu_fill = unbox_value<MenuFlyout>(Resources().Lookup(box_value(L"mf_fill")));
			m_menu_font = unbox_value<MenuFlyout>(Resources().Lookup(box_value(L"mf_font")));
			m_menu_layout = unbox_value<MenuFlyout>(Resources().Lookup(box_value(L"mf_layout")));
			m_menu_ungroup = unbox_value<MenuFlyout>(Resources().Lookup(box_value(L"mf_ungroup")));
		}

		// アプリケーションの中断・継続などのイベントハンドラーを設定する.
		{
			using winrt::Windows::UI::Xaml::Application;

			auto const& app{ Application::Current() };
			m_token_suspending = app.Suspending({ this, &MainPage::app_suspending });
			m_token_resuming = app.Resuming({ this, &MainPage::app_resuming });
			m_token_entered_background = app.EnteredBackground({ this, &MainPage::app_entered_background });
			m_token_leaving_background = app.LeavingBackground({ this, &MainPage::app_leaving_background });
		}

		// ウィンドウの表示が変わったときのイベントハンドラーを設定する.
		{
			auto const& win{ CoreWindow::GetForCurrentThread() };
			m_token_activated = win.Activated({ this, &MainPage::thread_activated });
			m_token_visibility_changed = win.VisibilityChanged({ this, &MainPage::thread_visibility_changed });
		}

		// ディスプレイの状態が変わったときのイベントハンドラーを設定する.
		{
			auto const& disp{ DisplayInformation::GetForCurrentView() };
			m_token_dpi_changed = disp.DpiChanged({ this, &MainPage::disp_dpi_changed });
			m_token_orientation_changed = disp.OrientationChanged({ this, &MainPage::disp_orientation_changed });
			m_token_contents_invalidated = disp.DisplayContentsInvalidated({ this, &MainPage::disp_contents_invalidated });
		}

		// アプリケーションを閉じる前の確認のハンドラーを設定する.
		{
			using winrt::Windows::UI::Core::Preview::SystemNavigationManagerPreview;
			m_token_close_requested = SystemNavigationManagerPreview::GetForCurrentView().CloseRequested({ this, &MainPage::navi_close_requested });
		}

		// アプリケーションが受け入れ可能なフォーマット ID をクリップボードに設定する.
		{
			using winrt::Windows::ApplicationModel::DataTransfer::StandardDataFormats;
			using winrt::Windows::ApplicationModel::DataTransfer::Clipboard;

			auto const& dp_view = Clipboard::GetContent();
			dp_view.SetAcceptedFormatId(CF_GPD);
			dp_view.SetAcceptedFormatId(StandardDataFormats::Text());
		}

		// D2D/DWRITE ファクトリを図形/文字列図形クラスに, 
		// 図形リストとページレイアウトを操作クラスに格納する.
		{
			Shape::s_d2d_factory = m_page_dx.m_d2dFactory.get();
			Shape::s_dwrite_factory = m_page_dx.m_dwriteFactory.get();
			Undo::set(&m_list_shapes, &m_page_layout);
		}

		// クリックの判定時間をシステムから得る.
		using winrt::Windows::UI::ViewManagement::UISettings;
		m_click_time = static_cast<uint64_t>(UISettings().DoubleClickTime()) * 1000L;
		// クリックの判定距離を物理 DPI に合わせる.
		auto const raw_dpi = DisplayInformation::GetForCurrentView().RawDpiX();
		auto const log_dpi = DisplayInformation::GetForCurrentView().LogicalDpi();
		m_click_dist = 6.0 * raw_dpi / log_dpi;

		auto _{ mfi_new_click(nullptr, nullptr) };
	}

	// ファイルメニューの「終了」が選択された
	IAsyncAction MainPage::mfi_exit_click(IInspectable const&, RoutedEventArgs const&)
	{
		using winrt::Windows::UI::Xaml::Application;
		using winrt::Windows::UI::Xaml::Controls::ContentDialogResult;

		if (m_stack_push) {
			// 操作スタックの更新フラグが立っている場合,
			const auto d_result = co_await cd_conf_save().ShowAsync();
			if (d_result == ContentDialogResult::None) {
				// 「キャンセル」が押された場合,
				co_return;
			}
			else if (d_result == ContentDialogResult::Primary) {
				// 「保存する」が押された場合,
				if (co_await file_save_async() != S_OK) {
					// 保存できなかった場合,
					co_return;
				}
			}
		}
		// 上記以外の場合,
		if (m_summary_visible) {
			summary_close();
		}
		undo_clear();
		s_list_clear(m_list_shapes);
#if defined(_DEBUG)
		if (debug_leak_cnt != 0) {
			cd_message_show(ICON_ALERT, L"Memory leak occurs", {});
		}
#endif
		ShapeText::release_available_fonts();
		m_page_dx.Release();
		m_sample_dx.Release();

		// 静的リソースから読み込んだコンテキストメニューを破棄する.
		{
			m_menu_stroke = nullptr;
			m_menu_fill = nullptr;
			m_menu_font = nullptr;
			m_menu_layout = nullptr;
			m_menu_ungroup = nullptr;
		}

		// コードビハインドで設定したハンドラーの設定を解除する.
		{
			using winrt::Windows::UI::Xaml::Application;
			auto const& app{ Application::Current() };
			app.Suspending(m_token_suspending);
			app.Resuming(m_token_resuming);
			app.EnteredBackground(m_token_entered_background);
			app.LeavingBackground(m_token_leaving_background);
			auto const& thread{ CoreWindow::GetForCurrentThread() };
			thread.Activated(m_token_activated);
			thread.VisibilityChanged(m_token_visibility_changed);
			auto const& disp{ DisplayInformation::GetForCurrentView() };
			disp.DpiChanged(m_token_dpi_changed);
			disp.OrientationChanged(m_token_orientation_changed);
			disp.DisplayContentsInvalidated(m_token_contents_invalidated);
			using winrt::Windows::UI::Core::Preview::SystemNavigationManagerPreview;
			SystemNavigationManagerPreview::GetForCurrentView().CloseRequested(m_token_close_requested);
		}

		// アプリケーションを終了する.
		Application::Current().Exit();
	}

	// ファイルメニューの「新規」が選択された
	IAsyncAction MainPage::mfi_new_click(IInspectable const&, RoutedEventArgs const&)
	{
		using winrt::Windows::UI::Xaml::Controls::ContentDialogResult;

		if (m_stack_push) {
			// 操作スタックの更新フラグが立っている場合,
			const auto d_result = co_await cd_conf_save().ShowAsync();
			if (d_result == ContentDialogResult::None) {
				// 「キャンセル」が押された場合,
				co_return;
			}
			else if (d_result == ContentDialogResult::Primary) {
				// 「保存する」が押された場合,
				if (co_await file_save_async() != S_OK) {
					// 保存できなかった場合,
					co_return;
				}
			}
		}
		if (m_summary_visible) {
			// 図形一覧パネルの表示フラグが立っている場合,
			summary_close();
		}
		undo_clear();
		s_list_clear(m_list_shapes);
#if defined(_DEBUG)
		if (debug_leak_cnt != 0) {
			// 「メモリリーク」メッセージダイアログを表示する.
			cd_message_show(ICON_ALERT, L"Memory leak occurs", {});
		}
#endif
		ShapeText::release_available_fonts();
		ShapeText::set_available_fonts();

		if (co_await layout_load_async() != S_OK) {
			// レイアウトの読み込みに失敗した場合,
			// 書体の属性を初期化する.
			{
				using winrt::Windows::UI::Xaml::Setter;
				using winrt::Windows::UI::Xaml::Controls::TextBlock;
				using winrt::Windows::UI::Xaml::Media::FontFamily;
				using winrt::Windows::UI::Text::FontWeight;
				using winrt::Windows::UI::Text::FontStretch;
				using winrt::Windows::UI::Xaml::Style;

				// リソースの取得に失敗した場合に備えて,
				// 固定の既定値を書体属性に格納する.
				m_page_layout.m_font_family = wchar_cpy(L"Segoe UI");
				m_page_layout.m_font_size = 14.0;
				m_page_layout.m_font_stretch = DWRITE_FONT_STRETCH::DWRITE_FONT_STRETCH_NORMAL;
				m_page_layout.m_font_style = DWRITE_FONT_STYLE::DWRITE_FONT_STYLE_NORMAL;
				m_page_layout.m_font_weight = DWRITE_FONT_WEIGHT::DWRITE_FONT_WEIGHT_NORMAL;
				// BodyTextBlockStyle をリソースディクショナリから得る.
				auto resource = Resources().TryLookup(box_value(L"BodyTextBlockStyle"));
				if (resource != nullptr) {
					auto style = resource.try_as<Style>();
					std::list<Style> stack;
					// リソースからスタイルを得る.
					while (style != nullptr) {
						// スタイルが空でない場合.
						// スタイルをスタックに積む.
						stack.push_back(style);
						// スタイルの継承元のスタイルを得る.
						style = style.BasedOn();
					}
					try {
						while (stack.empty() == false) {
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
									auto value = unbox_value<FontFamily>(setter.Value());
									m_page_layout.m_font_family = wchar_cpy(value.Source().c_str());
								}
								else if (prop == TextBlock::FontSizeProperty()) {
									// プロパティーが FontSize の場合,
									// セッターの値から, 書体の大きさを得る.
									auto value = unbox_value<double>(setter.Value());
									m_page_layout.m_font_size = value;
								}
								else if (prop == TextBlock::FontStretchProperty()) {
									// プロパティーが FontStretch の場合,
									// セッターの値から, 書体の伸縮を得る.
									auto value = unbox_value<int32_t>(setter.Value());
									m_page_layout.m_font_stretch = static_cast<DWRITE_FONT_STRETCH>(value);
								}
								else if (prop == TextBlock::FontStyleProperty()) {
									// プロパティーが FontStyle の場合,
									// セッターの値から, 字体を得る.
									auto value = unbox_value<int32_t>(setter.Value());
									m_page_layout.m_font_style = static_cast<DWRITE_FONT_STYLE>(value);
								}
								else if (prop == TextBlock::FontWeightProperty()) {
									// プロパティーが FontWeight の場合,
									// セッターの値から, 書体の太さを得る.
									auto value = unbox_value<int32_t>(setter.Value());
									m_page_layout.m_font_weight = static_cast<DWRITE_FONT_WEIGHT>(value);
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
				ShapeText::is_available_font(m_page_layout.m_font_family);
			}

			{
				m_page_layout.m_arrow_size = ARROW_SIZE();
				m_page_layout.m_arrow_style = ARROW_STYLE::NONE;
				m_page_layout.m_corner_rad = { GRIDLEN_PX, GRIDLEN_PX };
				m_page_layout.set_fill_color(m_page_dx.m_color_bkg);
				m_page_layout.set_font_color(m_page_dx.m_color_frg);
				m_page_layout.m_grid_base = static_cast<double>(GRIDLEN_PX) - 1.0;
				m_page_layout.m_grid_gray = GRID_GRAY;
				m_page_layout.m_grid_show = GRID_SHOW::BACK;
				m_page_layout.m_grid_snap = true;
				m_page_layout.set_page_color(m_page_dx.m_color_bkg);
				m_page_layout.m_page_scale = 1.0;
				const double dpi = DisplayInformation::GetForCurrentView().LogicalDpi();
				m_page_layout.m_page_size.width = static_cast<FLOAT>(std::floor(A4_PER_INCH.width * dpi));
				m_page_layout.m_page_size.height = static_cast<FLOAT>(std::floor(A4_PER_INCH.height * dpi));
				m_page_layout.set_stroke_color(m_page_dx.m_color_frg);
				m_page_layout.m_stroke_patt = STROKE_PATT();
				m_page_layout.m_stroke_style = D2D1_DASH_STYLE::D2D1_DASH_STYLE_SOLID;
				m_page_layout.m_stroke_width = 1.0F;
				m_page_layout.m_text_align_p = DWRITE_PARAGRAPH_ALIGNMENT::DWRITE_PARAGRAPH_ALIGNMENT_NEAR;
				m_page_layout.m_text_align_t = DWRITE_TEXT_ALIGNMENT::DWRITE_TEXT_ALIGNMENT_LEADING;
				m_page_layout.m_text_line = 0.0F;
				m_page_layout.m_text_margin = { 4.0F, 4.0F };
			}
			m_page_unit = LEN_UNIT::PIXEL;
			m_color_fmt = COLOR_CODE::DEC;
			m_status_bar = status_or(STATUS_BAR::CURS, STATUS_BAR::ZOOM);

		}
		// 背景色, 前景色, 文字範囲の背景色, 文字範囲の文字色をリソースから得る.
		{
			using winrt::Windows::UI::Color;
			using winrt::Windows::UI::Xaml::Media::Brush;

			m_page_dx.m_range_bcolor = { 0.0f, 1.0f / 3.0f, 2.0f / 3.0f, 1.0f };
			m_page_dx.m_range_tcolor = S_WHITE;
			m_page_dx.m_color_bkg = S_WHITE;
			m_page_dx.m_color_frg = S_BLACK;
			try {
				auto h_color = Resources().Lookup(box_value(L"SystemColorHighlightColor"));
				auto t_color = Resources().Lookup(box_value(L"SystemColorHighlightTextColor"));
				auto const& b_theme = Resources().Lookup(box_value(L"ApplicationPageBackgroundThemeBrush"));
				auto const& f_theme = Resources().Lookup(box_value(L"ApplicationForegroundThemeBrush"));
				cast_to(unbox_value<Color>(h_color), m_page_dx.m_range_bcolor);
				cast_to(unbox_value<Color>(t_color), m_page_dx.m_range_tcolor);
				cast_to(unbox_value<Brush>(b_theme), m_page_dx.m_color_bkg);
				cast_to(unbox_value<Brush>(f_theme), m_page_dx.m_color_frg);
			}
			catch (winrt::hresult_error) {
			}
			m_sample_dx.m_range_bcolor = m_page_dx.m_range_bcolor;
			m_sample_dx.m_range_tcolor = m_page_dx.m_range_tcolor;
			m_sample_dx.m_color_bkg = m_page_dx.m_color_bkg;
			m_sample_dx.m_color_frg = m_page_dx.m_color_frg;
		}
		{
			m_page_min.x = 0.0;
			m_page_min.y = 0.0;
			m_page_max.x = m_page_layout.m_page_size.width;
			m_page_max.y = m_page_layout.m_page_size.height;
		}
		file_recent_add(nullptr);
		finish_file_read();
	}

}
