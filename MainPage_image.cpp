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
	using winrt::Windows::Foundation::IAsyncAction;
	using winrt::Windows::UI::Xaml::Controls::ContentDialogResult;
	using winrt::Windows::UI::Xaml::Controls::Primitives::RangeBaseValueChangedEventArgs;
	using winrt::Windows::UI::Xaml::Controls::Primitives::SliderSnapsTo;
	using winrt::Windows::UI::Xaml::RoutedEventArgs;

	// �摜���j���[�́u�c�����ς��Ȃ��v���I�����ꂽ.
	void MainPage::image_keep_aspect_click(IInspectable const&, RoutedEventArgs const&) noexcept
	{
		m_image_keep_aspect = !m_image_keep_aspect;
	}

	// �摜���j���[�́u�c�����ς��Ȃ��v�Ɉ������.
	void MainPage::image_keep_aspect_is_checked(const bool keep_aspect)
	{
		tmfi_image_keep_aspect().IsChecked(keep_aspect);
		tmfi_image_keep_aspect_2().IsChecked(keep_aspect);
	}

	// �摜���j���[�́u���̉摜�ɖ߂��v���I�����ꂽ.
	void MainPage::image_revert_origin_click(IInspectable const&, RoutedEventArgs const&) noexcept
	{
		for (Shape* const s : m_main_sheet.m_shape_list) {
			if (s->is_deleted() || !s->is_selected() || typeid(*s) != typeid(ShapeImage)) {
				continue;
			}
			// �摜�̌��݂̈ʒu��傫���A�s�����x�𑀍�X�^�b�N�Ƀv�b�V������.
			ustack_push_image(s);
			static_cast<ShapeImage*>(s)->revert();
		}
		ustack_push_null();
		sheet_panle_size();
		sheet_draw();
	}

	// �摜���j���[�́u�s�����x...�v���I�����ꂽ.
	IAsyncAction MainPage::image_opac_click_async(IInspectable const&, RoutedEventArgs const&)
	{
		m_prop_sheet.set_attr_to(&m_main_sheet);
		const auto val = m_prop_sheet.m_image_opac * COLOR_MAX;
		prop_slider_0().Maximum(255.0);
		prop_slider_0().TickFrequency(1.0);
		prop_slider_0().SnapsTo(SliderSnapsTo::Ticks);
		prop_slider_0().Value(val);
		image_slider_set_header<UNDO_OP::IMAGE_OPAC, 0>(val);
		prop_check_box().IsChecked(m_prop_sheet.m_image_opac_importing);

		prop_slider_0().Visibility(UI_VISIBLE);
		prop_check_box().Visibility(UI_VISIBLE);
		const auto slider_0_token = prop_slider_0().ValueChanged({ this, &MainPage::image_slider_val_changed<UNDO_OP::IMAGE_OPAC, 0> });

		prop_image_load_async(static_cast<float>(scp_prop_panel().Width()), static_cast<float>(scp_prop_panel().Height()));

		cd_prop_dialog().Title(box_value(ResourceLoader::GetForCurrentView().GetString(L"str_image_opac")));
		const auto d_result = co_await cd_prop_dialog().ShowAsync();
		if (d_result == ContentDialogResult::Primary) {
			float samp_val;
			m_prop_sheet.m_shape_list.back()->get_image_opacity(samp_val);
			ustack_push_set<UNDO_OP::IMAGE_OPAC>(&m_main_sheet, samp_val);
			if (ustack_push_set<UNDO_OP::IMAGE_OPAC>(samp_val)) {
				ustack_push_null();
				ustack_is_enable();
				xcvd_is_enabled();
				sheet_draw();
			}
		}
		slist_clear(m_prop_sheet.m_shape_list);
		prop_slider_0().Visibility(UI_COLLAPSED);
		prop_slider_0().ValueChanged(slider_0_token);
		prop_check_box().Visibility(UI_COLLAPSED);
		sheet_draw();
	}

	// �l���X���C�_�[�̃w�b�_�[�Ɋi�[����.
	// U	����̎��
	// S	�X���C�_�[�̔ԍ�
	// val	�i�[����l
	// �߂�l	�Ȃ�.
	template <UNDO_OP U, int S>
	void MainPage::image_slider_set_header(const float val)
	{
		winrt::hstring text;

		if constexpr (U == UNDO_OP::IMAGE_OPAC) {
			constexpr wchar_t R[]{ L"str_opacity" };
			wchar_t buf[32];
			conv_col_to_str(m_color_code, val, buf);
			text = ResourceLoader::GetForCurrentView().GetString(R) + L": " + buf;
		}
		prop_set_slider_header<S>(text);
	}

	// �X���C�_�[�̒l���ύX���ꂽ.
	// U	����̎��
	// S	�X���C�_�[�̔ԍ�
	// args	ValueChanged �œn���ꂽ����
	// �߂�l	�Ȃ�
	template <UNDO_OP U, int S>
	void MainPage::image_slider_val_changed(IInspectable const&, RangeBaseValueChangedEventArgs const& args)
	{
		if constexpr (U == UNDO_OP::IMAGE_OPAC) {
			if constexpr (S == 0) {
				const float val = static_cast<float>(args.NewValue());
				image_slider_set_header<U, S>(val);
				m_prop_sheet.m_shape_list.back()->set_image_opacity(val / COLOR_MAX);
			}
		}
		if (scp_prop_panel().IsLoaded()) {
			prop_sample_draw();
		}
	}

}