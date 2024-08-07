﻿#pragma once
//------------------------------
// 1「ソリューションエクスプローラー」>「ビューを切り替える」>「フォルダービュー」
// 2「ビルド」>「構成マネージャー」>「アクティブソリューションプラットフォーム」を x64
// 3「プロジェクト」>「NuGetパッケージの管理」>「復元」. 必要なら「MicroSoft.UI.Xaml」と「Microsoft.Windows.CppWinRT」を更新.
// 4「デバッグ」>「GraphPaper のプロパティ」>「構成プロパティ」>「ターゲットプラットフォームの最小バージョン」>「10.0.17763.0」
// 	(MicroSoft.UI.Xaml の MenuBar には、Windows 10 Version 1809 (SDK 17763) 以降、または Windows UI ライブラリが必要)
//
// デバッガーの停止で終了したときはすべて 0 になるが,
// アプリケーションを「X」ボタンなどで終了したとき「スレッド 0xXXXX はコード 1 (0x1) で終了しました。」が表示される.
// とりわけ Application::Current().Exit で終了した場合, すべてのスレッドで 1 が表示される.
// Blank App (c++/winrt) でまっさらなアプリケーションを作成したときも同じ表示なので, とりあえず気にしないようにする.
// Microsoft.UI.Xaml のバージョンによっては不具合 (例えばコンパイルできないとか, サブメニューの RadiMenuFlyout のチェックマークが最初はつかない) が出る.
//
// MainPage.h
//
// MainPage.cpp	メインページの作成, アプリの終了
// MainPage_app.cpp	アプリケーションの中断と再開
// MainPage_color.cpp	色 (線枠, 塗りつぶし, 書体, 方眼, 用紙), 画像の不透明度
// MainPage_display.cpp	表示デバイスのハンドラー
// MainPage_drawing.cpp	作図ツール
// MainPage_edit.cpp	図形の編集
// MainPage_event.cpp	ポインターイベントのハンドラー
// MainPage_file.cpp	ファイルの読み書き
// MainPage_find.cpp	文字列の編集, 検索と置換
// MainPage_font.cpp	書体と文字列の配置
// MainPage_group.cpp	グループ化とグループの解除
// MainPage_help.cpp	長さの単位, 色の基数, ステータスバー, 頂点をくっつける閾値
// NainPage_image.cpp	画像
// MainPage_layout.cpp	レイアウト (方眼, 用紙, 背景パターン, 保存/リセット)
// MainPage_order.cpp	並び替え
// MainPage_dialog.cpp	属性ダイアログ
// MainPage_scroll.cpp	スクロールバー
// MainPage_select.cpp	図形の選択
// MainPage_status.cpp	ステータスバー
// MainPage_stroke.cpp	図形の属性 (破線, 線の太さ, 端点, 線の結合)
// MainPage_summary.cpp	図形の一覧
// MainPage_thread.cpp	ウィンドウ切り替えのハンドラー
// MainPage_undo.cpp	元に戻すとやり直し操作
// MainPage_xcvd.cpp	切り取り (x) とコピー (c), 貼り付け (v), 削除 (d)
//------------------------------
#include <ppltasks.h>
#include <winrt/Windows.ApplicationModel.ExtendedExecution.h>
#include <winrt/Windows.UI.Xaml.Input.h>
#include "MainPage.g.h"
#include "undo.h"

namespace winrt::GraphPaper::implementation
{
	using winrt::Microsoft::UI::Xaml::Controls::NumberBoxValueChangedEventArgs;
	using winrt::Windows::ApplicationModel::SuspendingEventArgs;
	using winrt::Windows::ApplicationModel::EnteredBackgroundEventArgs;
	using winrt::Windows::ApplicationModel::LeavingBackgroundEventArgs;
	using winrt::Windows::Graphics::Display::DisplayInformation;
	using winrt::Windows::Storage::StorageFile;
	using winrt::Windows::System::VirtualKeyModifiers;
	using winrt::Windows::UI::Color;
	using winrt::Windows::UI::Core::CoreCursor;
	using winrt::Windows::UI::Core::CoreCursorType;
	using winrt::Windows::UI::Core::CoreWindow;
	using winrt::Windows::UI::Core::Preview::SystemNavigationCloseRequestedPreviewEventArgs;
	using winrt::Windows::UI::Core::VisibilityChangedEventArgs;
	using winrt::Windows::UI::Core::WindowActivatedEventArgs;
	using winrt::Windows::UI::Text::Core::CoreTextEditContext;
	using winrt::Windows::UI::Text::Core::CoreTextServicesManager;
	using winrt::Windows::UI::Xaml::RoutedEventArgs;
	using winrt::Windows::UI::Xaml::Controls::ContentDialog;
	using winrt::Windows::UI::Xaml::Controls::ContentDialogOpenedEventArgs;
	using winrt::Windows::UI::Xaml::Controls::ContentDialogClosedEventArgs;
	using winrt::Windows::UI::Xaml::Controls::ItemClickEventArgs;
	using winrt::Windows::UI::Xaml::Controls::Primitives::ScrollEventArgs;
	using winrt::Windows::UI::Xaml::Controls::SelectionChangedEventArgs;
	using winrt::Windows::UI::Xaml::Controls::TextChangedEventArgs;
	using winrt::Windows::UI::Xaml::Input::KeyboardAcceleratorInvokedEventArgs;
	using winrt::Windows::UI::Xaml::Input::PointerRoutedEventArgs;
	using winrt::Windows::UI::Xaml::SizeChangedEventArgs;
	using winrt::Windows::UI::Xaml::Visibility;
	using winrt::Windows::ApplicationModel::DataTransfer::StandardDataFormats;
	using winrt::Windows::ApplicationModel::DataTransfer::Clipboard;
	using winrt::Windows::UI::Xaml::UIElement;
	using winrt::Windows::UI::Xaml::Input::GettingFocusEventArgs;
	using winrt::Windows::UI::Xaml::Window;

	extern const winrt::param::hstring CLIPBOARD_FORMAT_SHAPES;	// 図形データのクリップボード書式

	constexpr auto ICON_INFO = L"glyph_info";	// 情報アイコンの静的リソースのキー
	constexpr auto ICON_ALERT = L"glyph_block";	// 警告アイコンの静的リソースのキー
	constexpr auto ICON_DEBUG = L"\uEBE8";	// デバッグアイコン
	constexpr auto SNAP_INTERVAL_DEF_VAL = 2.0f * 6.0f;	// 点をくっつける間隔の既定値
	constexpr uint32_t VERT_CNT_MAX = 12;	// 折れ線の頂点の最大数.
	constexpr wchar_t LAYOUT_FILE[] = L"gp_layout.dat";	// レイアウトを格納するファイル名
	constexpr auto CLICK_DIST = 6.0;	// クリック判定距離 (この値を以内ならクリック)

	//-------------------------------
	// 色の基数
	//-------------------------------
	enum struct COLOR_CODE : uint32_t {
		DEC,	// 10 進数
		HEX,	// 16 進数	
		REAL,	// 実数
		PCT	// パーセント
	};

	// 色成分を文字列に変換する.
	void conv_col_to_str(const COLOR_CODE c_code, const double c_val, const size_t t_len, wchar_t t_buf[]) noexcept;

	// 色成分を文字列に変換する.
	template <size_t Z> inline void conv_col_to_str(const COLOR_CODE c_code, const double c_val, wchar_t(&t_buf)[Z]) noexcept
	{
		conv_col_to_str(c_code, c_val, Z, t_buf);
	}

	constexpr double UWP_COLOR_MAX = 255.0;	// UWP の色成分の最大値

