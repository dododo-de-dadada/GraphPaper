//-------------------------------
// MainPage_sample.cpp
// 見本
//-------------------------------
#include "pch.h"
#include "MainPage.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	// 見本ダイアログが開かれた.
	void MainPage::sample_opened(ContentDialog const&, ContentDialogOpenedEventArgs const&)
	{
		if (scp_sample_panel().IsLoaded()) {
			sample_draw();
		}
	}

	//　見本リストビューがロードされた.
	void MainPage::sample_list_loaded(IInspectable const&, RoutedEventArgs const&)
	{
		const auto item = lv_sample_list().SelectedItem();
		if (item != nullptr) {
			lv_sample_list().ScrollIntoView(item);
		}
	}

	// 見本を表示する
	void MainPage::sample_draw(void)
	{
#if defined(_DEBUG)
		if (scp_sample_panel().IsLoaded() != true) {
			return;
		}
#endif
		m_dx_mutex.lock();
		auto dc = m_sample_dx.m_d2dContext.get();
		dc->SaveDrawingState(m_sample_dx.m_state_block.get());
		dc->BeginDraw();
		dc->Clear(m_sample_sheet.m_sheet_color);
		D2D1_POINT_2F offset;
		float g_base;
		m_sample_sheet.get_grid_base(g_base);
		offset.x = static_cast<FLOAT>(std::fmod(m_sample_sheet.m_sheet_size.width * 0.5, g_base + 1.0));
		offset.y = offset.x;
		GRID_SHOW g_show;
		m_sample_sheet.get_grid_show(g_show);
		if (equal(g_show, GRID_SHOW::BACK)) {
			m_sample_sheet.draw_grid(m_sample_dx, offset);
		}
		if (m_sample_shape != nullptr) {
			m_sample_shape->draw(m_sample_dx);
		}
		if (equal(g_show, GRID_SHOW::FRONT)) {
			m_sample_sheet.draw_grid(m_sample_dx, offset);
		}
		winrt::check_hresult(dc->EndDraw());
		dc->RestoreDrawingState(m_sample_dx.m_state_block.get());
		m_sample_dx.Present();
		m_dx_mutex.unlock();
	}

	// 見本のスワップチェーンパネルの大きさが変わった.
	void MainPage::sample_panel_size_changed(IInspectable const&, RoutedEventArgs const&)
	{
		if (m_sample_dx.m_dxgi_swap_chain != nullptr) {
			m_sample_dx.m_dxgi_swap_chain = nullptr;
		}
		m_sample_sheet.set_to(&m_sheet_main);
		const auto w = scp_sample_panel().ActualWidth();
		const auto h = scp_sample_panel().ActualHeight();
		m_sample_sheet.m_sheet_size.width = static_cast<FLOAT>(w);
		m_sample_sheet.m_sheet_size.height = static_cast<FLOAT>(h);
		m_sample_dx.SetSwapChainPanel(scp_sample_panel());
		if (m_sample_type != SAMP_TYPE::NONE) {
			if (m_sample_type == SAMP_TYPE::FONT) {
				using winrt::Windows::ApplicationModel::Resources::ResourceLoader;
				const auto pad_w = w * 0.125;
				const auto pad_h = h * 0.25;
				const D2D1_POINT_2F s_pos{ static_cast<FLOAT>(pad_w), static_cast<FLOAT>(pad_h) };
				const D2D1_POINT_2F diff{ static_cast<FLOAT>(w - 2.0 * pad_w), static_cast<FLOAT>(w - 2.0 * pad_h) };
				const auto pang = ResourceLoader::GetForCurrentView().GetString(L"str_pangram");
				const wchar_t* text = nullptr;
				if (pang.empty()) {
					text = L"The quick brown fox jumps over a lazy dog.";
				}
				else {
					text = pang.c_str();
				}
				m_sample_shape = new ShapeText(s_pos, diff, wchar_cpy(text), &m_sample_sheet);
			}
			else if (m_sample_type == SAMP_TYPE::STROKE) {
				const auto pad = w * 0.125;
				const D2D1_POINT_2F s_pos{ static_cast<FLOAT>(pad), static_cast<FLOAT>(pad) };
				const D2D1_POINT_2F diff{ static_cast<FLOAT>(w - 2.0 * pad), static_cast<FLOAT>(h - 2.0 * pad) };
				m_sample_shape = new ShapeLine(s_pos, diff, &m_sample_sheet);
			}
			else if (m_sample_type == SAMP_TYPE::FILL) {
				const auto pad = w * 0.125;
				const D2D1_POINT_2F s_pos{ static_cast<FLOAT>(pad), static_cast<FLOAT>(pad) };
				const D2D1_POINT_2F diff{ static_cast<FLOAT>(w - 2.0 * pad), static_cast<FLOAT>(h - 2.0 * pad) };
				m_sample_shape = new ShapeRect(s_pos, diff, &m_sample_sheet);
			}
			else if (m_sample_type == SAMP_TYPE::JOIN) {
				const auto pad = w * 0.125;
				const D2D1_POINT_2F s_pos{ static_cast<FLOAT>(pad), static_cast<FLOAT>(pad) };
				const D2D1_POINT_2F diff{ static_cast<FLOAT>(w - 2.0 * pad), static_cast<FLOAT>(h - 2.0 * pad) };
				TOOL_POLY tool_poly { 3, true, true, false, true };
				m_sample_shape = new ShapePoly(s_pos, diff, &m_sample_sheet, tool_poly);
				const double offset = h / 16.0;
				m_sample_shape->set_anchor_pos(D2D1_POINT_2F{ static_cast<FLOAT>(-w * 0.25f), static_cast<FLOAT>(h * 0.5 - offset) }, ANCH_TYPE::ANCH_P0);
				m_sample_shape->set_anchor_pos(D2D1_POINT_2F{ static_cast<FLOAT>(w * 0.25),  static_cast<FLOAT>(h * 0.5) }, ANCH_TYPE::ANCH_P0 + 1);
				m_sample_shape->set_anchor_pos(D2D1_POINT_2F{ static_cast<FLOAT>(-w * 0.25f), static_cast<FLOAT>(h * 0.5 + offset) }, ANCH_TYPE::ANCH_P0 + 2);
			}
			else if (m_sample_type == SAMP_TYPE::MISC) {
				constexpr uint32_t misc_min = 3;
				constexpr uint32_t misc_max = 64;
				static uint32_t misc_cnt = misc_min;
				const auto pad = w * 0.125;
				const D2D1_POINT_2F samp_diff{ static_cast<FLOAT>(w - 3.0 * pad), static_cast<FLOAT>(h - 3.0 * pad) };
				const D2D1_POINT_2F rect_pos{ static_cast<FLOAT>(pad), static_cast<FLOAT>(pad) };
				const D2D1_POINT_2F poly_pos{ static_cast<FLOAT>(pad + pad), static_cast<FLOAT>(pad + pad) };
				auto const samp_rect = new ShapeRect(rect_pos, samp_diff, &m_sample_sheet);
				TOOL_POLY poly_tool{ misc_cnt >= misc_max ? misc_min : misc_cnt++, true, true, true, true };
				auto const samp_poly = new ShapePoly(poly_pos, samp_diff, &m_sample_sheet, poly_tool);
				ShapeGroup* const g = new ShapeGroup();
				m_sample_shape = g;
				g->m_list_grouped.push_back(samp_rect);
				g->m_list_grouped.push_back(samp_poly);
				m_sample_shape->set_select(false);
			}
			else {
				throw winrt::hresult_invalid_argument();
			}
#if defined(_DEBUG)
			debug_leak_cnt++;
#endif
		}
		sample_draw();

	}

}
