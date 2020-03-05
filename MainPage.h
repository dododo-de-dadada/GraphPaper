#pragma once
//------------------------------
//	1	「ソリューションエクスプローラー」>「ビューを切り替える」>「フォルダービュー」
//	2	「ビルド」>「構成マネージャー」>「アクティブソリューションプラットフォーム」を x64
//	3	「プロジェクト」>「NuGetパッケージの管理」>「復元」. 必要なら「MicroSoft.UI.Xaml」と「Microsoft.Windows.CppWinRT」を更新.
//	4	「デバッグ」>「GraphPaper のプロパティ」>「構成プロパティ」>「ターゲットプラットフォームの最小バージョン」>「10.0.18362.0」
//	MainPage.h
//
//	MainPage.cpp	メインページの作成, アプリの終了
//	MainPage_app.cpp	アプリケーションの中断と再開
//	MainPage_arrange.cpp	図形リストの要素の並び替え
//	MainPage_arrow.cpp	矢じりの形式と寸法を設定
//	MainPage_disp.cpp	表示デバイスのハンドラー
//	MainPage_file.cpp	ファイルの読み書き
//	MainPage_fill.cpp	塗りつぶしの設定
//	MainPage_find.cpp	文字列の検索/置換
//	MainPage_font.cpp	書体の設定
//	MainPage_grid.cpp	方眼の設定
//	MainPage_group.cpp	グループ化とグループの解除
//	MainPage_kybd.cpp	キーボードアクセラレーターのハンドラー
//	MainPage_page.cpp	ページの設定と表示
//	MainPage_pointer.cpp	ポインターイベントのハンドラー
//	MainPage_sample.cpp	見本ダイアログの設定, 表示
//	MainPage_scroll.cpp	スクロールバーの設定
//	MainPage_select.cpp	図形の選択
//	MainPage_status.cpp	ステータスバーの設定
//	MainPage_stroke.cpp	線枠の設定
//	MainPage_summary.cpp	図形一覧パネルの表示, 設定
//	MainPage_text.cpp	文字列の編集と検索/置換
//	MainPage_thread.cpp	ウィンドウ切り替えのハンドラー
//	MainPage_tool.cpp	作図ツールの設定
//	MainPage_undo.cpp	元に戻すとやり直す
//	MainPage_xcvd.cpp	切り取りとコピー, 貼り付け, 削除
//------------------------------
#include <ppltasks.h>
#include <winrt/Windows.ApplicationModel.ExtendedExecution.h>
#include <winrt/Windows.UI.Xaml.Input.h>
#include "MainPage.g.h"
#include "undo.h"

namespace winrt::GraphPaper::implementation
{
	using winrt::Windows::ApplicationModel::EnteredBackgroundEventArgs;
	using winrt::Windows::ApplicationModel::LeavingBackgroundEventArgs;
	using winrt::Windows::ApplicationModel::SuspendingEventArgs;
	using winrt::Windows::ApplicationModel::SuspendingOperation;
	using winrt::Windows::Foundation::Collections::IVector;
	using winrt::Windows::Foundation::IAsyncAction;
	using winrt::Windows::Foundation::IAsyncOperation;
	using winrt::Windows::Graphics::Display::DisplayInformation;
	using winrt::Windows::Storage::StorageFile;
	using winrt::Windows::UI::Core::CoreCursor;
	using winrt::Windows::UI::Core::CoreWindow;
	using winrt::Windows::UI::Core::VisibilityChangedEventArgs;
	using winrt::Windows::UI::Core::WindowActivatedEventArgs;
	using winrt::Windows::UI::Xaml::Controls::ContentDialog;
	using winrt::Windows::UI::Xaml::Controls::ContentDialogButtonClickEventArgs;
	using winrt::Windows::UI::Xaml::Controls::ContentDialogOpenedEventArgs;
	using winrt::Windows::UI::Xaml::Controls::ContentDialogClosedEventArgs;
	using winrt::Windows::UI::Xaml::Controls::MenuFlyout;
	using winrt::Windows::UI::Xaml::Controls::Primitives::ScrollEventArgs;
	using winrt::Windows::UI::Xaml::Controls::Primitives::RangeBaseValueChangedEventArgs;
	using winrt::Windows::UI::Xaml::Controls::SelectionChangedEventArgs;
	using winrt::Windows::UI::Xaml::Controls::TextChangedEventArgs;
	using winrt::Windows::UI::Xaml::Input::PointerRoutedEventArgs;
	using winrt::Windows::UI::Xaml::Input::KeyboardAcceleratorInvokedEventArgs;
	using winrt::Windows::UI::Xaml::RoutedEventArgs;
	using winrt::Windows::UI::Xaml::SizeChangedEventArgs;
	using winrt::Windows::UI::Xaml::Visibility;
	using winrt::Windows::UI::Xaml::Window;
	using winrt::Windows::System::VirtualKey;
	using winrt::Windows::System::VirtualKeyModifiers;

	// UI 要素の表示の状態
	constexpr auto VISIBLE = Visibility::Visible;	// 表示	
	constexpr auto COLLAPSED = Visibility::Collapsed;	// 非表示

	// 書式変換
	constexpr auto FMT_INCH = L"%.3lf";	// インチ単位の書式
	constexpr auto FMT_INCH_UNIT = L"%.3lfin";	// インチ単位の書式
	constexpr auto FMT_MILLI = L"%.3lf";	// ミリメートル単位の書式
	constexpr auto FMT_MILLI_UNIT = L"%.3lfmm";	// ミリメートル単位の書式
	constexpr auto FMT_POINT = L"%.2lf";	// ポイント単位の書式
	constexpr auto FMT_POINT_UNIT = L"%.2lfpt";	// ポイント単位の書式
	constexpr auto FMT_PIXEL = L"%.0lf";	// ピクセル単位の書式
	constexpr auto FMT_PIXEL_UNIT = L"%.0lfpx";	// ピクセル単位の書式
	constexpr auto FMT_ZOOM = L"%.lf%%";	// 倍率の書式
	constexpr auto FMT_GRID = L"%.3lf";	// グリッド単位の書式
	constexpr auto FMT_GRID_UNIT = L"%.3lfgr";	// グリッド単位の書式

