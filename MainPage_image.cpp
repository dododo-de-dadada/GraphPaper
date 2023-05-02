//-------------------------------
// MainPage_image.cpp
// �摜
//-------------------------------
#include "pch.h"
#include "MainPage.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	using winrt::Windows::ApplicationModel::Resources::ResourceLoader;
	using winrt::Windows::UI::Xaml::Controls::ContentDialogResult;
	using winrt::Windows::UI::Xaml::Controls::Primitives::SliderSnapsTo;

	// ���상�j���[�́u�摜�����摜�ɖ߂��v���I�����ꂽ.
	void MainPage::image_revert_click(IInspectable const&, RoutedEventArgs const&) noexcept
	{
		undo_push_null();
		for (Shape* const s : m_main_page.m_shape_list) {
			if (s->is_deleted() || !s->is_selected() || typeid(*s) != typeid(ShapeImage)) {
				continue;
			}
			// �摜�̌��݂̈ʒu��傫���A�s�����x�𑀍�X�^�b�N�Ƀv�b�V������.
			undo_push_image(s);
			static_cast<ShapeImage*>(s)->revert();
		}
		undo_menu_is_enabled();
		//xcvd_menu_is_enabled();
		main_panel_size();
		main_draw();
		status_bar_set_pos();
	}

	// ���상�j���[�́u�摜�̏c������ێ��v���I�����ꂽ.
	void MainPage::image_keep_asp_click(IInspectable const&, RoutedEventArgs const&) noexcept
	{
		m_image_keep_aspect = !m_image_keep_aspect;
		status_bar_set_pos();
	}

	// ���상�j���[�́u�摜�̏c������ێ��v�Ɉ������.
	void MainPage::image_keep_aspect_is_checked(const bool keep_aspect)
	{
		tmfi_menu_meth_image_keep_asp().IsChecked(keep_aspect);
		tmfi_menu_meth_image_keep_asp_2().IsChecked(keep_aspect);
	}

}