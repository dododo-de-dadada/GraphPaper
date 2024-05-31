//-----------------------------
// MainPage_help.cpp
// 長さの単位, 色の基数, ステータスバー, バージョン情報
//-----------------------------
#include "pch.h"
#include "MainPage.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	using winrt::Windows::UI::Xaml::Controls::ContentDialogResult;
	using winrt::Windows::ApplicationModel::Resources::ResourceLoader;
	using winrt::Windows::UI::Xaml::Visibility;

	// ヘルプメニューの「バージョン情報」が選択された.
	IAsyncAction MainPage::about_graph_paper_click(IInspectable const&, RoutedEventArgs const&)
	{
		tb_version().Visibility(Visibility::Visible);
		const auto def_text = cd_dialog_prop().DefaultButton();
		const auto pri_text = cd_dialog_prop().PrimaryButtonText();
		const auto close_text = cd_dialog_prop().CloseButtonText();
		cd_dialog_prop().PrimaryButtonText(L"");
		cd_dialog_prop().CloseButtonText(L"OK");
		cd_dialog_prop().Title(box_value(L"GraphPaper"));

		const auto samp_w = scp_dialog_panel().Width();
		const auto samp_h = scp_dialog_panel().Height();

		constexpr uint32_t misc_min = 3;
		constexpr uint32_t misc_max = 12;
		static uint32_t misc_cnt = misc_min;
		const auto mar = samp_w * 0.125;
		const D2D1_POINT_2F pos{
			static_cast<FLOAT>(samp_w - 2.0 * mar), static_cast<FLOAT>(samp_h - 2.0 * mar)
		};
		POLY_OPTION p_opt{ m_tool_polygon };
		p_opt.m_vertex_cnt = (misc_cnt >= misc_max ? misc_min : misc_cnt++);
		SHAPE* s = new ShapePoly(D2D1_POINT_2F{ 0.0f, 0.0f }, pos, &m_dialog_sheet, p_opt);
		D2D1_POINT_2F b_lt;
		D2D1_POINT_2F b_rb;
		D2D1_POINT_2F b_pos;
		s->get_bbox(D2D1_POINT_2F{ FLT_MAX, FLT_MAX }, D2D1_POINT_2F{ -FLT_MAX, -FLT_MAX }, b_lt, b_rb);
		pt_sub(b_rb, b_lt, b_pos);
		s->move(
			D2D1_POINT_2F{ static_cast<FLOAT>((samp_w - b_pos.x) * 0.5),
			static_cast<FLOAT>((samp_h - b_pos.y) * 0.5) });
		m_dialog_sheet.m_shape_list.push_back(s);
#if defined(_DEBUG)
		debug_leak_cnt++;
#endif
		m_mutex_event.lock();
		co_await cd_dialog_prop().ShowAsync();

		cd_dialog_prop().PrimaryButtonText(pri_text);
		cd_dialog_prop().CloseButtonText(close_text);
		cd_dialog_prop().DefaultButton(def_text);
		tb_version().Visibility(Visibility::Collapsed);
		slist_clear(m_dialog_sheet.m_shape_list);
		status_bar_set_pointer();
		m_mutex_event.unlock();
	}

	// ヘルプメニューの「長さの単位」のサブ項目が選択された.
	void MainPage::len_unit_click(IInspectable const& sender, RoutedEventArgs const&)
	{
		const auto old_unit = m_len_unit;
		LEN_UNIT new_val;
		if (sender == rmfi_menu_len_unit_grid()) {
			new_val = LEN_UNIT::GRID;
		}
		else if (sender == rmfi_menu_len_unit_inch()) {
			new_val = LEN_UNIT::INCH;
		}
		else if (sender == rmfi_menu_len_unit_milli()) {
			new_val = LEN_UNIT::MILLI;
		}
		else if (sender == rmfi_menu_len_unit_pixel()) {
			new_val = LEN_UNIT::PIXEL;
		}
		else if (sender == rmfi_menu_len_unit_point()) {
			new_val = LEN_UNIT::POINT;
		}
		else {
			throw winrt::hresult_not_implemented();
			return;
		}
		m_len_unit = new_val;
		status_bar_set_pointer();
		if (old_unit != new_val) {
			status_bar_set_grid_len();
			status_bar_set_sheet_size();
			status_bar_set_len_unit();
		}
	}

	// ヘルプメニューの「点を方眼にくっつける」が選択された.
	void MainPage::snap_grid_click(IInspectable const&, RoutedEventArgs const&)
	{
		m_snap_grid = tmfi_menu_snap_grid().IsChecked();
		status_bar_set_pointer();
	}

	// ヘルプメニューの「頂点をくっつける...」が選択された.
	IAsyncAction MainPage::snap_point_click_async(IInspectable const&, RoutedEventArgs const&) noexcept
	{
		const winrt::hstring str_snap_point{
			ResourceLoader::GetForCurrentView().GetString(L"str_snap_point") + L": "
		};
		const auto val = m_snap_point;
		wchar_t buf[32];
		conv_len_to_str<LEN_UNIT_NAME_APPEND>(m_len_unit, val, m_main_d2d.m_logical_dpi, m_dialog_sheet.m_grid_base + 1.0, buf);
		sd_snap_point().Header(box_value(str_snap_point + buf));
		sd_snap_point().Value(static_cast<double>(m_snap_point));
		m_mutex_event.lock();
		{
			const auto revoler{
				sd_snap_point().ValueChanged(winrt::auto_revoke, [this, str_snap_point](auto const&, auto const& args) {
					const auto val = args.NewValue();
					wchar_t buf[32];
					conv_len_to_str<LEN_UNIT_NAME_APPEND>(m_len_unit, val, m_main_d2d.m_logical_dpi, m_dialog_sheet.m_grid_base + 1.0, buf);
					sd_snap_point().Header(box_value(str_snap_point + buf));
				})
			};
			if (co_await cd_snap_point().ShowAsync() == ContentDialogResult::Primary) {
				m_snap_point = static_cast<float>(sd_snap_point().Value());
			}
		}
		m_mutex_event.unlock();
	}

}