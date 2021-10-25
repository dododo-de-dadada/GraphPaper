﻿#pragma once
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
// MainPage_arrange.cpp	図形の並び替え
// MainPage_arrow.cpp	矢じるしの形式と寸法
// MainPage_dash.cpp	線の種類
// MainPage_display.cpp	表示デバイスのハンドラー
// MainPage_drawing.cpp	作図ツール
// MainPage_event.cpp	ポインターイベントのハンドラー
// MainPage_file.cpp	ファイルの読み書き
// MainPage_fill.cpp	塗りつぶし
// MainPage_find.cpp	文字列の編集, 検索と置換
// MainPage_font.cpp	書体と文字列の配置
// MainPage_grid.cpp	方眼
// MainPage_group.cpp	グループ化とグループの解除
// NainPage_image.cpp	画像
// NainPage_join.cpp	線分のつなぎ
// MainPage_misc.cpp	長さの単位, 色の表記, ステータスバー, 頂点をくっつける閾値
// MainPage_sample.cpp	見本
// MainPage_scroll.cpp	スクロールバー
// MainPage_select.cpp	図形の選択
// MainPage_sheet.cpp	用紙の属性, 表示倍率, 設定の保存とリセット
// MainPage_status.cpp	ステータスバー
// MainPage_stroke.cpp	線枠
// MainPage_summary.cpp	図形の一覧
// MainPage_text.cpp	文字列の編集と検索/置換
// MainPage_thread.cpp	ウィンドウ切り替えのハンドラー
// MainPage_ustack.cpp	元に戻すとやり直し操作
// MainPage_xcvd.cpp	切り取りとコピー, 文字列の編集など
//------------------------------
#include <ppltasks.h>
#include <winrt/Windows.ApplicationModel.ExtendedExecution.h>
#include <winrt/Windows.UI.Xaml.Input.h>
#include "MainPage.g.h"
#include "undo.h"

namespace winrt::GraphPaper::implementation
{
	extern const winrt::param::hstring CLIPBOARD_FORMAT_SHAPES;	// 図形データのクリップボード書式
	//extern const winrt::param::hstring CLIPBOARD_TIFF;	// TIFF のクリップボード書式 (Windows10 ではたぶん使われない)

	constexpr auto FMT_INCH = L"%.3f";	// インチ単位の書式
	constexpr auto FMT_INCH_UNIT = L"%.3f \u33CC";	// インチ単位の書式
	constexpr auto FMT_MILLI = L"%.3f";	// ミリメートル単位の書式
	constexpr auto FMT_MILLI_UNIT = L"%.3f \u339C";	// ミリメートル単位の書式
	constexpr auto FMT_POINT = L"%.2f";	// ポイント単位の書式
	constexpr auto FMT_POINT_UNIT = L"%.2f pt";	// ポイント単位の書式
	constexpr auto FMT_PIXEL = L"%.1f";	// ピクセル単位の書式
	constexpr auto FMT_PIXEL_UNIT = L"%.1f px";	// ピクセル単位の書式
	constexpr auto FMT_ZOOM = L"%.f%%";	// 倍率の書式
	constexpr auto FMT_GRID = L"%.3f";	// グリッド単位の書式
	constexpr auto FMT_GRID_UNIT = L"%.3f gd";	// グリッド単位の書式
	constexpr auto ICON_INFO = L"glyph_info";	// 情報アイコンの静的リソースのキー
	constexpr auto ICON_ALERT = L"glyph_alert";	// 警告アイコンの静的リソースのキー
	constexpr auto VERT_STICK_DEF_VAL = 2.0f * 6.0f;	// 頂点をくっつける閾値の既定値
	constexpr auto SHEET_SIZE_DEF_VAL = D2D1_SIZE_F{ 8.0F * 96.0F, 11.0F * 96.0F };	// 用紙寸法の既定値 (ピクセル)
	constexpr auto UI_VISIBLE = winrt::Windows::UI::Xaml::Visibility::Visible;	// 表示	
	constexpr auto UI_COLLAPSED = winrt::Windows::UI::Xaml::Visibility::Collapsed;	// 非表示

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
	void conv_col_to_str(const COLOR_CODE c_code, const double value, const size_t t_len, wchar_t t_buf[]) noexcept;

	// 色成分を文字列に変換する.
	template <size_t Z> inline void conv_col_to_str(const COLOR_CODE c_code, const double value, wchar_t(&t_buf)[Z]) noexcept
	{
		conv_col_to_str(c_code, value, Z, t_buf);
	}

	constexpr double UWP_COLOR_MAX = 255.0;	// UWP の色成分の最大値

	// UWP の色を D2D1_COLOR_F に変換する.
	inline void conv_uwp_to_color(const winrt::Windows::UI::Color& a, D2D1_COLOR_F& b) noexcept
	{
		b.r = static_cast<FLOAT>(static_cast<double>(a.R) / UWP_COLOR_MAX);
		b.g = static_cast<FLOAT>(static_cast<double>(a.G) / UWP_COLOR_MAX);
		b.b = static_cast<FLOAT>(static_cast<double>(a.B) / UWP_COLOR_MAX);
		b.a = static_cast<FLOAT>(static_cast<double>(a.A) / UWP_COLOR_MAX);
	}

	//-------------------------------
	// 作図ツール
	//-------------------------------
	enum struct DRAWING_TOOL {
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
	// ポインターボタンの状態
	//-------------------------------
	enum struct EVENT_STATE {
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
	constexpr bool LEN_UNIT_HIDE = false;	// 単位名の非表示

	// 長さを文字列に変換する.
	template <bool B> void conv_len_to_str(const LEN_UNIT len_unit, const float val_pixel, const float dpi, const float g_len, const uint32_t t_len, wchar_t* t_buf) noexcept;

	// 長さを文字列に変換する.
	// B	単位名を付加するか判定する.
	// len_unit	長さの単位
	// val_pixel	ピクセル単位の長さ
	// dpi	DPI
	// g_len	グリッドの大きさ
	// t_buf	文字列を格納した固定配列
	template <bool B, size_t Z> inline void conv_len_to_str(const LEN_UNIT len_unit, const float val_pixel, const float dpi, const float g_len, wchar_t(&t_buf)[Z]) noexcept
	{
		conv_len_to_str<B>(len_unit, val_pixel, dpi, g_len, Z, t_buf);
	}

	//-------------------------------
	// メッセージダイアログのアイコン
	//-------------------------------
	enum struct MSG_ICON {
		ALERT,
		INFO
	};

	//-------------------------------
	// 見本の型
	//-------------------------------
	enum struct SAMPLE_TYPE {
		NONE,	// なし
		IMAGE,	// 画像
		FILL,	// 塗りつぶし
		FONT,	// 書体
		JOIN,	// 線のつなぎ
		MISC,	// その他
		STROKE	// 線枠
	};

	//-------------------------------
	// ステータスバーの状態
	//-------------------------------
	enum struct STATUS_BAR {
		POS = (1 | 2),	// カーソルの位置
		DRAW = 4,	// 作図ツール
		GRID = 8,	// 方眼の大きさ
		SHEET = (16 | 32),	// 用紙の大きさ
		UNIT = 64,	// 単位名
		ZOOM = 128,	// 拡大率
	};
	constexpr STATUS_BAR STATUS_BAR_DEF_VAL = static_cast<STATUS_BAR>(static_cast<uint32_t>(STATUS_BAR::DRAW) | static_cast<uint32_t>(STATUS_BAR::POS) | static_cast<uint32_t>(STATUS_BAR::ZOOM));