	//-------------------------------
	// UWP の色を D2D1_COLOR_F に変換する.
	//-------------------------------
	inline void conv_uwp_to_color(const Color& a, D2D1_COLOR_F& b) noexcept
	{
		b.r = static_cast<FLOAT>(static_cast<double>(a.R) / UWP_COLOR_MAX);
		b.g = static_cast<FLOAT>(static_cast<double>(a.G) / UWP_COLOR_MAX);
		b.b = static_cast<FLOAT>(static_cast<double>(a.B) / UWP_COLOR_MAX);
		b.a = static_cast<FLOAT>(static_cast<double>(a.A) / UWP_COLOR_MAX);
	}

	//-------------------------------
	// 作図ツール
	//-------------------------------
	enum struct DRAWING_TOOL : uint32_t {
		SELECT,	// 選択ツール
		ARC,	// 円弧
		BEZIER,	// 曲線
		ELLIPSE,	// だ円
		LINE,	// 線分
		POLY,	// 多角形
		RECT,	// 方形
		RRECT,	// 角丸方形
		TEXT,	// 文字列
		RULER,	// 定規
		EYEDROPPER,	// スポイトツール
		POINTER	// ポインターツール
	};

	//-------------------------------
	// ポインターボタンの状態
	//-------------------------------
	enum struct EVENT_STATE : uint32_t {
		BEGIN,	// 初期状態
		PRESS_LBTN,	// 左ボタンを押している状態
		PRESS_RBTN,	// 右ボタンを押している状態
		PRESS_RECT,	// 左ボタンを押して, 矩形選択している状態
		PRESS_MOVE,	// 左ボタンを押して, 図形を移動している状態
		PRESS_DEFORM,	// 左ボタンを押して, 図形を変形している状態
		PRESS_TEXT,	// 左ボタンを押して, 文字列を選択している状態
		CLICK,	// クリックした状態
		CLICK_LBTN,	// クリック後に左ボタンを押した状態
	};

	//-------------------------------
	// 長さの単位
	//-------------------------------
	enum struct LEN_UNIT : uint32_t {
		PIXEL,	// ピクセル
		INCH,	// インチ
		MILLI,	// ミリメートル
		POINT,	// ポイント
		GRID	// 方眼 (グリッド)
	};
	constexpr bool LEN_UNIT_NAME_APPEND = true;	// 単位名を付加する
	constexpr bool LEN_UNIT_NAME_NOT_APPEND = false;	// 単位名を付加しない

	// ピクセル長さをある単位の文字列に変換する.
	template <bool U> void conv_len_to_str(const LEN_UNIT len_unit, const double val, const double dpi, const double g_len, const uint32_t t_len, wchar_t* t_buf) noexcept;

	// ピクセル長さをある単位の文字列に変換する.
	// U	単位フラグ (true なら単位名を付ける, false なら値のみ)
	// len_unit	長さの単位
	// val	値 (ピクセル)
	// dpi	DPI
	// g_len	方眼の大きさ
	// t_buf[Z]	文字列を格納するバッファ
	template <bool U, size_t Z> inline void conv_len_to_str(const LEN_UNIT len_unit, const double val, const double dpi, const double g_len,	wchar_t(&t_buf)[Z]) noexcept
	{
		conv_len_to_str<U>(len_unit, val, dpi, g_len, Z, t_buf);
	}

	// ある長さをピクセル単位の長さに変換する.
	double conv_len_to_pixel(const LEN_UNIT l_unit, const double val, const double dpi, const double g_len) noexcept;

	//-------------------------------
	// メッセージダイアログのアイコン
	//-------------------------------
	enum struct MSG_ICON : uint32_t {
		ALERT,
		INFO
	};

	//-------------------------------
	// ステータスバーの状態
	//-------------------------------
	enum struct STATUS_BAR : uint32_t {
		POS = (1 | 2),	// カーソルの位置
		DRAW = 4,	// 作図ツール
		GRID = 8,	// 方眼の大きさ
		SHEET = (16 | 32),	// 用紙の大きさ
		UNIT = 64,	// 単位名
		ZOOM = 128,	// 拡大率
	};
	constexpr STATUS_BAR STATUS_BAR_DEF_VAL = static_cast<STATUS_BAR>(static_cast<uint32_t>(STATUS_BAR::DRAW) | static_cast<uint32_t>(STATUS_BAR::POS) | static_cast<uint32_t>(STATUS_BAR::ZOOM));

	// 列挙型を AND 演算する.
	// 戻り値	a & b
	inline STATUS_BAR status_and(const STATUS_BAR a, const STATUS_BAR b)
	{
		return static_cast<STATUS_BAR>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
	}

	// 待機カーソルを表示する.
	// 戻り値	それまで表示されていたカーソル.
	inline const CoreCursor wait_cursor_show(void)
	{
		const auto& CURS_WAIT = CoreCursor(CoreCursorType::Wait, 0);	// 左右カーソル
		const auto& core_win = Window::Current().CoreWindow();
		const auto& prev_cur = core_win.PointerCursor();
		if (prev_cur.Type() != CURS_WAIT.Type()) {
			core_win.PointerCursor(CURS_WAIT);
		}
		return prev_cur;
	}

	//-------------------------------
	// メインページ
	//-------------------------------
	struct MainPage : MainPageT<MainPage> {
		//winrt::Windows::UI::ViewManagement::InputPane m_edit_input{	// 編集ペイン (枠)
		//	winrt::Windows::UI::ViewManagement::InputPane::GetForCurrentView()
		//};
		winrt::hstring m_file_token_mru;	// 最近使ったファイルのトークン

		// 排他制御
		// 1. 図形や描画環境の変更中に描画させないための排他制御
		// 2. ファイルを書き込み中に終了を延長するための排他制御.
		// 3. ファイルピッカーのボタンが押されてストレージファイルが得られるまでの間,
		//    イベント処理させないための排他制御.
		// ファイルピッカーなどが返値を戻すまで時間がかかる.
		// その間にフォアグランドのスレッドが動作して, イベント処理が始まってしまう.
		// co_await の返値が得られる前に, イベント処理をしないよう排他制御する必要がある.
		std::atomic_bool m_summary_atomic{ false };	// 一覧の排他制御
		std::mutex m_mutex_draw;	// 描画させないための排他制御
		std::mutex m_mutex_exit;	// 終了を延長するための排他制御
		std::mutex m_mutex_event;	// イベント処理させないための排他制御

		// 文字列の検索と置換
		wchar_t* m_find_text = nullptr;	// 検索の検索文字列
		wchar_t* m_repl_text = nullptr;	// 検索の置換文字列
		bool m_find_case_sensitive = false;	// 大文字と小文字を区別する.
		bool m_find_wrap_around = false;	// 折り返しあり
		bool m_find_use_escseq = false;	// エスケープ文字列を使用

		CoreTextEditContext m_core_text{ CoreTextServicesManager::GetForCurrentView().CreateEditContext() };	// 編集コンテキスト (状態)
		ShapeText* m_core_text_focused = nullptr;	// 編集中の文字列図形
		bool m_core_text_comp = false;	// 入力変換フラグ. 変換中なら true, それ以外なら false.
		uint32_t m_core_text_comp_start = 0;	// 入力変換開始時の開始位置

		// ポインターイベント
		D2D1_POINT_2F m_event_point_curr{ 0.0F, 0.0F };	// ポインターの現在点
		D2D1_POINT_2F m_event_point_prev{ 0.0F, 0.0F };	// ポインターの前回点
		D2D1_POINT_2F m_event_point_pressed{ 0.0F, 0.0F };	// ポインターが押された点
		EVENT_STATE m_event_state = EVENT_STATE::BEGIN;	// ポインターが押された状態
		uint32_t m_event_hit_pressed = HIT_TYPE::HIT_SHEET;	// ポインターが押された判定部位
		SHAPE* m_event_shape_pressed = nullptr;	// ポインターが押された図形
		SHAPE* m_event_shape_last = nullptr;	// 最後にポインターが押された図形 (シフトキー押下で押した図形は含まない)
		uint64_t m_event_time_pressed = 0ULL;	// ポインターが押された時刻
		double m_event_click_dist = CLICK_DIST * DisplayInformation::GetForCurrentView().RawDpiX() / DisplayInformation::GetForCurrentView().LogicalDpi();	// クリックの判定距離 (DIPs)