	constexpr auto A4_PER_INCH = D2D1_SIZE_F{ 8.27f, 11.69f };	// A4 サイズの大きさ (インチ)
	constexpr auto GRIDLEN_PX = 48.0f;	// 方眼の大きさの初期値 (ピクセル)
	static const winrt::hstring FMT_DATA{ L"graph_paper_data" };	// 図形データのクリップボード書式

	//-------------------------------
	//	状態遷移
	//-------------------------------
	enum struct S_TRAN {
		BEGIN,	// 初期状態
		PRESS_L,	// 左ボタンを押している状態
		PRESS_R,	// 右ボタンを押している状態
		PRESS_AREA,	// 左ボタンを押して, 範囲を選択している状態
		PRESS_MOVE,	// 左ボタンを押して, 図形を移動している状態
		PRESS_FORM,	// 左ボタンを押して, 図形を変形している状態
		CLICK,	// クリックした状態
		CLICK_2,	// クリック後に左ボタンを押した状態
	};

	//-------------------------------
	//	ステータスバーの状態
	//-------------------------------
	enum struct STAT_BAR {
		GRID = 1,	// 方眼の長さ
		PAGE = (2 | 4),	// ページの大きさ
		CURS = (8 | 16),	// カーソルの位置
		ZOOM = 32,	// 拡大率
		DRAW = 64,	// 作図ツール
		UNIT = 128	// 長さの単位
	};

	//-------------------------------
	//	作図ツール
	//-------------------------------
	enum struct DRAW_TOOL {
		TOOL_SELECT,	// 選択ツール
		TOOL_BEZI,	// 曲線
		TOOL_ELLI,	// だ円
		TOOL_LINE,	// 線分
		TOOL_QUAD,	// 四辺形
		TOOL_RECT,	// 方形
		TOOL_RRECT,	// 角丸方形
		TOOL_TEXT,	// 文字列
		TOOL_SCALE	// 目盛り
	};

	//-------------------------------
	// 長さの単位
	//-------------------------------
	enum struct LEN_UNIT {
		PIXEL,	// ピクセル
		INCH,	// インチ
		MILLI,	// ミリメートル
		POINT,	// ポイント
		GRID	// 方眼 (グリッド)
	};
	//	ピクセル単位の長さを他の単位の文字列に変換する.
	void conv_val_to_len(const LEN_UNIT unit, const double px, const double dpi, const double g_len, wchar_t* buf, const uint32_t b_len);
	double conv_len_to_val(const LEN_UNIT unit, const double val, const double dpi, const double g_len);

	//-------------------------------
	//	色成分の形式
	//-------------------------------
	enum struct COL_STYLE {
		DEC,	// 10 進数
		HEX,	// 16 進数	
		FLT,	// 浮動小数
		CEN		// パーセント
	};
	//	色成分の値を文字列に変換する.
	void conv_val_to_col(const COL_STYLE style, const double val, wchar_t* buf, const uint32_t len);

	//-------------------------------
	//	見本の型
	//-------------------------------
	enum struct SAMP_TYPE {
		NONE,	// なし
		STROKE,	// ストローク
		FILL,	// 塗りつぶし
		FONT,	// 書体
	};

	enum struct MSG_ICON {
		ALERT,
		INFO
	};

	//-------------------------------
	//	メインページ
	//-------------------------------
	struct MainPage : MainPageT<MainPage> {
		std::mutex m_dx_mutex;		// DX のための同期プリミティブ

		winrt::hstring m_mru_token;	// 最近使ったファイルのトークン

		wchar_t* m_find_text = nullptr;		// 検索の検索文字列
		bool m_find_case = false;		// 英文字の区別フラグ
		wchar_t* m_find_repl = nullptr;		// 検索の置換文字列
		bool m_find_wrap = false;		// 回り込み検索フラグ

		SHAPE_DX m_page_dx;		// ページの描画環境
		ShapePanel m_page_panel;		// ページのパネル
		D2D1_POINT_2F m_page_min{ 0.0, 0.0 };		// パネルの左上位置 (値がマイナスのときは, 図形がページの外側にある)
		D2D1_POINT_2F m_page_max{ 0.0, 0.0 };		// パネルの右下位置 (値がページの大きさより大きいときは, 図形がページの外側にある)
		LEN_UNIT m_page_unit = LEN_UNIT::PIXEL;	// 長さの単位
		COL_STYLE m_col_style = COL_STYLE::DEC;	// 色成分の書式

		uint64_t m_click_time = 0L;		// クリックの判定時間
		double m_click_dist = 6.0;		// クリックの判定長さ
		D2D1_POINT_2F m_curr_pos{ 0.0, 0.0 };		// ポインターの現在位置
		D2D1_POINT_2F m_prev_pos{ 0.0, 0.0 };		// ポインターの前回位置
		S_TRAN m_press_state = S_TRAN::BEGIN;		// ポインターが押された状態
		ANCH_WHICH m_press_anchor = ANCH_WHICH::ANCH_OUTSIDE;		// ポインターが押された図形の部位
		D2D1_POINT_2F m_press_pos{ 0.0, 0.0 };		// ポインターが押された位置
		Shape* m_press_shape = nullptr;		// ポインターが押された図形
		Shape* m_press_shape_prev = nullptr;		// 前回ポインターが押された図形
		Shape* m_press_shape_summary = nullptr;		// 一覧でポインターが押された図形
		uint64_t m_press_time = 0;		// ポインターが押された時刻

		MenuFlyout m_menu_stroke;	// 線枠コンテキストメニュー
		MenuFlyout m_menu_fill;	// 塗りつぶしコンテキストメニュー
		MenuFlyout m_menu_font;	// 書体コンテキストメニュー
		MenuFlyout m_menu_page;	// ページコンテキストメニュー
		MenuFlyout m_menu_ungroup;	// グループ解除コンテキストメニュー

		DRAW_TOOL m_draw_tool = DRAW_TOOL::TOOL_SELECT;		// 作図ツール

		uint32_t m_list_select = 0;		// 選択された図形の数
		S_LIST_T m_list_shapes;		// 図形リスト

		U_STACK_T m_stack_redo;		// やり直し操作スタック
		U_STACK_T m_stack_undo;		// 元に戻す操作スタック
		uint32_t m_stack_nset = 0;	// 操作スタックに積まれた要素の組数
		bool m_stack_over = false;	// 操作スタックに積まれた要素が最大数を超えた
		bool m_stack_push = false;	// 操作スタックの更新フラグ (ヌルが積まれたら true)