	//-------------------------------
	// メインページ
	//-------------------------------
	struct MainPage : MainPageT<MainPage> {
		std::mutex m_d2d_mutex;	// 描画の排他制御
		winrt::hstring m_file_token_mru;	// 最近使ったファイルのトークン

		// 一覧
		std::atomic_bool m_summary_atomic{ false };	// 一覧の排他制御

		// 文字列の編集, 検索と置換
		bool m_edit_text_frame = false;	// 枠の大きさを合わせるか
		wchar_t* m_find_text = nullptr;	// 検索の検索文字列
		wchar_t* m_find_repl = nullptr;	// 検索の置換文字列
		bool m_find_text_case = false;	// 英文字の区別するか
		bool m_find_text_wrap = false;	// 回り込み検索するか

		// ポインターイベント
		D2D1_POINT_2F m_event_pos_curr{ 0.0F, 0.0F };	// ポインターの現在位置
		D2D1_POINT_2F m_event_pos_prev{ 0.0F, 0.0F };	// ポインターの前回位置
		EVENT_STATE m_event_state = EVENT_STATE::BEGIN;	// ポインターの押された状態
		uint32_t m_event_anp_pressed = ANP_TYPE::ANP_SHEET;	// ポインターが押された図形の部位
		D2D1_POINT_2F m_event_pos_pressed{ 0.0F, 0.0F };	// ポインターが押された位置
		Shape* m_event_shape_pressed = nullptr;	// ポインターが押された図形
		Shape* m_event_shape_prev = nullptr;	// 前回ポインターが押された図形
		uint64_t m_event_time_pressed = 0ULL;	// ポインターが押された時刻
		uint64_t m_event_click_time = 0ULL;	// クリックの判定時間 (マイクロ秒)
		double m_event_click_dist = 6.0;	// クリックの判定距離 (DIPs)

		// 作図ツール
		DRAWING_TOOL m_drawing_tool = DRAWING_TOOL::SELECT;	// 作図ツール
		POLY_OPTION m_drawing_poly_opt{ POLY_OPTION_DEFVAL };	// 多角形の作成方法

		// 図形リスト
		uint32_t m_list_sel_cnt = 0;	// 選択された図形の数

		// 図形
		bool m_image_keep_aspect = true;	// 画像の縦横比の維持

		// メイン用紙
		D2D_UI m_main_d2d;	// 用紙の描画環境
		ShapeSheet m_main_sheet;	// メインの用紙
		D2D1_POINT_2F m_main_min{ 0.0F, 0.0F };	// 用紙の左上位置 (値がマイナスのときは, 図形が用紙の外側にある)
		D2D1_POINT_2F m_main_max{ 0.0F, 0.0F };	// 用紙の右下位置 (値が用紙の大きさより大きいときは, 図形が用紙の外側にある)

		// 見本用紙
		D2D_UI m_sample_d2d;	// 見本の描画環境
		ShapeSheet m_sample_sheet;	// 見本の用紙

		// 元に戻す・やり直し操作
		uint32_t m_ustack_rcnt = 0;	// やり直し操作スタックに積まれた組数
		UNDO_STACK m_ustack_redo;	// やり直し操作スタック
		uint32_t m_ustack_ucnt = 0;	// 元に戻す操作スタックに積まれた組数
		UNDO_STACK m_ustack_undo;	// 元に戻す操作スタック
		bool m_ustack_updt = false;	// スタックが更新されたか判定

		// その他
		LEN_UNIT m_len_unit = LEN_UNIT::PIXEL;	// 長さの単位
		COLOR_CODE m_color_code = COLOR_CODE::DEC;	// 色成分の書式
		float m_vert_stick = VERT_STICK_DEF_VAL;	// 頂点をくっつける閾値
		STATUS_BAR m_status_bar = STATUS_BAR_DEF_VAL;	// ステータスバーの状態


		// スレッド
		bool m_thread_activated = false;	// アクティベートされた初回を判定
		bool m_thread_win_visible = false;	// ウィンドウが表示されてるか判定

		// コンテキストメニュー
		winrt::Windows::UI::Xaml::Controls::MenuFlyout m_menu_stroke{ nullptr };	// 線枠コンテキストメニュー
		winrt::Windows::UI::Xaml::Controls::MenuFlyout m_menu_fill{ nullptr };	// 塗りつぶしコンテキストメニュー
		winrt::Windows::UI::Xaml::Controls::MenuFlyout m_menu_font{ nullptr };	// 書体コンテキストメニュー
		winrt::Windows::UI::Xaml::Controls::MenuFlyout m_menu_sheet{ nullptr };	// 用紙コンテキストメニュー
		winrt::Windows::UI::Xaml::Controls::MenuFlyout m_menu_ungroup{ nullptr };	// グループ解除コンテキストメニュー
		winrt::Windows::UI::Xaml::Controls::MenuFlyout m_menu_ruler{ nullptr };	// 定規コンテキストメニュー
		winrt::Windows::UI::Xaml::Controls::MenuFlyout m_menu_image{ nullptr };	// 画像コンテキストメニュー

		// ハンドラートークン
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

		void sheet_panel_loading(IInspectable const& sender, winrt::Windows::Foundation::IInspectable const&);
		// 確認ダイアログを表示してその応答を得る.
		winrt::Windows::Foundation::IAsyncOperation<bool> ask_for_conf_async(void);
		// ファイルメニューの「終了」が選択された
		winrt::Windows::Foundation::IAsyncAction exit_click_async(IInspectable const&, winrt::Windows::UI::Xaml::RoutedEventArgs const&);
		// メインページを作成する.
		MainPage(void);
		// メッセージダイアログを表示する.
		void message_show(winrt::hstring const& glyph_key, winrt::hstring const& message_key, winrt::hstring const& desc_key);
		// ウィンドウの閉じるボタンが押された.
		void navi_close_requested(IInspectable const&, winrt::Windows::UI::Core::Preview::SystemNavigationCloseRequestedPreviewEventArgs const& args)
		{
			args.Handled(true);
			auto _{ exit_click_async(nullptr, nullptr) };
		}
		// ファイルメニューの「新規」が選択された
		winrt::Windows::Foundation::IAsyncAction new_click_async(IInspectable const&, winrt::Windows::UI::Xaml::RoutedEventArgs const&);

		//-------------------------------
		// MainPage_app.cpp
		// アプリケーションの中断と再開
		//-------------------------------

		// アプリケーションがバックグラウンドに移った.
		void app_entered_background(IInspectable const& sender, winrt::Windows::ApplicationModel::EnteredBackgroundEventArgs const& args);
		// アプリケーションがバックグラウンドに移った.
		void app_leaving_background(IInspectable const& sender, winrt::Windows::ApplicationModel::LeavingBackgroundEventArgs const& args);
		// アプリケーションが再開された.
		winrt::Windows::Foundation::IAsyncAction app_resuming_async(IInspectable const&, IInspectable const&);
		// アプリケーションが中断された.
		winrt::Windows::Foundation::IAsyncAction app_suspending_async(IInspectable const&, winrt::Windows::ApplicationModel::SuspendingEventArgs const& args);

