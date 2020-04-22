//-------------------------------
// MainPage_sheet.cpp
// �p���̕ۑ��ƃ��Z�b�g
//-------------------------------
#include "pch.h"
#include "MainPage.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	constexpr wchar_t FILE_NAME[] = L"ji32k7au4a83";	// �A�v���P�[�V�����f�[�^���i�[����t�@�C����

	// �p���f�[�^��ۑ�����t�H���_�[�𓾂�.
	static auto local_folder(void);

	// �p���f�[�^��ۑ�����t�H���_�[�𓾂�.
	static auto local_folder(void)
	{
		using winrt::Windows::Storage::ApplicationData;
		return ApplicationData::Current().LocalFolder();
	}

	// �p�����j���[�́u�p�������Z�b�g�v���I�����ꂽ.
	// �p���f�[�^��ۑ������t�@�C��������ꍇ, ������폜����.
	IAsyncAction MainPage::sheet_reset_click_async(IInspectable const&, RoutedEventArgs const&)
	{
		using winrt::Windows::Storage::StorageDeleteOption;

		auto item{ co_await local_folder().TryGetItemAsync(FILE_NAME) };
		if (item != nullptr) {
			auto s_file = item.try_as<StorageFile>();
			if (s_file != nullptr) {
				try {
					co_await s_file.DeleteAsync(StorageDeleteOption::PermanentDelete);
					mfi_sheet_reset().IsEnabled(false);
				}
				catch (winrt::hresult_error const&) {
				}
				s_file = nullptr;
			}
			item = nullptr;
		}
	}

	// �p���Ƃ��̑��̑���������������.
	void MainPage::sheet_init(void) noexcept
	{
		// ���̂̑���������������.
		{
			using winrt::Windows::UI::Xaml::Setter;
			using winrt::Windows::UI::Xaml::Controls::TextBlock;
			using winrt::Windows::UI::Xaml::Media::FontFamily;
			using winrt::Windows::UI::Text::FontWeight;
			using winrt::Windows::UI::Text::FontStretch;
			using winrt::Windows::UI::Xaml::Style;

			// ���\�[�X�̎擾�Ɏ��s�����ꍇ�ɔ�����,
			// �Œ�̊���l�����̑����Ɋi�[����.
			m_page_sheet.m_font_family = wchar_cpy(L"Segoe UI");
			m_page_sheet.m_font_size = 14.0;
			m_page_sheet.m_font_stretch = DWRITE_FONT_STRETCH::DWRITE_FONT_STRETCH_NORMAL;
			m_page_sheet.m_font_style = DWRITE_FONT_STYLE::DWRITE_FONT_STYLE_NORMAL;
			m_page_sheet.m_font_weight = DWRITE_FONT_WEIGHT::DWRITE_FONT_WEIGHT_NORMAL;
			// BodyTextBlockStyle �����\�[�X�f�B�N�V���i�����瓾��.
			auto resource = Resources().TryLookup(box_value(L"BodyTextBlockStyle"));
			if (resource != nullptr) {
				auto style = resource.try_as<Style>();
				std::list<Style> stack;
				// ���\�[�X����X�^�C���𓾂�.
				while (style != nullptr) {
					// �X�^�C������łȂ��ꍇ.
					// �X�^�C�����X�^�b�N�ɐς�.
					stack.push_back(style);
					// �X�^�C���̌p�����̃X�^�C���𓾂�.
					style = style.BasedOn();
				}
				try {
					while (stack.empty() != true) {
						// �X�^�b�N����łȂ��ꍇ,
						// �X�^�C�����X�^�b�N������o��.
						style = stack.back();
						stack.pop_back();
						// �X�^�C���̒��̊e�Z�b�^�[�ɂ���.
						auto const& setters = style.Setters();
						for (auto const& base : setters) {
							auto const& setter = base.try_as<Setter>();
							// �Z�b�^�[�̃v���p�e�B�[�𓾂�.
							auto const& prop = setter.Property();
							if (prop == TextBlock::FontFamilyProperty()) {
								// �v���p�e�B�[�� FontFamily �̏ꍇ,
								// �Z�b�^�[�̒l����, ���̖��𓾂�.
								auto value = unbox_value<FontFamily>(setter.Value());
								m_page_sheet.m_font_family = wchar_cpy(value.Source().c_str());
							}
							else if (prop == TextBlock::FontSizeProperty()) {
								// �v���p�e�B�[�� FontSize �̏ꍇ,
								// �Z�b�^�[�̒l����, ���̂̑傫���𓾂�.
								auto value = unbox_value<double>(setter.Value());
								m_page_sheet.m_font_size = value;
							}
							else if (prop == TextBlock::FontStretchProperty()) {
								// �v���p�e�B�[�� FontStretch �̏ꍇ,
								// �Z�b�^�[�̒l����, ���̂̐L�k�𓾂�.
								auto value = unbox_value<int32_t>(setter.Value());
								m_page_sheet.m_font_stretch = static_cast<DWRITE_FONT_STRETCH>(value);
							}
							else if (prop == TextBlock::FontStyleProperty()) {
								// �v���p�e�B�[�� FontStyle �̏ꍇ,
								// �Z�b�^�[�̒l����, ���̂𓾂�.
								auto value = unbox_value<int32_t>(setter.Value());
								m_page_sheet.m_font_style = static_cast<DWRITE_FONT_STYLE>(value);
							}
							else if (prop == TextBlock::FontWeightProperty()) {
								// �v���p�e�B�[�� FontWeight �̏ꍇ,
								// �Z�b�^�[�̒l����, ���̂̑����𓾂�.
								auto value = unbox_value<int32_t>(setter.Value());
								m_page_sheet.m_font_weight = static_cast<DWRITE_FONT_WEIGHT>(value);
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
			ShapeText::is_available_font(m_page_sheet.m_font_family);
		}

		{
			m_page_sheet.m_arrow_size = ARROW_SIZE();
			m_page_sheet.m_arrow_style = ARROW_STYLE::NONE;
			m_page_sheet.m_corner_rad = { GRIDLEN_PX, GRIDLEN_PX };
			m_page_sheet.set_fill_color(page_background());
			m_page_sheet.set_font_color(page_foreground());
			m_page_sheet.m_grid_base = static_cast<double>(GRIDLEN_PX) - 1.0;
			m_page_sheet.m_grid_gray = GRID_GRAY;
			m_page_sheet.m_grid_patt = GRID_PATT::PATT_1;
			m_page_sheet.m_grid_show = GRID_SHOW::BACK;
			m_page_sheet.m_grid_snap = true;
			m_page_sheet.set_page_color(page_background());
			m_page_sheet.m_page_scale = 1.0;
			const double dpi = DisplayInformation::GetForCurrentView().LogicalDpi();
			m_page_sheet.m_page_size = PAGE_SIZE;
			m_page_sheet.set_stroke_color(page_foreground());
			m_page_sheet.m_stroke_patt = STROKE_PATT();
			m_page_sheet.m_stroke_style = D2D1_DASH_STYLE::D2D1_DASH_STYLE_SOLID;
			m_page_sheet.m_stroke_width = 1.0F;
			m_page_sheet.m_text_align_p = DWRITE_PARAGRAPH_ALIGNMENT::DWRITE_PARAGRAPH_ALIGNMENT_NEAR;
			m_page_sheet.m_text_align_t = DWRITE_TEXT_ALIGNMENT::DWRITE_TEXT_ALIGNMENT_LEADING;
			m_page_sheet.m_text_line = 0.0F;
			m_page_sheet.m_text_margin = { 4.0F, 4.0F };
		}
		len_unit(LEN_UNIT::PIXEL);
		color_code(COLOR_CODE::DEC);
		status_bar(stbar_or(STATUS_BAR::CURS, STATUS_BAR::ZOOM));
	}

	// �ۑ����ꂽ�p���f�[�^��ǂݍ���.
	// �p���f�[�^��ۑ������t�@�C��������ꍇ, �����ǂݍ���.
	// �߂�l	�ǂݍ��߂��� S_OK.
	IAsyncOperation<winrt::hresult> MainPage::sheet_load_async(void)
	{
		mfi_sheet_reset().IsEnabled(false);
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
				mfi_sheet_reset().IsEnabled(true);
			}
			item = nullptr;
		}
		co_return hr;
	}

	// �p�����j���[�́u�p����ۑ��v���I�����ꂽ
	// ���[�J���t�H���_�[�Ƀt�@�C�����쐬��, �p���f�[�^��ۑ�����.
	IAsyncAction MainPage::sheet_save_click_async(IInspectable const&, RoutedEventArgs const&)
	{
		using winrt::Windows::Storage::CreationCollisionOption;

		try {
			auto s_file{ co_await local_folder().CreateFileAsync(FILE_NAME, CreationCollisionOption::ReplaceExisting) };
			if (s_file != nullptr) {
				co_await file_write_gpf_async(s_file, false, true);
				s_file = nullptr;
				mfi_sheet_reset().IsEnabled(true);
			}
		}
		catch (winrt::hresult_error const&) {
		}
		//using winrt::Windows::Storage::AccessCache::StorageApplicationPermissions;
		//auto const& mru_list = StorageApplicationPermissions::MostRecentlyUsedList();
		//mru_list.Clear();
	}

}