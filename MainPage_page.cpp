//-------------------------------
// MainPage_page.cpp
// �\���̐ݒ�
//-------------------------------
#include "pch.h"
#include "MainPage.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	using winrt::Windows::ApplicationModel::Resources::ResourceLoader;
	using winrt::Windows::UI::Text::FontStretch;
	using winrt::Windows::UI::Xaml::Controls::ContentDialogResult;
	using winrt::Windows::UI::Xaml::Controls::Primitives::SliderSnapsTo;
	using winrt::Windows::UI::Xaml::Controls::TextBox;

	//constexpr wchar_t DLG_TITLE[] = L"str_page_layouts";	// �\���̕\��

	//------------------------------
	// ���C�A�E�g���j���[�́u�y�[�W�̑傫���v���I�����ꂽ
	//------------------------------
	IAsyncAction MainPage::page_size_click_async(IInspectable const&, RoutedEventArgs const&)
	{
		m_prop_page.set_attr_to(&m_main_page);
		const double g_len = m_main_page.m_grid_base + 1.0;
		const double dpi = m_main_d2d.m_logical_dpi;
		wchar_t buf[32];
		conv_len_to_str<LEN_UNIT_NAME_NOT_APPEND>(m_len_unit, m_main_page.m_page_size.width, dpi, g_len, buf);
		tx_page_size_width().Text(buf);
		conv_len_to_str<LEN_UNIT_NAME_NOT_APPEND>(m_len_unit, m_main_page.m_page_size.height, dpi, g_len, buf);
		tx_page_size_height().Text(buf);
		conv_len_to_str<LEN_UNIT_NAME_NOT_APPEND>(m_len_unit, m_main_page.m_page_margin.left, dpi, g_len, buf);
		tx_page_margin_left().Text(buf);
		conv_len_to_str<LEN_UNIT_NAME_NOT_APPEND>(m_len_unit, m_main_page.m_page_margin.top, dpi, g_len, buf);
		tx_page_margin_top().Text(buf);
		conv_len_to_str<LEN_UNIT_NAME_NOT_APPEND>(m_len_unit, m_main_page.m_page_margin.right, dpi, g_len, buf);
		tx_page_margin_right().Text(buf);
		conv_len_to_str<LEN_UNIT_NAME_NOT_APPEND>(m_len_unit, m_main_page.m_page_margin.bottom, dpi, g_len, buf);
		tx_page_margin_bottom().Text(buf);

		if (m_len_unit == LEN_UNIT::GRID) {
			cb_len_unit().SelectedItem(box_value(cbi_len_unit_grid()));
		}
		else if (m_len_unit == LEN_UNIT::INCH) {
			cb_len_unit().SelectedItem(box_value(cbi_len_unit_inch()));
		}
		else if (m_len_unit == LEN_UNIT::MILLI) {
			cb_len_unit().SelectedItem(box_value(cbi_len_unit_milli()));
		}
		else if (m_len_unit == LEN_UNIT::POINT) {
			cb_len_unit().SelectedItem(box_value(cbi_len_unit_point()));
		}
		else {
			cb_len_unit().SelectedItem(box_value(cbi_len_unit_pixel()));
		}

		// ���̎��_�ł�, �e�L�X�g�{�b�N�X�ɐ��������l���i�[���Ă�, TextChanged �͌Ă΂�Ȃ�.
		// �v���C�}���[�{�^���͎g�p�\�ɂ��Ă���.
		cd_page_size_dialog().IsPrimaryButtonEnabled(true);
		cd_page_size_dialog().IsSecondaryButtonEnabled(m_main_page.m_shape_list.size() > 0);
		const auto d_result = co_await cd_page_size_dialog().ShowAsync();
		if (d_result == ContentDialogResult::Primary) {
			constexpr wchar_t INVALID_NUM[] = L"str_err_number";

			if (cbi_len_unit_grid().IsSelected()) {
				m_len_unit = LEN_UNIT::GRID;
			}
			else if (cbi_len_unit_inch().IsSelected()) {
				m_len_unit = LEN_UNIT::INCH;
			}
			else if (cbi_len_unit_milli().IsSelected()) {
				m_len_unit = LEN_UNIT::MILLI;
			}
			else if (cbi_len_unit_grid().IsSelected()) {
				m_len_unit = LEN_UNIT::GRID;
			}
			else if (cbi_len_unit_point().IsSelected()) {
				m_len_unit = LEN_UNIT::POINT;
			}
			else {
				m_len_unit = LEN_UNIT::PIXEL;
			}
			len_unit_is_checked(m_len_unit);

			float new_left;
			if (swscanf_s(tx_page_margin_left().Text().c_str(), L"%f", &new_left) != 1) {
				// �u�����Ȑ��l�ł��v���b�Z�[�W�_�C�A���O��\������.
				message_show(ICON_ALERT, INVALID_NUM, L"tx_page_margin_left/Header");
				co_return;
			}
			float new_top;
			if (swscanf_s(tx_page_margin_top().Text().c_str(), L"%f", &new_top) != 1) {
				// �u�����Ȑ��l�ł��v���b�Z�[�W�_�C�A���O��\������.
				message_show(ICON_ALERT, INVALID_NUM, L"tx_page_margin_top/Header");
				co_return;
			}
			float new_right;
			if (swscanf_s(tx_page_margin_right().Text().c_str(), L"%f", &new_right) != 1) {
				// �u�����Ȑ��l�ł��v���b�Z�[�W�_�C�A���O��\������.
				message_show(ICON_ALERT, INVALID_NUM, L"tx_page_margin_right/Header");
				co_return;
			}
			float new_bottom;
			if (swscanf_s(tx_page_margin_bottom().Text().c_str(), L"%f", &new_bottom) != 1) {
				// �u�����Ȑ��l�ł��v���b�Z�[�W�_�C�A���O��\������.
				message_show(ICON_ALERT, INVALID_NUM, L"tx_page_margin_bottom/Header");
				co_return;
			}
			// �\���̏c���̒����̒l���s�N�Z���P�ʂ̒l�ɕϊ�����.
			D2D1_RECT_F p_mar{
				static_cast<FLOAT>(conv_len_to_pixel(m_len_unit, new_left, dpi, g_len)),
				static_cast<FLOAT>(conv_len_to_pixel(m_len_unit, new_top, dpi, g_len)),
				static_cast<FLOAT>(conv_len_to_pixel(m_len_unit, new_right, dpi, g_len)),
				static_cast<FLOAT>(conv_len_to_pixel(m_len_unit, new_bottom, dpi, g_len))
			};

			float new_width;
			if (swscanf_s(tx_page_size_width().Text().c_str(), L"%f", &new_width) != 1) {
				// �u�����Ȑ��l�ł��v���b�Z�[�W�_�C�A���O��\������.
				message_show(ICON_ALERT, INVALID_NUM, L"tx_page_size_width/Header");
				co_return;
			}
			float new_height;
			if (swscanf_s(tx_page_size_height().Text().c_str(), L"%f", &new_height) != 1) {
				// �u�����Ȑ��l�ł��v���b�Z�[�W�_�C�A���O��\������.
				message_show(ICON_ALERT, INVALID_NUM, L"tx_page_size_height/Header");
				co_return;
			}
			D2D1_SIZE_F p_size{
				static_cast<FLOAT>(conv_len_to_pixel(m_len_unit, new_width, dpi, g_len)),
				static_cast<FLOAT>(conv_len_to_pixel(m_len_unit, new_height, dpi, g_len))
			};

			const bool flag_size = !equal(p_size, m_main_page.m_page_size);
			const bool flag_mar = !equal(p_mar, m_main_page.m_page_margin);
			if (flag_size || flag_mar) {
				if (flag_size) {
					ustack_push_set<UNDO_T::PAGE_SIZE>(&m_main_page, p_size);
				}
				if (flag_mar) {
					ustack_push_set<UNDO_T::PAGE_PAD>(&m_main_page, p_mar);
				}
				ustack_push_null();
				ustack_is_enable();
				main_bbox_update();
				main_panel_size();
				main_draw();
				status_bar_set_pos();
				status_bar_set_grid();
				status_bar_set_page();
				status_bar_set_unit();
			}
		}
		else if (d_result == ContentDialogResult::Secondary) {
			constexpr wchar_t INVALID_NUM[] = L"str_err_number";

			if (cbi_len_unit_grid().IsSelected()) {
				m_len_unit = LEN_UNIT::GRID;
			}
			else if (cbi_len_unit_inch().IsSelected()) {
				m_len_unit = LEN_UNIT::INCH;
			}
			else if (cbi_len_unit_milli().IsSelected()) {
				m_len_unit = LEN_UNIT::MILLI;
			}
			else if (cbi_len_unit_grid().IsSelected()) {
				m_len_unit = LEN_UNIT::GRID;
			}
			else if (cbi_len_unit_point().IsSelected()) {
				m_len_unit = LEN_UNIT::POINT;
			}
			else {
				m_len_unit = LEN_UNIT::PIXEL;
			}
			len_unit_is_checked(m_len_unit);

			float new_left;
			if (swscanf_s(tx_page_margin_left().Text().c_str(), L"%f", &new_left) != 1) {
				// �u�����Ȑ��l�ł��v���b�Z�[�W�_�C�A���O��\������.
				message_show(ICON_ALERT, INVALID_NUM, L"tx_page_margin_left/Header");
				co_return;
			}
			float new_top;
			if (swscanf_s(tx_page_margin_top().Text().c_str(), L"%f", &new_top) != 1) {
				// �u�����Ȑ��l�ł��v���b�Z�[�W�_�C�A���O��\������.
				message_show(ICON_ALERT, INVALID_NUM, L"tx_page_margin_top/Header");
				co_return;
			}
			float new_right;
			if (swscanf_s(tx_page_margin_right().Text().c_str(), L"%f", &new_right) != 1) {
				// �u�����Ȑ��l�ł��v���b�Z�[�W�_�C�A���O��\������.
				message_show(ICON_ALERT, INVALID_NUM, L"tx_page_margin_right/Header");
				co_return;
			}
			float new_bottom;
			if (swscanf_s(tx_page_margin_bottom().Text().c_str(), L"%f", &new_bottom) != 1) {
				// �u�����Ȑ��l�ł��v���b�Z�[�W�_�C�A���O��\������.
				message_show(ICON_ALERT, INVALID_NUM, L"tx_page_margin_bottom/Header");
				co_return;
			}
			// �����̒l���s�N�Z���P�ʂ̒l�ɕϊ�����.
			D2D1_RECT_F p_mar{	// �y�[�W�̗]��
				static_cast<FLOAT>(conv_len_to_pixel(m_len_unit, new_left, dpi, g_len)),
				static_cast<FLOAT>(conv_len_to_pixel(m_len_unit, new_top, dpi, g_len)),
				static_cast<FLOAT>(conv_len_to_pixel(m_len_unit, new_right, dpi, g_len)),
				static_cast<FLOAT>(conv_len_to_pixel(m_len_unit, new_bottom, dpi, g_len))
			};

			// ���X�g���̐}�`���͂ދ�`�𓾂�.
			D2D1_POINT_2F b_lt = { FLT_MAX, FLT_MAX };
			D2D1_POINT_2F b_rb = { -FLT_MAX, -FLT_MAX };
			//D2D1_POINT_2F b_size;
			slist_bound_shape(m_main_page.m_shape_list, b_lt, b_rb);

			// ��`�̑傫�����[��,�Ȃ璆�f����.
			if (b_rb.x - b_lt.x < 1.0F || b_rb.y - b_lt.y < 1.0F) {
				co_return;
			}

			// ����_�̍��W�̂����ꂩ�����Ȃ��, ���_�ƂȂ�悤��`���ړ�����.
			float dx = 0.0f;	// ��`���ړ���������
			float dy = 0.0f;	// ��`���ړ���������
			if (b_lt.x < 0.0F) {
				dx = -b_lt.x;
				b_lt.x = 0.0F;
				b_rb.x += dx;
			}
			if (b_lt.y < 0.0F) {
				dy = -b_lt.y;
				b_lt.y = 0.0F;
				b_rb.y += dy;
			}

			// ����_�̒l��, �}�`�����͂ރp�f�B���O�Ƃ݂Ȃ�,
			// ����_�̍��W�̂����ꂩ�����Ȃ��, ���̕������E���_���E���Ɉړ�����.
			if (b_lt.x > 0.0f) {
				b_rb.x += b_lt.x;
			}
			if (b_lt.y > 0.0f) {
				b_rb.y += b_lt.y;
			}

			// �E���_�ɗ]�����������l���y�[�W�̑傫���ƂȂ�.
			D2D1_SIZE_F p_size{	// �y�[�W�̑傫��.
				new_left + b_rb.x + new_right,
				new_top + b_rb.y + new_bottom
			};

			// �y�[�W�̑傫��, �]��, �����ꂩ���قȂ�.
			// ���邢�͋�`���ړ������Ȃ�
			const bool size_changed = !equal(p_size, m_main_page.m_page_size);
			const bool mar_chanfed = !equal(p_mar, m_main_page.m_page_margin);
			if (size_changed || mar_chanfed || dx > 0.0f || dy > 0.0f) {
				// ��`���ړ������Ȃ�, �}�`����`�Ɏ��܂�悤, �}�`���ړ�������.
				if (dx > 0.0f || dy > 0.0f) {
					constexpr auto ANY = true;
					ustack_push_move({ dx, dy }, ANY);
				}
				// �y�[�W�̑傫�����قȂ�Ȃ�, �X�V����.
				if (size_changed) {
					ustack_push_set<UNDO_T::PAGE_SIZE>(&m_main_page, p_size);
				}
				// �y�[�W�̗]�����قȂ�Ȃ�, �X�V����.
				if (mar_chanfed) {
					ustack_push_set<UNDO_T::PAGE_PAD>(&m_main_page, p_mar);
				}
				ustack_push_null();
				ustack_is_enable();
				main_bbox_update();
				main_panel_size();
				main_draw();
				status_bar_set_grid();
				status_bar_set_page();
			}
			status_bar_set_unit();
		}
		status_bar_set_pos();
	}

	// �y�[�W�̑傫���_�C�A���O�̃e�L�X�g�{�b�N�X�̒l���ύX���ꂽ.
	void MainPage::page_size_value_changed(IInspectable const& sender, NumberBoxValueChangedEventArgs const&)
	{
		const double dpi = m_main_d2d.m_logical_dpi;	// DPI
		const auto g_len = m_main_page.m_grid_base + 1.0;
		double w = tx_page_size_width().Value();
		double h = tx_page_size_height().Value();
		double l = tx_page_margin_left().Value();
		double t = tx_page_margin_top().Value();
		double r = tx_page_margin_right().Value();
		double b = tx_page_margin_bottom().Value();
		LEN_UNIT u;
		if (cbi_len_unit_grid().IsSelected()) {
			u = LEN_UNIT::GRID;
		}
		else if (cbi_len_unit_inch().IsSelected()) {
			u = LEN_UNIT::INCH;
		}
		else if (cbi_len_unit_milli().IsSelected()) {
			u = LEN_UNIT::MILLI;
		}
		else if (cbi_len_unit_point().IsSelected()) {
			u = LEN_UNIT::POINT;
		}
		else {
			u = LEN_UNIT::PIXEL;
		}
		if (u != LEN_UNIT::PIXEL) {
			w = conv_len_to_pixel(u, w, dpi, g_len);
			h = conv_len_to_pixel(u, h, dpi, g_len);
			l = conv_len_to_pixel(u, l, dpi, g_len);
			t = conv_len_to_pixel(u, t, dpi, g_len);
			r = conv_len_to_pixel(u, r, dpi, g_len);
			b = conv_len_to_pixel(u, b, dpi, g_len);
		}
		if (w >= 1.0 && w <= PAGE_SIZE_MAX && l + r <= w - 1.0 &&
			h >= 1.0 && h <= PAGE_SIZE_MAX && t + b <= h - 1.0) {
			cd_page_size_dialog().IsPrimaryButtonEnabled(true);
		}
		else {
			cd_page_size_dialog().IsPrimaryButtonEnabled(false);
		}
	}

	// �y�[�W�̑傫���_�C�A���O�̃R���{�{�b�N�X�̑I�����ύX���ꂽ.
	void MainPage::page_size_selection_changed(IInspectable const&, SelectionChangedEventArgs const& args) noexcept
	{
		LEN_UNIT old_unit = LEN_UNIT::PIXEL;
		for (const auto i : args.RemovedItems()) {
			if (i == cbi_len_unit_grid()) {
				old_unit = LEN_UNIT::GRID;
			}
			else if (i == cbi_len_unit_inch()) {
				old_unit = LEN_UNIT::INCH;
			}
			else if (i == cbi_len_unit_milli()) {
				old_unit = LEN_UNIT::MILLI;
			}
			else if (i == cbi_len_unit_pixel()) {
				old_unit = LEN_UNIT::PIXEL;
			}
			else if (i == cbi_len_unit_point()) {
				old_unit = LEN_UNIT::POINT;
			}
			else {
				return;
			}
		}
		LEN_UNIT new_unit = LEN_UNIT::PIXEL;
		for (const auto i : args.AddedItems()) {
			if (i == cbi_len_unit_grid()) {
				new_unit = LEN_UNIT::GRID;
			}
			else if (i == cbi_len_unit_inch()) {
				new_unit = LEN_UNIT::INCH;
			}
			else if (i == cbi_len_unit_milli()) {
				new_unit = LEN_UNIT::MILLI;
			}
			else if (i == cbi_len_unit_pixel()) {
				new_unit = LEN_UNIT::PIXEL;
			}
			else if (i == cbi_len_unit_point()) {
				new_unit = LEN_UNIT::POINT;
			}
			else {
				return;
			}
		}
		if (old_unit != new_unit) {
			const double dpi = m_main_d2d.m_logical_dpi;
			const double g_len = m_main_page.m_grid_base + 1.0;
			double val;
			if (swscanf_s(tx_page_size_width().Text().data(), L"%lf", &val)) {
				wchar_t buf[128];
				val = conv_len_to_pixel(old_unit, val, dpi, g_len);
				conv_len_to_str<false>(new_unit, val, dpi, g_len, buf);
				tx_page_size_width().Text(buf);
			}
			if (swscanf_s(tx_page_size_height().Text().data(), L"%lf", &val)) {
				wchar_t buf[128];
				val = conv_len_to_pixel(old_unit, val, dpi, g_len);
				conv_len_to_str<false>(new_unit, val, dpi, g_len, buf);
				tx_page_size_height().Text(buf);
			}
			if (swscanf_s(tx_page_margin_left().Text().data(), L"%lf", &val)) {
				wchar_t buf[128];
				val = conv_len_to_pixel(old_unit, val, dpi, g_len);
				conv_len_to_str<false>(new_unit, val, dpi, g_len, buf);
				tx_page_margin_left().Text(buf);
			}
			if (swscanf_s(tx_page_margin_top().Text().data(), L"%lf", &val)) {
				wchar_t buf[128];
				val = conv_len_to_pixel(old_unit, val, dpi, g_len);
				conv_len_to_str<false>(new_unit, val, dpi, g_len, buf);
				tx_page_margin_top().Text(buf);
			}
			if (swscanf_s(tx_page_margin_right().Text().data(), L"%lf", &val)) {
				wchar_t buf[128];
				val = conv_len_to_pixel(old_unit, val, dpi, g_len);
				conv_len_to_str<false>(new_unit, val, dpi, g_len, buf);
				tx_page_margin_right().Text(buf);
			}
			if (swscanf_s(tx_page_margin_bottom().Text().data(), L"%lf", &val)) {
				wchar_t buf[128];
				val = conv_len_to_pixel(old_unit, val, dpi, g_len);
				conv_len_to_str<false>(new_unit, val, dpi, g_len, buf);
				tx_page_margin_bottom().Text(buf);
			}
		}
	}

}