		STAT_BAR m_status_bar = status_or(STAT_BAR::CURS, STAT_BAR::ZOOM);	// ステータスバーの状態

		SHAPE_DX m_sample_dx;		// 見本の描画環境
		ShapePanel m_sample_panel;		// 見本のパネル
		Shape* m_sample_shape = nullptr;	// 見本の図形
		SAMP_TYPE m_sample_type = SAMP_TYPE::NONE;	// 見本の型

		bool m_summary_visible = false;	// 図形一覧パネルの表示フラグ
		bool m_window_visible = false;		// ウィンドウが表示されている/表示されてない

		//-------------------------------
		//	MainPage.cpp
		//	メインページの作成, アプリケーションの終了
		//-------------------------------

		//	メインページを破棄する.
		~MainPage(void);
		//	メッセージダイアログを表示する.
		void cd_message_show(winrt::hstring const& glyph, winrt::hstring const& message, winrt::hstring const& desc);
		//	編集メニュー項目の使用の可否を設定する.
		void enable_edit_menu(void);
		//	メインページを作成する.
		MainPage(void);
		//	メインページの内容を破棄する.
		void release(void);
		//	ページメニューの「バージョン情報」が選択された.
		void mfi_about_graph_paper_click(IInspectable const&, RoutedEventArgs const&)
		{
			cd_message_show(L"icon_info", L"str_appname", L"str_version");
		}
		//	長さの単位の名前を得る.
		winrt::hstring get_unit_name(void);
		//	ファイルメニューの「終了」が選択された
		IAsyncAction mfi_exit_click(IInspectable const&, RoutedEventArgs const&);

		//-------------------------------
		//	MainPage_app.cpp
		//	アプリケーションの中断と再開
		//-------------------------------

		//	アプリケーションがバックグラウンドに移った.
		void app_entered_background(IInspectable const& sender, EnteredBackgroundEventArgs const& args);
		//	アプリケーションを非同期に延長する.
		IAsyncAction app_extended_session_async(SuspendingOperation const& s_op);
		//	アプリケーションがバックグラウンドに移った.
		void app_leaving_background(IInspectable const& sender, LeavingBackgroundEventArgs const& args);
		//	アプリケーションが再開された.
		void app_resuming(IInspectable const& sender, IInspectable const& object);
		//	アプリケーションを非同期に再開する.
		IAsyncAction app_resuming_async(void);
		//	アプリケーションが中断された.
		void app_suspending(IInspectable const& sender, SuspendingEventArgs const& args);

		//-------------------------------
		//	MainPage_arrange.cpp
		//	図形の並び替え
		//-------------------------------

		//	選択された図形について, 次あるいは前の図形と入れ替える.
		template<typename T> void arrange_order(void);
		//	選択された図形を最背面または最前面に移動する.
		template<bool TO_BACK> void arrange_to(void);
		//	編集メニューの「前面に移動」が選択された.
		void mfi_bring_forward_click(IInspectable const&, RoutedEventArgs const&);
		//	編集メニューの「最前面に移動」が選択された.
		void mfi_bring_to_front_click(IInspectable const&, RoutedEventArgs const&);
		//	編集メニューの「ひとつ背面に移動」が選択された.
		void mfi_send_backward_click(IInspectable const&, RoutedEventArgs const&);
		//	編集メニューの「最背面に移動」が選択された.
		void mfi_send_to_back_click(IInspectable const&, RoutedEventArgs const&);

		//-------------------------------
		//	MainPage_arrow.cpp
		//	矢じりの形式や寸法を設定する
		//-------------------------------

		//	線枠メニューの「矢じりの種類」に印をつける.
		void arrow_style_check_menu(const ARROW_STYLE a_style);
		//	線枠メニューの「矢じりの種類」>「閉じた」が選択された.
		void rmfi_arrow_filled_click(IInspectable const&, RoutedEventArgs const&);
		//	線枠メニューの「矢じりの種類」>「なし」が選択された.
		void rmfi_arrow_none_click(IInspectable const&, RoutedEventArgs const&);
		//	線枠メニューの「矢じりの種類」>「開いた」が選択された.
		void rmfi_arrow_opened_click(IInspectable const&, RoutedEventArgs const&);
		//	線枠メニューの「矢じりの大きさ」が選択された.
		IAsyncAction mfi_arrow_size_click(IInspectable const&, RoutedEventArgs const&);
		//	値をスライダーのヘッダーに格納する.
		template <UNDO_OP U, int S> void arrow_set_slider_header(const double val);
		//	値をスライダーのヘッダーと図形に格納する.
		template <UNDO_OP U, int S> void arrow_set_slider(IInspectable const&, RangeBaseValueChangedEventArgs const&);

		//-------------------------------
		//	MainPage_disp.cpp
		//	表示デバイスのハンドラー
		//-------------------------------

		//	ディスプレーの再描画が必要となった場合のハンドラー
		void disp_contents_invalidated(DisplayInformation const& sender, IInspectable const& args);
		//	ディスプレーの DPI が変更された場合のハンドラー
		void disp_dpi_changed(DisplayInformation const& sender, IInspectable const& args);
		//	ディスプレーの向きが変更された場合のハンドラー
		void disp_orientation_changed(DisplayInformation const& sender, IInspectable const& args);

		//-------------------------------
		//	MainPage_file.cpp
		//	ファイルの読み書き
		//-------------------------------

