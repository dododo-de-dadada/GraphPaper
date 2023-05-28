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

	// 操作メニューの「画像を原画像に戻す」が選択された.
	void MainPage::image_revert_click(IInspectable const&, RoutedEventArgs const&) noexcept
	{
		undo_push_null();
		for (Shape* const s : m_main_page.m_shape_list) {
			if (s->is_deleted() || !s->is_selected() || typeid(*s) != typeid(ShapeImage)) {
				continue;
			}
			// 画像の現在の位置や大きさ、不透明度を操作スタックにプッシュする.
			undo_push_image(s);
			static_cast<ShapeImage*>(s)->revert();
		}
		main_panel_size();
		main_draw();
		status_bar_set_pos();
	}

}