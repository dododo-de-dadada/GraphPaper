//-------------------------------
// MainPage_image.cpp
// 画像
//-------------------------------
#include "pch.h"
#include "MainPage.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	using winrt::Windows::ApplicationModel::Resources::ResourceLoader;
	using winrt::Windows::UI::Xaml::Controls::ContentDialogResult;
	using winrt::Windows::UI::Xaml::Controls::Primitives::SliderSnapsTo;

	/*
	// 値をスライダーのヘッダーに格納する.
	// U	操作の識別子
	// S	スライダーの番号
	// val	格納する値
	// 戻り値	なし.
	void MainPage::image_slider_set_header(const float val)
	{
		constexpr wchar_t R[]{ L"str_opacity" };
		wchar_t buf[32];
		conv_col_to_str(m_color_code, val, buf);
		const winrt::hstring text = ResourceLoader::GetForCurrentView().GetString(R) + L": " + buf;
		dialog_set_slider_header<0>(text);
	}

	// スライダーの値が変更された.
	// U	操作の識別子
	// S	スライダーの番号
	// args	ValueChanged で渡された引数
	// 戻り値	なし
	void MainPage::image_slider_val_changed(IInspectable const&, RangeBaseValueChangedEventArgs const& args)
	{
		const float val = static_cast<float>(args.NewValue());
		image_slider_set_header(val);
		if (m_prop_page.back()->set_image_opacity(val / COLOR_MAX)) {
			dialog_draw();
		}
	}
	*/

}