		//	ストレージファイルを非同期に読む.
		IAsyncOperation<winrt::hresult> file_read_async(StorageFile const& s_file, const bool suspend = false) noexcept;
		//	名前を付けてファイルに非同期に保存する
		IAsyncOperation<winrt::hresult> file_save_as_async(const bool svg_allowed = false) noexcept;
		//	ファイルに非同期に保存する
		IAsyncOperation<winrt::hresult> file_save_async(void) noexcept;
		//	待機カーソルを表示, 表示する前のカーソルを得る.
		CoreCursor file_wait_cursor(void) const;
		//	図形データをストレージファイルに非同期に書き込む.
		IAsyncOperation<winrt::hresult> file_write_gpf_async(StorageFile const& s_file, const bool suspend = false);
		//	SVG としてデータライターに書き込む.
		void file_write_svg(DataWriter const& dt_writer);
		//	SVG としてストレージファイルに非同期に書き込む.
		IAsyncOperation<winrt::hresult> file_write_svg_async(StorageFile const& s_file);
		//	ファイルの読み込みが終了した.
		void finish_file_read(void);
		//	ファイルメニューの「新規」が選択された
		IAsyncAction mfi_new_click(IInspectable const&, RoutedEventArgs const&);
		//	ファイルメニューの「開く」が選択された
		IAsyncAction mfi_open_click(IInspectable const&, RoutedEventArgs const&);
		//	ファイルメニューの「名前を付けて保存」が選択された
		void mfi_save_as_click(IInspectable const&, RoutedEventArgs const&);
		//	ファイルメニューの「保存」が選択された
		void mfi_save_click(IInspectable const&, RoutedEventArgs const&);
		//	最近使ったファイルを非同期に読む.
		IAsyncAction file_read_recent_async(const uint32_t i);
		//	ファイルメニューの「最近使ったファイル 1」が選択された
		void mfi_file_recent_1_click(IInspectable const&, RoutedEventArgs const&);
		//	ファイルメニューの「最近使ったファイル 2」が選択された
		void mfi_file_recent_2_click(IInspectable const&, RoutedEventArgs const&);
		//	ファイルメニューの「最近使ったファイル 3」が選択された
		void mfi_file_recent_3_click(IInspectable const&, RoutedEventArgs const&);
		//	ファイルメニューの「最近使ったファイル 4」が選択された
		void mfi_file_recent_4_click(IInspectable const&, RoutedEventArgs const&);
		//	ファイルメニューの「最近使ったファイル 5」が選択された
		void mfi_file_recent_5_click(IInspectable const&, RoutedEventArgs const&);
		//	最近使ったファイルにストレージファイルを追加する.
		void mru_add_file(StorageFile const& s_file);
		//	最近使ったファイルのトークンからストレージファイルを得る.
		IAsyncOperation<StorageFile> mru_get_file(const winrt::hstring token);
		//	最近使ったファイルからトークンを得る.
		winrt::hstring mru_get_token(const uint32_t i);
		//	最近使ったファイルのメニュー項目を更新する.
		void mru_update_menu_items(void);

		//-------------------------------
		//	MainPage_fill.cpp
		//	塗りつぶしの設定
		//-------------------------------

		//	塗りつぶしメニューの「色」が選択された.
		IAsyncAction mfi_fill_color_click(IInspectable const&, RoutedEventArgs const&);
		//	値をスライダーのヘッダーと図形に格納する.
		template <UNDO_OP U, int S> void fill_set_slider(IInspectable const&, RangeBaseValueChangedEventArgs const&);
		//	値をスライダーのヘッダーに格納する.
		template <UNDO_OP U, int S> void fill_set_slider_header(double val);

		//-------------------------------
		//　MainPage_font.cpp
		//　書体の設定
		//-------------------------------

		//　書体メニューの「字体」に印をつける.
		void font_style_check_menu(const DWRITE_FONT_STYLE f_style);
		//　リストビュー「見本リスト」がロードされた.
		void lv_sample_list_loaded(IInspectable const&, RoutedEventArgs const& /*e*/);
		//　書体メニューの「色」が選択された.
		IAsyncAction mfi_font_color_click(IInspectable const&, RoutedEventArgs const&);
		//　書体メニューの「書体名」が選択された.
		IAsyncAction mfi_font_family_click(IInspectable const&, RoutedEventArgs const&);
		//　書体メニューの「イタリック体」が選択された.
		void rmfi_font_italic_click(IInspectable const&, RoutedEventArgs const&);
		//　書体メニューの「標準」が選択された.
		void rmfi_font_normal_click(IInspectable const&, RoutedEventArgs const&);
		//　書体メニューの「斜体」が選択された.
		void rmfi_font_oblique_click(IInspectable const&, RoutedEventArgs const&);
		//　書体メニューの「大きさ」が選択された.
		IAsyncAction mfi_font_size_click(IInspectable const&, RoutedEventArgs const&);
		//　書体メニューの「伸縮」が選択された.
		IAsyncAction mfi_font_stretch_click(IInspectable const&, RoutedEventArgs const&);
		//　書体メニューの「太さ」が選択された.
		IAsyncAction mfi_font_weight_click(IInspectable const&, RoutedEventArgs const&);
		//　値をスライダーのヘッダーに格納する.
		template <UNDO_OP U, int S> void font_set_slider_header(const double val);
		//　値をスライダーのヘッダーと図形に格納する.
		template <UNDO_OP U, int S> void font_set_slider(IInspectable const&, RangeBaseValueChangedEventArgs const&);
		//	書体メニューの「文字列のそろえ」に印をつける.
		void text_align_t_check_menu(const DWRITE_TEXT_ALIGNMENT t_align);
		//	書体メニューの「段落のそろえ」に印をつける.
		void text_align_p_check_menu(const DWRITE_PARAGRAPH_ALIGNMENT p_align);
		//	書体メニューの「行の高さ」>「狭める」が選択された.
		void mfi_text_line_height_contract_click(IInspectable const&, RoutedEventArgs const&);
		//	書体メニューの「行の高さ」>「広げる」が選択された.
		void mfi_text_line_height_expand_click(IInspectable const&, RoutedEventArgs const&);
		//	書体メニューの「行の高さ」>「高さ」が選択された.
		IAsyncAction mfi_text_line_height_click(IInspectable const&, RoutedEventArgs const&);
		//	書体メニューの「余白」が選択された.
		IAsyncAction mfi_text_margin_click(IInspectable const&, RoutedEventArgs const&);
		//	書体メニューの「段落のそろえ」>「中段」が選択された.
		void rmfi_text_align_middle_click(IInspectable const&, RoutedEventArgs const&);
		//	書体メニューの「段落のそろえ」>「下よせ」が選択された.
		void rmfi_text_align_bottom_click(IInspectable const&, RoutedEventArgs const&);
		//	書体メニューの「段落のそろえ」>「上よせ」が選択された.
		void rmfi_text_align_top_click(IInspectable const&, RoutedEventArgs const&);
		//	書体メニューの「文字列のそろえ」>「中央」が選択された.
		void rmfi_text_align_center_click(IInspectable const&, RoutedEventArgs const&);
		//	書体メニューの「文字列のそろえ」>「均等」が選択された.
		void rmfi_text_align_justified_click(IInspectable const&, RoutedEventArgs const&);
		//	書体メニューの「文字列のそろえ」>「左よせ」が選択された.
		void rmfi_text_align_left_click(IInspectable const&, RoutedEventArgs const&);
		//	書体メニューの「文字列のそろえ」>「右よせ」が選択された.
		void rmfi_text_align_right_click(IInspectable const&, RoutedEventArgs const&);
		//	値をスライダーのヘッダーに格納する.
		template <UNDO_OP U, int S> void text_set_slider_header(const double val);
		//	値をスライダーのヘッダーと図形に格納する.
		template <UNDO_OP U, int S> void text_set_slider(IInspectable const&, RangeBaseValueChangedEventArgs const&);