		//-------------------------------
		// MainPage_join.cpp
		// 線分のつなぎと端
		//-------------------------------

		// 線枠メニューの「端の種類」に印をつける.
		void cap_style_is_checked(const CAP_STYLE& s_cap);
		// 線枠メニューの「端の種類」が選択された.
		void cap_style_click(IInspectable const& sender, winrt::Windows::UI::Xaml::RoutedEventArgs const&);
		// 線枠メニューの「つなぎの種類」>「額ぶちの制限」が選択された.
		winrt::Windows::Foundation::IAsyncAction join_limit_click_async(IInspectable const&, winrt::Windows::UI::Xaml::RoutedEventArgs const&);
		// 線枠メニューの「つなぎの種類」に印をつける.
		void join_style_is_checked(const D2D1_LINE_JOIN s_join);
		// 線枠メニューの「つなぎの形式」が選択された.
		void join_style_click(IInspectable const& sender, winrt::Windows::UI::Xaml::RoutedEventArgs const&);
		// 値をスライダーのヘッダーに格納する.
		template <UNDO_OP U, int S> void join_slider_set_header(const float value);
		// スライダーの値が変更された.
		template <UNDO_OP U, int S> void join_slider_value_changed(IInspectable const&, winrt::Windows::UI::Xaml::Controls::Primitives::RangeBaseValueChangedEventArgs const& args);

		//-------------------------------
		// MainPage_arrange.cpp
		// 図形の並び替え
		//-------------------------------

		// 選択された図形を次または前の図形と入れ替える.
		template<typename T> void arrange_order(void);
		// 編集メニューの「前面に移動」が選択された.
		void arrange_bring_forward_click(IInspectable const&, winrt::Windows::UI::Xaml::RoutedEventArgs const&);
		// 編集メニューの「最前面に移動」が選択された.
		void arrange_bring_to_front_click(IInspectable const&, winrt::Windows::UI::Xaml::RoutedEventArgs const&);
		// 編集メニューの「ひとつ背面に移動」が選択された.
		void arrange_send_backward_click(IInspectable const&, winrt::Windows::UI::Xaml::RoutedEventArgs const&);
		// 編集メニューの「最背面に移動」が選択された.
		void arrange_send_to_back_click(IInspectable const&, winrt::Windows::UI::Xaml::RoutedEventArgs const&);

		//-------------------------------
		// MainPage_arrow.cpp
		// 矢じるしの形式と寸法
		//-------------------------------

		// 線枠メニューの「矢じるしの種類」に印をつける.
		void arrow_style_is_checked(const ARROW_STYLE value);
		// 線枠メニューの「矢じるしの種類」が選択された.
		void arrow_style_click(IInspectable const& sender, winrt::Windows::UI::Xaml::RoutedEventArgs const&);
		// 線枠メニューの「矢じるしの大きさ」が選択された.
		winrt::Windows::Foundation::IAsyncAction arrow_size_click_async(IInspectable const&, winrt::Windows::UI::Xaml::RoutedEventArgs const&);
		// 値をスライダーのヘッダーに格納する.
		template <UNDO_OP U, int S> 
		void arrow_slider_set_header(const float value);
		// スライダーの値が変更された.
		template <UNDO_OP U, int S> 
		void arrow_slider_value_changed(IInspectable const&, winrt::Windows::UI::Xaml::Controls::Primitives::RangeBaseValueChangedEventArgs const&);

		//-------------------------------
		// MainPage_disp.cpp
		// 表示デバイスのハンドラー
		//-------------------------------

		// ディスプレーの再描画が必要となった場合のハンドラー
		void display_contents_invalidated(winrt::Windows::Graphics::Display::DisplayInformation const& sender, IInspectable const& args);
		// ディスプレーの DPI が変更された場合のハンドラー
		void display_dpi_changed(winrt::Windows::Graphics::Display::DisplayInformation const& sender, IInspectable const& args);
		// ディスプレーの向きが変更された場合のハンドラー
		void display_orientation_changed(winrt::Windows::Graphics::Display::DisplayInformation const& sender, IInspectable const& args);

		//-------------------------------
		// MainPage_event.cpp
		// ポインターイベントのハンドラー
		//-------------------------------

		// ポインターのボタンが上げられた.
		void event_canceled(IInspectable const& sender, winrt::Windows::UI::Xaml::Input::PointerRoutedEventArgs const& args);
		// ポインターがページのスワップチェーンパネルの中に入った.
		void event_entered(IInspectable const& sender, winrt::Windows::UI::Xaml::Input::PointerRoutedEventArgs const& args);
		// ポインターがページのスワップチェーンパネルから出た.
		void event_exited(IInspectable const& sender, winrt::Windows::UI::Xaml::Input::PointerRoutedEventArgs const& args);
		// 図形の作成を終了する.
		void event_finish_creating(const D2D1_POINT_2F b_pos, const D2D1_POINT_2F b_vec);
		// 文字列図形の作成を終了する.
		winrt::Windows::Foundation::IAsyncAction event_finish_creating_text_async(const D2D1_POINT_2F b_pos, const D2D1_POINT_2F b_vec);
		// 押された図形の変形を終了する.
		void event_finish_forming(void);
		// 選択された図形の移動を終了する.
		void event_finish_moving(void);
		// 範囲選択を終了する.
		void event_finish_selecting_area(const winrt::Windows::System::VirtualKeyModifiers k_mod);
		// ポインターが動いた.
		void event_moved(IInspectable const& sender, winrt::Windows::UI::Xaml::Input::PointerRoutedEventArgs const& args);
		// ポインターのボタンが押された.
		void event_pressed(IInspectable const& sender, winrt::Windows::UI::Xaml::Input::PointerRoutedEventArgs const& args);
		// ポインターのボタンが上げられた.
		void event_released(IInspectable const& sender, winrt::Windows::UI::Xaml::Input::PointerRoutedEventArgs const& args);
		// ポインターの形状を設定する.
		void event_set_curs_style(void);
		// ポインターの現在位置を設定する.
		void event_set_pos_cur(winrt::Windows::UI::Xaml::Input::PointerRoutedEventArgs const& args);
		// コンテキストメニューを表示する.
		void event_show_context_menu(void);
		// ポインターのホイールボタンが操作された.
		void event_wheel_changed(IInspectable const& sender, winrt::Windows::UI::Xaml::Input::PointerRoutedEventArgs const& args);

		//-------------------------------
		// MainPage_file.cpp
		// ファイルの読み書き
		//-------------------------------

