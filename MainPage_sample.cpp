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
	void MainPage::cd_sample_opened(ContentDialog const& /*sender*/, ContentDialogOpenedEventArgs const& /*args*/)
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
		auto ox = std::fmod(m_sample_panel.m_page_size.width * 0.5, m_sample_panel.m_grid_len + 1.0);
		D2D1_POINT_2F offset;
		offset.x = static_cast<FLOAT>(ox);
		offset.y = offset.x;
		if (m_sample_panel.m_grid_show == GRID_SHOW::BACK) {
			m_sample_panel.draw_grid_line(m_sample_dx, offset);
		}
		if (m_sample_shape != nullptr) {
			m_sample_dx.m_anch_brush->SetColor(m_sample_panel.m_anch_color);
			m_sample_shape->draw(m_sample_dx);
		}
		if (m_sample_panel.m_grid_show == GRID_SHOW::FRONT) {
			m_sample_panel.draw_grid_line(m_sample_dx, offset);
		}
		winrt::check_hresult(dc->EndDraw());
		dc->RestoreDrawingState(m_sample_dx.m_state_block.get());
		m_sample_dx.Present();
	}

	// 見本のパネルがロードされた.
	void MainPage::sample_panel_loaded(void)
	{
		if (m_sample_dx.m_dxgi_swap_chain) {
			m_sample_dx.m_dxgi_swap_chain = nullptr;
		}
		m_sample_panel = m_page_panel;
		auto w = scp_sample_panel().ActualWidth();
		auto h = scp_sample_panel().ActualHeight();
		m_sample_panel.m_page_size.width = static_cast<FLOAT>(w);
		m_sample_panel.m_page_size.height = static_cast<FLOAT>(h);
		m_sample_dx.SetSwapChainPanel(scp_sample_panel());
	}

	//	見本ダイアログを表示する.
	void MainPage::show_cd_sample(const wchar_t* r_key)
	{
		using winrt::Windows::ApplicationModel::Resources::ResourceLoader;

		cd_sample().Title(box_value(ResourceLoader::GetForCurrentView().GetString(r_key)));
		auto _{ cd_sample().ShowAsync() };
	}
}