		//-------------------------------
		//　MainPage_grid.cpp
		//　方眼の設定
		//-------------------------------

		//　ページメニューの「方眼の表示」に印をつける.
		void grid_show_check_menu(const GRID_SHOW g_show);
		//　ページメニューの「方眼の大きさ」>「大きさ」が選択された.
		IAsyncAction mfi_grid_len_click(IInspectable const&, RoutedEventArgs const&);
		//　ページメニューの「方眼の大きさ」>「狭める」が選択された.
		void mfi_grid_len_contract_click(IInspectable const&, RoutedEventArgs const&);
		//　ページメニューの「方眼の大きさ」>「広げる」が選択された.
		void mfi_grid_len_expand_click(IInspectable const&, RoutedEventArgs const&);
		//　ページメニューの「方眼線の濃さ」が選択された.
		IAsyncAction mfi_grid_opac_click(IInspectable const&, RoutedEventArgs const&);
		//　ページメニューの「方眼線の表示」>「最背面」が選択された.
		void rmfi_grid_show_back_click(IInspectable const&, RoutedEventArgs const&);
		//　ページメニューの「方眼線の表示」>「最前面」が選択された.
		void rmfi_grid_show_front_click(IInspectable const&, RoutedEventArgs const&);
		//　ページメニューの「方眼線の表示」>「隠す」が選択された.
		void rmfi_grid_show_hide_click(IInspectable const&, RoutedEventArgs const&);
		//　ページメニューの「方眼にそろえる」が選択された.
		void tmfi_grid_snap_click(IInspectable const&, RoutedEventArgs const&);
		//　値をスライダーのヘッダーと図形に格納する.
		template <UNDO_OP U, int S> void grid_set_slider_header(const double val);
		//　値をスライダーのヘッダーに格納する.
		template <UNDO_OP U, int S> void grid_set_slider(IInspectable const&, RangeBaseValueChangedEventArgs const&);

		//-------------------------------
		//	MainPage_group.cpp
		//	グループ化, グループを解除する
		//-------------------------------

		// 「グループ化」が選択された.
		void mfi_group_click(IInspectable const&, RoutedEventArgs const&);
		// 「グループの解除」が選択された.
		void mfi_ungroup_click(IInspectable const&, RoutedEventArgs const&);

		//-------------------------------
		//　MainPage_keyacc.cpp
		//　キーアクセラレーターのハンドラー
		//-------------------------------

		//　Shft + 下矢印キーが押された.
		void ka_range_next_invoked(IInspectable const&, KeyboardAcceleratorInvokedEventArgs const&);
		//　Shift + 上矢印キーが押された.
		void ka_range_prev_invoked(IInspectable const&, KeyboardAcceleratorInvokedEventArgs const&);
		//　下矢印キーが押された.
		void ka_select_next_invoked(IInspectable const&, KeyboardAcceleratorInvokedEventArgs const&);
		//　上矢印キーが押された.
		void ka_select_prev_invoked(IInspectable const&, KeyboardAcceleratorInvokedEventArgs const&);
		//　Escape が押された.
		void ka_tool_select_invoked(IInspectable const&, KeyboardAcceleratorInvokedEventArgs const&);
		//　Cntrol + PgDn が押された.
		//void ka_bring_forward_invoked(IInspectable const&, KeyboardAcceleratorInvokedEventArgs const&);
		//　Cntrol + End が押された.
		//void ka_bring_to_front_invoked(IInspectable const&, KeyboardAcceleratorInvokedEventArgs const&);
		//　Cntrol + C が押された.
		//void ka_copy_invoked(IInspectable const&, KeyboardAcceleratorInvokedEventArgs const&);
		//　Cntrol + X が押された.
		//void ka_cut_invoked(IInspectable const&, KeyboardAcceleratorInvokedEventArgs const&);
		//　Delete が押された.
		//void ka_delete_invoked(IInspectable const&, KeyboardAcceleratorInvokedEventArgs const&);
		//　Cntrol + E が押された.
		//void ka_edit_text_invoked(IInspectable const&, KeyboardAcceleratorInvokedEventArgs const&);
		//　Cntrol + F が押された.
		//void ka_find_text_invoked(IInspectable const&, KeyboardAcceleratorInvokedEventArgs const&);
		//　Cntrol + G が押された.
		//void ka_group_invoked(IInspectable const&, KeyboardAcceleratorInvokedEventArgs const&);
		//　Cntrol + N が押された.
		//void ka_new_invoked(IInspectable const&, KeyboardAcceleratorInvokedEventArgs const&);
		//　Cntrol + O が押された.
		//void ka_open_invoked(IInspectable const&, KeyboardAcceleratorInvokedEventArgs const&);
		//　Cntrol + V が押された.
		//void ka_paste_invoked(IInspectable const&, KeyboardAcceleratorInvokedEventArgs const&);
		//　Cntrol + Y が押された.
		//void ka_redo_invoked(IInspectable const&, KeyboardAcceleratorInvokedEventArgs const&);
		//　Cntrol + Shift + S が押された.
		//void ka_save_as_invoked(IInspectable const&, KeyboardAcceleratorInvokedEventArgs const&);
		//　Cntrol + S が押された.
		//void ka_save_invoked(IInspectable const&, KeyboardAcceleratorInvokedEventArgs const&);
		//　Cntrol + A が押された.
		//void ka_select_all_invoked(IInspectable const&, KeyboardAcceleratorInvokedEventArgs const&);
		//　Cntrol + PgUp が押された.
		//void ka_send_backward_invoked(IInspectable const&, KeyboardAcceleratorInvokedEventArgs const&);
		//　Cntrol + Home が押された.
		//void ka_send_to_back_invoked(IInspectable const&, KeyboardAcceleratorInvokedEventArgs const&);
		//　Cntrol + L が押された.
		//void ka_summaty_invoked(IInspectable const&, KeyboardAcceleratorInvokedEventArgs const&);
		//　Cntrol + Z が押された.
		//void ka_undo_invoked(IInspectable const&, KeyboardAcceleratorInvokedEventArgs const&);
		//　Cntrol + U が押された.
		//void ka_ungroup_invoked(IInspectable const&, KeyboardAcceleratorInvokedEventArgs const&);
		//　Cntrol + 0 が押された.
		//void ka_zoom_reset_invoked(IInspectable const&, KeyboardAcceleratorInvokedEventArgs const&);