		// ファイルシステムへのアクセス権を確認して, 設定を促す.
		//IAsyncAction file_check_broad_access(void) const;
		// ストレージファイルを非同期に読む.
		template <bool SUSPEND, bool SETTEING>
		winrt::Windows::Foundation::IAsyncOperation<winrt::hresult> file_read_async(winrt::Windows::Storage::StorageFile s_file) noexcept;
		// 名前を付けてファイルに非同期に保存する
		winrt::Windows::Foundation::IAsyncOperation<winrt::hresult> file_save_as_async(const bool svg_allowed = false) noexcept;
		// ファイルに非同期に保存する
		winrt::Windows::Foundation::IAsyncOperation<winrt::hresult> file_save_async(void) noexcept;
		// 待機カーソルを表示, 表示する前のカーソルを得る.
		winrt::Windows::UI::Core::CoreCursor file_wait_cursor(void) const;
		// 図形データをストレージファイルに非同期に書き込む.
		template <bool SUSPEND, bool SETTING>
		winrt::Windows::Foundation::IAsyncOperation<winrt::hresult> file_write_gpf_async(winrt::Windows::Storage::StorageFile s_file);
		// 図形データを SVG としてストレージファイルに非同期に書き込む.
		winrt::Windows::Foundation::IAsyncOperation<winrt::hresult> file_write_svg_async(winrt::Windows::Storage::StorageFile s_file);
		// ファイルに画像図形の画像を保存する.
		winrt::Windows::Foundation::IAsyncOperation<winrt::hresult> file_save_img_async(ShapeImage* s, const wchar_t suggested_name[], /*-->*/wchar_t img_name[], const size_t name_len);
		// ファイルの読み込みが終了した.
		void file_finish_reading(void);
		// ファイルメニューの「画像をインポートする」が選択された
		winrt::Windows::Foundation::IAsyncAction file_import_img_click(IInspectable const&, winrt::Windows::UI::Xaml::RoutedEventArgs const&);
		// ファイルメニューの「開く」が選択された
		winrt::Windows::Foundation::IAsyncAction file_open_click_async(IInspectable const&, winrt::Windows::UI::Xaml::RoutedEventArgs const&);
		// ファイルメニューの「名前を付けて保存」が選択された
		void file_save_as_click(IInspectable const&, winrt::Windows::UI::Xaml::RoutedEventArgs const&);
		// ファイルメニューの「上書き保存」が選択された
		void file_save_click(IInspectable const&, winrt::Windows::UI::Xaml::RoutedEventArgs const&);
		// 最近使ったファイルを非同期に読む.
		winrt::Windows::Foundation::IAsyncAction file_recent_read_async(const uint32_t i);
		// ファイルメニューの「最近使ったファイル 」のサブ項目が選択された
		void file_recent_click(IInspectable const&, winrt::Windows::UI::Xaml::RoutedEventArgs const&);
		// 最近使ったファイルにストレージファイルを追加する.
		void file_recent_add(winrt::Windows::Storage::StorageFile const& s_file);
		// 最近使ったファイルのトークンからストレージファイルを得る.
		winrt::Windows::Foundation::IAsyncOperation<winrt::Windows::Storage::StorageFile> file_recent_get_async(const winrt::hstring token);
		// 最近使ったファイルのメニュー項目を更新する.
		void file_recent_update_menu(void);

		//-------------------------------
		// MainPage_fill.cpp
		// 塗りつぶし
		//-------------------------------

		// 塗りつぶしメニューの「色」が選択された.
		winrt::Windows::Foundation::IAsyncAction fill_color_click_async(IInspectable const&, winrt::Windows::UI::Xaml::RoutedEventArgs const&);
		// スライダーの値が変更された.
		template <UNDO_OP U, int S>
		void fill_slider_value_changed(IInspectable const&, winrt::Windows::UI::Xaml::Controls::Primitives::RangeBaseValueChangedEventArgs const&);
		// 値をスライダーのヘッダーに格納する.
		template <UNDO_OP U, int S>
		void fill_slider_set_header(const float value);

		//-------------------------------
		// MainPage_find.cpp
		// 文字列の編集と, 検索/置換
		//-------------------------------

		// 図形が持つ文字列を編集する.
		winrt::Windows::Foundation::IAsyncAction edit_text_async(ShapeText* s);
		// 編集メニューの「文字列の編集」が選択された.
		void edit_text_click(IInspectable const&, winrt::Windows::UI::Xaml::RoutedEventArgs const&);
		// 文字列検索パネルの「閉じる」ボタンが押された.
		void find_text_close_click(IInspectable const&, winrt::Windows::UI::Xaml::RoutedEventArgs const&);
		//　文字列検索パネルの「次を検索」ボタンが押された.
		void find_text_next_click(IInspectable const&, winrt::Windows::UI::Xaml::RoutedEventArgs const&);
		// 文字列検索パネルの「すべて置換」ボタンが押された.
		void find_replace_all_click(IInspectable const&, winrt::Windows::UI::Xaml::RoutedEventArgs const&);
		// 文字列検索パネルの「置換して次に」ボタンが押された.
		void find_replace_click(IInspectable const&, winrt::Windows::UI::Xaml::RoutedEventArgs const&);
		// データリーダーから検索の値を読み込む.
		//void find_text_read(DataReader const& dt_reader);
		// 文字列検索パネルから値を格納する.
		void find_text_set(void);
		// 編集メニューの「文字列の検索/置換」が選択された.
		void find_text_click(IInspectable const&, winrt::Windows::UI::Xaml::RoutedEventArgs const&);
		// 検索文字列が変更された.
		void find_text_what_changed(IInspectable const&, winrt::Windows::UI::Xaml::Controls::TextChangedEventArgs const&);

		//-------------------------------
		//　MainPage_font.cpp
		//　書体
		//-------------------------------

		// 書体メニューの「字体」に印をつける.
		void font_style_is_checked(const DWRITE_FONT_STYLE f_style);
		// 書体メニューの「色」が選択された.
		winrt::Windows::Foundation::IAsyncAction font_color_click_async(IInspectable const&, winrt::Windows::UI::Xaml::RoutedEventArgs const&);
		// 書体メニューの「書体名」が選択された.
		winrt::Windows::Foundation::IAsyncAction font_family_click_async(IInspectable const&, winrt::Windows::UI::Xaml::RoutedEventArgs const&);
		// 書体メニューの「イタリック体」が選択された.
		void font_style_italic_click(IInspectable const&, winrt::Windows::UI::Xaml::RoutedEventArgs const&);
		// 書体メニューの「標準」が選択された.
		void font_style_normal_click(IInspectable const&, winrt::Windows::UI::Xaml::RoutedEventArgs const&);
		// 書体メニューの「斜体」が選択された.
		void font_style_oblique_click(IInspectable const&, winrt::Windows::UI::Xaml::RoutedEventArgs const&);
		// 書体メニューの「大きさ」が選択された.
		winrt::Windows::Foundation::IAsyncAction font_size_click_async(IInspectable const&, winrt::Windows::UI::Xaml::RoutedEventArgs const&);
		// 書体メニューの「伸縮」が選択された.
		winrt::Windows::Foundation::IAsyncAction font_stretch_click_async(IInspectable const&, winrt::Windows::UI::Xaml::RoutedEventArgs const&);
		// 書体メニューの「太さ」が選択された.
		winrt::Windows::Foundation::IAsyncAction font_weight_click_async(IInspectable const&, winrt::Windows::UI::Xaml::RoutedEventArgs const&);
		// 値をスライダーのヘッダーに格納する.
		template <UNDO_OP U, int S>
		void font_slider_set_header(const float value);
		// スライダーの値が変更された.
		template <UNDO_OP U, int S>
		void font_slider_value_changed(IInspectable const&, winrt::Windows::UI::Xaml::Controls::Primitives::RangeBaseValueChangedEventArgs const&);

