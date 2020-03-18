//-------------------------------
// MainPage_layout.cpp
// レイアウトの初期化, 保存と削除
//-------------------------------
#include "pch.h"
#include "MainPage.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	constexpr wchar_t FILE_NAME[] = L"ji32k7au4a83";	// アプリケーションデータを格納するファイル名

	// レイアウトデータを保存するフォルダーを得る.
	static auto local_folder(void);

	// レイアウトデータを保存するフォルダーを得る.
	static auto local_folder(void)
	{
		using winrt::Windows::Storage::ApplicationData;
		return ApplicationData::Current().LocalFolder();
	}

	// ページレイアウトを既定値で初期化する.
	void MainPage::layout_init(void)
	{
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

		// 文字範囲の背景色, 文字範囲の文字色をリソースから得る.
		{
			using winrt::Windows::UI::Color;
			m_page_dx.m_range_bcolor = { 0.0f, 1.0f / 3.0f, 2.0f / 3.0f, 1.0f };
			m_page_dx.m_range_tcolor = S_WHITE;
			try {
				auto b_res = Resources().Lookup(box_value(L"SystemColorHighlightColor"));
				auto t_res = Resources().Lookup(box_value(L"SystemColorHighlightTextColor"));
				cast_to(unbox_value<Color>(b_res), m_page_dx.m_range_bcolor);
				cast_to(unbox_value<Color>(t_res), m_page_dx.m_range_tcolor);
			}
			catch (winrt::hresult_error) {
			}
		}

		{
			using winrt::Windows::UI::Xaml::Media::Brush;

			const double dpi = DisplayInformation::GetForCurrentView().LogicalDpi();
			// 色の初期値はテーマに依存する.
			D2D1_COLOR_F b_color = S_WHITE;
			D2D1_COLOR_F f_color = S_BLACK;
			try {
				auto const& b_res = Resources().Lookup(box_value(L"ApplicationPageBackgroundThemeBrush"));
				auto const& f_res = Resources().Lookup(box_value(L"ApplicationForegroundThemeBrush"));
				cast_to(unbox_value<Brush>(b_res), b_color);
				cast_to(unbox_value<Brush>(f_res), f_color);
			}
			catch (winrt::hresult_error e) {
			}
			m_page_layout.m_arrow_size = ARROW_SIZE();
			m_page_layout.m_arrow_style = ARROW_STYLE::NONE;
			m_page_layout.m_corner_rad = { GRIDLEN_PX, GRIDLEN_PX };
			m_page_layout.set_fill_color(b_color);
			m_page_layout.set_font_color(f_color);
			m_page_layout.m_grid_base = static_cast<double>(GRIDLEN_PX) - 1.0;
			m_page_layout.m_grid_opac = GRID_OPAC;
			m_page_layout.m_grid_show = GRID_SHOW::BACK;
			m_page_layout.m_grid_snap = true;
			m_page_layout.set_page_color(b_color);
			m_page_layout.m_page_scale = 1.0;
			m_page_layout.m_page_size.width = static_cast<FLOAT>(std::floor(A4_PER_INCH.width * dpi));
			m_page_layout.m_page_size.height = static_cast<FLOAT>(std::floor(A4_PER_INCH.height * dpi));
			m_page_layout.set_stroke_color(f_color);
			m_page_layout.m_stroke_pattern = STROKE_PATTERN();
			m_page_layout.m_stroke_style = D2D1_DASH_STYLE::D2D1_DASH_STYLE_SOLID;
			m_page_layout.m_stroke_width = 1.0F;
			m_page_layout.m_text_align_p = DWRITE_PARAGRAPH_ALIGNMENT::DWRITE_PARAGRAPH_ALIGNMENT_NEAR;
			m_page_layout.m_text_align_t = DWRITE_TEXT_ALIGNMENT::DWRITE_TEXT_ALIGNMENT_LEADING;
			m_page_layout.m_text_line = 0.0F;
			m_page_layout.m_text_margin = { 4.0F, 4.0F };
		}

	}

	// 保存されたレイアウトデータを読み込む.
	// レイアウトを保存したファイルがローカルフォルダーにある場合, それを読み込む.
	// 戻り値	読み込めたら S_OK.
	IAsyncOperation<winrt::hresult> MainPage::layout_load_async(void)
	{
		mfi_layout_reset().IsEnabled(false);
		auto hr = E_FAIL;
		auto item{ co_await local_folder().TryGetItemAsync(FILE_NAME) };
		if (item != nullptr) {
			auto s_file = item.try_as<StorageFile>();
			if (s_file != nullptr) {
				try {
					hr = co_await file_read_async(s_file, false, true);
				}
				catch (winrt::hresult_error const& e) {
					hr = e.code();
				}
				s_file = nullptr;
				mfi_layout_reset().IsEnabled(true);
			}
			item = nullptr;
		}
		co_return hr;
	}

	// レイアウトメニューの「レイアウトを保存」が選択された
	// ローカルフォルダーにファイルを作成し, レイアウトを保存する.
	IAsyncAction MainPage::mfi_layout_save_click_async(IInspectable const&, RoutedEventArgs const&)
	{
		using winrt::Windows::Storage::CreationCollisionOption;

		try {
			auto s_file{ co_await local_folder().CreateFileAsync(FILE_NAME, CreationCollisionOption::ReplaceExisting) };
			if (s_file != nullptr) {
				co_await file_write_layout_async(s_file);
				s_file = nullptr;
				mfi_layout_reset().IsEnabled(true);
			}
		}
		catch (winrt::hresult_error const&) {
		}
		//using winrt::Windows::Storage::AccessCache::StorageApplicationPermissions;
		//auto const& mru_list = StorageApplicationPermissions::MostRecentlyUsedList();
		//mru_list.Clear();
	}

	// レイアウトメニューの「レイアウトをリセット」が選択された.
	// レイアウトを保存したファイルがローカルフォルダーにある場合, それを削除する.
	IAsyncAction MainPage::mfi_layout_reset_click_async(IInspectable const&, RoutedEventArgs const&)
	{
		using winrt::Windows::Storage::StorageDeleteOption;

		auto item{ co_await local_folder().TryGetItemAsync(FILE_NAME) };
		if (item != nullptr) {
			auto s_file = item.try_as<StorageFile>();
			if (s_file != nullptr) {
				try {
					co_await s_file.DeleteAsync(StorageDeleteOption::PermanentDelete);
					mfi_layout_reset().IsEnabled(false);
				}
				catch (winrt::hresult_error const&) {
				}
				s_file = nullptr;
			}
			item = nullptr;
		}
	}

}