		// 作図ツール
		DRAWING_TOOL m_tool = DRAWING_TOOL::SELECT;	// 作図ツール
		POLY_OPTION m_tool_polygon{ TOOL_POLYGON_DEFVAL };	// 多角形ツール
		bool m_eyedropper_filled = false;	// 抽出されたか判定
		D2D1_COLOR_F m_eyedropper_color{};	// 抽出された色.

		// メイン用紙
		SHAPE_SHEET m_main_sheet;	// 用紙
		D2D_UI m_main_d2d;	// 描画環境
		D2D1_POINT_2F m_main_bbox_lt{ 0.0f, 0.0f };	// 用紙と図形, 全体が収まる境界ボックスの左上点 (方眼の左上点を原点とする)
		D2D1_POINT_2F m_main_bbox_rb{ 0.0f, 0.0f };	// 用紙と図形, 全体が収まる境界ボックスの右下点 (方眼の左上点を原点とする)
		float m_main_scale = 1.0f;	// 用紙の拡大率
		bool m_main_sheet_focused = false;	// 用紙のフォーカスフラグ (UI 要素によるキーボードアクセラレターをより分けるために使用されるフラグ)

		// 背景パターン
		winrt::com_ptr<IWICFormatConverter> m_background_wic{ nullptr };	// 背景の画像ブラシ
		bool m_background_show = false;	// 背景の市松模様を表示
		D2D1_COLOR_F m_background_color{ COLOR_WHITE };	// 背景の色

		// ダイアログで使用する用紙
		SHAPE_SHEET m_dialog_sheet;	// 用紙
		D2D_UI m_dialog_d2d;	// 描画環境

		// 元に戻す・やり直し操作
		UNDO_STACK m_redo_stack;	// やり直し操作スタック
		UNDO_STACK m_undo_stack;	// 元に戻す操作スタック
		uint32_t m_undo_select_cnt = 0;	// 選択された図形の数
		uint32_t m_undo_undeleted_cnt = 0;	// 削除されていない図形の数
		bool m_undo_is_updated = false;

#ifdef _DEBUG
		void debug_cnt(void) {
			wchar_t debug_buf[64];
			swprintf_s(debug_buf, L"%zu", m_undo_stack.size());
			status_bar_debug().Text(debug_buf);

			uint32_t select_cnt = 0;
			uint32_t undeleted_cnt = 0;
			uint32_t undeleted_text = 0;
			uint32_t select_group = 0;
			uint32_t select_text = 0;
			uint32_t select_line = 0;
			uint32_t select_image = 0;
			uint32_t select_ruler = 0;
			uint32_t select_arc = 0;
			uint32_t select_polyline = 0;
			uint32_t select_polygon = 0;
			uint32_t select_exist_cap = 0;
			for (auto s : m_main_sheet.m_shape_list) {
				if (s->is_deleted()) {
					continue;
				}
				undeleted_cnt++;
				if (typeid(*s) == typeid(ShapeText)) {
					undeleted_text++;
				}
				if (!s->is_selected()) {
					continue;
				}
				select_cnt++;
				if (s->exist_cap()) {
					select_exist_cap++;
				}
				if (typeid(*s) == typeid(SHAPE_GROUP)) {
					select_group++;
				}
				else if (typeid(*s) == typeid(ShapeText)) {
					select_text++;
				}
				else if (typeid(*s) == typeid(ShapeLine)) {
					select_line++;
				}
				else if (typeid(*s) == typeid(SHAPE_IMAGE)) {
					select_image++;
				}
				else if (typeid(*s) == typeid(ShapeRuler)) {
					select_ruler++;
				}
				else if (typeid(*s) == typeid(SHAPE_ARC)) {
					select_arc++;
				}
				else if (typeid(*s) == typeid(ShapePoly)) {
					if (s->exist_cap()) {
						select_polyline++;
					}
					else {
						select_polygon++;
					}
				}
			}
			if (undeleted_cnt != m_undo_undeleted_cnt) {
				__debugbreak();
			}
			//if (undeleted_text != m_undo_undeleted_text) {
			//	__debugbreak();
			//}
			//if (select_cnt != m_undo_select_cnt) {
			//	__debugbreak();
			//}
			//if (select_group != m_undo_selected_group) {
			//	__debugbreak();
			//}
			//if (select_text != m_undo_selected_text) {
			//	__debugbreak();
			//}
			//if (select_line != m_undo_selected_line) {
			//	__debugbreak();
			//}
			//if (select_image != m_undo_selected_image) {
			//	__debugbreak();
			//}
			//if (select_ruler != m_undo_selected_ruler) {
			//	__debugbreak();
			//}
			//if (select_arc != m_undo_selected_arc) {
			//	__debugbreak();
			//}
			//if (select_polyline != m_undo_selected_polyline) {
			//	__debugbreak();
			//}
			//if (select_polygon != m_undo_selected_polygon) {
			//	__debugbreak();
			//}
			//if (select_exist_cap != m_undo_selected_exist_cap) {
			//	__debugbreak();
			//}
		}
#endif

		// その他
		LEN_UNIT m_len_unit = LEN_UNIT::PIXEL;	// 長さの単位
		COLOR_CODE m_color_code = COLOR_CODE::DEC;	// 色成分の記法
		bool m_snap_grid = true;	// 点を方眼にくっつける.
		float m_snap_point = SNAP_INTERVAL_DEF_VAL;	// 点と点をくっつける間隔
		STATUS_BAR m_status_bar_flag = STATUS_BAR_DEF_VAL;	// ステータスバーの状態
		//winrt::guid m_enc_id = BitmapEncoder::BmpEncoderId();	// 既定の画像形式 (エンコード識別子)

		// スレッド
		bool m_thread_activated = false;	// アクティベートされた初回を判定
		bool m_thread_win_visible = false;	// ウィンドウが表示されてるか判定

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

		//PrintDocument m_print_doc;
		//IPrintDocumentSource m_print_source;

		//-------------------------------
		// MainPage_core_text.cpp
		// 文字入力
		//-------------------------------

		// 入力中の文字列が押された.
		template <bool SHIFT_KEY> void core_text_pressed(void) noexcept
		{
			if constexpr (SHIFT_KEY) {
				bool trail;
				const auto end = m_core_text_focused->get_text_pos(m_event_point_curr, trail);
				const auto start = m_main_sheet.m_core_text_range.m_start;
				undo_push_text_select(m_core_text_focused, start, end, trail);
				main_sheet_draw();
			}
		}
		// 文字列入力のためのハンドラーを設定する.
		void core_text_setup_handler(void) noexcept;
		// 入力中の文字列の長さを得る.
		uint32_t core_text_len(void) const noexcept;
		// 入力中の文字列のキャレット位置を得る.
		uint32_t core_text_pos(void) const noexcept;
		// 入力中の文字列の選択範囲の文字列を得る.
		winrt::hstring core_text_substr(void) const noexcept;
		// 入力中の文字列の選択範囲の長さを得る.
		uint32_t core_text_selected_len(void) const noexcept;
		// 入力中の文字列の選択範囲の文字を削除する.
		void core_text_delete_selection(void) noexcept;
		// 入力中の文字列の選択範囲に文字列を挿入する.
		void core_text_insert(const wchar_t* ins_text, const uint32_t ins_len) noexcept;
		// 文字列の入力中に削除キーが押された.
		template <bool SHIFT_KEY> void core_text_delete_key(void) noexcept;
		// 文字列の入力中に左矢じるしキーが押された.
		template <bool SHIFT_KEY> void core_text_left_key(void) noexcept;
		// 文字列の入力中に右矢じるしキーが押された.
		template <bool SHIFT_KEY> void core_text_right_key(void) noexcept;
		// 文字列の入力中に上矢じるしキーが押された.
		template <bool SHIFT_KEY> void core_text_up_key(void) noexcept;
		// 文字列の入力中に改行キーが押された.
		void core_text_enter_key(void) noexcept;
		// 文字列の入力中に下矢じるしキーが押された.
		template <bool SHIFT_KEY> void core_text_down_key(void) noexcept;
		// 文字列の入力中に後退キーが押された.
		void core_text_backspace_key(void) noexcept;