		//-------------------------------
		// MainPage_grid.cpp
		// 方眼
		//-------------------------------

		// 用紙メニューの「方眼の強調」に印をつける.
		void grid_emph_is_checked(const GRID_EMPH& g_emph);
		// 用紙メニューの「方眼の表示」に印をつける.
		void grid_show_is_checked(const GRID_SHOW g_show);
		// 用紙メニューの「方眼の大きさ」>「大きさ」が選択された.
		winrt::Windows::Foundation::IAsyncAction grid_len_click_async(IInspectable const&, winrt::Windows::UI::Xaml::RoutedEventArgs const&);
		// 用紙メニューの「方眼の大きさ」>「狭める」が選択された.
		void grid_len_con_click(IInspectable const&, winrt::Windows::UI::Xaml::RoutedEventArgs const&);
		// 用紙メニューの「方眼の大きさ」>「広げる」が選択された.
		void grid_len_exp_click(IInspectable const&, winrt::Windows::UI::Xaml::RoutedEventArgs const&);
		// 用紙メニューの「方眼の色」が選択された.
		winrt::Windows::Foundation::IAsyncAction grid_color_click_async(IInspectable const&, winrt::Windows::UI::Xaml::RoutedEventArgs const&);
		// 用紙メニューの「方眼の強調」が選択された.
		void grid_emph_click(IInspectable const& sender, winrt::Windows::UI::Xaml::RoutedEventArgs const&);
		// 用紙メニューの「方眼の表示」が選択された.
		void grid_show_click(IInspectable const& sender, winrt::Windows::UI::Xaml::RoutedEventArgs const&);
		// 用紙メニューの「方眼に合わせる」が選択された.
		void grid_snap_click(IInspectable const&, winrt::Windows::UI::Xaml::RoutedEventArgs const&);
		// 値をスライダーのヘッダーと図形に格納する.
		template <UNDO_OP U, int S> void grid_slider_set_header(const float value);
		// スライダーの値が変更された.
		template <UNDO_OP U, int S> void grid_slider_value_changed(IInspectable const&, winrt::Windows::UI::Xaml::Controls::Primitives::RangeBaseValueChangedEventArgs const&);

		//-------------------------------
		// MainPage_group.cpp
		// グループ化とグループの解除
		//-------------------------------

		// 編集メニューの「グループ化」が選択された.
		void group_click(IInspectable const&, winrt::Windows::UI::Xaml::RoutedEventArgs const&);
		// 編集メニューの「グループの解除」が選択された.
		void ungroup_click(IInspectable const&, winrt::Windows::UI::Xaml::RoutedEventArgs const&);

		//-------------------------------
		//　MainPage_keyacc.cpp
		//　キーアクセラレーターのハンドラー
		//-------------------------------

		//　Cntrol + PgDn が押された.
		//void kacc_bring_forward_invoked(IInspectable const&, KeyboardAcceleratorInvokedEventArgs const&);
		//　Cntrol + End が押された.
		//void kacc_bring_to_front_invoked(IInspectable const&, KeyboardAcceleratorInvokedEventArgs const&);
		//　Cntrol + C が押された.
		//void kacc_copy_invoked(IInspectable const&, KeyboardAcceleratorInvokedEventArgs const&);
		//　Cntrol + X が押された.
		//void kacc_cut_invoked(IInspectable const&, KeyboardAcceleratorInvokedEventArgs const&);
		//　Delete が押された.
		//void kacc_delete_invoked(IInspectable const&, KeyboardAcceleratorInvokedEventArgs const&);
		//　Cntrol + E が押された.
		//void kacc_edit_text_invoked(IInspectable const&, KeyboardAcceleratorInvokedEventArgs const&);
		//　Cntrol + F が押された.
		//void kacc_find_text_invoked(IInspectable const&, KeyboardAcceleratorInvokedEventArgs const&);
		//　Cntrol + G が押された.
		//void kacc_group_invoked(IInspectable const&, KeyboardAcceleratorInvokedEventArgs const&);
		//　Cntrol + N が押された.
		//void kacc_new_invoked(IInspectable const&, KeyboardAcceleratorInvokedEventArgs const&);
		//　Cntrol + O が押された.
		//void kacc_open_invoked(IInspectable const&, KeyboardAcceleratorInvokedEventArgs const&);
		//　Cntrol + V が押された.
		//void kacc_paste_invoked(IInspectable const&, KeyboardAcceleratorInvokedEventArgs const&);
		//　Cntrol + Y が押された.
		//void kacc_redo_invoked(IInspectable const&, KeyboardAcceleratorInvokedEventArgs const&);
		//　Cntrol + Shift + S が押された.
		//void kacc_save_as_invoked(IInspectable const&, KeyboardAcceleratorInvokedEventArgs const&);
		//　Cntrol + S が押された.
		//void kacc_save_invoked(IInspectable const&, KeyboardAcceleratorInvokedEventArgs const&);
		//　Cntrol + A が押された.
		//void kacc_select_all_invoked(IInspectable const&, KeyboardAcceleratorInvokedEventArgs const&);
		//　Cntrol + PgUp が押された.
		//void kacc_send_backward_invoked(IInspectable const&, KeyboardAcceleratorInvokedEventArgs const&);
		//　Cntrol + Home が押された.
		//void kacc_send_to_back_invoked(IInspectable const&, KeyboardAcceleratorInvokedEventArgs const&);
		//　Cntrol + L が押された.
		//void kacc_summaty_invoked(IInspectable const&, KeyboardAcceleratorInvokedEventArgs const&);
		//　Cntrol + Z が押された.
		//void kacc_undo_invoked(IInspectable const&, KeyboardAcceleratorInvokedEventArgs const&);
		//　Cntrol + U が押された.
		//void kacc_ungroup_invoked(IInspectable const&, KeyboardAcceleratorInvokedEventArgs const&);
		//　Cntrol + 0 が押された.
		//void kacc_zoom_reset_invoked(IInspectable const&, KeyboardAcceleratorInvokedEventArgs const&);

		//-----------------------------
		// MainPage_image.cpp
		// 画像
		//-----------------------------

		// 画像メニューの「縦横比を変えない」が選択された.
		void image_keep_aspect_click(IInspectable const&, winrt::Windows::UI::Xaml::RoutedEventArgs const&) noexcept;
		// 画像メニューの「縦横比を変えない」に印をつける.
		void image_keep_aspect_is_checked(const bool keep_aspect);
		// 画像メニューの「元の画像に戻す」が選択された.
		void image_revert_origin_click(IInspectable const&, winrt::Windows::UI::Xaml::RoutedEventArgs const&) noexcept;
		// 画像メニューの「不透明度...」が選択された.
		winrt::Windows::Foundation::IAsyncAction image_opac_click_async(IInspectable const&, winrt::Windows::UI::Xaml::RoutedEventArgs const&);
		// 値をスライダーのヘッダーに格納する.
		template <UNDO_OP U, int S>
		void image_slider_set_header(const float value);
		// スライダーの値が変更された.
		template <UNDO_OP U, int S>
		void image_slider_value_changed(IInspectable const&, winrt::Windows::UI::Xaml::Controls::Primitives::RangeBaseValueChangedEventArgs const& args);

