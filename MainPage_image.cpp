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