		//-------------------------------
		// MainPage.cpp
		// メインページの作成, 
		// メインのスワップチェーンパネルのハンドラー
		// メインの用紙図形の処理
		//-------------------------------

		// 更新された図形をもとにメインの用紙の境界矩形を更新する.
		void MainPage::main_bbox_update(const SHAPE* s) noexcept
		{
			s->get_bbox(m_main_bbox_lt, m_main_bbox_rb, m_main_bbox_lt, m_main_bbox_rb);
		}
		// メインの用紙図形の境界矩形を更新する.
		void main_bbox_update(void) noexcept;
		// メインページを作成する.
		MainPage(void);
		// メインのスワップチェーンパネルがロードされた.
		void main_panel_loaded(IInspectable const& sender, RoutedEventArgs const& args);
		// メインのスワップチェーンパネルの大きさが変わった.
		void main_panel_size_changed(IInspectable const& sender, SizeChangedEventArgs const& args);
		// メインのスワップチェーンパネルの倍率が変わった.
		void main_panel_scale_changed(IInspectable const& sender, IInspectable const&);
		// メインのスワップチェーンパネルの大きさを設定する.
		void main_panel_size(void);
		// メイン用紙を表示する.
		void main_sheet_draw(void);
		// メッセージダイアログを表示する.
		void message_show(winrt::hstring const& glyph, winrt::hstring const& message, winrt::hstring const& desc);
		// ウィンドウの閉じるボタンが押された.
		void navi_close_requested(IInspectable const&, SystemNavigationCloseRequestedPreviewEventArgs const& args)
		{
			args.Handled(true);
			auto _{ file_exit_click_async(nullptr, nullptr) };
		}
		// ファイルメニューの「印刷」が選ばれた.
		IAsyncAction MainPage::print_click_async(const IInspectable&, const RoutedEventArgs&);

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
		// MainPage_order.cpp
		// 並び替え
		//-------------------------------

		// 選択された図形を次または前の図形と入れ替える.
		template<typename T> void order_swap(void);
		// 編集メニューの「前面に移動」が選択された.
		void bring_forward_click(IInspectable const&, RoutedEventArgs const&);
		// 編集メニューの「最前面に移動」が選択された.
		void bring_to_front_click(IInspectable const&, RoutedEventArgs const&);
		// 編集メニューの「ひとつ背面に移動」が選択された.
		void send_backward_click(IInspectable const&, RoutedEventArgs const&);
		// 編集メニューの「最背面に移動」が選択された.
		void send_to_back_click(IInspectable const&, RoutedEventArgs const&);

		//-------------------------------
		// MainPage_disp.cpp
		// 表示デバイスのハンドラー
		//-------------------------------

		// ディスプレーの再描画が必要となった場合のハンドラー
		void display_contents_invalidated(DisplayInformation const& sender, IInspectable const& args);
		// ディスプレーの DPI が変更された場合のハンドラー
		void display_dpi_changed(DisplayInformation const& sender, IInspectable const& args);
		// ディスプレーの向きが変更された場合のハンドラー
		void display_orientation_changed(DisplayInformation const& sender, IInspectable const& args);

		//-------------------------------
		// MainPage_event.cpp
		// ポインターイベントのハンドラー
		//-------------------------------

		// ポインターの現在点に, イベント引数の値を格納する.
		void event_set_position(PointerRoutedEventArgs const& args) noexcept
		{
			// 引数として渡された点に, 拡大率の逆数を乗じ, 表示されている左上点とスクロールバーの値を加える.
			// 得られた値を, ポインターの現在点に格納する.
			const auto p{ args.GetCurrentPoint(scp_main_panel()).Position() };
			m_event_point_curr.x = static_cast<FLOAT>(sb_horz().Value() + p.X / m_main_scale + m_main_bbox_lt.x);
			m_event_point_curr.y = static_cast<FLOAT>(sb_vert().Value() + p.Y / m_main_scale + m_main_bbox_lt.y);
		}
		// ポインターのボタンが上げられた.
		void event_canceled(IInspectable const& sender, PointerRoutedEventArgs const& args);
		// ポインターがスワップチェーンパネルの中に入った.
		void event_entered(IInspectable const& sender, PointerRoutedEventArgs const& args);
		// ポインターがスワップチェーンパネルから出た.
		void event_exited(IInspectable const& sender, PointerRoutedEventArgs const& args);
		// 指定した部位の色を検出する.
		void event_eyedropper_detect(const SHAPE* s, const uint32_t hit);
		// 図形の作成を終了する.
		void event_finish_creating(const D2D1_POINT_2F start, const D2D1_POINT_2F end_to);
		// 図形の変形を終了する.
		void event_adjust_after_deforming(const VirtualKeyModifiers key_mod);
		// 図形の移動を終了する.
		void event_adjust_after_moving(void);
		// 範囲選択を終了する.
		void event_finish_rect_selection(const VirtualKeyModifiers key_mod);
		// ポインターが動いた.
		void event_moved(IInspectable const& sender, PointerRoutedEventArgs const& args);
		// ポインターのボタンが押された.
		void event_pressed(IInspectable const& sender, PointerRoutedEventArgs const& args);
		// ポインターのボタンが上げられた.
		void event_released(IInspectable const& sender, PointerRoutedEventArgs const& args);
		// ポインターの形状を設定する.
		void event_set_cursor(void);
		// ポインターのホイールボタンが操作された.
		void event_wheel_changed(IInspectable const& sender, PointerRoutedEventArgs const& args);
		// 文字列編集ダイアログを表示する.
		IAsyncAction event_edit_text_async(void);

		//-------------------------------
		// MainPage_file.cpp
		// ファイルの読み書き
		//-------------------------------

		// ファイルシステムへのアクセス権を確認して, 設定を促す.
		//IAsyncAction file_check_broad_access(void) const;
		// ファイルへの更新を確認する.
		IAsyncOperation<bool> file_confirm_dialog(void);
		// ファイルメニューの「終了」が選択された
		IAsyncAction file_exit_click_async(IInspectable const&, RoutedEventArgs const&);
		// ファイルの読み込みが終了した.
		void file_finish_reading(void);
		// ファイルメニューの「画像をインポート」が選択された
		IAsyncAction file_import_image_click_async(IInspectable const&, RoutedEventArgs const&);
		// ファイルメニューの「新規」が選択された
		IAsyncAction file_new_click_async(IInspectable const&, RoutedEventArgs const&);
		// ファイルメニューの「開く」が選択された
		IAsyncAction file_open_click_async(IInspectable const&, RoutedEventArgs const&);
		// ファイルメニューの「名前を付けて保存」が選択された
		IAsyncAction file_save_as_click_async(IInspectable const&, RoutedEventArgs const&) noexcept;
		// ファイルメニューの「上書き保存」が選択された
		IAsyncAction file_save_click_async(IInspectable const&, RoutedEventArgs const&) noexcept;
		// ストレージファイルを非同期に読む.
		template <bool RESUME, bool SETTING_ONLY> IAsyncOperation<winrt::hresult> file_read_gpf_async(StorageFile s_file) noexcept;
		// 図形データをストレージファイルに非同期に書き込む.
		template <bool SUSPEND, bool SETTING> IAsyncOperation<winrt::hresult> file_write_gpf_async(StorageFile gpf_file);
		// ファイルメニューの「他の形式としてエクスポートする」が選択された
		IAsyncAction file_export_as_click_async(IInspectable const&, RoutedEventArgs const&);
		// ファイルメニューの「最近使ったファイル 」のサブ項目が選択された
		IAsyncAction recent_file_click_async(IInspectable const&, RoutedEventArgs const&);
		// 最近使ったファイルにストレージファイルを追加する.
		void recent_file_add(StorageFile const& s_file);
		// 最近使ったファイルのトークンからストレージファイルを得る.
		IAsyncOperation<StorageFile> recent_file_token_async(const winrt::hstring token);
		// 最近使ったファイルのメニュー項目を更新する.
		void recent_file_menu_update(void);