		//-----------------------------
		// MainPage_misc.cpp
		// その他
		// 長さの単位, 色の表記, ステータスバー, バージョン情報
		//-----------------------------

		winrt::Windows::Foundation::IAsyncAction about_graph_paper_click(IInspectable const&, winrt::Windows::UI::Xaml::RoutedEventArgs const&);
		// その他メニューの「色の表記」に印をつける.
		void color_code_is_checked(const COLOR_CODE c_code);
		// その他メニューの「色の表記」のサブ項目が選択された.
		void color_code_click(IInspectable const& sender, winrt::Windows::UI::Xaml::RoutedEventArgs const&);
		// その他メニューの「頂点をくっつける...」が選択された.
		winrt::Windows::Foundation::IAsyncAction stick_to_vertex_click_async(IInspectable const&, winrt::Windows::UI::Xaml::RoutedEventArgs const&) noexcept;
		// 値をスライダーのヘッダーに格納する.
		void misc_vert_stick_set_header(const float value) noexcept;
		// スライダーの値が変更された.
		void misc_vert_stick_value_changed(IInspectable const&, winrt::Windows::UI::Xaml::Controls::Primitives::RangeBaseValueChangedEventArgs const& args) noexcept;
		// その他メニューの「長さの単位」に印をつける.
		void len_unit_is_checked(const LEN_UNIT l_unit);
		// その他メニューの「長さの単位」のサブ項目が選択された.
		void len_unit_click(IInspectable const&, winrt::Windows::UI::Xaml::RoutedEventArgs const&);

		//-------------------------------
		// MainPage_sample.cpp
		// 見本
		//-------------------------------

		//D2D_UI& sample_dx(void) { return m_sample_d2d; }
		// 見本ダイアログが開かれた.
		void sample_dialog_opened(winrt::Windows::UI::Xaml::Controls::ContentDialog const& sender, winrt::Windows::UI::Xaml::Controls::ContentDialogOpenedEventArgs const& args);
		// 見本ダイアログが開かれた.
		void sample_dialog_closed(winrt::Windows::UI::Xaml::Controls::ContentDialog const& sender, winrt::Windows::UI::Xaml::Controls::ContentDialogClosedEventArgs const& args);
		// 見本を表示する]
		void sample_draw(void);
		// 見本のスワップチェーンパネルの大きさが変わった.
		void sample_panel_size_changed(IInspectable const&, winrt::Windows::UI::Xaml::SizeChangedEventArgs const&);
		//　リストビュー「見本リスト」がロードされた.
		void sample_list_view_loaded(IInspectable const&, winrt::Windows::UI::Xaml::RoutedEventArgs const&);
		winrt::Windows::Foundation::IAsyncAction sample_image_load_async(const float samp_w, const float samp_h);
		void sample_panel_loaded(IInspectable const& sender, winrt::Windows::UI::Xaml::RoutedEventArgs const&);
		void sample_panel_scale_changed(IInspectable const&, IInspectable const&);
		template<int S> void sample_set_slider_header(const winrt::hstring& text);

		//-------------------------------
		// MainPage_scroll.cpp
		// スクロールバー
		//-------------------------------

		// スクロールバーが操作された
		void scroll(IInspectable const& sender, winrt::Windows::UI::Xaml::Controls::Primitives::ScrollEventArgs const& args);
		// スクロールバーの値を設定する.
		void scroll_set(const double aw, const double ah);
		// 図形が表示されるようパネルをスクロールする.
		bool scroll_to(const Shape* s);

		//-------------------------------
		// MainPage_select.cpp
		// 図形の選択
		//-------------------------------

		// 編集メニューの「すべて選択」が選択された.
		void select_all_click(IInspectable const&, winrt::Windows::UI::Xaml::RoutedEventArgs const&);
		// 領域に含まれる図形を選択し, 含まれない図形の選択を解除する.
		bool select_area(const D2D1_POINT_2F area_min, const D2D1_POINT_2F area_max);
		// 次の図形を選択する.
		//template <VirtualKeyModifiers M, VirtualKey K> void select_next_shape(void);
		// 範囲の中の図形を選択して, それ以外の図形の選択をはずす.
		bool select_range(Shape* const s_from, Shape* const s_to);
		// 図形を選択する.
		void select_shape(Shape* const s, const winrt::Windows::System::VirtualKeyModifiers k_mod);
		// 領域に含まれる図形の選択を反転する.
		bool toggle_area(const D2D1_POINT_2F area_min, const D2D1_POINT_2F area_max);
		// すべての図形の選択を解除する.
		bool unselect_all(const bool t_range_only = false);
		//　Shft + 下矢印キーが押された.
		//void select_range_next_invoked(IInspectable const&, KeyboardAcceleratorInvokedEventArgs const&);
		//　Shift + 上矢印キーが押された.
		//void select_range_prev_invoked(IInspectable const&, KeyboardAcceleratorInvokedEventArgs const&);
		//　下矢印キーが押された.
		//void select_shape_next_invoked(IInspectable const&, KeyboardAcceleratorInvokedEventArgs const&);
		//　上矢印キーが押された.
		//void select_shape_prev_invoked(IInspectable const&, KeyboardAcceleratorInvokedEventArgs const&);
		//　Escape が押された.
		void select_tool_invoked(IInspectable const&, winrt::Windows::UI::Xaml::Input::KeyboardAcceleratorInvokedEventArgs const&);

		//-------------------------------
		// MainPage_sheet.cpp
		// 用紙の属性, 表示倍率, 設定の保存と削除
		//-------------------------------

		void sheet_xaml_root_changed(winrt::Windows::UI::Xaml::XamlRoot const&, winrt::Windows::UI::Xaml::XamlRootChangedEventArgs const& args);

