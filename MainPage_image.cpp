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

	// �摜���j���[�́u�摜�̏c������ێ��v���I�����ꂽ.
	void MainPage::image_keep_aspect_click(IInspectable const&, RoutedEventArgs const&) noexcept
	{
		m_image_keep_aspect = !m_image_keep_aspect;
		status_bar_set_pos();
	}

	// �摜���j���[�́u�摜�̏c������ێ��v�Ɉ������.
	void MainPage::image_keep_aspect_is_checked(const bool keep_aspect)
	{
		tmfi_menu_image_keep_aspect().IsChecked(keep_aspect);
		tmfi_menu_image_keep_aspect_2().IsChecked(keep_aspect);
	}

	// �摜���j���[�́u���摜�ɖ߂��v���I�����ꂽ.
	void MainPage::image_revert_click(IInspectable const&, RoutedEventArgs const&) noexcept
	{
		for (Shape* const s : m_main_page.m_shape_list) {
			if (s->is_deleted() || !s->is_selected() || typeid(*s) != typeid(ShapeImage)) {
				continue;
			}
			// �摜�̌��݂̈ʒu��傫���A�s�����x�𑀍�X�^�b�N�Ƀv�b�V������.
			undo_push_image(s);
			static_cast<ShapeImage*>(s)->revert();
		}
		undo_push_null();
		undo_menu_is_enabled();
		//xcvd_menu_is_enabled();
		main_panel_size();
		main_draw();
		status_bar_set_pos();
	}

	// �摜���j���[�́u�s�����x...�v���I�����ꂽ.
	IAsyncAction MainPage::image_opac_click_async(IInspectable const&, RoutedEventArgs const&)
	{
		const winrt::hstring str_opacity{ ResourceLoader::GetForCurrentView().GetString(L"str_opacity") + L": " };
		const winrt::hstring str_title{ ResourceLoader::GetForCurrentView().GetString(L"str_image_opac") };
		const winrt::hstring str_color_code{ ResourceLoader::GetForCurrentView().GetString(L"str_color_code") };

		m_mutex_event.lock();
		m_prop_page.set_attr_to(&m_main_page);

		dialog_image_load_async(
			static_cast<float>(scp_dialog_panel().Width()),
			static_cast<float>(scp_dialog_panel().Height()));

		const float val = static_cast<float>(conv_color_comp(m_prop_page.m_image_opac));
		dialog_slider_0().Minimum(0.0);
		dialog_slider_0().Maximum(255.0);
		dialog_slider_0().TickFrequency(1.0);
		dialog_slider_0().SnapsTo(SliderSnapsTo::Ticks);
		dialog_slider_0().Value(val);
		wchar_t buf[32];
		conv_col_to_str(m_color_code, val, buf);
		dialog_slider_0().Header(box_value(str_opacity + buf));
		dialog_slider_0().Visibility(Visibility::Visible);

		dialog_combo_box_0().Header(box_value(str_color_code));
		dialog_combo_box_0().Items().Append(box_value(rmfi_menu_color_code_dec().Text()));
		dialog_combo_box_0().Items().Append(box_value(rmfi_menu_color_code_hex().Text()));
		dialog_combo_box_0().Items().Append(box_value(rmfi_menu_color_code_real().Text()));
		dialog_combo_box_0().Items().Append(box_value(rmfi_menu_color_code_pct().Text()));
		if (m_color_code == COLOR_CODE::DEC) {
			dialog_combo_box_0().SelectedIndex(0);
		}
		else if (m_color_code == COLOR_CODE::HEX) {
			dialog_combo_box_0().SelectedIndex(1);
		}
		else if (m_color_code == COLOR_CODE::REAL) {
			dialog_combo_box_0().SelectedIndex(2);
		}
		else if (m_color_code == COLOR_CODE::PCT) {
			dialog_combo_box_0().SelectedIndex(3);
		}
		dialog_combo_box_0().Visibility(Visibility::Visible);

		cd_dialog_prop().Title(box_value(str_title));
		{
			const auto revoker{
				dialog_slider_0().ValueChanged(
					winrt::auto_revoke,
					[this, str_opacity](IInspectable const&, RangeBaseValueChangedEventArgs const& args) {
						const float val = static_cast<float>(args.NewValue());
						wchar_t buf[32];
						conv_col_to_str(m_color_code, val, buf);
						dialog_slider_0().Header(box_value(str_opacity + buf));
						if (m_prop_page.back()->set_image_opacity(val / COLOR_MAX)) {
							dialog_draw();
						}
					}
				)
			};
			const auto revoker4{
				dialog_combo_box_0().SelectionChanged(winrt::auto_revoke, [this, str_opacity](IInspectable const&, SelectionChangedEventArgs const&) {
					COLOR_CODE c_code;
					if (dialog_combo_box_0().SelectedIndex() == 0) {
						c_code = COLOR_CODE::DEC;
					}
					else if (dialog_combo_box_0().SelectedIndex() == 1) {
						c_code = COLOR_CODE::HEX;
					}
					else if (dialog_combo_box_0().SelectedIndex() == 2) {
						c_code = COLOR_CODE::REAL;
					}
					else if (dialog_combo_box_0().SelectedIndex() == 3) {
						c_code = COLOR_CODE::PCT;
					}
					else {
						return;
					}
					wchar_t buf[32];
					conv_col_to_str(c_code, dialog_slider_0().Value(), buf);
					dialog_slider_0().Header(box_value(str_opacity + buf));
				})
			};
			if (co_await cd_dialog_prop().ShowAsync() == ContentDialogResult::Primary) {
				float new_val;
				m_prop_page.back()->get_image_opacity(new_val);
				if (dialog_combo_box_0().SelectedIndex() == 0) {
					m_color_code = COLOR_CODE::DEC;
				}
				else if (dialog_combo_box_0().SelectedIndex() == 1) {
					m_color_code = COLOR_CODE::HEX;
				}
				else if (dialog_combo_box_0().SelectedIndex() == 2) {
					m_color_code = COLOR_CODE::REAL;
				}
				else if (dialog_combo_box_0().SelectedIndex() == 3) {
					m_color_code = COLOR_CODE::PCT;
				}
				color_code_is_checked(m_color_code);
				if (undo_push_set<UNDO_T::IMAGE_OPAC>(new_val)) {
					undo_push_null();
					undo_menu_is_enabled();
					main_draw();
				}
			}
		}
		slist_clear(m_prop_page.m_shape_list);
		dialog_slider_0().Visibility(Visibility::Collapsed);
		dialog_combo_box_0().Visibility(Visibility::Collapsed);
		dialog_combo_box_0().Items().Clear();
		main_draw();
		m_mutex_event.unlock();
	}

	/*
	// �l���X���C�_�[�̃w�b�_�[�Ɋi�[����.
	// U	����̎��ʎq
	// S	�X���C�_�[�̔ԍ�
	// val	�i�[����l
	// �߂�l	�Ȃ�.
	void MainPage::image_slider_set_header(const float val)
	{
		constexpr wchar_t R[]{ L"str_opacity" };
		wchar_t buf[32];
		conv_col_to_str(m_color_code, val, buf);
		const winrt::hstring text = ResourceLoader::GetForCurrentView().GetString(R) + L": " + buf;
		dialog_set_slider_header<0>(text);
	}

	// �X���C�_�[�̒l���ύX���ꂽ.
	// U	����̎��ʎq
	// S	�X���C�_�[�̔ԍ�
	// args	ValueChanged �œn���ꂽ����
	// �߂�l	�Ȃ�
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