		//-------------------------------
		// MainPage_color.cpp
		// 色設定
		//-------------------------------

		// 「...色」が選択された.
		template <UNDO_T U> IAsyncAction color_click_async(void);
		// ヘルプメニューの「色成分の記法」のサブ項目が選択された.
		void color_code_click(IInspectable const& sender, RoutedEventArgs const&);
		// 属性メニューの「塗りつぶし色」が選択された.
		void color_fill_click(IInspectable const&, RoutedEventArgs const&) { color_click_async<UNDO_T::FILL_COLOR>(); }
		// 書体メニューの「書体の色」が選択された.
		void color_font_click(IInspectable const&, RoutedEventArgs const&) { color_click_async<UNDO_T::FONT_COLOR>(); }
		// レイアウトメニューの「方眼の色」が選択された.
		void color_grid_click(IInspectable const&, RoutedEventArgs const&) { color_click_async<UNDO_T::GRID_COLOR>(); }
		// 属性メニューの「画像の不透明度...」が選択された.
		IAsyncAction color_image_opac_click_async(IInspectable const&, RoutedEventArgs const&);
		// レイアウトメニューの「用紙の色」が選択された.
		void color_sheet_click(IInspectable const&, RoutedEventArgs const&) { color_click_async<UNDO_T::SHEET_COLOR>(); }
		// 属性メニューの「線枠の色」が選択された.
		void color_stroke_click(IInspectable const&, RoutedEventArgs const&) { color_click_async<UNDO_T::STROKE_COLOR>(); }

		//-------------------------------
		// MainPage_edit.cpp
		// 多角形の終端, 文字列の編集, 円弧の傾きの編集
		//-------------------------------

		// 操作メニューの「原画像に戻す」が選択された.
		void revert_image_to_original_click(IInspectable const&, RoutedEventArgs const&) noexcept;
		// 「パスの方向を反転」がクリックされた.
		void reverse_path_click(const IInspectable& /*sender*/, const RoutedEventArgs& /*args*/)
		{
			bool changed = false;
			for (SHAPE* s : m_main_sheet.m_shape_list) {
				if (s->is_deleted() || !s->is_selected() || dynamic_cast<SHAPE_ARROW*>(s) == nullptr) {
					continue;
				}
				if (!changed) {
					changed = true;
					undo_push_null();
				}
				m_undo_stack.push_back(new UndoReverse(s));
			}
			if (changed) {
				menu_is_enable();
				main_sheet_draw();
				status_bar_set_pointer();
			}
		}
		// 「反時計周りに円弧を描く」/「時計周りに円弧を描く」が選択された
		void draw_arc_direction_click(const IInspectable& sender, const RoutedEventArgs& /*args*/);
		void open_or_close_poly_end_click(IInspectable const& sender, RoutedEventArgs const&);

		//-------------------------------
		// MainPage_find.cpp
		// 文字列の編集と, 検索/置換
		//-------------------------------

		// 次を検索する.
		bool find_next(void);
		// 編集メニューの「文字列の検索/置換」が選択された.
		void find_and_replace_click(IInspectable const&, RoutedEventArgs const&);
		// 文字列検索パネルの「閉じる」ボタンが押された.
		void find_and_replace_close_click(IInspectable const&, RoutedEventArgs const&);
		//　文字列検索パネルの「次を検索」ボタンが押された.
		void find_next_click(IInspectable const&, RoutedEventArgs const&);
		// 文字列検索パネルの値を保存する.
		void find_text_preserve(void);
		// 検索文字列が変更された.
		void find_text_what_changed(IInspectable const&, TextChangedEventArgs const&);
		// 置換して次を検索する.
		bool replace_text(void);
		// 文字列検索パネルの「すべて置換」ボタンが押された.
		void replace_all_click(IInspectable const&, RoutedEventArgs const&);
		// 文字列検索パネルの「置換して次に」ボタンが押された.
		void replace_text_click(IInspectable const&, RoutedEventArgs const&);

		//-------------------------------
		//　MainPage_font.cpp
		//　書体
		//-------------------------------

		// 書体メニューの「書体名」が選択された.
		IAsyncAction font_family_click_async(IInspectable const&, RoutedEventArgs const&);
		// 書体メニューの「字体」のサブ項目が選択された.
		void font_style_click(IInspectable const&, RoutedEventArgs const&);
		// 書体メニューの「書体の大きさ」が選択された.
		IAsyncAction font_size_click_async(IInspectable const&, RoutedEventArgs const&);
		// 書体メニューの「書体の幅」のサブ項目が選択された.
		void font_stretch_click(IInspectable const&, RoutedEventArgs const&);
		// 書体メニューの「書体の太さ」のサブ項目が選択された.
		void font_weight_click(IInspectable const&, RoutedEventArgs const&);

		//-------------------------------
		// MainPage_group.cpp
		// グループ化とグループの解除
		//-------------------------------

		// 編集メニューの「グループ化」が選択された.
		void group_click(IInspectable const&, RoutedEventArgs const&);
		// 編集メニューの「グループの解除」が選択された.
		void ungroup_click(IInspectable const&, RoutedEventArgs const&);

		//-------------------------------
		//　MainPage_kacc.cpp
		//　キーアクセラレーターのハンドラー
		//-------------------------------

		void MainPage::kacc_back_invoked(IInspectable const&, KeyboardAcceleratorInvokedEventArgs const& args)
		{
			core_text_backspace_key();
			args.Handled(true);
		}

		void MainPage::kacc_delete_invoked(IInspectable const&, KeyboardAcceleratorInvokedEventArgs const& args)
		{
			if (m_core_text_focused == nullptr) {
				delete_click(nullptr, nullptr);
			}
			else {
				core_text_delete_key<false>();
			}
			args.Handled(true);
		}

		void MainPage::kacc_delete_shift_invoked(IInspectable const&, KeyboardAcceleratorInvokedEventArgs const& args)
		{
			if (m_core_text_focused == nullptr) {
				delete_click(nullptr, nullptr);
			}
			else {
				core_text_delete_key<true>();
			}
			args.Handled(true);
		}

		void MainPage::kacc_down_invoked(IInspectable const&, KeyboardAcceleratorInvokedEventArgs const& args)
		{
			core_text_down_key<false>();
			args.Handled(true);
		}

		void MainPage::kacc_down_shift_invoked(IInspectable const&, KeyboardAcceleratorInvokedEventArgs const& args)
		{
			core_text_down_key<true>();
			args.Handled(true);
		}

		void MainPage::kacc_enter_invoked(IInspectable const&, KeyboardAcceleratorInvokedEventArgs const& args)
		{
			core_text_enter_key();
			args.Handled(true);
		}

		void MainPage::kacc_escape_invoked(IInspectable const&, KeyboardAcceleratorInvokedEventArgs const&)
		{
			menu_tool_selection().IsChecked(true);
			drawing_tool_click(menu_tool_selection(), nullptr);
		}

		void MainPage::kacc_left_invoked(IInspectable const&, KeyboardAcceleratorInvokedEventArgs const& args)
		{
			core_text_left_key<false>();
			args.Handled(true);
		}

