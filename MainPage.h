#pragma once
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
// MainPage_color.cpp	色 (線枠, 塗りつぶし, 書体, 方眼, ページ), 画像の不透明度
// MainPage_display.cpp	表示デバイスのハンドラー
// MainPage_drawing.cpp	作図ツール
// MainPage_edit.cpp	円弧や文字列の編集
// MainPage_event.cpp	ポインターイベントのハンドラー
// MainPage_file.cpp	ファイルの読み書き
// MainPage_find.cpp	文字列の編集, 検索と置換
// MainPage_font.cpp	書体と文字列の配置
// MainPage_group.cpp	グループ化とグループの解除
// MainPage_help.cpp	長さの単位, 色の基数, ステータスバー, 頂点をくっつける閾値
// NainPage_image.cpp	画像
// MainPage_layout.cpp	レイアウト (方眼, ページ, 背景パターン, 保存/リセット)
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
	using winrt::Windows::ApplicationModel::SuspendingEventArgs;
	using winrt::Windows::ApplicationModel::EnteredBackgroundEventArgs;
	using winrt::Windows::ApplicationModel::LeavingBackgroundEventArgs;
	using winrt::Windows::Foundation::IInspectable;
	using winrt::Windows::Graphics::Display::DisplayInformation;
	using winrt::Windows::Graphics::Imaging::BitmapEncoder;
	using winrt::Windows::Storage::StorageFile;
	using winrt::Windows::System::VirtualKeyModifiers;
	using winrt::Windows::UI::Color;
	using winrt::Windows::UI::Core::CoreCursor;
	using winrt::Windows::UI::Core::CoreCursorType;
	using winrt::Windows::UI::Core::CoreWindow;
	using winrt::Windows::UI::Core::Preview::SystemNavigationCloseRequestedPreviewEventArgs;
	using winrt::Windows::UI::Core::VisibilityChangedEventArgs;
	using winrt::Windows::UI::Core::WindowActivatedEventArgs;
	using winrt::Windows::UI::Xaml::Controls::MenuFlyout;
	using winrt::Windows::UI::Xaml::Visibility;
	using winrt::Windows::UI::Xaml::RoutedEventArgs;
	using winrt::Windows::UI::Xaml::Controls::ContentDialog;
	using winrt::Windows::UI::Xaml::Controls::ContentDialogOpenedEventArgs;
	using winrt::Windows::UI::Xaml::Controls::ContentDialogClosedEventArgs;
	using winrt::Windows::UI::Xaml::Controls::ItemClickEventArgs;
	using winrt::Windows::UI::Xaml::Controls::Primitives::RangeBaseValueChangedEventArgs;
	using winrt::Windows::UI::Xaml::Controls::Primitives::ScrollEventArgs;
	using winrt::Windows::UI::Xaml::Controls::SelectionChangedEventArgs;
	using winrt::Windows::UI::Xaml::Controls::TextChangedEventArgs;
	using winrt::Windows::UI::Xaml::Input::KeyboardAcceleratorInvokedEventArgs;
	using winrt::Windows::UI::Xaml::Input::PointerRoutedEventArgs;
	using winrt::Windows::UI::Xaml::SizeChangedEventArgs;
	using winrt::Microsoft::UI::Xaml::Controls::NumberBoxValueChangedEventArgs;
	using winrt::Windows::UI::Xaml::Visibility;
	using winrt::Windows::UI::Xaml::Controls::TextBox;
	using winrt::Windows::UI::ViewManagement::InputPane;

	extern const winrt::param::hstring CLIPBOARD_FORMAT_SHAPES;	// 図形データのクリップボード書式
	//extern const winrt::param::hstring CLIPBOARD_TIFF;	// TIFF のクリップボード書式 (Windows10 ではたぶん使われない)

	constexpr auto ICON_INFO = L"glyph_info";	// 情報アイコンの静的リソースのキー
	constexpr auto ICON_ALERT = L"glyph_block";	// 警告アイコンの静的リソースのキー
	constexpr auto ICON_DEBUG = L"\uEBE8";	// デバッグアイコン
	constexpr auto SNAP_INTERVAL_DEF_VAL = 2.0f * 6.0f;	// 点をくっつける間隔の既定値
	constexpr uint32_t VERT_CNT_MAX = 12;	// 折れ線の頂点の最大数.
	constexpr wchar_t LAYOUT_FILE[] = L"gp_layout.dat";	// レイアウトを格納するファイル名

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
		EYEDROPPER	// スポイトツール
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
	template <bool B> void conv_len_to_str(const LEN_UNIT len_unit, const double val, const double dpi, const double g_len, const uint32_t t_len, wchar_t* t_buf) noexcept;

	// ピクセル長さをある単位の文字列に変換する.
	template <bool B, size_t Z> inline void conv_len_to_str(
		const LEN_UNIT len_unit,	// 長さの単位
		const double val,	// 値 (ピクセル)
		const double dpi,	// DPI
		const double g_len,	// 方眼の大きさ
		wchar_t(&t_buf)[Z]	// 文字列を格納するバッファ
	) noexcept
	{
		conv_len_to_str<B>(len_unit, val, dpi, g_len, Z, t_buf);
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
		PAGE = (16 | 32),	// ページの大きさ
		UNIT = 64,	// 単位名
		ZOOM = 128,	// 拡大率
	};
	constexpr STATUS_BAR STATUS_BAR_DEF_VAL = static_cast<STATUS_BAR>(static_cast<uint32_t>(STATUS_BAR::DRAW) | static_cast<uint32_t>(STATUS_BAR::POS) | static_cast<uint32_t>(STATUS_BAR::ZOOM));

	// 待機カーソルを表示, 表示する前のカーソルを得る.
	const CoreCursor wait_cursor_show(void);

	//-------------------------------
	// メインページ
	//-------------------------------
	struct MainPage : MainPageT<MainPage> {
		// テキスト編集
		winrt::Windows::UI::Text::Core::CoreTextEditContext m_edit_context{	// 編集コンテキスト (状態)
			winrt::Windows::UI::Text::Core::CoreTextServicesManager::GetForCurrentView().CreateEditContext()
		};
		winrt::Windows::UI::ViewManagement::InputPane m_edit_input{	// 編集ペイン (枠)
			winrt::Windows::UI::ViewManagement::InputPane::GetForCurrentView()
		};
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

		// 文字列の編集, 検索と置換
		bool m_fit_text_frame = false;	// 枠を文字列に合わせる
		wchar_t* m_find_text = nullptr;	// 検索の検索文字列
		wchar_t* m_find_repl = nullptr;	// 検索の置換文字列
		bool m_find_text_case = false;	// 英文字の区別
		bool m_find_text_wrap = false;	// 回り込み検索

		// trail = false    trail = true
		//   start  end        start end
		//     |     |           |   |
		// 0 1 2 3 4 5 6     0 1 2 3 4 5 6
		//    +-----+           +-----+
		// a b|c d e|f g     a b|c d e|f g
		//    +-----+           +-----+
		// 複数行あるとき, キャレットが行末にあるか, それとも次の行頭にあるか, 区別するため.
		ShapeText* m_edit_text_shape = nullptr;	// 編集中の文字列図形
		bool m_edit_text_comp = false;	// 漢字変換中 (変換中なら true, そうでなければ false)
		int m_edit_text_start = 0;	// 漢字変換開始時の開始位置
		int m_edit_text_end = 0;	// 漢字変換開始時の終了位置
		bool m_edit_text_trail = false;	// 漢字変換開始時のキャレット

		// ポインターイベント
		D2D1_POINT_2F m_event_pos_curr{ 0.0F, 0.0F };	// ポインターの現在位置
		D2D1_POINT_2F m_event_pos_prev{ 0.0F, 0.0F };	// ポインターの前回位置
		D2D1_POINT_2F m_event_pos_pressed{ 0.0F, 0.0F };	// ポインターが押された点
		EVENT_STATE m_event_state = EVENT_STATE::BEGIN;	// ポインターが押された状態
		uint32_t m_event_loc_pressed = LOC_TYPE::LOC_PAGE;	// ポインターが押された部位
		Shape* m_event_shape_pressed = nullptr;	// ポインターが押された図形
		Shape* m_event_shape_last = nullptr;	// 最後にポインターが押された図形 (シフトキー押下で押した図形は含まない)
		uint64_t m_event_time_pressed = 0ULL;	// ポインターが押された時刻
		double m_event_click_dist = 6.0;	// クリックの判定距離 (DIPs)
		bool m_eyedropper_filled = false;	// 抽出されたか判定
		D2D1_COLOR_F m_eyedropper_color{};	// 抽出された色.

		// 作図ツール
		DRAWING_TOOL m_drawing_tool = DRAWING_TOOL::SELECT;	// 作図ツール
		POLY_OPTION m_drawing_poly_opt{ POLY_OPTION_DEFVAL };	// 多角形の作成方法

		// 図形リスト
		uint32_t m_list_sel_cnt = 0;	// 選択された図形の数

		// 画像
		bool m_image_keep_aspect = true;	// 画像の縦横比の維持/可変

		// メインページ
		ShapePage m_main_page;	// ページ
		D2D_UI m_main_d2d;	// 描画環境
		D2D1_POINT_2F m_main_bbox_lt{ 0.0f, 0.0f };	// ページと図形, 全体が収まる境界ボックスの左上点 (方眼の左上点を原点とする)
		D2D1_POINT_2F m_main_bbox_rb{ 0.0f, 0.0f };	// ページと図形, 全体が収まる境界ボックスの右下点 (方眼の左上点を原点とする)

		float m_main_scale = 1.0f;	// メインページの拡大率

		// 背景パターン
		winrt::com_ptr<IWICFormatConverter> m_wic_background{ nullptr };	// 背景の画像ブラシ
		bool m_background_show = false;	// 背景の市松模様を表示
		D2D1_COLOR_F m_background_color{ COLOR_WHITE };	// 背景の色

		// 属性のページ
		ShapePage m_prop_page;	// ページ
		D2D_UI m_prop_d2d;	// 描画環境

		// 元に戻す・やり直し操作
		//uint32_t m_ustack_rcnt = 0;	// やり直し操作スタックに積まれた組数
		UNDO_STACK m_ustack_redo;	// やり直し操作スタック
		//uint32_t m_ustack_ucnt = 0;	// 元に戻す操作スタックに積まれた組数
		UNDO_STACK m_ustack_undo;	// 元に戻す操作スタック
		bool m_ustack_is_changed = false;	// スタックが更新されたか判定

		// その他
		LEN_UNIT m_len_unit = LEN_UNIT::PIXEL;	// 長さの単位
		COLOR_CODE m_color_code = COLOR_CODE::DEC;	// 色成分の記法
		bool m_snap_grid = true;	// 点を方眼にくっつける.
		float m_snap_point = SNAP_INTERVAL_DEF_VAL;	// 点と点をくっつける間隔
		STATUS_BAR m_status_bar = STATUS_BAR_DEF_VAL;	// ステータスバーの状態
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
		// MainPage.cpp
		// メインページの作成, 
		// メインのスワップチェーンパネルのハンドラー
		// メインのページ図形の処理
		//-------------------------------

		// 更新された図形をもとにメインのページの境界矩形を更新する.
		void MainPage::main_bbox_update(const Shape* s) noexcept
		{
			s->get_bbox(m_main_bbox_lt, m_main_bbox_rb, m_main_bbox_lt, m_main_bbox_rb);
		}
		// メインのページ図形の境界矩形を更新する.
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
		// メインのページ図形を表示する.
		void main_draw(void);
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
		void order_bring_forward_click(IInspectable const&, RoutedEventArgs const&);
		// 編集メニューの「最前面に移動」が選択された.
		void order_bring_to_front_click(IInspectable const&, RoutedEventArgs const&);
		// 編集メニューの「ひとつ背面に移動」が選択された.
		void order_send_backward_click(IInspectable const&, RoutedEventArgs const&);
		// 編集メニューの「最背面に移動」が選択された.
		void order_send_to_back_click(IInspectable const&, RoutedEventArgs const&);

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

		// ポインターのボタンが上げられた.
		void event_canceled(IInspectable const& sender, PointerRoutedEventArgs const& args);
		// ポインターがスワップチェーンパネルの中に入った.
		void event_entered(IInspectable const& sender, PointerRoutedEventArgs const& args);
		// ポインターがスワップチェーンパネルから出た.
		void event_exited(IInspectable const& sender, PointerRoutedEventArgs const& args);
		// 指定した部位の色を検出する.
		void event_eyedropper_detect(const Shape* s, const uint32_t loc);
		// 図形の作成を終了する.
		void event_finish_creating(const D2D1_POINT_2F start, const D2D1_POINT_2F pos);
		// 文字列図形の作成を終了する.
		//IAsyncAction event_finish_creating_text_async(const D2D1_POINT_2F start, const D2D1_POINT_2F pos);
		// 図形の変形を終了する.
		void event_finish_deforming(void);
		// 図形の移動を終了する.
		void event_finish_moving(void);
		// 範囲選択を終了する.
		void event_finish_rect_selection(const VirtualKeyModifiers k_mod);
		// ポインターが動いた.
		void event_moved(IInspectable const& sender, PointerRoutedEventArgs const& args);
		// ポインターのボタンが押された.
		void event_pressed(IInspectable const& sender, PointerRoutedEventArgs const& args);
		// ポインターのボタンが上げられた.
		void event_released(IInspectable const& sender, PointerRoutedEventArgs const& args);
		// ポインターの形状を設定する.
		void event_set_cursor(void);
		// ポインターの現在位置に, イベント引数の値を格納する.
		void event_set_position(PointerRoutedEventArgs const& args);
		// コンテキストメニューを表示する.
		void event_show_popup(void);
		// ポインターのホイールボタンが操作された.
		void event_wheel_changed(IInspectable const& sender, PointerRoutedEventArgs const& args);
		// ポップアップメニューの線枠と塗りつぶしの各項目を設定する.
		void event_arrange_popup_prop(const bool visible, const Shape* s);
		// ポップアップメニューの書体と文字列の各項目を設定する.
		void event_arrange_popup_font(const bool visible);
		// ポップアップメニューの画像の各項目を設定する.
		void event_arrange_popup_image(const bool visible);
		// ポップアップメニューの方眼とページ、背景パターンの各項目を設定する.
		void event_arrange_popup_layout(const bool visible);

		//-------------------------------
		// MainPage_edit.cpp
		// 文字列の編集
		//-------------------------------

		// 文字列の編集中か判定する.
		bool is_text_editing(void) const noexcept
		{
			// 文字列編集中の図形があって, かつそれが最後に押された図形である
			return m_edit_text_shape != nullptr && m_edit_text_shape == m_event_shape_last;
		}

		winrt::hstring text_sele_get(void) const noexcept
		{
			//ShapeText* t = m_edit_text_shape;
			const auto len = m_edit_text_shape->get_text_len();
			const auto end = min(m_main_page.m_select_trail ? m_main_page.m_select_end + 1 : m_main_page.m_select_end, len);
			const auto s = min(m_main_page.m_select_start, end);
			const auto e = max(m_main_page.m_select_start, end);
			return winrt::hstring{ m_edit_text_shape->m_text + s, e - s };
		}

		void text_sele_delete(void) noexcept
		{
			const ShapeText* t = m_edit_text_shape;
			const auto len = t->get_text_len();
			const auto end = min(m_main_page.m_select_trail ? m_main_page.m_select_end + 1 : m_main_page.m_select_end, len);
			const auto start = min(m_main_page.m_select_start, len);
			// 選択範囲があるなら
			if (end != start) {
				undo_push_null();
				m_ustack_undo.push_back(new UndoText2(m_edit_text_shape, nullptr));
				undo_menu_is_enabled();
				main_draw();
			}

		}
		void text_sele_insert(const wchar_t* ins_text, const uint32_t ins_len) noexcept
		{
			const ShapeText* t = m_edit_text_shape;
			const auto old_len = t->get_text_len();
			const auto end = min(m_main_page.m_select_trail ? m_main_page.m_select_end + 1 : m_main_page.m_select_end, old_len);
			const auto start = min(m_edit_text_comp ? m_edit_text_start : m_main_page.m_select_start, old_len);
			const auto s = min(start, end);
			const auto e = max(start, end);
			if (s < e) {
				if (!m_edit_text_comp) {
					undo_push_null();
				}
				else {
					for (Undo* u = m_ustack_undo.back(); u != nullptr; u = m_ustack_undo.back()) {
						m_ustack_undo.pop_back();
						u->exec();
						delete u;
					}
				}
				m_ustack_undo.push_back(new UndoText2(m_edit_text_shape, ins_text));
				undo_push_text_select(m_edit_text_shape, s + ins_len, s + ins_len, false);
				undo_menu_is_enabled();
				main_draw();
			}
			else if (ins_len > 0) {
				if (!m_edit_text_comp) {
					undo_push_null();
				}
				else {
					for (Undo* u = m_ustack_undo.back(); u != nullptr; u = m_ustack_undo.back()) {
						m_ustack_undo.pop_back();
						u->exec();
						delete u;
					}
				}
				m_ustack_undo.push_back(new UndoText2(m_edit_text_shape, ins_text));
				undo_push_text_select(m_edit_text_shape, s + ins_len, s + ins_len, false);
				undo_menu_is_enabled();
				main_draw();
			}
		}

		void text_char_delete(const bool shift_key) noexcept
		{
			// シフトキー押下でなく選択範囲がなくキャレット位置が文末でないなら
			const ShapeText* t = m_edit_text_shape;
			const auto len = t->get_text_len();
			const auto end = min(m_main_page.m_select_trail ? m_main_page.m_select_end + 1 : m_main_page.m_select_end, len);
			const auto start = min(m_main_page.m_select_start, len);
			if (!shift_key && end == start && end < len) {
				undo_push_null();
				undo_push_text_select(m_edit_text_shape, end, end + 1, false);
				m_ustack_undo.push_back(new UndoText2(m_edit_text_shape, nullptr));
				undo_menu_is_enabled();
				main_draw();
			}
			// 選択範囲があるなら
			else if (end != start) {
				undo_push_null();
				m_ustack_undo.push_back(new UndoText2(m_edit_text_shape, nullptr));
				undo_menu_is_enabled();
				main_draw();
			}
			winrt::Windows::UI::Text::Core::CoreTextRange modified_ran{
				start, end
			};
			winrt::Windows::UI::Text::Core::CoreTextRange new_ran{
				m_main_page.m_select_start,
				m_main_page.m_select_trail ? m_main_page.m_select_end + 1 : m_main_page.m_select_end
			};
			m_edit_context.NotifyTextChanged(modified_ran, 0, new_ran);
		}

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
		// ファイルメニューの「最近使ったファイル 」のサブ項目が選択された
		IAsyncAction file_recent_click_async(IInspectable const&, RoutedEventArgs const&);
		// 最近使ったファイルにストレージファイルを追加する.
		void file_recent_add(StorageFile const& s_file);
		// 最近使ったファイルのトークンからストレージファイルを得る.
		IAsyncOperation<StorageFile> file_recent_token_async(const winrt::hstring token);
		// 最近使ったファイルのメニュー項目を更新する.
		void file_recent_menu_update(void);
		// 図形データをストレージファイルに非同期に書き込む.
		template <bool SUSPEND, bool SETTING> IAsyncOperation<winrt::hresult> file_write_gpf_async(StorageFile gpf_file);
		// ファイルメニューの「他の形式としてエクスポートする」が選択された
		IAsyncAction file_export_as_click_async(IInspectable const&, RoutedEventArgs const&);

		//-------------------------------
		// MainPage_color.cpp
		// 色設定
		//-------------------------------

		// 「...色」が選択された.
		template <UNDO_T U> IAsyncAction color_click_async(void);
		// ヘルプメニューの「色成分の記法」に印をつける.
		void color_code_is_checked(const COLOR_CODE c_code);
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
		// レイアウトメニューの「ページの色」が選択された.
		void color_page_click(IInspectable const&, RoutedEventArgs const&) { color_click_async<UNDO_T::PAGE_COLOR>(); }
		// 属性メニューの「線枠の色」が選択された.
		void color_stroke_click(IInspectable const&, RoutedEventArgs const&) { color_click_async<UNDO_T::STROKE_COLOR>(); }

		//-------------------------------
		// MainPage_edit.cpp
		// 多角形の終端, 文字列の編集, 円弧の傾きの編集
		//-------------------------------

		// 編集メニューの「円弧の傾きの編集」が選択された.
		IAsyncAction meth_arc_click_async(IInspectable const&, RoutedEventArgs const&);
		// 編集メニューの「多角形の終端」のサブ項目が選択された.
		void meth_poly_end_click(IInspectable const&, RoutedEventArgs const&);
		// 編集メニューの「文字列の編集」が選択された.
		IAsyncAction meth_text_edit_click_async(IInspectable const&, RoutedEventArgs const&);
		// 操作メニューの「枠を文字列に合わせる」が選択された.
		void meth_text_fit_frame_click(IInspectable const&, RoutedEventArgs const&);

		//-------------------------------
		// MainPage_find.cpp
		// 文字列の編集と, 検索/置換
		//-------------------------------

		// 次を検索する.
		bool find_next(void);
		// 編集メニューの「文字列の検索/置換」が選択された.
		void find_text_click(IInspectable const&, RoutedEventArgs const&);
		// 文字列検索パネルの「閉じる」ボタンが押された.
		void find_text_close_click(IInspectable const&, RoutedEventArgs const&);
		//　文字列検索パネルの「次を検索」ボタンが押された.
		void find_next_click(IInspectable const&, RoutedEventArgs const&);
		// 文字列検索パネルの値を保存する.
		void find_text_preserve(void);
		// 検索文字列が変更された.
		void find_text_what_changed(IInspectable const&, TextChangedEventArgs const&);
		// 置換して次を検索する.
		bool replace_and_find(void);
		// 文字列検索パネルの「すべて置換」ボタンが押された.
		void replace_all_click(IInspectable const&, RoutedEventArgs const&);
		// 文字列検索パネルの「置換して次に」ボタンが押された.
		void replace_and_find_click(IInspectable const&, RoutedEventArgs const&);

		//-------------------------------
		//　MainPage_font.cpp
		//　書体
		//-------------------------------

		// 書体メニューの「書体名」が選択された.
		IAsyncAction font_family_click_async(IInspectable const&, RoutedEventArgs const&);
		// 書体メニューの「字体」のサブ項目が選択された.
		void font_style_click(IInspectable const&, RoutedEventArgs const&);
		// 書体メニューの「字体」のサブ項目に印をつける
		void font_style_is_checked(const DWRITE_FONT_STYLE f_style);
		// 書体メニューの「書体の大きさ」が選択された.
		IAsyncAction font_size_click_async(IInspectable const&, RoutedEventArgs const&);
		// 書体メニューの「書体の幅」のサブ項目が選択された.
		void font_stretch_click(IInspectable const&, RoutedEventArgs const&);
		// 書体メニューの「書体の幅」のサブ項目に印をつける
		void font_stretch_is_checked(const DWRITE_FONT_STRETCH val);
		// 書体メニューの「書体の太さ」のサブ項目が選択された.
		void font_weight_click(IInspectable const&, RoutedEventArgs const&);
		// 書体メニューの「書体の太さ」のサブ項目に印をつける
		void font_weight_is_checked(const DWRITE_FONT_WEIGHT val);

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

		// 操作メニューの「画像の縦横比を維持」が選択された.
		void image_keep_asp_click(IInspectable const&, RoutedEventArgs const&) noexcept;
		// 操作メニューの「画像の縦横比を維持」に印をつける.
		void image_keep_aspect_is_checked(const bool keep_aspect);
		// 操作メニューの「原画像に戻す」が選択された.
		void image_revert_click(IInspectable const&, RoutedEventArgs const&) noexcept;

		//-----------------------------
		// MainPage_help.cpp
		// ヘルプ
		// 長さの単位, 色の基数, バージョン情報
		//-----------------------------

		IAsyncAction about_graph_paper_click(IInspectable const&, RoutedEventArgs const&);
		// ヘルプメニューの「長さの単位」に印をつける.
		void len_unit_is_checked(const LEN_UNIT l_unit);
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
		bool scroll_to(const Shape* const s);

		//-------------------------------
		// MainPage_select.cpp
		// 図形の選択
		//-------------------------------

		// 編集メニューの「すべて選択」が選択された.
		void select_shape_all_click(IInspectable const&,RoutedEventArgs const&);
		// 矩形に含まれる図形を選択し, 含まれない図形の選択を解除する.
		bool select_shape_inside(const D2D1_POINT_2F area_lt, const D2D1_POINT_2F area_rb);
		// 範囲の中の図形を選択して, それ以外の図形の選択をはずす.
		bool select_shape_range(Shape* const s_from, Shape* const s_to);
		// 矩形に含まれる図形の選択を反転する.
		bool toggle_shape_inside(const D2D1_POINT_2F area_lt, const D2D1_POINT_2F area_rb);
		// すべての図形の選択を解除する.
		bool unselect_shape_all(const bool t_range_only = false);
		//　Shft + 下矢印キーが押された.
		//void select_shape_range_next_invoked(IInspectable const&, KeyboardAcceleratorInvokedEventArgs const&);
		//　Shift + 上矢印キーが押された.
		//void select_shape_range_prev_invoked(IInspectable const&, KeyboardAcceleratorInvokedEventArgs const&);
		//　下矢印キーが押された.
		//void select_shape_next_invoked(IInspectable const&, KeyboardAcceleratorInvokedEventArgs const&);
		//　上矢印キーが押された.
		//void select_shape_prev_invoked(IInspectable const&, KeyboardAcceleratorInvokedEventArgs const&);
		//　Escape が押された.
		void select_tool_invoked(IInspectable const&, KeyboardAcceleratorInvokedEventArgs const&);

		//-------------------------------
		// MainPage_layout.cpp
		// レイアウト
		//-------------------------------

		// 背景パターンの画像ブラシを得る.
		void background_get_brush(void) noexcept;
		// レイアウトメニューの「背景パターン」>「市松模様を表示」がクリックされた.
		void background_pattern_click(IInspectable const&, RoutedEventArgs const&);
		// レイアウトメニューの「背景パターン」のサブ項目に印をつける.
		void background_color_is_checked(const bool checker_board, const D2D1_COLOR_F& color);
		// レイアウトメニューの「方眼の強調」のサブ項目に印をつける.
		void grid_emph_is_checked(const GRID_EMPH& g_emph);
		// レイアウトメニューの「方眼の表示」のサブ項目にに印をつける.
		void grid_show_is_checked(const GRID_SHOW g_show);
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
		// レイアウトメニューの「ページの大きさ」が選択された
		IAsyncAction page_size_click_async(IInspectable const&, RoutedEventArgs const&);
		// ページの大きさダイアログのテキストボックスの値が変更された.
		void page_size_value_changed(IInspectable const&, NumberBoxValueChangedEventArgs const&);
		// ページの大きさダイアログのコンボボックスの選択が変更された.
		void page_size_selection_changed(IInspectable const&, SelectionChangedEventArgs const& args) noexcept;
		// レイアウトメニューの「ページのズーム」が選択された.
		void page_zoom_click(IInspectable const& sender, RoutedEventArgs const&);
		// レイアウトメニューの「ページのズーム」のサブ項目に印をつける.
		void page_zoom_is_checked(float scale);

		//-------------------------------
		// MainPage_status.cpp
		// ステータスバー
		//-------------------------------

		// レイアウトメニューの「ステータスバー」が選択された.
		void status_bar_click(IInspectable const&, RoutedEventArgs const&);
		// レイアウトメニューの「ステータスバー」に印をつける.
		void status_bar_is_checked(const STATUS_BAR a);
		// ポインターの位置をステータスバーに格納する.
		void status_bar_set_pos(void);
		// 作図ツールをステータスバーに格納する.
		void status_bar_set_draw(void);
		// 方眼の大きさをステータスバーに格納する.
		void status_bar_set_grid(void);
		// ページの大きさをステータスバーに格納する.
		void status_bar_set_page(void);
		// 単位をステータスバーに格納する.
		void status_bar_set_unit(void);
		// 拡大率をステータスバーに格納する.
		void status_bar_set_zoom(void);

		//------------------------------
		// MainPage_stroke.cpp
		// 線枠
		//------------------------------

		// 属性メニューの「矢じるしの形式」のサブ項目が選択された.
		void stroke_arrow_click(IInspectable const& sender, RoutedEventArgs const&);
		// 属性メニューの「矢じるしの形式」のサブ項目に印をつける.
		void stroke_arrow_is_checked(const ARROW_STYLE val);
		// 属性メニューの「矢じるしの大きさ」が選択された.
		IAsyncAction stroke_arrow_size_click_async(IInspectable const&, RoutedEventArgs const&);
		// 属性メニューの「端の形式」のサブ項目に印をつける.
		void stroke_cap_is_checked(const D2D1_CAP_STYLE& s_cap);
		// 属性メニューの「端の形式」のサブ項目が選択された.
		void stroke_cap_click(IInspectable const& sender, RoutedEventArgs const&);
		// 属性メニューの「破線の形式」のサブ項目が選択された.
		void stroke_dash_click(IInspectable const& sender, RoutedEventArgs const&);
		// 属性メニューの「破線の形式」のサブ項目に印をつける.
		void stroke_dash_is_checked(const D2D1_DASH_STYLE d_style);
		// 属性メニューの「破線の配列」が選択された.
		IAsyncAction stroke_dash_pat_click_async(IInspectable const&, RoutedEventArgs const&);
		// 属性メニューの「線の結合の形式」>「尖り制限」が選択された.
		IAsyncAction stroke_join_limit_click_async(IInspectable const&, RoutedEventArgs const&);
		// 属性メニューの「線の結合の形式」のサブ項目に印をつける.
		void stroke_join_is_checked(const D2D1_LINE_JOIN s_join);
		// 属性メニューの「結合の形式」のサブ項目が選択された.
		void stroke_join_click(IInspectable const& sender, RoutedEventArgs const&);
		// 属性メニューの「線枠の太さ」のサブ項目が選択された.
		void stroke_width_click(IInspectable const& sender, RoutedEventArgs const&);
		// 属性メニューの「線枠の太さ」>「その他」が選択された.
		IAsyncAction stroke_width_click_async(IInspectable const&, RoutedEventArgs const&);
		// 属性メニューの「線枠の太さ」のサブ項目に印をつける.
		void stroke_width_is_checked(const float s_width) noexcept;

		//-------------------------------
		// MainPage_summary.cpp
		// 一覧
		//-------------------------------

		// 図形を一覧に追加する.
		void summary_append(Shape* const s);
		// 一覧の中で図形を入れ替える.
		void summary_order(Shape* const s, Shape* const t);
		// 一覧を消去する.
		void summary_clear(void);
		// 一覧パネルを閉じて消去する.
		//void summary_close(void);
		// 一覧の「閉じる」ボタンが押された.
		void summary_close_click(IInspectable const&, RoutedEventArgs const&);
		// 一覧に図形を挿入する.
		void summary_insert_at(Shape* const s, const uint32_t i);
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
		void summary_selection_changed(IInspectable const& sender, SelectionChangedEventArgs const& args);
		// 一覧の図形を選択解除する.
		void summary_unselect(Shape* const s);
		// 一覧の項目を全て選択解除する.
		void summary_unselect_shape_all(void);
		// 一覧の表示を更新する.
		void summary_update(void);

		//------------------------------
		// MainPage_svg.cpp
		// 線枠の色, 太さ
		//------------------------------

		// 図形データを SVG としてストレージファイルに非同期に書き込む.
		// 図形をデータライターに PDF として書き込む.
		IAsyncOperation<winrt::hresult> export_as_pdf_async(const StorageFile& pdf_file) noexcept;
		IAsyncOperation<winrt::hresult> export_as_svg_async(const StorageFile& svg_file) noexcept;
		IAsyncOperation<winrt::hresult> export_as_raster_async(const StorageFile& image_file) noexcept;

		//-------------------------------
		//　MainPage_text.cpp
		//　文字列の配置
		//-------------------------------

		// 書体メニューの「文字列のそろえ」のサブ項目が選択された
		void text_align_horz_click(IInspectable const& sender, RoutedEventArgs const&);
		// 書体メニューの「文字列のそろえ」のサブ項目に印をつける.
		void text_align_horz_is_checked(const DWRITE_TEXT_ALIGNMENT val);
		// 書体メニューの「段落のそろえ」のサブ項目が選択された
		void text_align_vert_click(IInspectable const& sender, RoutedEventArgs const&);
		// 書体メニューの「段落のそろえ」のサブ項目に印をつける.
		void text_align_vert_is_checked(const DWRITE_PARAGRAPH_ALIGNMENT val);
		// 書体メニューの「行間...」が選択された.
		IAsyncAction text_line_sp_click_async(IInspectable const&, RoutedEventArgs const&);
		// 書体メニューの「余白...」が選択された.
		IAsyncAction text_pad_click_async(IInspectable const&, RoutedEventArgs const&);
		// 書体メニューの「文字列の折り返し」のサブ項目が選択された
		void text_word_wrap_click(IInspectable const& sender, RoutedEventArgs const&);
		// 書体メニューの「文字列の折り返し」のサブ項目に印をつける
		void text_word_wrap_is_checked(const DWRITE_WORD_WRAPPING val);

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
		// 作図メニューに印をつける.
		void drawing_tool_is_checked(const DRAWING_TOOL val);
		// 作図メニューの多角形の頂点数にチェックをつける.
		void drawing_poly_vtx_is_checked(const uint32_t val);
		// 作図メニューの多角形の選択肢にチェックをつける.
		void drawing_poly_opt_is_checked(const POLY_OPTION& val);

		//-----------------------------
		// MainPage_undo.cpp
		// 元に戻すとやり直し操作
		//-----------------------------

		// 元に戻す/やり直しメニューの可否を設定する.
		void undo_menu_is_enabled(void);
		// 編集メニューの「やり直し」が選択された.
		void redo_click(IInspectable const&, RoutedEventArgs const&);
		// 編集メニューの「元に戻す」が選択された.
		void undo_click(IInspectable const&, RoutedEventArgs const&);
		// 操作スタックを消去し, 含まれる操作を破棄する.
		void undo_clear(void);
		// 操作を実行する.
		void undo_exec(Undo* u);
		// 無効な操作をポップする.
		bool undo_pop_invalid(void);
		// 図形を追加して, その操作をスタックに積む.
		void undo_push_append(Shape* s)
		{
			m_ustack_undo.push_back(new UndoAppend(s));
		}
		// 図形をグループ図形に追加して, その操作をスタックに積む.
		void undo_push_append(ShapeGroup* g, Shape* s)
		{
			m_ustack_undo.push_back(new UndoAppendG(g, s));
		}
		// 図形を入れ替えて, その操作をスタックに積む.
		void undo_push_order(Shape* const s, Shape* const t)
		{
			m_ustack_undo.push_back(new UndoOrder(s, t));
		}
		// 指定した部位の点をスタックに保存する.
		void undo_push_position(Shape* const s, const uint32_t loc)
		{
			m_ustack_undo.push_back(new UndoDeform(s, loc));
		}
		// 画像の現在の位置や大きさ、不透明度を操作スタックにプッシュする.
		void undo_push_image(Shape* const s)
		{
			m_ustack_undo.push_back(new UndoImage(static_cast<ShapeImage*>(s)));
		}
		// 図形を挿入して, その操作をスタックに積む.
		void MainPage::undo_push_insert(Shape* s, Shape* s_pos)
		{
			m_ustack_undo.push_back(new UndoInsert(s, s_pos));
		}
		// 選択された (あるいは全ての) 図形の位置をスタックに保存してから差分だけ移動する.
		void undo_push_move(const D2D1_POINT_2F pos, const bool any = false);
		// 一連の操作の区切としてヌル操作をスタックに積む.
		void undo_push_null(void);
		// 図形をグループから取り去り, その操作をスタックに積む.
		void undo_push_remove(Shape* g, Shape* s)
		{
			m_ustack_undo.push_back(new UndoRemoveG(g, s));
		}
		// 図形を取り去り, その操作をスタックに積む.
		void undo_push_remove(Shape* s)
		{
			m_ustack_undo.push_back(new UndoRemove(s));
		}
		// 図形の選択を反転して, その操作をスタックに積む.
		void undo_push_select(Shape* const s);
		// 値を図形へ格納して, その操作をスタックに積む.
		template <UNDO_T U, typename T> void undo_push_set(Shape* const s, T const& val);
		// 値を選択された図形に格納して, その操作をスタックに積む.
		template <UNDO_T U, typename T> bool undo_push_set(T const& val);
		// 図形の値の保存を実行して, その操作をスタックに積む.
		template <UNDO_T U> void undo_push_set(Shape* const s);
		void undo_push_text_unselect(ShapeText* s)
		{
			const auto start = m_main_page.m_select_trail ? m_main_page.m_select_end + 1 : m_main_page.m_select_end;
			undo_push_text_select(s, start, m_main_page.m_select_end, m_main_page.m_select_trail);
		}
		// 文字列の選択を実行して, その操作をスタックに積む.
		void undo_push_text_select(Shape* s, const int start, const int end, const bool trail)
		{
			// 文字列の選択の操作が連続するかぎり,
			// スタックをさかのぼって, 同じ図形に対する文字列の選択があったなら
			// 図形の文字列の選択を直接上書きする. スタックに操作を積まない.
			for (auto u = m_ustack_undo.rbegin();
				u != m_ustack_undo.rend() && *u != nullptr &&
				typeid(*u) == typeid(UndoTextSelect); u++) {
				if ((*u)->m_shape == s) {
					ShapeText* t = static_cast<ShapeText*>(s);
					m_main_page.m_select_start = start;
					m_main_page.m_select_end = end;
					m_main_page.m_select_trail = trail;
					return;
				}
			}
			// そうでなければ, スタックに操作を積む.
			m_ustack_undo.push_back(new UndoTextSelect(s, start, end, trail));
		}
		// データリーダーから操作スタックを読み込む.
		void undo_read_stack(DataReader const& dt_reader);
		// データリーダーに操作スタックを書き込む.
		void undo_write_stack(DataWriter const& dt_writer);

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
		// 編集メニューの可否を設定する.
		void xcvd_menu_is_enabled(void);
		// 編集メニューの「貼り付け」が選択された.
		void xcvd_paste_click(IInspectable const&, RoutedEventArgs const&);
		// 画像を貼り付ける.
		IAsyncAction xcvd_paste_image(void);
		// 図形を貼り付ける.
		IAsyncAction xcvd_paste_shape(void);
		// 文字列を貼り付ける.
		IAsyncAction xcvd_paste_text(void);
		// 貼り付ける点を得る
		void xcvd_paste_pos(D2D1_POINT_2F& p, const D2D1_POINT_2F q) const noexcept;

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
};
	template void MainPage::undo_push_set<UNDO_T::MOVE>(Shape* const s);
	template void MainPage::undo_push_set<UNDO_T::IMAGE_OPAC>(Shape* const s);



}

namespace winrt::GraphPaper::factory_implementation
{
	struct MainPage : MainPageT<MainPage, implementation::MainPage>
	{
	};
}