		//-------------------------------
		//　MainPage_page.cpp
		//　ページの設定と表示
		//-------------------------------

		//　ページの寸法入力ダイアログの「適用」ボタンが押された.
		void cd_page_size_pri_btn_click(ContentDialog const&, ContentDialogButtonClickEventArgs const&);
		// ページの寸法入力ダイアログの「図形に合わせる」ボタンが押された.
		void cd_page_size_sec_btn_click(ContentDialog const&, ContentDialogButtonClickEventArgs const&);
		//　ページの単位と書式ダイアログの「適用」ボタンが押された.
		void cd_page_unit_pri_btn_click(ContentDialog const&, ContentDialogButtonClickEventArgs const&);
		//	ページメニューの「色」が選択された.
		IAsyncAction mfi_page_color_click(IInspectable const&, RoutedEventArgs const&);
		//	ページメニューの「大きさ」が選択された
		void mfi_page_size_click(IInspectable const&, RoutedEventArgs const&);
		//	ページメニューの「単位と書式」が選択された
		void mfi_page_unit_click(IInspectable const&, RoutedEventArgs const&);
		//	ページと図形を表示する.
		void page_draw(void);
		//	値をスライダーのヘッダーに格納する.
		template <UNDO_OP U, int S> void page_set_slider_header(double val);
		//	値をスライダーのヘッダーと図形に格納する.
		template <UNDO_OP U, int S> void page_set_slider(IInspectable const&, RangeBaseValueChangedEventArgs const&);
		//	ページのパネルがロードされた.
		void scp_page_panel_loaded(IInspectable const& sender, RoutedEventArgs const& args);
		//	ページのパネルの寸法が変わった.
		void scp_page_panel_size_changed(IInspectable const& sender, SizeChangedEventArgs const& args);
		//	ページのパネルの大きさを設定する.
		void set_page_panle_size(void);
		//	ページ寸法ダイアログの「ページの幅」「ページの高さ」テキストボックスの値が変更された.
		void tx_page_size_text_changed(IInspectable const&, TextChangedEventArgs const&);

		//-------------------------------
		//	MainPage_pointer.cpp
		//	ポインターイベントのハンドラー
		//-------------------------------

		//	図形を作成する.
		void create_shape(void);
		//	編集ダイアログを表示して文字列図形を作成する.
		void create_shape_text(void);
		// 範囲選択を終了する.
		void finish_area_select(const VirtualKeyModifiers k_mod);
		// 図形の作成を終了する.
		void finish_create_shape(void);
		// 図形の移動を終了する.
		void finish_move_shape(void);
		// 押された図形の編集を終了する.
		void finish_form_shape(void);
		// ポインターのボタンが上げられた.
		void scp_pointer_canceled(IInspectable const& sender, PointerRoutedEventArgs const& args);
		// ポインターがページのパネルの中に入った.
		void scp_pointer_entered(IInspectable const& sender, PointerRoutedEventArgs const& args);
		// ポインターがページのパネルから出た.
		void scp_pointer_exited(IInspectable const& sender, PointerRoutedEventArgs const& args);
		// ポインターが動いた.
		void scp_pointer_moved(IInspectable const& sender, PointerRoutedEventArgs const& args);
		// ポインターのボタンが押された.
		void scp_pointer_pressed(IInspectable const& sender, PointerRoutedEventArgs const& args);
		// ポインターのボタンが上げられた.
		void scp_pointer_released(IInspectable const& sender, PointerRoutedEventArgs const& args);
		// ポインターのホイールボタンが操作された.
		void scp_pointer_wheel_changed(IInspectable const& sender, PointerRoutedEventArgs const& args);
		// 状況に応じた形状のポインターを設定する.
		void set_pointer(void);
		// コンテキストメニューを表示する.
		void show_context_menu(void);

		//-------------------------------
		//	MainPage_sample.cpp
		//	見本ダイアログの設定, 表示
		//-------------------------------

		//	見本ダイアログが開かれた.
		void cd_sample_opened(ContentDialog const& sender, ContentDialogOpenedEventArgs const& args);
		//	見本のページと見本の図形を表示する
		void sample_draw(void);
		//	見本パネルの大きさが変わった.
		void scp_sample_panel_size_changed(IInspectable const&, RoutedEventArgs const&);

		//-------------------------------
		//	MainPage_scroll.cpp
		//	スクロールバーの設定
		//-------------------------------

		// スクロールバーが操作された
		void sb_scroll(IInspectable const& sender, ScrollEventArgs const& args);
		// スクロールバーの値を設定する.
		void scroll_set(const double aw, const double ah);
		// 図形が表示されるようパネルをスクロールする.
		bool scroll_to_shape(Shape* s);

		//-------------------------------
		//	MainPage_select.cpp
		//	図形の選択
		//-------------------------------

		// 編集メニューの「すべて選択」が選択された.
		void mfi_select_all_click(IInspectable const&, RoutedEventArgs const&);
		// 領域に含まれる図形を選択し, 含まれない図形の選択を解除する.
		bool select_area(const D2D1_POINT_2F a_min, const D2D1_POINT_2F a_max);
		// 次の図形を選択する.
		template <VirtualKeyModifiers M, VirtualKey K> void select_next_shape(void);
		// 指定した範囲にある図形を選択して, そうでない図形は選択しない.
		bool select_range(Shape* const s_from, Shape* const s_to);
		// 図形を選択する.
		void select_shape(Shape* s, const VirtualKeyModifiers vk);
		// 領域に含まれる図形の選択を反転する.
		bool toggle_area(const D2D1_POINT_2F a_min, const D2D1_POINT_2F a_max);
		// すべての図形の選択を解除する.
		bool unselect_all(const bool t_range_only = false);

		//-------------------------------
		//	MainPage_status.cpp
		//	ステータスバーの設定
		//-------------------------------