		void MainPage::kacc_left_shift_invoked(IInspectable const&, KeyboardAcceleratorInvokedEventArgs const& args)
		{
			core_text_left_key<true>();
			args.Handled(true);
		}

		void MainPage::kacc_right_invoked(IInspectable const&, KeyboardAcceleratorInvokedEventArgs const& args)
		{
			core_text_right_key<false>();
			args.Handled(true);
		}

		void MainPage::kacc_right_shift_invoked(IInspectable const&, KeyboardAcceleratorInvokedEventArgs const& args)
		{
			core_text_right_key<true>();
			args.Handled(true);
		}

		void MainPage::kacc_up_invoked(IInspectable const&, KeyboardAcceleratorInvokedEventArgs const& args)
		{
			core_text_up_key<false>();
			args.Handled(true);
		}

		void MainPage::kacc_up_shift_invoked(IInspectable const&, KeyboardAcceleratorInvokedEventArgs const& args)
		{
			core_text_up_key<true>();
			args.Handled(true);
		}

		//-----------------------------
		// MainPage_help.cpp
		// ヘルプ
		// 長さの単位, 色の基数, バージョン情報
		//-----------------------------

		IAsyncAction about_graph_paper_click(IInspectable const&, RoutedEventArgs const&);
		// ヘルプメニューの「長さの単位」のサブ項目が選択された.
		void len_unit_click(IInspectable const&, RoutedEventArgs const&);
		// ヘルプメニューの「点を点にくっつけるしきい値...」が選択された.
		IAsyncAction snap_point_click_async(IInspectable const&, RoutedEventArgs const&) noexcept;
		// ヘルプメニューの「点を方眼にくっつける」が選択された.
		void snap_grid_click(IInspectable const&, RoutedEventArgs const&);

		//-------------------------------
		// MainPage_prop.cpp
		// 属性ダイアログ
		//-------------------------------

		// 属性ダイアログがアンロードされた
		void dialog_prop_unloaded(IInspectable const&, RoutedEventArgs const&);
		// 属性ダイアログが開いた.
		void dialog_prop_opened(ContentDialog const&, ContentDialogOpenedEventArgs const&);
		// 属性ダイアログが閉じた.
		void dialog_prop_closed(ContentDialog const&, ContentDialogClosedEventArgs const&);
		// 属性ダイアログの図形を表示する.
		void dialog_draw(void);
		// 属性ダイアログのリストがロードされた.
		void dialog_list_loaded(IInspectable const&, RoutedEventArgs const&);
		// 属性ダイアログの画像を読み込む.
		IAsyncAction dialog_image_load_async(const float samp_w, const float samp_h);
		// 属性ダイアログのスワップチェーンパネルが読み込まれた.
		void dialog_panel_loaded(IInspectable const& sender, RoutedEventArgs const&);
		// 属性ダイアログのスワップチェーンパネルの大きさが変わった.
		void dialog_panel_size_changed(IInspectable const&, SizeChangedEventArgs const&);
		// 属性ダイアログのスワップチェーンパネルの倍率が変わった.
		void dialog_panel_scale_changed(IInspectable const&, IInspectable const&);

		//-------------------------------
		// MainPage_scroll.cpp
		// スクロールバー
		//-------------------------------

		// スクロールバーが操作された
		void scroll(IInspectable const& sender, ScrollEventArgs const& args);
		// スクロールバーの値を設定する.
		void scroll_set(const double aw, const double ah);
		// 図形が表示されるようパネルをスクロールする.
		bool scroll_to(const SHAPE* const s);

		//-------------------------------
		// MainPage_select.cpp
		// 図形の選択
		//-------------------------------

		// 編集メニューの「すべて選択」が選択された.
		void select_all_click(IInspectable const&, RoutedEventArgs const&);
		// 矩形に含まれる図形を選択し, 含まれない図形の選択を解除する.
		bool select_inside_shape(const D2D1_POINT_2F area_lt, const D2D1_POINT_2F area_rb);
		// 範囲の中の図形を選択して, それ以外の図形の選択をはずす.
		bool select_range_shape(SHAPE* const s_from, SHAPE* const s_to);
		// 矩形に含まれる図形の選択を反転する.
		bool toggle_inside_shape(const D2D1_POINT_2F area_lt, const D2D1_POINT_2F area_rb);
		// すべての図形の選択を解除する.
		bool unselect_all_shape(void);
		// 図形を選択して, それ以外の図形の選択をはずす.
		bool select_shape(SHAPE* const s);

		//-------------------------------
		// MainPage_layout.cpp
		// レイアウト
		//-------------------------------

		// 背景パターンの画像ブラシを得る.
		void background_get_brush(void) noexcept;
		// レイアウトメニューの「背景パターン」>「市松模様を表示」がクリックされた.
		void background_pattern_click(IInspectable const&, RoutedEventArgs const&);
		// レイアウトメニューの「方眼の大きさ」>「大きさ」が選択された.
		IAsyncAction grid_len_click_async(IInspectable const&, RoutedEventArgs const&);
		// レイアウトメニューの「方眼の大きさ」>「狭める」が選択された.
		void grid_len_con_click(IInspectable const&, RoutedEventArgs const&);
		// レイアウトメニューの「方眼の大きさ」>「広げる」が選択された.
		void grid_len_exp_click(IInspectable const&, RoutedEventArgs const&);
		// レイアウトメニューの「方眼の強調」が選択された.
		void grid_emph_click(IInspectable const& sender, RoutedEventArgs const&);
		// レイアウトメニューの「方眼の表示」が選択された.
		void grid_show_click(IInspectable const& sender, RoutedEventArgs const&);
		// レイアウトを既定値に戻す.
		void layout_init(void) noexcept;
		// レイアウトメニューの「レイアウトを既定値に戻す」が選択された.
		IAsyncAction layout_reset_click_async(IInspectable const&, RoutedEventArgs const&);
		// レイアウトメニューの「レイアウトを保存」が選択された
		IAsyncAction layout_save_click_async(IInspectable const&, RoutedEventArgs const&);
		// レイアウトメニューの「用紙の大きさ」が選択された
		IAsyncAction sheet_size_click_async(IInspectable const&, RoutedEventArgs const&);
		// 用紙の大きさダイアログのテキストボックスの値が変更された.
		void sheet_size_value_changed(IInspectable const&, NumberBoxValueChangedEventArgs const&);
		// 用紙の大きさダイアログのコンボボックスの選択が変更された.
		void sheet_size_selection_changed(IInspectable const&, SelectionChangedEventArgs const& args) noexcept;
		// レイアウトメニューの「用紙のズーム」が選択された.
		void sheet_zoom_click(IInspectable const& sender, RoutedEventArgs const&);

		//-------------------------------
		// MainPage_status.cpp
		// ステータスバー
		//-------------------------------

		// レイアウトメニューの「ステータスバー」が選択された.
		void status_bar_click(IInspectable const&, RoutedEventArgs const&);
		// ポインターの座標値をステータスバーに格納する.
		void status_bar_set_pointer(void);
		// 作図ツールをステータスバーに格納する.
		void status_bar_set_drawing_tool(void);
		// 方眼の大きさをステータスバーに格納する.
		void status_bar_set_grid_len(void);
		// 用紙の大きさをステータスバーに格納する.
		void status_bar_set_sheet_size(void);
		// 単位をステータスバーに格納する.
		void status_bar_set_len_unit(void);
		// 拡大率をステータスバーに格納する.
		void status_bar_set_sheet_zoom(void);

		//------------------------------
		// MainPage_stroke.cpp
		// 線枠
		//------------------------------