		// 図形の属性関連に印をつける.
		void sheet_attr_is_checked(void) noexcept;
		// 用紙の属性を初期化する.
		void sheet_init(void) noexcept;
		// 用紙メニューの「用紙の色」が選択された.
		winrt::Windows::Foundation::IAsyncAction sheet_color_click_async(IInspectable const&, winrt::Windows::UI::Xaml::RoutedEventArgs const&);
		// 用紙メニューの「用紙の大きさ」が選択された
		winrt::Windows::Foundation::IAsyncAction sheet_size_click_async(IInspectable const&, winrt::Windows::UI::Xaml::RoutedEventArgs const&);
		// 用紙の左上位置と右下位置を更新する.
		void sheet_update_bbox(void) noexcept;
		// 図形をもとに用紙の左上位置と右下位置を更新する.
		void sheet_update_bbox(const Shape* s) noexcept;
		// 用紙を表示する.
		void sheet_draw(void);
		// 前景色を得る.
		const D2D1_COLOR_F& sheet_foreground(void) const noexcept;
		// 保存された用紙とその他の属性を読み込む.
		winrt::Windows::Foundation::IAsyncOperation<winrt::hresult> sheet_pref_load_async(void);
		// 用紙メニューの「用紙設定をリセット」が選択された.
		winrt::Windows::Foundation::IAsyncAction sheet_pref_reset_click_async(IInspectable const&, winrt::Windows::UI::Xaml::RoutedEventArgs const&);
		// 用紙メニューの「用紙設定を保存」が選択された.
		winrt::Windows::Foundation::IAsyncAction sheet_pref_save_click_async(IInspectable const&, winrt::Windows::UI::Xaml::RoutedEventArgs const&);
		// 値をスライダーのヘッダーに格納する.
		template <UNDO_OP U, int S> void sheet_slider_set_header(const float value);
		// スライダーの値が変更された.
		template <UNDO_OP U, int S> void sheet_slider_value_changed(IInspectable const&, winrt::Windows::UI::Xaml::Controls::Primitives::RangeBaseValueChangedEventArgs const&);
		// 用紙の大きさの最大値 (ピクセル) を得る.
		constexpr float sheet_size_max(void) const noexcept { return 32767.0F; }
		// 用紙のスワップチェーンパネルがロードされた.
		void sheet_panel_loaded(IInspectable const& sender, winrt::Windows::UI::Xaml::RoutedEventArgs const& args);
		// 用紙のスワップチェーンパネルの寸法が変わった.
		void sheet_panel_size_changed(IInspectable const& sender, winrt::Windows::UI::Xaml::SizeChangedEventArgs const& args);
		// 用紙のスワップチェーンパネルの寸法が変わった.
		void sheet_panel_scale_changed(IInspectable const& sender, IInspectable const&);
		// 用紙のスワップチェーンパネルの大きさを設定する.
		void sheet_panle_size(void);
		// 用紙寸法ダイアログの「用紙の幅」「用紙の高さ」テキストボックスの値が変更された.
		void sheet_size_text_changed(IInspectable const&, winrt::Windows::UI::Xaml::Controls::TextChangedEventArgs const&);
		// 用紙メニューの「表示倍率」が選択された.
		void sheet_zoom_click(IInspectable const& sender, winrt::Windows::UI::Xaml::RoutedEventArgs const&);
		// 表示を拡大または縮小する.
		void sheet_zoom_delta(const int32_t delta) noexcept;

		//-------------------------------
		// MainPage_status.cpp
		// ステータスバー
		//-------------------------------

		// その他メニューの「ステータスバー」が選択された.
		void status_bar_click(IInspectable const&, winrt::Windows::UI::Xaml::RoutedEventArgs const&);
		// その他メニューの「ステータスバー」に印をつける.
		void status_bar_is_checked(const STATUS_BAR a);
		// 列挙型を OR 演算する.
		//static STATUS_BAR status_or(const STATUS_BAR a, const STATUS_BAR b) noexcept;
		// ポインターの位置をステータスバーに格納する.
		void status_bar_set_pos(void);
		// 作図ツールをステータスバーに格納する.
		void status_bar_set_draw(void);
		// 方眼の大きさをステータスバーに格納する.
		void status_bar_set_grid(void);
		// 用紙の大きさをステータスバーに格納する.
		void status_bar_set_sheet(void);
		// 単位をステータスバーに格納する.
		void status_bar_set_unit(void);
		// 拡大率をステータスバーに格納する.
		void status_bar_set_zoom(void);

		//------------------------------
		// MainPage_dash.cpp
		// 破線
		//------------------------------

		// 線枠メニューの「種類」のサブ項目が選択された.
		void dash_style_click(IInspectable const& sender, winrt::Windows::UI::Xaml::RoutedEventArgs const&);
		// 線枠メニューの「種類」に印をつける.
		void dash_style_is_checked(const D2D1_DASH_STYLE d_style);
		// 線枠メニューの「破線の配列」が選択された.
		winrt::Windows::Foundation::IAsyncAction dash_patt_click_async(IInspectable const&, winrt::Windows::UI::Xaml::RoutedEventArgs const&);
		// 値をスライダーのヘッダーに格納する.
		template<UNDO_OP U, int S> void dash_slider_set_header(const float value);
		// スライダーの値が変更された.
		template<UNDO_OP U, int S> void dash_slider_value_changed(IInspectable const&, winrt::Windows::UI::Xaml::Controls::Primitives::RangeBaseValueChangedEventArgs const&);

		//------------------------------
		// MainPage_stroke.cpp
		// 線枠の色, 太さ
		//------------------------------

		// 線枠メニューの「色」が選択された.
		winrt::Windows::Foundation::IAsyncAction stroke_color_click_async(IInspectable const&, winrt::Windows::UI::Xaml::RoutedEventArgs const&);
		// 線枠メニューの「太さ」が選択された.
		winrt::Windows::Foundation::IAsyncAction stroke_width_click_async(IInspectable const&, winrt::Windows::UI::Xaml::RoutedEventArgs const&);
		// 値をスライダーのヘッダーに格納する.
		template<UNDO_OP U, int S> void stroke_slider_set_header(const float value);
		// スライダーの値が変更された.
		template<UNDO_OP U, int S> void stroke_slider_value_changed(IInspectable const&, winrt::Windows::UI::Xaml::Controls::Primitives::RangeBaseValueChangedEventArgs const&);

		//-------------------------------
		// MainPage_summary.cpp
		// 一覧
		//-------------------------------

		// 図形を一覧に追加する.
		void summary_append(Shape* const s);
		// 一覧の中で図形を入れ替える.
		void summary_arrange(Shape* const s, Shape* const t);
		// 一覧を消去する.
		void summary_clear(void);
		// 一覧パネルを閉じて消去する.
		//void summary_close(void);
		// 一覧の「閉じる」ボタンが押された.
		void summary_close_click(IInspectable const&, winrt::Windows::UI::Xaml::RoutedEventArgs const&);
		// 一覧に図形を挿入する.
		void summary_insert_at(Shape* const s, const uint32_t i);
		// 一覧が表示されてるか判定する.
		bool summary_is_visible(void) { return m_summary_atomic.load(std::memory_order_acquire); }
		// 一覧の項目が選択された.
		void summary_item_click(IInspectable const&, winrt::Windows::UI::Xaml::Controls::ItemClickEventArgs const&);
		// 編集メニューの「リストの表示」が選択された.
		void summary_list_click(IInspectable const&, winrt::Windows::UI::Xaml::RoutedEventArgs const&);
		// 一覧がロードされた.
		void summary_loaded(IInspectable const& sender, winrt::Windows::UI::Xaml::RoutedEventArgs const& e);
		// 操作を一覧に反映する.
		void summary_reflect(const Undo* u);
		// 一覧を作成しなおす.
		void summary_remake(void);
		// 一覧から図形を消去する.
		uint32_t summary_remove(Shape* const s);
		// 一覧の図形を選択する.
		void summary_select(Shape* const s);
		// 一覧の項目を全て選択する.
		void summary_select_all(void);
		// 一覧の最初の項目を選択する.
		void summary_select_head(void);
		// 一覧の最後の項目を選択する.
		void summary_select_tail(void);
		// 一覧の項目が選択された.
		void summary_selection_changed(IInspectable const& sender, winrt::Windows::UI::Xaml::Controls::SelectionChangedEventArgs const& e);
		// 一覧の図形を選択解除する.
		void summary_unselect(Shape* const s);
		// 一覧の項目を全て選択解除する.
		void summary_unselect_all(void);
		// 一覧の表示を更新する.
		void summary_update(void);

		//-------------------------------
		//　MainPage_text.cpp
		//　文字列の配置
		//-------------------------------