		// ページメニューの「ステータスバー」が選択された.
		void mi_status_bar_click(IInspectable const&, RoutedEventArgs const&);
		// ステータスバーのメニュー項目に印をつける.
		void status_check_menu(const STAT_BAR a);
		// 列挙型を OR 演算する.
		static STAT_BAR status_or(const STAT_BAR a, const STAT_BAR b) noexcept;
		// ステータスバーの状態をデータリーダーから読み込む.
		void status_read(DataReader const& dt_reader);
		// ポインターの位置をステータスバーに格納する.
		void status_set_curs(void);
		// 方眼の大きさをステータスバーに格納する.
		void status_set_grid(void);
		// ページの大きさをステータスバーに格納する.
		void status_set_page(void);
		// 作図ツールをステータスバーに格納する.
		void status_set_draw(void);
		// 単位をステータスバーに格納する.
		void status_set_unit(void);
		// 拡大率をステータスバーに格納する.
		void status_set_zoom(void);
		// ステータスバーの表示を設定する.
		void status_visibility(void);
		// ステータスバーの状態をデータライターに書き込む.
		void status_write(DataWriter const& dt_writer);

		//------------------------------
		//	MainPage_stroke.cpp
		//	線枠の設定
		//------------------------------

		//	線枠メニューの「種類」に印をつける.
		void stroke_style_check_menu(const D2D1_DASH_STYLE d_style);
		//	線枠メニューの「色」が選択された.
		IAsyncAction mfi_stroke_color_click(IInspectable const&, RoutedEventArgs const&);
		//	線枠メニューの「破線」が選択された.
		void rmfi_stroke_dash_click(IInspectable const&, RoutedEventArgs const&);
		//	線枠メニューの「一点破線」が選択された.
		void rmfi_stroke_dash_dot_click(IInspectable const&, RoutedEventArgs const&);
		//	線枠メニューの「二点破線」が選択された.
		void rmfi_stroke_dash_dot_dot_click(IInspectable const&, RoutedEventArgs const&);
		//	線枠メニューの「点線」が選択された.
		void rmfi_stroke_dot_click(IInspectable const&, RoutedEventArgs const&);
		//	線枠メニューの「実線」が選択された.
		void rmfi_stroke_solid_click(IInspectable const&, RoutedEventArgs const&);
		//	線枠メニューの「破線の配列」が選択された.
		IAsyncAction mfi_stroke_pattern_click(IInspectable const&, RoutedEventArgs const&);
		//	線枠メニューの「太さ」が選択された.
		IAsyncAction mfi_stroke_width_click(IInspectable const&, RoutedEventArgs const&);
		//	値をスライダーのヘッダーに格納する.
		template<UNDO_OP U, int S> void stroke_set_slider_header(double val);
		//	値をスライダーのヘッダーと図形に格納する.
		template<UNDO_OP U, int S> void stroke_set_slider(IInspectable const&, RangeBaseValueChangedEventArgs const&);

		//-------------------------------
		//	MainPage_summary.cpp
		//	図形一覧パネルの表示, 設定
		//-------------------------------

		//	図形一覧パネルの「閉じる」ボタンが押された.
		void btn_summary_close_click(IInspectable const&, RoutedEventArgs const&);
		//	図形一覧の項目が選択された.
		void lv_summary_selection_changed(IInspectable const& sender, SelectionChangedEventArgs const& e);
		//	図形一覧パネルがロードされた.
		void lv_summary_loaded(IInspectable const& sender, RoutedEventArgs const& e);
		//	編集メニューの「リストを表示」が選択された.
		void mfi_summary_click(IInspectable const&, RoutedEventArgs const&);
		//	図形を一覧に追加する.
		void summary_append(Shape* s);
		//	一覧の中で図形を入れ替える.
		void summary_arrange(Shape* s, Shape* t);
		//	図形一覧を消去する.
		void summary_clear(void);
		//	図形一覧パネルを閉じて消去する.
		void summary_close(void);
		//	図形を一覧に挿入する.
		void summary_insert(Shape* s, uint32_t i);
		// これから実行する操作を図形一覧に反映する.
		void summary_reflect(Undo* u);
		//	図形一覧を作成しなおす.
		void summary_remake(void);
		//	図形を一覧から消去する.
		uint32_t summary_remove(Shape* s);
		//	一覧の項目を選択する.
		void summary_select(uint32_t i);
		//	一覧の図形を選択する.
		void summary_select(Shape* s);
		//	一覧の項目を全て選択する.
		void summary_select_all(void);
		//	一覧の最初の項目を選択する.
		void summary_select_head(void);
		//	一覧の最後の項目を選択する.
		void summary_select_tail(void);
		//	一覧の項目を選択解除する.
		void summary_unselect(uint32_t i);
		//	一覧の図形を選択解除する.
		void summary_unselect(Shape* s);
		//	一覧の項目を全て選択解除する.
		void summary_unselect_all(void);
		//	一覧の表示を更新する.
		void summary_update(void);

		//-------------------------------
		//	MainPage_text.cpp
		//	文字列の編集と検索/置換
		//-------------------------------

		//	文字列検索パネルの「閉じる」ボタンが押された.
		void btn_text_find_close_click(IInspectable const&, RoutedEventArgs const&);
		//　文字列検索パネルの「次を検索」ボタンが押された.
		void btn_text_find_next_click(IInspectable const&, RoutedEventArgs const&);
		//　文字列検索パネルの「すべて置換」ボタンが押された.
		void btn_text_replace_all_click(IInspectable const&, RoutedEventArgs const&);
		//　文字列検索パネルの「置換して次に」ボタンが押された.
		void btn_text_replace_click(IInspectable const&, RoutedEventArgs const&);
		//　検索の値をデータリーダーから読み込む.
		void text_find_read(DataReader const& dt_reader);
		//	文字列検索パネルから値を格納する.
		void text_find_set_to(void);
		//　	図形リストの中から文字列を検索する.
		bool text_find_whithin_shapes(void);
		//　検索の値をデータリーダーに書き込む.
		void text_find_write(DataWriter const& dt_writer);
		//　編集メニューの「文字列の検索/置換」が選択された.
		void mfi_text_find_click(IInspectable const&, RoutedEventArgs const&);
		//　検索文字列が変更された.
		void tx_text_find_what_changed(IInspectable const&, TextChangedEventArgs const&);
		//	文字範囲が選択された図形と文字範囲を見つける.
		S_LIST_T::iterator text_find_range_selected(DWRITE_TEXT_RANGE& t_range);
		//	図形が持つ文字列を編集する.
		void text_edit_in(ShapeText* s);
		//	編集メニューの「文字列の編集」が選択された.
		void mfi_text_edit_click(IInspectable const&, RoutedEventArgs const&);