		// 属性メニューの「矢じるしの形式」のサブ項目が選択された.
		void stroke_arrow_click(IInspectable const& sender, RoutedEventArgs const&);
		// 属性メニューの「矢じるしの大きさ」が選択された.
		IAsyncAction stroke_arrow_size_click_async(IInspectable const&, RoutedEventArgs const&);
		// 属性メニューの「端の形式」のサブ項目が選択された.
		void stroke_cap_click(IInspectable const& sender, RoutedEventArgs const&);
		// 属性メニューの「破線の形式」のサブ項目が選択された.
		void stroke_dash_click(IInspectable const& sender, RoutedEventArgs const&);
		// 属性メニューの「破線の配列」が選択された.
		IAsyncAction stroke_dash_pat_click_async(IInspectable const&, RoutedEventArgs const&);
		// 属性メニューの「線の結合の形式」>「尖り制限」が選択された.
		IAsyncAction stroke_join_limit_click_async(IInspectable const&, RoutedEventArgs const&);
		// 属性メニューの「結合の形式」のサブ項目が選択された.
		void stroke_join_click(IInspectable const& sender, RoutedEventArgs const&);
		// 属性メニューの「線枠の太さ」のサブ項目が選択された.
		void stroke_width_click(IInspectable const& sender, RoutedEventArgs const&);
		// 属性メニューの「線枠の太さ」>「その他」が選択された.
		IAsyncAction stroke_width_click_async(IInspectable const&, RoutedEventArgs const&);

		//-------------------------------
		// MainPage_summary.cpp
		// 一覧
		//-------------------------------

		// 図形を一覧に追加する.
		void summary_append(SHAPE* const s);
		// 一覧の中で図形を入れ替える.
		void summary_order(SHAPE* const s, SHAPE* const t);
		// 一覧を消去する.
		void summary_clear(void);
		// 一覧パネルを閉じて消去する.
		//void summary_close(void);
		// 一覧の「閉じる」ボタンが押された.
		void summary_close_click(IInspectable const&, RoutedEventArgs const&);
		// 一覧に図形を挿入する.
		void summary_insert_at(SHAPE* const s, const uint32_t i);
		// 一覧が表示されてるか判定する.
		bool summary_is_visible(void) { return m_summary_atomic.load(std::memory_order_acquire); }
		// 一覧の項目が選択された.
		void summary_item_click(IInspectable const&, ItemClickEventArgs const&);
		// 編集メニューの「リストの表示」が選択された.
		void summary_list_click(IInspectable const&, RoutedEventArgs const&);
		// 一覧がロードされた.
		void summary_loaded(IInspectable const& sender, RoutedEventArgs const& e);
		// 操作を一覧に反映する.
		void summary_reflect(const Undo* u);
		// 一覧を作成しなおす.
		void summary_remake(void);
		// 一覧から図形を消去する.
		uint32_t summary_remove(SHAPE* const s);
		// 一覧の図形を選択する.
		void summary_select(SHAPE* const s);
		// 一覧の項目を全て選択する.
		void summary_select_all(void);
		// 一覧の最初の項目を選択する.
		void summary_select_head(void);
		// 一覧の最後の項目を選択する.
		void summary_select_tail(void);
		// 一覧の項目が選択された.
		void summary_selection_changed(IInspectable const& sender, SelectionChangedEventArgs const& args);
		// 一覧の図形を選択解除する.
		void summary_unselect(SHAPE* const s);
		// 一覧の項目を全て選択解除する.
		void summary_unselect_all_shape(void);
		// 一覧の項目を全て選択解除する.
		void summary_select_shape(SHAPE* const s);
		// 一覧の表示を更新する.
		void summary_update(void);

		//------------------------------
		// MainPage_svg.cpp
		// 線枠の色, 太さ
		//------------------------------

		// 図形をデータライターに PDF として書き込む.
		IAsyncOperation<winrt::hresult> export_as_pdf_async(const StorageFile& pdf_file) noexcept;
		// 図形をデータライターに SVG として書き込む.
		IAsyncOperation<winrt::hresult> export_as_svg_async(const StorageFile& svg_file) noexcept;
		// 図形をデータライターにラスター画像として書き込む.
		IAsyncOperation<winrt::hresult> export_as_raster_async(const StorageFile& image_file) noexcept;

		//-------------------------------
		//　MainPage_text.cpp
		//　文字列の配置
		//-------------------------------

		// 書体メニューの「文字列のそろえ」のサブ項目が選択された
		void text_align_horz_click(IInspectable const& sender, RoutedEventArgs const&);
		// 書体メニューの「段落のそろえ」のサブ項目が選択された
		void text_align_vert_click(IInspectable const& sender, RoutedEventArgs const&);
		// 書体メニューの「行間...」が選択された.
		IAsyncAction text_line_sp_click_async(IInspectable const&, RoutedEventArgs const&);
		// 書体メニューの「余白...」が選択された.
		IAsyncAction text_pad_click_async(IInspectable const&, RoutedEventArgs const&);
		// 書体メニューの「文字列の折り返し」のサブ項目が選択された
		void text_word_wrap_click(IInspectable const& sender, RoutedEventArgs const&);

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
		void drawing_tool_click(IInspectable const& sender, RoutedEventArgs const&);

		//-----------------------------
		// MainPage_undo.cpp
		// 元に戻すとやり直し操作
		//-----------------------------

