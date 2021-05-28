#pragma once
//------------------------------
// 1	「ソリューションエクスプローラー」>「ビューを切り替える」>「フォルダービュー」
// 2	「ビルド」>「構成マネージャー」>「アクティブソリューションプラットフォーム」を x64
// 3	「プロジェクト」>「NuGetパッケージの管理」>「復元」. 必要なら「MicroSoft.UI.Xaml」と「Microsoft.Windows.CppWinRT」を更新.
// 4	「デバッグ」>「GraphPaper のプロパティ」>「構成プロパティ」>「ターゲットプラットフォームの最小バージョン」>「10.0.17763.0」
// 	(MicroSoft.UI.Xaml の MenuBar には、Windows 10 Version 1809 (SDK 17763) 以降、または Windows UI ライブラリが必要)
//
// デバッガーの停止で終了したときはすべて 0 になるが,
// アプリケーションを「×」ボタンなどで終了したとき「スレッド 0xXXXX はコード 1 (0x1) で終了しました。」が表示される.
// とりわけ Application::Current().Exit で終了した場合, すべてのスレッドで 1 が表示される.
// Blank App (c++/winrt) でまっさらなアプリケーションを作成したときも同じ表示なので, とりあえず気にしないようにする.
// Microsoft.UI.Xaml のバージョンによっては不具合 (例えばコンパイルできないとか, サブメニューの RadiMenuFlyout のチェックマークが最初はつかない) が出る.
//
// MainPage.h
//
// MainPage.cpp	メインページの作成, アプリの終了
// MainPage_app.cpp	アプリケーションの中断と再開
// MainPage_arrng.cpp	図形の並び替え
// MainPage_arrow.cpp	矢じりの形式と寸法
// MainPage_disp.cpp	表示デバイスのハンドラー
// MainPage_file.cpp	ファイルの読み書き
// MainPage_fill.cpp	塗りつぶし
// MainPage_font.cpp	書体と文字列の配置
// MainPage_grid.cpp	方眼
// MainPage_group.cpp	グループ化とグループの解除
// NainPage_join.cpp	線分の繋がり
// MainPage_kybd.cpp	キーボードアクセラレータのハンドラー
// MainPage_misc.cpp	長さの単位, 色の表記, ステータスバー, バージョン情報
// MainPage_pointer.cpp	ポインターイベントのハンドラー
// MainPage_sample.cpp	見本
// MainPage_sheet.cpp	用紙の各属性の設定
// MainPage_scroll.cpp	スクロールバー
// MainPage_select.cpp	図形の選択
// MainPage_stbar.cpp	ステータスバー
// MainPage_stroke.cpp	線枠
// MainPage_smry.cpp	図形の一覧
// MainPage_text.cpp	文字列の編集と検索/置換
// MainPage_thread.cpp	ウィンドウ切り替えのハンドラー
// MainPage_tool.cpp	作図ツール
// MainPage_undo.cpp	元に戻すとやり直し
// MainPage_xcvd.cpp	切り取りとコピー, 貼り付け, 削除
// MainPage_zoom.cpp	表示倍率
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
	using winrt::Windows::UI::Core::Preview::SystemNavigationCloseRequestedPreviewEventArgs;
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
	constexpr auto FMT_INCH_UNIT = L"%.3lf in";	// インチ単位の書式
	constexpr auto FMT_MILLI = L"%.3lf";	// ミリメートル単位の書式
	constexpr auto FMT_MILLI_UNIT = L"%.3lf mm";	// ミリメートル単位の書式
	constexpr auto FMT_POINT = L"%.2lf";	// ポイント単位の書式
	constexpr auto FMT_POINT_UNIT = L"%.2lf pt";	// ポイント単位の書式
	constexpr auto FMT_PIXEL = L"%.1lf";	// ピクセル単位の書式
	constexpr auto FMT_PIXEL_UNIT = L"%.1lf px";	// ピクセル単位の書式
	constexpr auto FMT_ZOOM = L"%.lf%%";	// 倍率の書式
	constexpr auto FMT_GRID = L"%.3lf";	// グリッド単位の書式
	constexpr auto FMT_GRID_UNIT = L"%.3lf gr";	// グリッド単位の書式
	constexpr auto ICON_INFO = L"icon_info";	// 情報アイコンの静的リソースのキー
	constexpr auto ICON_ALERT = L"icon_alert";	// 警告アイコンの静的リソースのキー

	constexpr auto SHEET_SIZE_DEF = D2D1_SIZE_F{ 8.0F * 96.0F, 11.0F * 96.0F };	// 用紙寸法の既定値 (ピクセル)
	constexpr auto SCALE_MAX = 128.0f;	// 表示倍率の最大値
	constexpr auto SCALE_MIN = 1.0f / 128.0f;	// 表示倍率の最小値
	static const winrt::hstring CF_GPD{ L"graph_paper_data" };	// 図形データのクリップボード書式

	//-------------------------------
	// ポインターボタンの状態
	//-------------------------------
	enum struct PBTN_STATE {
		BEGIN,	// 初期状態
		PRESS_LBTN,	// 左ボタンを押している状態
		PRESS_RBTN,	// 右ボタンを押している状態
		PRESS_AREA,	// 左ボタンを押して, 範囲を選択している状態
		PRESS_MOVE,	// 左ボタンを押して, 図形を移動している状態
		PRESS_FORM,	// 左ボタンを押して, 図形を変形している状態
		CLICK,	// クリックした状態
		CLICK_LBTN,	// クリック後に左ボタンを押した状態
	};

	//-------------------------------
	// ステータスバーの状態
	//-------------------------------
	enum struct SBAR_FLAG {
		GRID = 1,	// 方眼の大きさ
		SHEET = (2 | 4),	// 用紙の大きさ
		CURS = (8 | 16),	// カーソルの位置
		ZOOM = 32,	// 拡大率
		DRAW = 64,	// 作図ツール
		UNIT = 128	// 長さの単位
	};

	//-------------------------------
	// 作図ツール
	//-------------------------------
	enum struct TOOL_DRAW {
		SELECT,	// 選択ツール
		BEZI,	// 曲線
		ELLI,	// だ円
		LINE,	// 線分
		POLY,	// 多角形
		RECT,	// 方形
		RRECT,	// 角丸方形
		TEXT,	// 文字列
		RULER,	// 定規
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
	constexpr bool LEN_UNIT_SHOW = true;	// 単位名の表示
	constexpr bool LEN_UNIT_HIDE = true;	// 単位名の非表示

	// 長さを文字列に変換する.
	template <bool B> void conv_len_to_str(const LEN_UNIT len_unit, const double value, const double dpi, const double g_len, const uint32_t t_len, wchar_t* t_buf);

	// 長さを文字列に変換する.
	template <bool B, size_t Z> void conv_len_to_str(const LEN_UNIT len_unit, const double value, const double dpi, const double g_len, wchar_t(&t_buf)[Z])
	{
		conv_len_to_str<B>(len_unit, value, dpi, g_len, Z, t_buf);
	}

	//-------------------------------
	// 色の表記
	//-------------------------------
	enum struct COLOR_CODE {
		DEC,	// 10 進数
		HEX,	// 16 進数	
		REAL,	// 実数
		CENT	// パーセント
	};

	// 色成分を文字列に変換する.
	void conv_col_to_str(const COLOR_CODE c_code, const double value, const size_t t_len, wchar_t t_buf[]);

	// 色成分を文字列に変換する.
	template <size_t Z> void conv_col_to_str(const COLOR_CODE c_code, const double value, wchar_t(&t_buf)[Z])
	{
		conv_col_to_str(c_code, value, Z, t_buf);
	}

	//-------------------------------
	// 見本の型
	//-------------------------------
	enum struct SAMP_TYPE {
		NONE,	// なし
		STROKE,	// 線枠
		FILL,	// 塗りつぶし
		FONT,	// 書体
		JOIN,	// 線の連結
		MISC	// その他
	};

	//-------------------------------
	// メッセージダイアログのアイコン
	//-------------------------------
	enum struct MSG_ICON {
		ALERT,
		INFO
	};

	//-------------------------------
	// メインページ
	//-------------------------------
	struct MainPage : MainPageT<MainPage> {
		std::mutex m_dx_mutex;	// 描画環境の排他制御
		winrt::hstring m_file_token_mru;	// 最近使ったファイルのトークン

		bool m_smry_descend = true;
		std::atomic_bool m_smry_atomic{ false };	// 図形一覧の排他制御

		// text

		wchar_t* m_text_find = nullptr;	// 検索の検索文字列
		wchar_t* m_text_repl = nullptr;	// 検索の置換文字列
		bool m_text_find_case = false;	// 英文字の区別フラグ
		bool m_text_find_wrap = false;	// 回り込み検索フラグ
		bool m_text_adjust = false;	// 文字列に合わせるフラグ

		// misc

		LEN_UNIT m_len_unit = LEN_UNIT::PIXEL;	// 長さの単位
		COLOR_CODE m_color_code = COLOR_CODE::DEC;	// 色成分の書式

		// sbar

		SBAR_FLAG m_status_bar = sbar_or(SBAR_FLAG::CURS, SBAR_FLAG::ZOOM);	// ステータスバーの状態

		// drawing tool

		TOOL_DRAW m_tool_draw = TOOL_DRAW::SELECT;		// 作図ツール
		TOOL_POLY m_tool_poly{ TOOL_POLY_DEF };	// 多角形の作図ツール

		// main

		uint32_t m_cnt_selected = 0;		// 選択された図形の数
		MenuFlyout m_menu_stroke = nullptr;	// 線枠コンテキストメニュー
		MenuFlyout m_menu_fill = nullptr;	// 塗りつぶしコンテキストメニュー
		MenuFlyout m_menu_font = nullptr;	// 書体コンテキストメニュー
		MenuFlyout m_menu_sheet = nullptr;	// 用紙コンテキストメニュー
		MenuFlyout m_menu_ungroup = nullptr;	// グループ解除コンテキストメニュー

		// s_list

		S_LIST_T m_list_shapes;		// 図形リスト

		// sheet

		SHAPE_DX m_sheet_dx;		// 用紙の描画環境
		ShapeSheet m_sheet_main;		// 用紙
		D2D1_POINT_2F m_sheet_min{ 0.0F, 0.0F };		// 用紙の左上位置 (値がマイナスのときは, 図形が用紙の外側にある)
		D2D1_POINT_2F m_sheet_max{ 0.0F, 0.0F };		// 用紙の右下位置 (値が用紙の大きさより大きいときは, 図形が用紙の外側にある)

		// ponter

		D2D1_POINT_2F m_pointer_cur{ 0.0F, 0.0F };		// ポインターの現在位置
		D2D1_POINT_2F m_pointer_pre{ 0.0F, 0.0F };		// ポインターの前回位置
		PBTN_STATE m_pointer_state = PBTN_STATE::BEGIN;		// ポインターの押された状態
		uint32_t m_pointer_anchor = ANCH_TYPE::ANCH_SHEET;		// ポインターが押された図形の部位
		D2D1_POINT_2F m_pointer_pressed{ 0.0F, 0.0F };		// ポインターが押された位置
		Shape* m_pointer_shape = nullptr;		// ポインターが押された図形
		Shape* m_pointer_shape_prev = nullptr;		// 前回ポインターが押された図形
		Shape* m_pointer_shape_smry = nullptr;		// 一覧でポインターが押された図形
		uint64_t m_pointer_time = 0ULL;		// ポインターが押された時刻
		uint64_t m_pointer_click_time = 0ULL;		// クリックの判定時間 (マイクロ秒)
		double m_pointer_click_dist = 6.0;		// クリックの判定距離 (DIPs)

		// undo stack

		uint32_t m_stack_rcnt = 0;	// やり直し操作スタックに積まれた要素の組数
		U_STACK_T m_stack_redo;		// やり直し操作スタック
		uint32_t m_stack_ucnt = 0;	// 元に戻す操作スタックに積まれた要素の組数
		U_STACK_T m_stack_undo;		// 元に戻す操作スタック
		bool m_stack_updt = false;	// 操作スタックの更新フラグ (ヌルが積まれたら true)

		// sample

		SHAPE_DX m_sample_dx;		// 見本用紙の描画環境
		ShapeSheet m_sample_sheet;		// 見本用紙
		Shape* m_sample_shape = nullptr;	// 見本用紙の図形
		SAMP_TYPE m_sample_type = SAMP_TYPE::NONE;	// 見本用紙の型

		bool m_window_visible = false;		// ウィンドウの表示フラグ

		winrt::event_token m_token_suspending;	// アプリケーションの中断ハンドラーのトークン
		winrt::event_token m_token_resuming;	// アプリケーションの再開ハンドラーのトークン
		winrt::event_token m_token_entered_background;		// アプリケーションのバックグランド切り替えハンドラーのトークン
		winrt::event_token m_token_leaving_background;		// アプリケーションのフォアグランド切り替えハンドラーのトークン
		winrt::event_token m_token_activated;	// ウィンドウの前面背面切り替えハンドラーのトークン
		winrt::event_token m_token_visibility_changed;	// ウィンドウの表示切り替えハンドラーのトークン
		winrt::event_token m_token_dpi_changed;	// ディスプレーの解像度切り替えハンドラーのトークン
		winrt::event_token m_token_orientation_changed;	// ディスプレーの方向切り替えハンドラーのトークン
		winrt::event_token m_token_contents_invalidated;	// ディスプレーの表示内容切り替えハンドラーのトークン
		winrt::event_token m_token_close_requested;	// アプリケーションを閉じるハンドラーのトークン

		//-------------------------------
		// MainPage.cpp
		// メインページの作成, アプリの終了
		//-------------------------------

		// 内容が変更されていたなら, 確認ダイアログを表示してその応答を得る.
		IAsyncOperation<bool> ask_for_conf_async(void);
		// 編集メニュー項目の使用の可否を設定する.
		void edit_menu_enable(void);
		// ファイルメニューの「終了」が選択された
		IAsyncAction exit_click_async(IInspectable const&, RoutedEventArgs const&);
		// メインページを作成する.
		MainPage(void);
		// メッセージダイアログを表示する.
		void message_show(winrt::hstring const& glyph_key, winrt::hstring const& message_key, winrt::hstring const& desc_key);
		// ウィンドウの閉じるボタンが押された.
		void navi_close_requested(IInspectable const&, SystemNavigationCloseRequestedPreviewEventArgs const& args)
		{
			args.Handled(true);
			auto _{ exit_click_async(nullptr, nullptr) };
		}
		// ファイルメニューの「新規」が選択された
		IAsyncAction new_click_async(IInspectable const&, RoutedEventArgs const&);

		//-------------------------------
		// MainPage_app.cpp
		// アプリケーションの中断と再開
		//-------------------------------

		// アプリケーションがバックグラウンドに移った.
		void app_entered_background(IInspectable const& sender, EnteredBackgroundEventArgs const& args);
		// アプリケーションがバックグラウンドに移った.
		void app_leaving_background(IInspectable const& sender, LeavingBackgroundEventArgs const& args);
		// アプリケーションが再開された.
		IAsyncAction app_resuming_async(IInspectable const&, IInspectable const&);
		// アプリケーションが中断された.
		IAsyncAction app_suspending_async(IInspectable const&, SuspendingEventArgs const& args);

		//-------------------------------
		// MainPage_join.cpp
		// 線の連結
		//-------------------------------

		IAsyncAction join_limit_click_async(IInspectable const&, RoutedEventArgs const&);
		void join_style_check_menu(const D2D1_LINE_JOIN j_style);
		void join_style_click(IInspectable const& sender, RoutedEventArgs const&);
		template <UNDO_OP U, int S> void join_set_slider_header(const float value);
		template <UNDO_OP U, int S> void join_set_slider(IInspectable const&, RangeBaseValueChangedEventArgs const& args);

		//-------------------------------
		// MainPage_arrng.cpp
		// 図形の並び替え
		//-------------------------------

		// 選択された図形を次または前の図形と入れ替える.
		template<typename T> void arrng_order(void);
		// 選択された図形を最背面または最前面に移動する.
		template<bool B> void arrng_to(void);
		// 編集メニューの「前面に移動」が選択された.
		void arrng_bring_forward_click(IInspectable const&, RoutedEventArgs const&);
		// 編集メニューの「最前面に移動」が選択された.
		void arrng_bring_to_front_click(IInspectable const&, RoutedEventArgs const&);
		// 編集メニューの「ひとつ背面に移動」が選択された.
		void arrng_send_backward_click(IInspectable const&, RoutedEventArgs const&);
		// 編集メニューの「最背面に移動」が選択された.
		void arrng_send_to_back_click(IInspectable const&, RoutedEventArgs const&);

		//-------------------------------
		// MainPage_arrow.cpp
		// 矢じりの形式と寸法
		//-------------------------------

		// 線枠メニューの「矢じりの種類」に印をつける.
		void arrow_style_check_menu(const ARROWHEAD_STYLE h_style);
		// 線枠メニューの「矢じりの種類」が選択された.
		void arrow_style_click(IInspectable const& sender, RoutedEventArgs const&);
		// 線枠メニューの「矢じりの大きさ」が選択された.
		IAsyncAction arrow_size_click_async(IInspectable const&, RoutedEventArgs const&);
		// 値をスライダーのヘッダーに格納する.
		template <UNDO_OP U, int S> void arrow_set_slider_header(const float value);
		// 値をスライダーのヘッダーと、見本の図形に格納する.
		template <UNDO_OP U, int S> void arrow_set_slider(IInspectable const&, RangeBaseValueChangedEventArgs const&);

		//-------------------------------
		// MainPage_disp.cpp
		// 表示デバイスのハンドラー
		//-------------------------------

		// ディスプレーの再描画が必要となった場合のハンドラー
		void disp_contents_invalidated(DisplayInformation const& sender, IInspectable const& args);
		// ディスプレーの DPI が変更された場合のハンドラー
		void disp_dpi_changed(DisplayInformation const& sender, IInspectable const& args);
		// ディスプレーの向きが変更された場合のハンドラー
		void disp_orientation_changed(DisplayInformation const& sender, IInspectable const& args);

		//-------------------------------
		// MainPage_file.cpp
		// ファイルの読み書き
		//-------------------------------

		// ストレージファイルを非同期に読む.
		IAsyncOperation<winrt::hresult> file_read_async(StorageFile const& s_file, const bool suspend = false, const bool layout = false) noexcept;
		// 名前を付けてファイルに非同期に保存する
		IAsyncOperation<winrt::hresult> file_save_as_async(const bool svg_allowed = false) noexcept;
		// ファイルに非同期に保存する
		IAsyncOperation<winrt::hresult> file_save_async(void) noexcept;
		// 待機カーソルを表示, 表示する前のカーソルを得る.
		CoreCursor file_wait_cursor(void) const;
		// 図形データをストレージファイルに非同期に書き込む.
		IAsyncOperation<winrt::hresult> file_write_gpf_async(StorageFile const& s_file, const bool suspend = false, const bool layout = false);
		// 図形データを SVG としてストレージファイルに非同期に書き込む.
		IAsyncOperation<winrt::hresult> file_write_svg_async(StorageFile const& s_file);
		// ファイルの読み込みが終了した.
		void file_finish_reading(void);
		// ファイルメニューの「開く」が選択された
		IAsyncAction file_open_click_async(IInspectable const&, RoutedEventArgs const&);
		// ファイルメニューの「名前を付けて保存」が選択された
		void file_save_as_click(IInspectable const&, RoutedEventArgs const&);
		// ファイルメニューの「上書き保存」が選択された
		void file_save_click(IInspectable const&, RoutedEventArgs const&);
		// 最近使ったファイルを非同期に読む.
		IAsyncAction file_recent_read_async(const uint32_t i);
		// ファイルメニューの「最近使ったファイル 」のサブ項目が選択された
		void file_recent_click(IInspectable const&, RoutedEventArgs const&);
		// 最近使ったファイルにストレージファイルを追加する.
		void file_recent_add(StorageFile const& s_file);
		// 最近使ったファイルのトークンからストレージファイルを得る.
		IAsyncOperation<StorageFile> file_recent_get_async(const winrt::hstring token);
		// 最近使ったファイルのメニュー項目を更新する.
		void file_recent_update_menu(void);

		//-------------------------------
		// MainPage_fill.cpp
		// 塗りつぶし
		//-------------------------------

		// 塗りつぶしメニューの「色」が選択された.
		IAsyncAction fill_color_click_async(IInspectable const&, RoutedEventArgs const&);
		// 値をスライダーのヘッダーと、見本の図形に格納する.
		template <UNDO_OP U, int S> void fill_set_slider(IInspectable const&, RangeBaseValueChangedEventArgs const&);
		// 値をスライダーのヘッダーに格納する.
		template <UNDO_OP U, int S> void fill_set_slider_header(const float value);

		//-------------------------------
		//　MainPage_font.cpp
		//　書体と文字列の配置
		//-------------------------------

		// 書体メニューの「字体」に印をつける.
		void font_style_check_menu(const DWRITE_FONT_STYLE f_style);
		// 書体メニューの「色」が選択された.
		IAsyncAction font_color_click_async(IInspectable const&, RoutedEventArgs const&);
		// 書体メニューの「書体名」が選択された.
		IAsyncAction font_family_click_async(IInspectable const&, RoutedEventArgs const&);
		// 書体メニューの「イタリック体」が選択された.
		void font_style_italic_click(IInspectable const&, RoutedEventArgs const&);
		// 書体メニューの「標準」が選択された.
		void font_style_normal_click(IInspectable const&, RoutedEventArgs const&);
		// 書体メニューの「斜体」が選択された.
		void font_style_oblique_click(IInspectable const&, RoutedEventArgs const&);
		// 書体メニューの「大きさ」が選択された.
		IAsyncAction font_size_click_async(IInspectable const&, RoutedEventArgs const&);
		// 書体メニューの「伸縮」が選択された.
		IAsyncAction font_stretch_click_async(IInspectable const&, RoutedEventArgs const&);
		// 書体メニューの「太さ」が選択された.
		IAsyncAction font_weight_click_async(IInspectable const&, RoutedEventArgs const&);
		// 値をスライダーのヘッダーに格納する.
		template <UNDO_OP U, int S> void font_set_slider_header(const float value);
		// 値をスライダーのヘッダーと、見本の図形に格納する.
		template <UNDO_OP U, int S> void font_set_slider(IInspectable const&, RangeBaseValueChangedEventArgs const&);
		// 書体メニューの「文字列のそろえ」に印をつける.
		void text_align_t_check_menu(const DWRITE_TEXT_ALIGNMENT t_align);
		// 書体メニューの「段落のそろえ」に印をつける.
		void text_align_p_check_menu(const DWRITE_PARAGRAPH_ALIGNMENT p_align);
		// 書体メニューの「大きさを合わせる」が選択された.
		void text_bbox_adjust_click(IInspectable const&, RoutedEventArgs const&);
		// 書体メニューの「行の高さ」>「狭める」が選択された.
		void text_line_con_click(IInspectable const&, RoutedEventArgs const&);
		// 書体メニューの「行の高さ」>「広げる」が選択された.
		void text_line_exp_click(IInspectable const&, RoutedEventArgs const&);
		// 書体メニューの「行の高さ」>「高さ」が選択された.
		IAsyncAction text_line_click_async(IInspectable const&, RoutedEventArgs const&);
		// 書体メニューの「余白」が選択された.
		IAsyncAction text_margin_click_async(IInspectable const&, RoutedEventArgs const&);
		// 書体メニューの「段落のそろえ」>「中段」が選択された.
		void text_align_p_mid_click(IInspectable const&, RoutedEventArgs const&);
		// 書体メニューの「段落のそろえ」>「下よせ」が選択された.
		void text_align_p_bot_click(IInspectable const&, RoutedEventArgs const&);
		// 書体メニューの「段落のそろえ」>「上よせ」が選択された.
		void text_align_p_top_click(IInspectable const&, RoutedEventArgs const&);
		// 書体メニューの「文字列のそろえ」>「中央」が選択された.
		void text_align_t_center_click(IInspectable const&, RoutedEventArgs const&);
		// 書体メニューの「文字列のそろえ」>「均等」が選択された.
		void text_align_t_just_click(IInspectable const&, RoutedEventArgs const&);
		// 書体メニューの「文字列のそろえ」>「左よせ」が選択された.
		void text_align_t_left_click(IInspectable const&, RoutedEventArgs const&);
		// 書体メニューの「文字列のそろえ」>「右よせ」が選択された.
		void text_align_t_right_click(IInspectable const&, RoutedEventArgs const&);
		// 値をスライダーのヘッダーに格納する.
		template <UNDO_OP U, int S> void text_set_slider_header(const float value);
		// 値をスライダーのヘッダーと、見本の図形に格納する.
		template <UNDO_OP U, int S> void text_set_slider(IInspectable const&, RangeBaseValueChangedEventArgs const&);

		//-------------------------------
		// MainPage_grid.cpp
		// 方眼
		//-------------------------------

		// 用紙メニューの「方眼の強調」に印をつける.
		void grid_emph_check_menu(const GRID_EMPH& g_emph);
		// 用紙メニューの「方眼の表示」に印をつける.
		void grid_show_check_menu(const GRID_SHOW g_show);
		// 用紙メニューの「方眼の大きさ」>「大きさ」が選択された.
		IAsyncAction grid_len_click_async(IInspectable const&, RoutedEventArgs const&);
		// 用紙メニューの「方眼の大きさ」>「狭める」が選択された.
		void grid_len_con_click(IInspectable const&, RoutedEventArgs const&);
		// 用紙メニューの「方眼の大きさ」>「広げる」が選択された.
		void grid_len_exp_click(IInspectable const&, RoutedEventArgs const&);
		// 用紙メニューの「方眼の濃さ」が選択された.
		IAsyncAction grid_gray_click_async(IInspectable const&, RoutedEventArgs const&);
		// 用紙メニューの「方眼の強調」が選択された.
		void grid_emph_click(IInspectable const& sender, RoutedEventArgs const&);
		// 用紙メニューの「方眼の表示」が選択された.
		void grid_show_click(IInspectable const& sender, RoutedEventArgs const&);
		// 用紙メニューの「方眼にそろえる」が選択された.
		void grid_snap_click(IInspectable const&, RoutedEventArgs const&);
		// 値をスライダーのヘッダーと図形に格納する.
		template <UNDO_OP U, int S> void grid_set_slider_header(const float value);
		// 値をスライダーのヘッダーと、見本の図形に格納する.
		template <UNDO_OP U, int S> void grid_set_slider(IInspectable const&, RangeBaseValueChangedEventArgs const&);

		//-------------------------------
		// MainPage_group.cpp
		// グループ化とグループの解除
		//-------------------------------

		// 編集メニューの「グループ化」が選択された.
		void group_click(IInspectable const&, RoutedEventArgs const&);
		// 編集メニューの「グループの解除」が選択された.
		void ungroup_click(IInspectable const&, RoutedEventArgs const&);

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
		//void ka_text_find_invoked(IInspectable const&, KeyboardAcceleratorInvokedEventArgs const&);
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
		// MainPage_sheet.cpp
		// 用紙の各属性の設定
		//-------------------------------

		// 用紙メニューの「用紙の色」が選択された.
		IAsyncAction sheet_color_click_async(IInspectable const&, RoutedEventArgs const&);
		// 用紙メニューの「用紙の大きさ」が選択された
		IAsyncAction sheet_size_click_async(IInspectable const&, RoutedEventArgs const&);
		// 用紙の左上位置と右下位置を更新する.
		void sheet_update_bbox(void) noexcept;
		// 図形をもとに用紙の左上位置と右下位置を更新する.
		void sheet_update_bbox(const Shape* s) noexcept;
		// 用紙を表示する.
		void sheet_draw(void);
		// 前景色を得る.
		const D2D1_COLOR_F& sheet_foreground(void) const noexcept;
		// 値を用紙の右下位置に格納する.
		void sheet_max(const D2D1_POINT_2F p_max) noexcept;
		// 用紙の右下位置を得る.
		const D2D1_POINT_2F sheet_max(void) const noexcept;
		// 値を用紙の左上位置に格納する.
		void sheet_min(const D2D1_POINT_2F p_min) noexcept;
		// 用紙の左上位置を得る.
		const D2D1_POINT_2F sheet_min(void) const noexcept;
		// 値をスライダーのヘッダーに格納する.
		template <UNDO_OP U, int S> void sheet_set_slider_header(const float value);
		// 値をスライダーのヘッダーと、見本の図形に格納する.
		template <UNDO_OP U, int S> void sheet_set_slider(IInspectable const&, RangeBaseValueChangedEventArgs const&);
		// 用紙の大きさの最大値 (ピクセル) を得る.
		constexpr float sheet_size_max(void) const noexcept { return 32767.0F; }
		// 用紙のスワップチェーンパネルがロードされた.
		void sheet_panel_loaded(IInspectable const& sender, RoutedEventArgs const& args);
		// 用紙のスワップチェーンパネルの寸法が変わった.
		void sheet_panel_size_changed(IInspectable const& sender, SizeChangedEventArgs const& args);
		// 用紙のスワップチェーンパネルの大きさを設定する.
		void sheet_panle_size(void);
		// 用紙寸法ダイアログの「用紙の幅」「用紙の高さ」テキストボックスの値が変更された.
		void sheet_size_text_changed(IInspectable const&, TextChangedEventArgs const&);
		// 用紙の描画環境を得る
		SHAPE_DX& sheet_dx(void) noexcept { return m_sheet_dx; }
		// 用紙とその他の属性を初期化する.
		void sheet_init(void) noexcept;

		//-------------------------------
		// MainPage_pref.cpp
		// 設定の保存と削除
		//-------------------------------

		// 保存された用紙とその他の属性を読み込む.
		IAsyncOperation<winrt::hresult> pref_load_async(void);
		// 用紙メニューの「設定を削除」が選択された.
		IAsyncAction pref_delete_click_async(IInspectable const&, RoutedEventArgs const&);
		// 用紙メニューの「設定を保存」が選択された.
		IAsyncAction pref_save_click_async(IInspectable const&, RoutedEventArgs const&);

		//-----------------------------
		// MainPage_misc.cpp
		// 長さの単位, 色の表記, ステータスバー, バージョン情報
		//-----------------------------

		// その他メニューの「バージョン情報」が選択された.
		IAsyncAction about_graph_paper_click(IInspectable const&, RoutedEventArgs const&)
		{
			//using winrt::Windows::ApplicationModel::Resources::ResourceLoader;

			tb_version().Visibility(VISIBLE);
			const auto def_btn = cd_sample().DefaultButton();
			const auto pri_text = cd_sample().PrimaryButtonText();
			const auto close_text = cd_sample().CloseButtonText();
			cd_sample().PrimaryButtonText(L"");
			cd_sample().CloseButtonText(L"OK");
			cd_sample().Title(box_value(L"GraphPaper"));
			m_sample_type = SAMP_TYPE::MISC;
			co_await cd_sample().ShowAsync();
			cd_sample().PrimaryButtonText(pri_text);
			cd_sample().CloseButtonText(close_text);
			cd_sample().DefaultButton(def_btn);
			tb_version().Visibility(COLLAPSED);
			delete m_sample_shape;
#if defined(_DEBUG)
			debug_leak_cnt--;
#endif
			m_sample_shape = nullptr;
			// バージョン情報のメッセージダイアログを表示する.
			//message_show(ICON_INFO, L"str_appname", L"str_version");
		}
		// 値を色の表記に格納する.
		void color_code(const COLOR_CODE code) noexcept { m_color_code = code; }
		// 色の表記を得る.
		const COLOR_CODE color_code(void) const noexcept { return m_color_code; }
		// その他メニューの「色の表記」に印をつける.
		void color_code_check_menu(void);
		// その他メニューの「色の表記」のサブ項目が選択された.
		void color_code_click(IInspectable const& sender, RoutedEventArgs const&);
		// 値を長さの単位に格納する.
		void len_unit(const LEN_UNIT value) noexcept { m_len_unit = value; }
		// 長さの単位を得る.
		const LEN_UNIT len_unit(void) const noexcept { return m_len_unit; }
		// その他メニューの「長さの単位」に印をつける.
		void len_unit_check_menu(void);
		// その他メニューの「長さの単位」のサブ項目が選択された.
		void len_unit_click(IInspectable const&, RoutedEventArgs const&);

		//-------------------------------
		// MainPage_pointer.cpp
		// ポインターイベントのハンドラー
		//-------------------------------

		// ポインターが押された図形の部位
		void pointer_anchor(const uint32_t anchor) noexcept { m_pointer_anchor = anchor; }
		// コンテキストメニューを表示する.
		void pointer_context_menu(void);
		// ポインターの現在位置を得る.
		const D2D1_POINT_2F& pointer_cur(void) const noexcept { return m_pointer_cur; }
		// イベント引数からポインターの現在位置を得る.
		void pointer_cur_pos(PointerRoutedEventArgs const& args);
		// 範囲選択を終了する.
		void pointer_finish_selecting_area(const VirtualKeyModifiers k_mod);
		// 文字列図形の作成を終了する.
		IAsyncAction pointer_finish_creating_text_async(const D2D1_POINT_2F diff);
		// 図形の作成を終了する.
		void pointer_finish_creating(const D2D1_POINT_2F diff);
		// 図形の移動を終了する.
		void pointer_finish_moving(void);
		// 押された図形の編集を終了する.
		void pointer_finish_forming(void);
		// ポインターが押された位置を得る.
		const D2D1_POINT_2F pointer_pressed(void) const noexcept { return m_pointer_pressed; }
		// 状況に応じた形状のポインターを設定する.
		void pointer_set(void);
		// 値をポインターが押された図形に格納する.
		void pointer_shape(Shape* const s) noexcept { m_pointer_shape = s; }
		// 値をポインターが押された図形に格納する.
		void pointer_shape_prev(Shape* const prev) noexcept { m_pointer_shape_prev = prev; }
		// ポインターが押された図形を得る.
		Shape* pointer_shape_prev(void) const noexcept { return m_pointer_shape_prev; }
		// 値をポインターが押された図形に格納する.
		void pointer_shape_smry(Shape* const smry) noexcept { m_pointer_shape_smry = smry; }
		// ポインターが押された図形を得る.
		Shape* pointer_shape_smry(void) const noexcept { return m_pointer_shape_smry; }
		// ポインターの押された状態に格納する.
		void pointer_state(const PBTN_STATE state) noexcept { m_pointer_state = state; }
		// ポインターの押された状態を得る.
		const PBTN_STATE pointer_state(void) const noexcept { return m_pointer_state; }
		// ポインターのボタンが上げられた.
		void pointer_canceled(IInspectable const& sender, PointerRoutedEventArgs const& args);
		// ポインターがページのスワップチェーンパネルの中に入った.
		void pointer_entered(IInspectable const& sender, PointerRoutedEventArgs const& args);
		// ポインターがページのスワップチェーンパネルから出た.
		void pointer_exited(IInspectable const& sender, PointerRoutedEventArgs const& args);
		// ポインターが動いた.
		void pointer_moved(IInspectable const& sender, PointerRoutedEventArgs const& args);
		// ポインターのボタンが押された.
		void pointer_pressed(IInspectable const& sender, PointerRoutedEventArgs const& args);
		// ポインターのボタンが上げられた.
		void pointer_released(IInspectable const& sender, PointerRoutedEventArgs const& args);
		// ポインターのホイールボタンが操作された.
		void pointer_wheel_changed(IInspectable const& sender, PointerRoutedEventArgs const& args);

		//-------------------------------
		// MainPage_sample.cpp
		// 見本
		//-------------------------------

		SHAPE_DX& sample_dx(void) { return m_sample_dx; }
		// 見本ダイアログが開かれた.
		void sample_opened(ContentDialog const& sender, ContentDialogOpenedEventArgs const& args);
		// 見本を表示する
		void sample_draw(void);
		// 見本のスワップチェーンパネルの大きさが変わった.
		void sample_panel_size_changed(IInspectable const&, RoutedEventArgs const&);
		//　リストビュー「見本リスト」がロードされた.
		void sample_list_loaded(IInspectable const&, RoutedEventArgs const&);

		//-------------------------------
		// MainPage_scroll.cpp
		// スクロールバー
		//-------------------------------

		// スクロールバーが操作された
		void scroll(IInspectable const& sender, ScrollEventArgs const& args);
		// スクロールバーの値を設定する.
		void scroll_set(const double aw, const double ah);
		// 図形が表示されるようパネルをスクロールする.
		bool scroll_to(const Shape* s);

		//-------------------------------
		// MainPage_select.cpp
		// 図形の選択
		//-------------------------------

		// 編集メニューの「すべて選択」が選択された.
		void select_all_click(IInspectable const&, RoutedEventArgs const&);
		// 領域に含まれる図形を選択し, 含まれない図形の選択を解除する.
		bool select_area(const D2D1_POINT_2F a_min, const D2D1_POINT_2F a_max);
		// 次の図形を選択する.
		template <VirtualKeyModifiers M, VirtualKey K> void select_next_shape(void);
		// 指定した範囲にある図形を選択して, そうでない図形は選択しない.
		bool select_range(Shape* const s_from, Shape* const s_to);
		// 図形を選択する.
		void select_shape(Shape* const s, const VirtualKeyModifiers vk);
		// 領域に含まれる図形の選択を反転する.
		bool toggle_area(const D2D1_POINT_2F a_min, const D2D1_POINT_2F a_max);
		// すべての図形の選択を解除する.
		bool unselect_all(const bool t_range_only = false);

		//-------------------------------
		// MainPage_stbar.cpp
		// ステータスバー
		//-------------------------------

		// 値をステータスバーの状態に格納する.
		void status_bar(const SBAR_FLAG stbar) noexcept { m_status_bar = stbar; }
		// ステータスバーの状態を得る.
		const SBAR_FLAG status_bar(void) const noexcept { return m_status_bar; }
		// その他メニューの「ステータスバー」が選択された.
		void sbar_click(IInspectable const&, RoutedEventArgs const&);
		// ステータスバーのメニュー項目に印をつける.
		void sbar_check_menu(const SBAR_FLAG a);
		// 列挙型を OR 演算する.
		static SBAR_FLAG sbar_or(const SBAR_FLAG a, const SBAR_FLAG b) noexcept;
		// ステータスバーの状態をデータリーダーから読み込む.
		void sbar_read(DataReader const& dt_reader);
		// ポインターの位置をステータスバーに格納する.
		void sbar_set_curs(void);
		// 作図ツールをステータスバーに格納する.
		void sbar_set_draw(void);
		// 方眼の大きさをステータスバーに格納する.
		void sbar_set_grid(void);
		// 用紙の大きさをステータスバーに格納する.
		void sbar_set_sheet(void);
		// 単位をステータスバーに格納する.
		void sbar_set_unit(void);
		// 拡大率をステータスバーに格納する.
		void sbar_set_zoom(void);
		// ステータスバーの表示を設定する.
		void sbar_visibility(void);
		// ステータスバーの状態をデータライターに書き込む.
		void sbar_write(DataWriter const& dt_writer);

		//------------------------------
		// MainPage_stroke.cpp
		// 線枠
		//------------------------------

		// 線枠メニューの「種類」に印をつける.
		void stroke_dash_style_check_menu(const D2D1_DASH_STYLE d_style);
		// 線枠メニューの「色」が選択された.
		IAsyncAction stroke_color_click_async(IInspectable const&, RoutedEventArgs const&);
		// 線枠メニューの「種類」のサブ項目が選択された.
		void stroke_dash_style_click(IInspectable const& sender, RoutedEventArgs const&);
		// 線枠メニューの「破線の配列」が選択された.
		IAsyncAction stroke_dash_patt_click_async(IInspectable const&, RoutedEventArgs const&);
		// 線枠メニューの「太さ」が選択された.
		IAsyncAction stroke_width_click_async(IInspectable const&, RoutedEventArgs const&);
		// 値をスライダーのヘッダーに格納する.
		template<UNDO_OP U, int S> void stroke_set_slider_header(const float value);
		// 値をスライダーのヘッダーと、見本の図形に格納する.
		template<UNDO_OP U, int S> void stroke_set_slider(IInspectable const&, RangeBaseValueChangedEventArgs const&);

		//-------------------------------
		// MainPage_smry.cpp
		// 図形の一覧
		//-------------------------------

		// 図形一覧の「閉じる」ボタンが押された.
		void smry_close_click(IInspectable const&, RoutedEventArgs const&);
		// 図形一覧の項目が選択された.
		void smry_selection_changed(IInspectable const& sender, SelectionChangedEventArgs const& e);
		// 図形一覧がロードされた.
		void smry_loaded(IInspectable const& sender, RoutedEventArgs const& e);
		// 編集メニューの「リストの表示」が選択された.
		void mfi_smry_click(IInspectable const&, RoutedEventArgs const&);
		// 図形を一覧に追加する.
		void smry_append(Shape* const s);
		// 一覧の中で図形を入れ替える.
		void smry_arrng(Shape* const s, Shape* const t);
		// 図形一覧を消去する.
		void smry_clear(void);
		// 図形一覧パネルを閉じて消去する.
		void smry_close(void);
		// 図形を一覧に挿入する.
		void smry_insert(Shape* const s, const uint32_t i);
		// 操作を図形一覧に反映する.
		void smry_reflect(const Undo* u);
		// 図形一覧を作成しなおす.
		void smry_remake(void);
		// 図形を一覧から消去する.
		uint32_t smry_remove(Shape* const s);
		// 一覧の項目を選択する.
		//void smry_select(uint32_t i);
		// 一覧の図形を選択する.
		void smry_select(Shape* const s);
		// 一覧の項目を全て選択する.
		void smry_select_all(void);
		// 一覧の最初の項目を選択する.
		void smry_select_head(void);
		// 一覧の最後の項目を選択する.
		void smry_select_tail(void);
		// 一覧の項目を選択解除する.
		//void smry_unselect(uint32_t i);
		// 一覧の図形を選択解除する.
		void smry_unselect(Shape* const s);
		// 一覧の項目を全て選択解除する.
		void smry_unselect_all(void);
		// 一覧の表示を更新する.
		void smry_update(void);

		//-------------------------------
		// MainPage_text.cpp
		// 文字列の編集と検索/置換
		//-------------------------------

		// 文字列検索パネルの「閉じる」ボタンが押された.
		void text_find_close_click(IInspectable const&, RoutedEventArgs const&);
		//　文字列検索パネルの「次を検索」ボタンが押された.
		void text_find_next_click(IInspectable const&, RoutedEventArgs const&);
		// 文字列検索パネルの「すべて置換」ボタンが押された.
		void text_replace_all_click(IInspectable const&, RoutedEventArgs const&);
		// 文字列検索パネルの「置換して次に」ボタンが押された.
		void text_replace_click(IInspectable const&, RoutedEventArgs const&);
		// 文字列の大きさに合わせるフラグに格納する.
		void text_adjust(const bool adjust) noexcept { m_text_adjust = adjust; }
		// 文字列の大きさに合わせるフラグを得る.
		const bool text_adjust(void) const noexcept { return m_text_adjust; }
		// 検索の値をデータリーダーから読み込む.
		void text_find_read(DataReader const& dt_reader);
		// 文字列検索パネルから値を格納する.
		void text_find_set(void);
		// 検索の値をデータリーダーに書き込む.
		void text_find_write(DataWriter const& dt_writer);
		// 編集メニューの「文字列の検索/置換」が選択された.
		void text_find_click(IInspectable const&, RoutedEventArgs const&);
		// 検索文字列が変更された.
		void text_find_what_changed(IInspectable const&, TextChangedEventArgs const&);
		// 図形が持つ文字列を編集する.
		IAsyncAction text_edit_async(ShapeText* s);
		// 編集メニューの「文字列の編集」が選択された.
		void text_edit_click(IInspectable const&, RoutedEventArgs const&);

		//-------------------------------
		// MainPage_thread.cpp
		// ウィンドウ切り替えのハンドラー
		//-------------------------------

		// ウィンドウの実行/停止が切り替わった.
		void thread_activated(IInspectable const& sender, WindowActivatedEventArgs const& args);
		// ウィンドウの表示/非表示が変わった.
		void thread_visibility_changed(CoreWindow const& sender, VisibilityChangedEventArgs const& args);

		//-------------------------------
		// MainPage_tool.cpp
		// 作図ツール
		//-------------------------------

		// 作図メニューの項目が選択された.
		void tool_draw_click(IInspectable const& sender, RoutedEventArgs const&);
		// 作図ツールを得る.
		const TOOL_DRAW tool_draw(void) const noexcept { return m_tool_draw; }
		// 多角形の作図ツールを得る.
		const TOOL_POLY& tool_poly(void) const noexcept { return m_tool_poly; }
		// 作図メニューの状態を読み込む.
		void tool_read(DataReader const& dt_reader);
		// 作図メニューの状態を書き込む.
		void tool_write(DataWriter const& dt_writer);

		//-----------------------------
		// MainPage_undo.cpp
		// 元に戻すとやり直し
		//-----------------------------

		// 元に戻す/やり直しメニュー項目の使用の可否を設定する.
		void undo_menu_enable(void);
		// 編集メニューの「やり直し」が選択された.
		void redo_click(IInspectable const&, RoutedEventArgs const&);
		// 編集メニューの「元に戻す」が選択された.
		void undo_click(IInspectable const&, RoutedEventArgs const&);
		// 操作スタックを消去し, 含まれる操作を破棄する.
		void undo_clear(void);
		// 操作を実行する.
		void undo_exec(Undo* u);
		// 無効な操作の組をポップする.
		bool undo_pop_if_invalid(void);
		// 操作スタックの更新フラグを得る.
		const bool undo_pushed(void) const noexcept { return m_stack_updt; }
		// 値を操作スタックの更新フラグに格納する.
		void undo_pushed(const bool pushed) noexcept { m_stack_updt = pushed; }
		// 図形を追加して, その操作をスタックに積む.
		void undo_push_append(Shape* const s);
		// 図形をグループ図形に追加して, その操作をスタックに積む.
		void undo_push_append(ShapeGroup* const g, Shape* const s);
		// 図形を入れ替えて, その操作をスタックに積む.
		void undo_push_arrng(Shape* const s, Shape* const t);
		// 図形の頂点や制御点の位置をスタックに保存する.
		void undo_push_anchor(Shape* const s, const uint32_t anchor);
		// 図形を挿入して, その操作をスタックに積む.
		void undo_push_insert(Shape* const s, Shape* const s_pos);
		// 図形の位置をスタックに保存してから差分だけ移動する.
		void undo_push_move(const D2D1_POINT_2F diff, const bool all = false);
		// 一連の操作の区切としてヌル操作をスタックに積む.
		void undo_push_null(void);
		// 図形をグループから削除して, その操作をスタックに積む.
		void undo_push_remove(Shape* const g, Shape* const s);
		// 図形を削除して, その操作をスタックに積む.
		void undo_push_remove(Shape* const s);
		// 図形の選択を反転して, その操作をスタックに積む.
		void undo_push_select(Shape* const s);
		// 値を図形へ格納して, その操作をスタックに積む.
		template <UNDO_OP U, typename T> void undo_push_set(Shape* const s, T const& value);
		// 値を選択された図形に格納して, その操作をスタックに積む.
		template <UNDO_OP U, typename T> bool undo_push_set(T const& value);
		// 図形の値をスタックに保存する.
		template <UNDO_OP U> void undo_push_set(Shape* const s);
		// 操作スタックをデータリーダーから読み込む.
		void undo_read(DataReader const& dt_reader);
		// 操作スタックをデータリーダーに書き込む.
		void undo_write(DataWriter const& dt_writer);

		//-------------------------------
		// MainPage_xcvd.cpp
		// 切り取りとコピー, 貼り付け, 削除
		//-------------------------------

		// 編集メニューの「コピー」が選択された.
		IAsyncAction xcvd_copy_click_async(IInspectable const&, RoutedEventArgs const&);
		// 編集メニューの「切り取り」が選択された.
		IAsyncAction xcvd_cut_click_async(IInspectable const&, RoutedEventArgs const&);
		// 編集メニューの「削除」が選択された.
		void xcvd_delete_click(IInspectable const&, RoutedEventArgs const&);
		// 編集メニューの「貼り付け」が選択された.
		IAsyncAction xcvd_paste_click_async(IInspectable const&, RoutedEventArgs const&);
		// クリップボードにデータが含まれているか判定する.
		template <size_t Z> bool xcvd_contains(const winrt::hstring(&formats)[Z]) const
		{
			return xcvd_contains(formats, Z);
		}
		// クリップボードにデータが含まれているか判定する.
		bool xcvd_contains(const winrt::hstring formats[], const size_t f_cnt) const;

		//-------------------------------
		// MainPage_zoom.cpp
		// 表示倍率
		//-------------------------------

		// 用紙メニューの「拡大縮小」>「拡大」が選択された.
		void mfi_zoom_in_clicked(IInspectable const&, RoutedEventArgs const&);
		// 用紙メニューの「拡大縮小」>「縮小」が選択された.
		void mfi_zoom_out_clicked(IInspectable const&, RoutedEventArgs const&);
		// 用紙メニューの「拡大縮小」>「100%に戻す」が選択された.
		void mfi_zoom_reset_click(IInspectable const&, RoutedEventArgs const&);

	};

}

namespace winrt::GraphPaper::factory_implementation
{
	struct MainPage : MainPageT<MainPage, implementation::MainPage>
	{
	};
}