		//-------------------------------
		//	MainPage_thread.cpp
		//	ウィンドウ切り替えのハンドラー
		//-------------------------------

		//	ウィンドウが前面に出された.
		void thread_activated(IInspectable const& sender, WindowActivatedEventArgs const& args);
		//	ウィンドウの表示状態が変わった.
		void thread_visibility_changed(CoreWindow const& sender, VisibilityChangedEventArgs const& args);

		//-------------------------------
		//	MainPage_tool.cpp
		//	作図メニューのハンドラー
		//-------------------------------

		//	作図メニューの「曲線」が選択された.
		void rmfi_tool_bezi_click(IInspectable const&, RoutedEventArgs const&);
		//	作図メニューの「だ円」が選択された.
		void rmfi_tool_elli_click(IInspectable const&, RoutedEventArgs const&);
		//	作図メニューの「直線」が選択された.
		void rmfi_tool_line_click(IInspectable const&, RoutedEventArgs const&);
		//	作図メニューの「四へん形」が選択された.
		void rmfi_tool_quad_click(IInspectable const&, RoutedEventArgs const&);
		//	作図メニューの「方形」が選択された.
		void rmfi_tool_rect_click(IInspectable const&, RoutedEventArgs const&);
		//	作図メニューの「角丸方形」が選択された.
		void rmfi_tool_rrect_click(IInspectable const&, RoutedEventArgs const&);
		//	作図メニューの「図形を選択」が選択された.
		void rmfi_tool_select_click(IInspectable const&, RoutedEventArgs const&);
		//	作図メニューの「文字列」が選択された.
		void rmfi_tool_text_click(IInspectable const&, RoutedEventArgs const&);
		//	作図メニューの「目盛り」が選択された.
		void rmfi_tool_scale_click(IInspectable const&, RoutedEventArgs const&);

		//-----------------------------
		//	MainPage_undo.cpp
		//	元に戻すとやり直す
		//-----------------------------

		//	元に戻す/やり直すメニュー項目の使用の可否を設定する.
		void enable_undo_menu(void);
		//	編集メニューの「やり直し」が選択された.
		void mfi_redo_click(IInspectable const&, RoutedEventArgs const&);
		//	編集メニューの「元に戻す」が選択された.
		void mfi_undo_click(IInspectable const&, RoutedEventArgs const&);
		//	やり直す操作スタックを消去する.
		void redo_clear(void);
		//	操作スタックを消去する.
		void undo_clear(void);
		//	操作を実行する.
		void undo_exec(Undo* u);
		//	無効な操作の組をポップする.
		bool undo_pop_if_invalid(void);
		//	図形を追加して, その操作をスタックに積む.
		void undo_push_append(Shape* s);
		//	図形を入れ替えて, その操作をスタックに積む.
		void undo_push_arrange(Shape* s, Shape* t);
		//	図形の部位の位置を変更して, 変更前の値をスタックに積む.
		void undo_push_form(Shape*, const ANCH_WHICH a, const D2D1_POINT_2F a_pos);
		//	図形を挿入して, その操作をスタックに積む.
		void undo_push_insert(Shape* s, Shape* s_pos);
		//	一連の操作の区切りであるヌルをスタックに積む.
		void undo_push_null(void);
		//	図形を差分だけ移動して, 移動前の値をスタックに積む.
		void undo_push_pos(const D2D1_POINT_2F d);
		//	図形をグループから削除して, その操作をスタックに積む.
		void undo_push_remove(Shape* g, Shape* s);
		//	図形を削除して, その操作をスタックに積む.
		void undo_push_remove(Shape* s);
		//	図形の選択を反転して, その操作をスタックに積む.
		void undo_push_select(Shape* s);
		//	値を図形へ格納して, その操作をスタックに積む.
		template <UNDO_OP U, typename T> void undo_push_set(Shape* s, T const& val);
		//	図形の値をスタックに保存する.
		template <UNDO_OP U> void undo_push_set(Shape* s);
		//	値を図形に格納して, その操作をスタックに積む.
		template<UNDO_OP U, typename T> void undo_push_value(T const& val);
		//	操作スタックをデータリーダーから読み込む.
		void undo_read(DataReader const& dt_reader);
		//	操作スタックをデータリーダーに書き込む.
		void undo_write(DataWriter const& dt_writer);

		//-------------------------------
		//	MainPage_xcvd.cpp
		//	切り取りとコピー, 貼り付け, 削除
		//-------------------------------

		//	クリップボードにデータが含まれているか調べる.
		bool clipboard_contains(winrt::hstring const c_formats[], const uint32_t c_count) const;
		//	選択された図形をクリップボードに非同期に保存する.
		template <uint32_t X> IAsyncAction clipboard_copy_async(void);
		//	クリップボードに保存された図形を非同期に貼り付ける.
		IAsyncAction clipboard_paste_async(void);
		//	編集メニューの「コピー」が選択された.
		void mfi_copy_click(IInspectable const&, RoutedEventArgs const&);
		//	編集メニューの「切り取り」が選択された.
		void mfi_cut_click(IInspectable const&, RoutedEventArgs const&);
		//	編集メニューの「削除」が選択された.
		void mfi_delete_click(IInspectable const&, RoutedEventArgs const&);
		//	編集メニューの「貼り付け」が選択された.
		void mfi_paste_click(IInspectable const&, RoutedEventArgs const&);

		//-------------------------------
		//	MainPage_zoom.cpp
		//	表示倍率の設定
		//-------------------------------

		//	ページメニューの「拡大縮小」>「拡大」が選択された.
		void mfi_zoom_in_clicked(IInspectable const&, RoutedEventArgs const&);
		//	ページメニューの「拡大縮小」>「縮小」が選択された.
		void mfi_zoom_out_clicked(IInspectable const&, RoutedEventArgs const&);
		//	ページメニューの「拡大縮小」>「100%に戻す」が選択された.
		void mfi_zoom_reset_click(IInspectable const&, RoutedEventArgs const&);
};

}

namespace winrt::GraphPaper::factory_implementation
{
	struct MainPage : MainPageT<MainPage, implementation::MainPage>
	{
	};
}
