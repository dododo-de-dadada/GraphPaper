//-------------------------------
// MainPage_sample.cpp
// 見本
//-------------------------------
#include "pch.h"
#include "MainPage.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
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

	//　見本リストビューがロードされた.
	void MainPage::sample_list_loaded(IInspectable const&, RoutedEventArgs const&)
	{
		const auto item = lv_sample_list().SelectedItem();
		if (item != nullptr) {
			lv_sample_list().ScrollIntoView(item);
		}
	}

	// 見本ダイアログが開かれた.
	void MainPage::sample_opened(ContentDialog const&, ContentDialogOpenedEventArgs const&)
	{
		if (scp_sample_panel().IsLoaded()) {
			sample_draw();
		}
	}

	// 見本のスワップチェーンパネルの大きさが変わった.
	IAsyncAction MainPage::sample_panel_size_changed_async(IInspectable const&, RoutedEventArgs const&)
	{
		if (m_sample_dx.m_dxgi_swap_chain != nullptr) {
			m_sample_dx.m_dxgi_swap_chain = nullptr;
		}
		m_sample_sheet.set_attr_to(&m_sheet_main);
		const auto samp_w = scp_sample_panel().ActualWidth();
		const auto samp_h = scp_sample_panel().ActualHeight();
		m_sample_sheet.m_sheet_size.width = static_cast<FLOAT>(samp_w);
		m_sample_sheet.m_sheet_size.height = static_cast<FLOAT>(samp_h);
		m_sample_dx.SetSwapChainPanel(scp_sample_panel());
		if (m_sample_type != SAMPLE_TYPE::NONE) {
			if (m_sample_type == SAMPLE_TYPE::FONT) {
				using winrt::Windows::ApplicationModel::Resources::ResourceLoader;
				const auto padd_w = samp_w * 0.125;
				const auto padd_h = samp_h * 0.25;
				const D2D1_POINT_2F b_pos{ static_cast<FLOAT>(padd_w), static_cast<FLOAT>(padd_h) };
				const D2D1_POINT_2F b_vec{ static_cast<FLOAT>(samp_w - 2.0 * padd_w), static_cast<FLOAT>(samp_w - 2.0 * padd_h) };
				const auto pang = ResourceLoader::GetForCurrentView().GetString(L"str_pangram");
				const wchar_t* text = nullptr;
				if (pang.empty()) {
					text = L"The quick brown fox jumps over a lazy dog.";
				}
				else {
					text = pang.c_str();
				}
				m_sample_shape = new ShapeText(b_pos, b_vec, wchar_cpy(text), &m_sample_sheet);
			}
			else if (m_sample_type == SAMPLE_TYPE::STROKE) {
				const auto padd = samp_w * 0.125;
				const D2D1_POINT_2F b_pos{ static_cast<FLOAT>(padd), static_cast<FLOAT>(padd) };
				const D2D1_POINT_2F b_vec{ static_cast<FLOAT>(samp_w - 2.0 * padd), static_cast<FLOAT>(samp_h - 2.0 * padd) };
				m_sample_shape = new ShapeLineA(b_pos, b_vec, &m_sample_sheet);
			}
			else if (m_sample_type == SAMPLE_TYPE::FILL) {
				const auto padd = samp_w * 0.125;
				const D2D1_POINT_2F b_pos{ static_cast<FLOAT>(padd), static_cast<FLOAT>(padd) };
				const D2D1_POINT_2F b_vec{ static_cast<FLOAT>(samp_w - 2.0 * padd), static_cast<FLOAT>(samp_h - 2.0 * padd) };
				m_sample_shape = new ShapeRect(b_pos, b_vec, &m_sample_sheet);
			}
			else if (m_sample_type == SAMPLE_TYPE::JOIN) {
				const auto padd = samp_w * 0.125;
				const D2D1_POINT_2F b_pos{ static_cast<FLOAT>(padd), static_cast<FLOAT>(padd) };
				const D2D1_POINT_2F b_vec{ static_cast<FLOAT>(samp_w - 2.0 * padd), static_cast<FLOAT>(samp_h - 2.0 * padd) };
				POLY_OPTION p_opt { 3, true, true, false, true };
				m_sample_shape = new ShapePoly(b_pos, b_vec, &m_sample_sheet, p_opt);
				const double offset = samp_h / 16.0;
				m_sample_shape->set_pos_anch(D2D1_POINT_2F{ static_cast<FLOAT>(-samp_w * 0.25f), static_cast<FLOAT>(samp_h * 0.5 - offset) }, ANCH_TYPE::ANCH_P0, m_misc_pile_up);
				m_sample_shape->set_pos_anch(D2D1_POINT_2F{ static_cast<FLOAT>(samp_w * 0.25),  static_cast<FLOAT>(samp_h * 0.5) }, ANCH_TYPE::ANCH_P0 + 1, m_misc_pile_up);
				m_sample_shape->set_pos_anch(D2D1_POINT_2F{ static_cast<FLOAT>(-samp_w * 0.25f), static_cast<FLOAT>(samp_h * 0.5 + offset) }, ANCH_TYPE::ANCH_P0 + 2, m_misc_pile_up);
			}
			else if (m_sample_type == SAMPLE_TYPE::BITMAP) {
				using winrt::Windows::Foundation::Uri;

				winrt::apartment_context context;
				co_await winrt::resume_background();
				try {
					const auto s_file{ co_await StorageFile::GetFileFromApplicationUriAsync(Uri{ L"ms-appx:///Assets/sample.bmp" }) };
					const auto ra_stream{ co_await s_file.OpenReadAsync() };
					const auto dt_reader{ DataReader(ra_stream.GetInputStreamAt(0)) };
					const auto ra_size = static_cast<uint32_t>(ra_stream.Size());
					if (co_await dt_reader.LoadAsync(ra_size) == ra_size) {
						const D2D1_POINT_2F c_pos{ samp_w * 0.5f, samp_h * 0.5f };
						ShapeBitmap* s = new ShapeBitmap(c_pos, dt_reader);
						s->m_pos.x = samp_w * 0.125;
						s->m_pos.y = samp_h * 0.125;
						s->m_view.width = samp_w - samp_w * 0.25;
						s->m_view.height = samp_h - samp_h * 0.25;
						s->m_opac = m_sample_sheet.m_image_opac;
						m_sample_shape = s;
					}
				}
				catch (winrt::hresult_error& e) {
					auto a = e.code();
				}
				co_await context;
			}
			else if (m_sample_type == SAMPLE_TYPE::MISC) {
				constexpr uint32_t misc_min = 3;
				constexpr uint32_t misc_max = 12;
				static uint32_t misc_cnt = misc_min;
				const auto padd = samp_w * 0.125;
				const D2D1_POINT_2F samp_vec{ static_cast<FLOAT>(samp_w - 2.0 * padd), static_cast<FLOAT>(samp_h - 2.0 * padd) };
				POLY_OPTION p_opt{ m_tool_poly };
				p_opt.m_vertex_cnt = (misc_cnt >= misc_max ? misc_min : misc_cnt++);
				m_sample_shape = new ShapePoly(D2D1_POINT_2F{ 0.0f, 0.0f }, samp_vec, &m_sample_sheet, p_opt);
				D2D1_POINT_2F b_min;
				D2D1_POINT_2F b_max;
				D2D1_POINT_2F b_vec;
				m_sample_shape->get_bound(D2D1_POINT_2F{ FLT_MAX, FLT_MAX }, D2D1_POINT_2F{ -FLT_MAX, -FLT_MAX }, b_min, b_max);
				pt_sub(b_max, b_min, b_vec);
				m_sample_shape->move(D2D1_POINT_2F{ static_cast<FLOAT>((samp_w - b_vec.x) * 0.5), static_cast<FLOAT>((samp_h - b_vec.y) * 0.5) });
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