		// 書体メニューの「文字列のそろえ」に印をつける.
		void text_align_t_is_checked(const DWRITE_TEXT_ALIGNMENT value);
		// 書体メニューの「段落のそろえ」に印をつける.
		void text_align_p_is_checked(const DWRITE_PARAGRAPH_ALIGNMENT value);
		// 書体メニューの「枠を文字列に合わせる」が選択された.
		void text_fit_frame_to_click(IInspectable const&, winrt::Windows::UI::Xaml::RoutedEventArgs const&);
		// 書体メニューの「行間...」が選択された.
		winrt::Windows::Foundation::IAsyncAction text_line_sp_click_async(IInspectable const&, winrt::Windows::UI::Xaml::RoutedEventArgs const&);
		// 書体メニューの「余白...」が選択された.
		winrt::Windows::Foundation::IAsyncAction text_padding_click_async(IInspectable const&, winrt::Windows::UI::Xaml::RoutedEventArgs const&);
		// 書体メニューの「段落のそろえ」が選択された.
		void text_align_p_click(IInspectable const& sender, winrt::Windows::UI::Xaml::RoutedEventArgs const&);
		// 書体メニューの「文字列のそろえ」が選択された.
		void text_align_t_click(IInspectable const& sender, winrt::Windows::UI::Xaml::RoutedEventArgs const&);
		// 値をスライダーのヘッダーに格納する.
		template <UNDO_OP U, int S> void text_slider_set_header(const float value);
		// スライダーの値が変更された.
		template <UNDO_OP U, int S> void text_slider_value_changed(IInspectable const&, winrt::Windows::UI::Xaml::Controls::Primitives::RangeBaseValueChangedEventArgs const&);

		//-------------------------------
		// MainPage_thread.cpp
		// ウィンドウ切り替えのハンドラー
		//-------------------------------

		// ウィンドウの実行/停止が切り替わった.
		void thread_activated(IInspectable const& sender, winrt::Windows::UI::Core::WindowActivatedEventArgs const& args);
		// ウィンドウの表示/非表示が変わった.
		void thread_visibility_changed(winrt::Windows::UI::Core::CoreWindow const& sender, winrt::Windows::UI::Core::VisibilityChangedEventArgs const& args);

		//-------------------------------
		// MainPage_tool.cpp
		// 作図ツール
		//-------------------------------

		// 作図メニューの項目が選択された.
		void drawing_tool_click(IInspectable const& sender, winrt::Windows::UI::Xaml::RoutedEventArgs const&);
		// 作図メニューに印をつける.
		void drawing_tool_is_checked(const DRAWING_TOOL value);
		// 作図メニューの多角形の頂点数にチェックをつける.
		void drawing_poly_vtx_is_checked(const uint32_t value);
		// 作図メニューの多角形の選択肢にチェックをつける.
		void drawing_poly_opt_is_checked(const POLY_OPTION& value);

		//-----------------------------
		// MainPage_ustack.cpp
		// 元に戻すとやり直し操作
		//-----------------------------

		// 元に戻す/やり直しメニューの可否を設定する.
		void ustack_is_enable(void);
		// 編集メニューの「やり直し」が選択された.
		void ustack_redo_click(IInspectable const&, winrt::Windows::UI::Xaml::RoutedEventArgs const&);
		// 編集メニューの「元に戻す」が選択された.
		void ustack_undo_click(IInspectable const&, winrt::Windows::UI::Xaml::RoutedEventArgs const&);
		// 操作スタックを消去し, 含まれる操作を破棄する.
		void ustack_clear(void);
		// 操作を実行する.
		void ustack_exec(Undo* u);
		// 無効な操作の組をポップする.
		bool ustack_pop_if_invalid(void);
		// 図形を追加して, その操作をスタックに積む.
		void ustack_push_append(Shape* const s);
		// 図形をグループ図形に追加して, その操作をスタックに積む.
		void ustack_push_append(ShapeGroup* const g, Shape* const s);
		// 図形を入れ替えて, その操作をスタックに積む.
		void ustack_push_arrange(Shape* const s, Shape* const t);
		// 図形の頂点をスタックに保存する.
		void ustack_push_position(Shape* const s, const uint32_t anp);
		// 画像の現在の位置や大きさ、不透明度を操作スタックにプッシュする.
		void ustack_push_image(Shape* const s);
		// 図形を挿入して, その操作をスタックに積む.
		void ustack_push_insert(Shape* const s, Shape* const s_pos);
		// 図形の位置をスタックに保存してから差分だけ移動する.
		void ustack_push_move(const D2D1_POINT_2F d_vec, const bool all = false);
		// 一連の操作の区切としてヌル操作をスタックに積む.
		void ustack_push_null(void);
		// 図形をグループから削除して, その操作をスタックに積む.
		void ustack_push_remove(Shape* const g, Shape* const s);
		// 図形を削除して, その操作をスタックに積む.
		void ustack_push_remove(Shape* const s);
		// 図形の選択を反転して, その操作をスタックに積む.
		void ustack_push_select(Shape* const s);
		// 値を図形へ格納して, その操作をスタックに積む.
		template <UNDO_OP U, typename T> void ustack_push_set(Shape* const s, T const& value);
		// 値を選択された図形に格納して, その操作をスタックに積む.
		template <UNDO_OP U, typename T> bool ustack_push_set(T const& value);
		// 図形の値をスタックに保存する.
		template <UNDO_OP U> void ustack_push_set(Shape* const s);
		// データリーダーから操作スタックを読み込む.
		void ustack_read(winrt::Windows::Storage::Streams::DataReader const& dt_reader);
		// データリーダーに操作スタックを書き込む.
		void ustack_write(winrt::Windows::Storage::Streams::DataWriter const& dt_writer);

		//-------------------------------
		// MainPage_xcvd.cpp
		// 切り取りとコピー, 文字列の編集など
		//-------------------------------

		// 編集メニューの「コピー」が選択された.
		winrt::Windows::Foundation::IAsyncAction xcvd_copy_click_async(IInspectable const&, winrt::Windows::UI::Xaml::RoutedEventArgs const&);
		// 編集メニューの「切り取り」が選択された.
		winrt::Windows::Foundation::IAsyncAction xcvd_cut_click_async(IInspectable const&, winrt::Windows::UI::Xaml::RoutedEventArgs const&);
		// 編集メニューの「削除」が選択された.
		void xcvd_delete_click(IInspectable const&, winrt::Windows::UI::Xaml::RoutedEventArgs const&);
		// 編集メニューの可否を設定する.
		void xcvd_is_enabled(void);
		// 編集メニューの「貼り付け」が選択された.
		void xcvd_paste_click(IInspectable const&, winrt::Windows::UI::Xaml::RoutedEventArgs const&);
		// 図形を貼り付ける.
		winrt::Windows::Foundation::IAsyncAction xcvd_paste_shape(void);
		// 文字列を貼り付ける.
		winrt::Windows::Foundation::IAsyncAction xcvd_paste_text(void);
		// 画像を貼り付ける.
		winrt::Windows::Foundation::IAsyncAction xcvd_paste_image(void);
	};

}

namespace winrt::GraphPaper::factory_implementation
{
	struct MainPage : MainPageT<MainPage, implementation::MainPage>
	{
	};
}
