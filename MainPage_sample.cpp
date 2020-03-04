//-------------------------------
// MainPage_sample.cpp
// 見本ダイアログの設定, 表示
//-------------------------------
#include "pch.h"
#include "MainPage.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	// 見本ダイアログが開かれた.
	void MainPage::cd_sample_opened(ContentDialog const&, ContentDialogOpenedEventArgs const&)
	{
		if (scp_sample_panel().IsLoaded()) {
			sample_draw();
		}
	}

	// 見本のページと見本の図形を表示する.
	void MainPage::sample_draw(void)
	{
#if defined(_DEBUG)
		if (scp_sample_panel().IsLoaded() == false) {
			return;
		}
#endif
		std::lock_guard<std::mutex> lock(m_dx_mutex);

		auto dc = m_sample_dx.m_d2dContext.get();
		dc->SaveDrawingState(m_sample_dx.m_state_block.get());
		dc->BeginDraw();
		dc->Clear(m_sample_panel.m_page_color);
		auto ox = std::fmod(m_sample_panel.m_page_size.width * 0.5, m_sample_panel.m_grid_size + 1.0);
		D2D1_POINT_2F offset;
		offset.x = static_cast<FLOAT>(ox);
		offset.y = offset.x;
		if (m_sample_panel.m_grid_show == GRID_SHOW::BACK) {
			m_sample_panel.TOOL_grid_line(m_sample_dx, offset);
		}
		if (m_sample_shape != nullptr) {
			m_sample_dx.m_anch_brush->SetColor(m_sample_panel.m_anch_color);
			m_sample_shape->draw(m_sample_dx);
		}
		if (m_sample_panel.m_grid_show == GRID_SHOW::FRONT) {
			m_sample_panel.TOOL_grid_line(m_sample_dx, offset);
		}
		winrt::check_hresult(dc->EndDraw());
		dc->RestoreDrawingState(m_sample_dx.m_state_block.get());
		m_sample_dx.Present();
	}

	//	見本パネルの大きさが変わった.
	void MainPage::scp_sample_panel_size_changed(IInspectable const&, RoutedEventArgs const&)
	{
		if (m_sample_dx.m_dxgi_swap_chain != nullptr) {
			m_sample_dx.m_dxgi_swap_chain = nullptr;
		}
		m_sample_panel = m_page_panel;
		const auto w = scp_sample_panel().ActualWidth();
		const auto h = scp_sample_panel().ActualHeight();
		m_sample_panel.m_page_size.width = static_cast<FLOAT>(w);
		m_sample_panel.m_page_size.height = static_cast<FLOAT>(h);
		m_sample_dx.SetSwapChainPanel(scp_sample_panel());
		if (m_sample_type != SAMP_TYPE::NONE) {
			const auto padding = w * 0.125;
			const D2D1_POINT_2F pos = {
				static_cast<FLOAT>(padding),
				static_cast<FLOAT>(padding)
			};
			const D2D1_POINT_2F vec = {
				static_cast<FLOAT>(w - 2.0 * padding),
				static_cast<FLOAT>(h - 2.0 * padding)
			};
			if (m_sample_type == SAMP_TYPE::FONT) {
				using winrt::Windows::ApplicationModel::Resources::ResourceLoader;
				auto const& r_loader = ResourceLoader::GetForCurrentView();
				const auto t = wchar_cpy(r_loader.GetString(L"str_pangram").c_str());
				m_sample_shape = new ShapeText(pos, vec, t, &m_sample_panel);
#if defined(_DEBUG)
				debug_leak_cnt++;
#endif
			}
			else if (m_sample_type == SAMP_TYPE::STROKE) {
				m_sample_shape = new ShapeLine(pos, vec, &m_sample_panel);
#if defined(_DEBUG)
				debug_leak_cnt++;
#endif
			}
			else if (m_sample_type == SAMP_TYPE::FILL) {
				m_sample_shape = new ShapeRect(pos, vec, &m_sample_panel);
#if defined(_DEBUG)
				debug_leak_cnt++;
#endif
			}
		}
		sample_draw();

	}

}