		// メニューを設定する.
		void menu_is_enable(void) noexcept;
		// メニューを設定する.
		template<UNDO_T U> void menu_is_checked(void);
		// 編集メニューの「やり直し」が選択された.
		void redo_click(IInspectable const&, RoutedEventArgs const&)
		{
			undo_exec(m_redo_stack, m_undo_stack);
		}
		// 編集メニューの「元に戻す」が選択された.
		void MainPage::undo_click(IInspectable const&, RoutedEventArgs const&)
		{
			undo_exec(m_undo_stack, m_redo_stack);
		}
		void undo_exec(UNDO_STACK& undo_stack, UNDO_STACK& redo_stack);
		// 操作スタックを消去し, 含まれる操作を破棄する.
		void undo_clear(void);
		// 無効な操作をポップする.
		bool undo_pop_invalid(void);
		// 図形を追加して, その操作をスタックに積む.
		void undo_push_append(SHAPE* s)
		{
			m_undo_stack.push_back(new UndoAppend(s));
			if (s->is_selected()) {
				m_undo_select_cnt++;
			}
			if (!s->is_deleted()) {
				m_undo_undeleted_cnt++;
			}
#ifdef _DEBUG
			debug_cnt();
#endif
		}
		// 図形をグループ図形に追加して, その操作をスタックに積む.
		void undo_push_append(SHAPE_GROUP* g, SHAPE* s)
		{
			m_undo_stack.push_back(new UndoAppendG(g, s));
		}
		// 図形を入れ替えて, その操作をスタックに積む.
		void undo_push_order(SHAPE* const s, SHAPE* const t)
		{
			m_undo_stack.push_back(new UndoOrder(s, t));
		}
		// 指定した判定部位の座標をスタックに保存する.
		void undo_push_position(SHAPE* const s, const uint32_t hit_type)
		{
			if (typeid(*s) == typeid(SHAPE_IMAGE)) {
				m_undo_stack.push_back(new UndoImage(static_cast<SHAPE_IMAGE* const>(s)));
			}
			else {
				m_undo_stack.push_back(new UndoDeform(s, hit_type));
			}
		}
		// 画像の現在の位置や大きさ、不透明度を操作スタックにプッシュする.
		void undo_push_image(SHAPE* const s)
		{
			m_undo_stack.push_back(new UndoImage(static_cast<SHAPE_IMAGE*>(s)));
		}
		// 図形を挿入して, その操作をスタックに積む.
		void MainPage::undo_push_insert(SHAPE* s, SHAPE* s_at)
		{
			m_undo_stack.push_back(new UndoInsert(s, s_at));
			if (s->is_selected()) {
				//undo_selected_cnt<true>(s);
				m_undo_select_cnt++;
			}
			if (!s->is_deleted()) {
				m_undo_undeleted_cnt++;
			}
#ifdef _DEBUG
			debug_cnt();
#endif
		}
		// 図形の位置をスタックに保存してから差分だけ移動する.
		template <bool ANY> void undo_push_move(const D2D1_POINT_2F to);
		// 一連の操作の区切としてヌル操作をスタックに積む.
		void undo_push_null(void);
		// 図形をグループから取り去り, その操作をスタックに積む.
		void undo_push_remove(SHAPE* g, SHAPE* s)
		{
			m_undo_stack.push_back(new UndoRemoveG(g, s));
		}
		// 図形を取り去り, その操作をスタックに積む.
		void undo_push_remove(SHAPE* s)
		{
			m_undo_stack.push_back(new UndoRemove(s));
			if (s->is_selected()) {
				//undo_selected_cnt<false>(s);
				m_undo_select_cnt--;
			}
			if (s->is_deleted()) {
				m_undo_undeleted_cnt--;
			}
#ifdef _DEBUG
			if (!s->is_deleted()) {
				__debugbreak();
			}
			debug_cnt();
#endif
		}
		// 図形の選択を反転して, その操作をスタックに積む.
		void undo_push_toggle(SHAPE* const s);
		// 値を図形へ格納して, その操作をスタックに積む.
		template <UNDO_T U, typename T> void undo_push_set(SHAPE* const s, T const& val);
		// 値を選択された図形に格納して, その操作をスタックに積む.
		template <UNDO_T U, typename T> bool undo_push_set(T const& val);
		// 図形の値の保存を実行して, その操作をスタックに積む.
		template <UNDO_T U> void undo_push_set(SHAPE* const s);
		// 文字列の選択範囲を解除する. キャレット位置は変わらない.
		void undo_push_text_unselect(ShapeText* s)
		{
			const auto start = m_main_sheet.m_core_text_range.m_trail ? m_main_sheet.m_core_text_range.m_end + 1 : m_main_sheet.m_core_text_range.m_end;
			undo_push_text_select(s, start, m_main_sheet.m_core_text_range.m_end, m_main_sheet.m_core_text_range.m_trail);
		}
		// 文字列の選択を実行して, その操作をスタックに積む.
		void undo_push_text_select(SHAPE* s, const uint32_t start, const uint32_t end, const bool trail)
		{
			// 文字列の選択の操作が連続するかぎり,
			// スタックをさかのぼって, 同じ図形に対する文字列の選択があったなら
			// 図形の文字列の選択を直接上書きする. スタックに操作を積まない.
			for (auto u = m_undo_stack.rbegin(); u != m_undo_stack.rend() && *u != nullptr && typeid(*u) == typeid(UndoValue<UNDO_T::CORE_TEXT_RANGE>); u++) {
			//for (auto u = m_undo_stack.rbegin(); u != m_undo_stack.rend() && *u != nullptr && typeid(*u) == typeid(UndoTextSelect); u++) {
				if ((*u)->m_shape != s) {
					continue;
				}
				m_main_sheet.m_core_text_range.m_start = start;
				m_main_sheet.m_core_text_range.m_end = end;
				m_main_sheet.m_core_text_range.m_trail = trail;
				return;
			}
			// そうでなければ, スタックに操作を積む.
			//m_undo_stack.push_back(new UndoTextSelect(s, start, end, trail));
			m_undo_stack.push_back(new UndoValue<UNDO_T::CORE_TEXT_RANGE>(&m_main_sheet, { start, end, trail }));
		}
		// データリーダーから操作スタックを読み込む.
		void undo_read_stack(DataReader const& dt_reader);
		// データリーダーに操作スタックを書き込む.
		void undo_write_stack(DataWriter const& dt_writer);

		//-------------------------------
		// MainPage_xcvd.cpp
		// 切り取りとコピー, 貼り付け, 削除
		//-------------------------------

		// UI 要素がフォーカスを得る.
		// スワップチェーンパネル以外の UI 要素, たとえばテキストボックスがフォーカスを獲得するときに呼び出されて,
		// 用紙のフォーカスフラグを下す. 
		void ui_elem_getting_focus(UIElement const&, GettingFocusEventArgs const&)
		{
			// テキストブロックがサポートするコマンドは以下の通り.
			// Copy
			// Cut
			// Paste
			// Select all
			// Undo
			m_main_sheet_focused = false;
			status_bar_debug().Text(L"m_main_sheet_focused = false");
		}

		void ui_elem_lost_focus(IInspectable const&, RoutedEventArgs const&)
		{
			m_main_sheet_focused = true;
			status_bar_debug().Text(L"m_main_sheet_focused = true");
		}

		// 編集メニューの「コピー」が選択された.
		IAsyncAction copy_click_async(IInspectable const&, RoutedEventArgs const&);
		// 編集メニューの「切り取り」が選択された.
		IAsyncAction cut_click_async(IInspectable const&, RoutedEventArgs const&);
		// 編集メニューの「削除」が選択された.
		void delete_click(IInspectable const&, RoutedEventArgs const&);
		// 編集メニューの「貼り付け」が選択された.
		void paste_click(IInspectable const&, RoutedEventArgs const&);
		// 画像を貼り付ける.
		IAsyncAction xcvd_paste_image(void);
		// 図形を貼り付ける.
		IAsyncAction xcvd_paste_shape(void);
		// 文字列を貼り付ける.
		IAsyncAction xcvd_paste_text(void);
		// 貼り付ける点を得る
		void xcvd_paste_pos(D2D1_POINT_2F& paste_pt, const D2D1_POINT_2F src_pt) const noexcept;

		void Page_Loaded(const IInspectable& /*sender*/, const RoutedEventArgs& /*args*/)
		{
			/*
			m_print_doc = PrintDocument();
			m_print_source = m_print_doc.DocumentSource();
			m_print_doc.Paginate([=](const IInspectable& sender, const PaginateEventArgs& args) {
				PrintTaskOptions opt = args.PrintTaskOptions();
			PrintPageDescription desc = opt.GetPageDescription(0);
				}
			);
			m_print_doc.AddPages([=](const IInspectable& sender, const AddPagesEventArgs& args) {
				PrintDocument m_print_doc = winrt::unbox_value<PrintDocument>(sender);
			m_print_doc.AddPage(PrintPage());
			m_print_doc.AddPagesComplete();
				}
			);
			m_print_doc.GetPreviewPage([=](const IInspectable& sender, const GetPreviewPageEventArgs& args) {
				PrintDocument m_print_doc = winrt::unbox_value<PrintDocument>(sender);
			m_print_doc.SetPreviewPage(args.PageNumber(), PrintPage());
				}
			);

			PrintManager mgr = PrintManager::GetForCurrentView();
			mgr.PrintTaskRequested([=](const IInspectable& PrintManager& sender, const PrintTaskRequestedEventArgs& args) {
				PrintTask p_task = nullptr;
			p_task = args.Request().CreatePrintTask(L"Print", [=](const PrintTaskSourceRequestedArgs& args) {
				p_task.Completed([=](const IInspectable& sender, const PrintTaskCompletedEventArgs& args) {
					if (args.Completion() == PrintTaskCompletion::Failed) {

					}
					});
			args.SetSource(m_print_source);
				});
				});
			// Add the (newly created) page to the print canvas which is part of the visual tree and force it to go
			// through layout so that the linked containers correctly distribute the content inside them.
			PrintCanvas().Children().Append(PrintPage());
			PrintCanvas().InvalidateMeasure();
			PrintCanvas().UpdateLayout();
			*/
		}
		void menu_getting_focus(UIElement const& sender, GettingFocusEventArgs const& args);
};
	template void MainPage::undo_push_set<UNDO_T::MOVE>(SHAPE* const s);
	template void MainPage::undo_push_set<UNDO_T::IMAGE_OPAC>(SHAPE* const s);



}

namespace winrt::GraphPaper::factory_implementation
{
	struct MainPage : MainPageT<MainPage, implementation::MainPage>
	{
	};
}
