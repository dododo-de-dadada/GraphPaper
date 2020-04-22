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
		m_mutex_page.lock();
		auto dc = m_sample_dx.m_d2dContext.get();
		dc->SaveDrawingState(m_sample_dx.m_state_block.get());
		dc->BeginDraw();
		dc->Clear(m_sample_sheet.m_page_color);
		auto ox = std::fmod(m_sample_sheet.m_page_size.width * 0.5, m_sample_sheet.m_grid_base + 1.0);
		D2D1_POINT_2F offset;
		offset.x = static_cast<FLOAT>(ox);
		offset.y = offset.x;
		if (m_sample_sheet.m_grid_show == GRID_SHOW::BACK) {
			m_sample_sheet.draw_grid(m_sample_dx, offset);
		}
		if (m_sample_shape != nullptr) {
			m_sample_shape->draw(m_sample_dx);
		}
		if (m_sample_sheet.m_grid_show == GRID_SHOW::FRONT) {
			m_sample_sheet.draw_grid(m_sample_dx, offset);
		}
		winrt::check_hresult(dc->EndDraw());
		dc->RestoreDrawingState(m_sample_dx.m_state_block.get());
		m_sample_dx.Present();
		m_mutex_page.unlock();
	}

	// 見本のスワップチェーンパネルの大きさが変わった.
	void MainPage::sample_panel_size_changed(IInspectable const&, RoutedEventArgs const&)
	{
		if (m_sample_dx.m_dxgi_swap_chain != nullptr) {
			m_sample_dx.m_dxgi_swap_chain = nullptr;
		}
		m_sample_sheet.set_to(&m_page_sheet);
		const auto w = scp_sample_panel().ActualWidth();
		const auto h = scp_sample_panel().ActualHeight();
		m_sample_sheet.m_page_size.width = static_cast<FLOAT>(w);
		m_sample_sheet.m_page_size.height = static_cast<FLOAT>(h);
		m_sample_dx.SetSwapChainPanel(scp_sample_panel());
		if (m_sample_type != SAMP_TYPE::NONE) {
			const auto padding = w * 0.125;
			const D2D1_POINT_2F s_pos = {
				static_cast<FLOAT>(padding),
				static_cast<FLOAT>(padding)
			};
			const D2D1_POINT_2F diff = {
				static_cast<FLOAT>(w - 2.0 * padding),
				static_cast<FLOAT>(h - 2.0 * padding)
			};
			if (m_sample_type == SAMP_TYPE::FONT) {
				using winrt::Windows::ApplicationModel::Resources::ResourceLoader;
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
				m_sample_shape = new ShapeLine(s_pos, diff, &m_sample_sheet);
			}
			else if (m_sample_type == SAMP_TYPE::FILL) {
				m_sample_shape = new ShapeRect(s_pos, diff, &m_sample_sheet);
			}
			else {
				throw winrt::hresult_not_implemented();
			}
#if defined(_DEBUG)
			debug_leak_cnt++;
#endif
		}
		sample_draw();

	}

}
