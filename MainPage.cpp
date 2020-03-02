//-------------------------------
// MainPage.cpp
// メインページの作成と表示
//-------------------------------
#include "pch.h"
#include "MainPage.h"
#include "MainPage.g.cpp"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	//	色成分の値を文字列に変換する.
	//	style	色成分の形式
	//	val	色成分の値
	//	buf	文字列の配列
	//	len	文字列の最大長 ('\0' を含む長さ)
	void conv_val_to_col(const COL_STYLE style, const double val, wchar_t* buf, const uint32_t len)
	{
		if (style == COL_STYLE::DEC) {
			swprintf_s(buf, len, L"%.0lf", std::round(val));
		}
		else if (style == COL_STYLE::HEX) {
			swprintf_s(buf, len, L"%02X", static_cast<uint32_t>(std::round(val)));
		}
		else if (style == COL_STYLE::FLT) {
			swprintf_s(buf, len, L"%.4lf", val / COLOR_MAX);
		}
		else if (style == COL_STYLE::CEN) {
			swprintf_s(buf, len, L"%.1lf%%", val / COLOR_MAX * 100.0);
		}
	}

	//	ピクセル単位の長さを他の単位の文字列に変換する.
	//	unit	長さの単位
	//	val	ピクセル単位の長さ
	//	dpi	DPI
	//	g_len	方眼の大きさ
	//	buf	文字列の配列
	//	b_len	文字列の最大長 ('\0' を含む長さ)
	void conv_val_to_len(const LEN_UNIT unit, const double val, const double dpi, const double g_len, wchar_t* buf, const uint32_t b_len)
	{
		if (unit == LEN_UNIT::PIXEL) {
			swprintf_s(buf, b_len, FMT_PIXEL_UNIT, val);
		}
		else if (unit == LEN_UNIT::INCH) {
			swprintf_s(buf, b_len, FMT_INCH_UNIT, val / dpi);
		}
		else if (unit == LEN_UNIT::MILLI) {
			swprintf_s(buf, b_len, FMT_MILLI_UNIT, val * MM_PER_INCH / dpi);
		}
		else if (unit == LEN_UNIT::POINT) {
			swprintf_s(buf, b_len, FMT_POINT_UNIT, val * PT_PER_INCH / dpi);
		}
		else if (unit == LEN_UNIT::GRID) {
			swprintf_s(buf, b_len, FMT_GRID_UNIT, val / g_len);
		}
	}

	//	メインページを破棄する.
	MainPage::~MainPage(void)
	{
		//	メインページの内容を破棄する.
		release();
	}

	//	メッセージダイアログを表示する.
	//	msg	メッセージ
	//	desc	説明文
	//	戻り値	なし
	void MainPage::cd_message_show(winrt::hstring const& msg, winrt::hstring const& desc)
	{
		using winrt::Windows::UI::Xaml::Controls::ContentDialog;
		using winrt::Windows::ApplicationModel::Resources::ResourceLoader;
		using winrt::Windows::UI::Xaml::Controls::ContentDialogButton;
		const wchar_t QUOT[] = L"\"";	// 引用符
		const wchar_t NL[] = L"\u2028";	// テキストブロック内での改行
		const wchar_t CLOSE[] = L"str_close";	// 「閉じる」文字列のリソース名

		//	コンテキストダイアログを作成する.
		auto dialog = ContentDialog();
		//	リソースローダーを得る.
		auto const& r_loader = ResourceLoader::GetForCurrentView();
		//	表示する文字列をリソースローダーから得る.
		winrt::hstring cont;
		try {
			cont = r_loader.GetString(msg);
		}
		catch (winrt::hresult_error const&) {}
		if (cont.empty()) {
			//	文字列が空の場合,
			//	リソース名をそのまま表示する文字列に格納する.
			cont = msg;
		}
		//	説明文をリソースローダーから得る.
		winrt::hstring b;
		try {
			b = r_loader.GetString(desc);
		}
		catch (winrt::hresult_error const&) {}
		if (b.empty()) {
			if (desc.empty() == false) {
				//	説明文が空でない場合,
				//	改行と, 引用符で囲んだ説明文を表示する文字列に加える.
				cont = cont + NL + QUOT + desc + QUOT;
			}
		}
		else {
			cont = cont + NL + b;
		}
		//	「閉じる」文字列をリソースローダーから得る.
		const auto close = r_loader.GetString(CLOSE);
		//	表示する文字列をダイアログの内容に格納する.
		dialog.Content(box_value(cont));
		//	ダイアログのクローズボタンに「閉じる」文字列を格納する.
		dialog.CloseButtonText(close);
		//	クローズボタンを既定のボタンに設定する.
		dialog.DefaultButton(ContentDialogButton::Close);
		//	ダイアログを非同期に表示する.
		auto _{ dialog.ShowAsync() };
	}

	//	メッセージダイアログが閉じた.
	void MainPage::cd_message_closed(ContentDialog const& sender, ContentDialogClosedEventArgs const& /*args*/)
	{
		//	ダイアログを解放する.
		UnloadObject(sender);
	}

	// 編集メニュー項目の使用の可否を設定する.
	void MainPage::enable_edit_menu(void)
	{
		//	選択の有無や動的な型ごとに図形を数え,
		//	それらによって, メニュー項目の可否を判定する.
		uint32_t cnt = 0;	// 消去フラグがない図形の数
		uint32_t sel = 0;	// 選択された図形の数
		uint32_t grp = 0;	// 選択されたグループ図形の数
		uint32_t run_len = 0;	// 選択された図形のランレングスの数
		uint32_t t_sel = 0;	// 選択された文字列図形の数
		uint32_t t_cnt = 0;	// 文字列図形の数
		Shape* p = nullptr;	// ひとつ前の図形
		bool fore_sel = false;	// 最前面の図形の選択フラグ
		bool back_sel = false;	// 最背面の図形の選択フラグ
		bool prev_sel = false;	// ひとつ前の図形の選択

		for (auto s : m_list_shapes) {
			if (s->is_deleted()) {
				//	消去フラグが立っている場合,
				//	継続する.
				continue;
			}
			//	消去フラグがない図形の数をインクリメントする.
			cnt++;
			//	図形の動的な型を得る.
			auto const& s_type = typeid(*s);
			if (s_type == typeid(ShapeText)) {
				//	型が文字列の場合,
				//	文字列図形の数をインクリメントする.
				t_cnt++;
			}
			//	図形の選択フラグを最前面の図形の選択フラグに格納する.
			fore_sel = s->is_selected();
			if (fore_sel) {
				sel++;
				if (cnt == 1) {
					//	消去フラグがない図形の数が 1 の場合,
					//	最前面の図形の選択フラグの最背面の図形の選択フラグに格納する.
					back_sel = true;
				}
				if (s_type == typeid(ShapeGroup)) {
					//	図形の動的な型がグループの場合,
					//	選択されたグループ図形の数をインクリメントする.
					grp++;
				}
				else if (s_type == typeid(ShapeText)) {
					//	図形の動的な型が文字列の場合,
					//	選択された文字列図形の数をインクリメントする.
					t_sel++;
				}
				if (p == nullptr || prev_sel == false) {
					//	ひとつ背面の図形がヌル
					//	またはひとつ背面の図形の選択フラグがない場合,
					run_len++;
				}
			}
			p = s;
			prev_sel = fore_sel;
		}

		mfi_cut().IsEnabled(sel > 0);
		mfi_copy().IsEnabled(sel > 0);
		using winrt::Windows::ApplicationModel::DataTransfer::StandardDataFormats;
		winrt::hstring formats[2]{ FMT_DATA, StandardDataFormats::Text() };
		mfi_paste().IsEnabled(clipboard_contains(formats, 2));
		mfi_delete().IsEnabled(sel > 0);
		mfi_select_all().IsEnabled(sel < cnt);
		mfi_group().IsEnabled(sel > 1);
		mfi_ungroup().IsEnabled(grp > 0);
		mfi_text_edit().IsEnabled(t_sel > 0);
		mfi_text_find().IsEnabled(t_cnt > 0);
		// 前面に配置可能か調べる.
		// 1. 複数のランレングスがある.
		// 2. または, 少なくとも 1 つは選択された図形があり, 
		//    かつ最前面の図形は選択されいない.
		bool enable_forw = (run_len > 1 || (sel > 0 && !fore_sel));
		// 背面に配置可能か調べる.
		// 1. 複数のランレングスがある.
		// 2. または, 少なくとも 1 つは選択された図形があり, 
		//    かつ最背面の図形は選択されいない.
		bool enable_back = (run_len > 1 || (sel > 0 && !back_sel));
		mfi_bring_forward().IsEnabled(enable_forw);
		mfi_bring_to_front().IsEnabled(enable_forw);
		mfi_send_to_back().IsEnabled(enable_back);
		mfi_send_backward().IsEnabled(enable_back);
		mfi_summary().IsEnabled(cnt > 0);
		m_list_select = sel;
	}

	//	長さの単位の名前を得る.
	winrt::hstring MainPage::get_unit_name(void)
	{
		using winrt::Windows::ApplicationModel::Resources::ResourceLoader;

		if (m_page_unit == LEN_UNIT::GRID) {
			return ResourceLoader::GetForCurrentView().GetString(L"cxi_unit_grid/Content");
		}
		else if (m_page_unit == LEN_UNIT::INCH) {
			return ResourceLoader::GetForCurrentView().GetString(L"cxi_unit_inch/Content");
		}
		else if (m_page_unit == LEN_UNIT::MILLI) {
			return ResourceLoader::GetForCurrentView().GetString(L"cxi_unit_milli/Content");
		}
		else if (m_page_unit == LEN_UNIT::PIXEL) {
			return ResourceLoader::GetForCurrentView().GetString(L"cxi_unit_pixel/Content");
		}
		else if (m_page_unit == LEN_UNIT::POINT) {
			return ResourceLoader::GetForCurrentView().GetString(L"cxi_unit_point/Content");
		}
		return {};
	}

	// メインページを作成する.
	MainPage::MainPage(void)
	{
		using winrt::Windows::ApplicationModel::Resources::ResourceLoader;
		using winrt::Windows::UI::ViewManagement::ApplicationView;
		using winrt::Windows::UI::ViewManagement::UISettings;

		// お約束.
		InitializeComponent();

		// コンテキストメニューをリソースから読み込む.
		m_menu_stroke = unbox_value<MenuFlyout>(Resources().Lookup(box_value(L"mf_stroke")));
		m_menu_fill = unbox_value<MenuFlyout>(Resources().Lookup(box_value(L"mf_fill")));
		m_menu_font = unbox_value<MenuFlyout>(Resources().Lookup(box_value(L"mf_font")));
		m_menu_page = unbox_value<MenuFlyout>(Resources().Lookup(box_value(L"mf_page")));
		m_menu_ungroup = unbox_value<MenuFlyout>(Resources().Lookup(box_value(L"mf_ungroup")));

		// クリックの判定時間をシステムから得る.
		m_click_time = static_cast<uint64_t>(UISettings().DoubleClickTime()) * 1000L;

		// アプリケーションの中断・継続などのイベントハンドラーを設定する.
		{
			using winrt::Windows::UI::Xaml::Application;
			auto const& app{ Application::Current() };
			app.Suspending({ this, &MainPage::app_suspending });
			app.Resuming({ this, &MainPage::app_resuming });
			app.EnteredBackground({ this, &MainPage::app_entered_background });
			app.LeavingBackground({ this, &MainPage::app_leaving_background });
		}

		// ウィンドウの表示が変わったときのイベントハンドラーを設定する.
		{
			auto const& thread{ CoreWindow::GetForCurrentThread() };
			thread.Activated({ this, &MainPage::thread_activated });
			thread.VisibilityChanged({ this, &MainPage::thread_visibility_changed });
		}

		// ディスプレイの状態が変わったときのイベントハンドラーを設定する.
		{
			auto const& disp{ DisplayInformation::GetForCurrentView() };
			disp.DpiChanged({ this, &MainPage::disp_dpi_changed });
			disp.OrientationChanged({ this, &MainPage::disp_orientation_changed });
			disp.DisplayContentsInvalidated({ this, &MainPage::disp_contents_invalidated });
		}

		// D2D/DWRITE ファクトリを図形/文字列図形クラスに, 
		// 図形リストとページパネルを操作クラスに格納する.
		{
			Shape::s_d2d_factory = m_page_dx.m_d2dFactory.get();
			Shape::s_dwrite_factory = m_page_dx.m_dwriteFactory.get();
			Undo::set(&m_list_shapes, &m_page_panel);
		}

		// 文字範囲の背景色, 文字範囲の文字色をリソースから得る.
		{
			using winrt::Windows::UI::Color;
			try {
				auto b_key = box_value(L"SystemColorHighlightColor");
				auto t_key = box_value(L"SystemColorHighlightTextColor");
				auto b_res = Resources().Lookup(b_key);
				auto t_res = Resources().Lookup(t_key);
				cast_to(unbox_value<Color>(b_res), m_page_dx.m_range_bcolor);
				cast_to(unbox_value<Color>(t_res), m_page_dx.m_range_tcolor);
			}
			catch (winrt::hresult_error) {
				m_page_dx.m_range_bcolor = { 0.0f, 1.0f / 3.0f, 2.0f / 3.0f, 1.0f };
				m_page_dx.m_range_tcolor = S_WHITE;
			}
		}

		// ページパネルの属性を初期化する.
		const auto dpi = DisplayInformation::GetForCurrentView().LogicalDpi();
		{
			using winrt::Windows::UI::Xaml::Media::Brush;
			m_page_panel.m_corner_rad.x = GRIDLEN_PX;
			m_page_panel.m_corner_rad.y = m_page_panel.m_corner_rad.x;
			m_page_panel.m_grid_len = static_cast<double>(GRIDLEN_PX) - 1.0;
			m_page_panel.m_page_size.width = std::floor(A4_PER_INCH.width * dpi);
			m_page_panel.m_page_size.height = std::floor(A4_PER_INCH.height * dpi);
			// 色の初期値はテーマに依存する.
			D2D1_COLOR_F b_col;
			D2D1_COLOR_F f_col;
			try {
				auto b_key = box_value(L"ApplicationPageBackgroundThemeBrush");
				auto f_key = box_value(L"ApplicationForegroundThemeBrush");
				auto const& b_res = Resources().Lookup(b_key);
				auto const& f_res = Resources().Lookup(f_key);
				cast_to(unbox_value<Brush>(b_res), b_col);
				cast_to(unbox_value<Brush>(f_res), f_col);
			}
			catch (winrt::hresult_error e) {
				b_col = S_WHITE;
				f_col = S_BLACK;
			}
			m_page_panel.set_page_color(b_col);
			m_page_panel.m_stroke_color = f_col;
			m_page_panel.m_fill_color = b_col;
			m_page_panel.m_font_color = f_col;
		}

		// ページパネルの書体の属性を初期化する.
		{
			wchar_t lang[LOCALE_NAME_MAX_LENGTH];	// 地域・言語名
			// 地域・言語名を得る.
			// DWriteFactory からフォントコレクションを得る.
			// 地域・言語名とフォントコレクションを利用可能な書体名に格納する.
			// 地域・言語名を指定してシステムから UI 本文用の書体を得る.
			// コレクションから UI 本文用と同じ書体を検索する.
			// それが存在する場合, それをコレクションから得られた書体名を既定の書体名に格納する.
			// それが存在しない場合, UI 本文用の書体を既定の書体名に格納する.
			// UI 本文用の書体名, 太さ, 字体, 幅を図形属性の既定値に格納する.
			// UI 本文用の書体を破棄する.
			// フォントコレクションを破棄する.
			GetUserDefaultLocaleName(lang, LOCALE_NAME_MAX_LENGTH);
			winrt::com_ptr<IDWriteFontCollection> coll;
			winrt::check_hresult(
				m_page_dx.m_dwriteFactory->GetSystemFontCollection(coll.put())
			);
			ShapeText::set_available_fonts(coll.get(), lang);
			auto name = tx_edit().FontFamily().Source();
			UINT32 index = 0;
			BOOL exists = false;
			winrt::check_hresult(
				coll->FindFamilyName(name.c_str(), &index, &exists)
			);
			if (exists) {
				m_page_panel.m_font_family = ShapeText::get_available_font(index);
			}
			else {
				m_page_panel.m_font_family = wchar_cpy(name.c_str());
			}
			m_page_panel.m_font_size = tx_edit().FontSize();
			m_page_panel.m_font_stretch = static_cast<DWRITE_FONT_STRETCH>(tx_edit().FontStretch());
			m_page_panel.m_font_style = static_cast<DWRITE_FONT_STYLE>(tx_edit().FontStyle());
			m_page_panel.m_font_weight = static_cast<DWRITE_FONT_WEIGHT>(tx_edit().FontWeight().Weight);
			coll = nullptr;
		}
		{
			//m_page_dx.m_anch_brush->SetColor(m_page_panel.m_anch_color);
			//m_page_dx.m_aux_brush->SetColor(m_page_panel.m_aux_color);
			m_page_min.x = 0.0;
			m_page_min.y = 0.0;
			m_page_max.x = m_page_panel.m_page_size.width;
			m_page_max.y = m_page_panel.m_page_size.height;
			m_page_unit = LEN_UNIT::PIXEL;
			m_col_style = COL_STYLE::DEC;
		}
		mru_add_file(nullptr);
		finish_file_read();
	}

	//	メインページの内容を破棄する.
	void MainPage::release(void)
	{
		if (m_summary_visible) {
			summary_close();
		}
		//	操作スタックを消去する.
		undo_clear();
		s_list_clear(m_list_shapes);
#if defined(_DEBUG)
		if (debug_leak_cnt != 0) {
			cd_message_show(L"Memory leak occurs", {});
		}
#endif
	}

}
