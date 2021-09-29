//-------------------------------
// MainPage_sample.cpp
// 見本
//-------------------------------
#include "pch.h"
#include "MainPage.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	using winrt::Windows::Foundation::Uri;
	using winrt::Windows::Storage::FileAccessMode;
	using winrt::Windows::Graphics::Imaging::BitmapAlphaMode;
	using winrt::Windows::Graphics::Imaging::BitmapDecoder;
	using winrt::Windows::Graphics::Imaging::BitmapPixelFormat;
	using winrt::Windows::Storage::Streams::IRandomAccessStream;
	using winrt::Windows::ApplicationModel::Resources::ResourceLoader;

	// 見本を表示する
	void MainPage::sample_draw(void)
	{
#if defined(_DEBUG)
		if (scp_sample_panel().IsLoaded() != true) {
			return;
		}
#endif
		m_dx_mutex.lock();
		auto dc = m_sample_dx.m_d2d_context.get();
		dc->SaveDrawingState(m_sample_dx.m_state_block.get());
		dc->BeginDraw();
		dc->Clear(m_sample_sheet.m_sheet_color);
		const float offset = static_cast<FLOAT>(std::fmod(m_sample_sheet.m_sheet_size.width * 0.5, m_sample_sheet.m_grid_base + 1.0));
		m_sample_sheet.m_grid_offset.x = offset;
		m_sample_sheet.m_grid_offset.y = offset;
		m_sample_sheet.draw(m_sample_dx);
		/*
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
		*/
		winrt::check_hresult(dc->EndDraw());
		dc->RestoreDrawingState(m_sample_dx.m_state_block.get());
		m_sample_dx.Present(scp_sample_panel());
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

	IAsyncAction MainPage::sample_image_load_async(const float samp_w, const float samp_h)
	{
		bool ok;
		winrt::apartment_context context;
		co_await winrt::resume_background();
		try {
			const StorageFile samp_file{ co_await StorageFile::GetFileFromApplicationUriAsync(Uri{ L"ms-appx:///Assets/4.1.05.tiff" }) };
			const IRandomAccessStream samp_strm{ co_await samp_file.OpenAsync(FileAccessMode::Read) };
			const BitmapDecoder samp_dec{ co_await BitmapDecoder::CreateAsync(samp_strm) };
			const SoftwareBitmap samp_bmp{ co_await samp_dec.GetSoftwareBitmapAsync(BitmapPixelFormat::Bgra8, BitmapAlphaMode::Straight) };
			ShapeImage* samp_shape = new ShapeImage(
				D2D1_POINT_2F{ static_cast<FLOAT>(samp_w * 0.125), static_cast<FLOAT>(samp_h * 0.125) },
				D2D1_SIZE_F{ static_cast<float>(samp_w * 0.75), static_cast<FLOAT>(samp_h * 0.75) },
				samp_bmp,
				m_sample_sheet.m_image_opac);
			samp_bmp.Close();
			m_sample_sheet.m_shape_list.push_back(samp_shape);
#if defined(_DEBUG)
			debug_leak_cnt++;
#endif
			ok = true;
		}
		catch (winrt::hresult_error&) {
			ok = false;
		}
		co_await context;
		if (ok) {
			sample_draw();
		}
	}

	// 見本のスワップチェーンパネルの大きさが変わった.
	void MainPage::sample_panel_size_changed(IInspectable const&, RoutedEventArgs const&)
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
		if (m_sample_type == SAMPLE_TYPE::FONT) {
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
			//m_sample_shape = new ShapeText(b_pos, b_vec, wchar_cpy(text), &m_sample_sheet);
			m_sample_sheet.m_shape_list.push_back(new ShapeText(b_pos, b_vec, wchar_cpy(text), &m_sample_sheet));
#if defined(_DEBUG)
			debug_leak_cnt++;
#endif
			sample_draw();
		}
		else if (m_sample_type == SAMPLE_TYPE::STROKE) {
			const auto padd = samp_w * 0.125;
			const D2D1_POINT_2F b_pos{ static_cast<FLOAT>(padd), static_cast<FLOAT>(padd) };
			const D2D1_POINT_2F b_vec{ static_cast<FLOAT>(samp_w - 2.0 * padd), static_cast<FLOAT>(samp_h - 2.0 * padd) };
			//m_sample_shape = new ShapeLineA(b_pos, b_vec, &m_sample_sheet);
			m_sample_sheet.m_shape_list.push_back(new ShapeLineA(b_pos, b_vec, &m_sample_sheet));
#if defined(_DEBUG)
			debug_leak_cnt++;
#endif
			sample_draw();
		}
		else if (m_sample_type == SAMPLE_TYPE::FILL) {
			const auto padd = samp_w * 0.125;
			const D2D1_POINT_2F b_pos{ static_cast<FLOAT>(padd), static_cast<FLOAT>(padd) };
			const D2D1_POINT_2F b_vec{ static_cast<FLOAT>(samp_w - 2.0 * padd), static_cast<FLOAT>(samp_h - 2.0 * padd) };
			//m_sample_shape = new ShapeRect(b_pos, b_vec, &m_sample_sheet);
			m_sample_sheet.m_shape_list.push_back(new ShapeRect(b_pos, b_vec, &m_sample_sheet));
#if defined(_DEBUG)
			debug_leak_cnt++;
#endif
			sample_draw();
		}
		else if (m_sample_type == SAMPLE_TYPE::JOIN) {
			const auto padd = samp_w * 0.125;
			const D2D1_POINT_2F b_pos{ static_cast<FLOAT>(padd), static_cast<FLOAT>(padd) };
			const D2D1_POINT_2F b_vec{ static_cast<FLOAT>(samp_w - 2.0 * padd), static_cast<FLOAT>(samp_h - 2.0 * padd) };
			POLY_OPTION p_opt { 3, true, true, false, true };
			//m_sample_shape = new ShapePoly(b_pos, b_vec, &m_sample_sheet, p_opt);
			auto s = new ShapePoly(b_pos, b_vec, &m_sample_sheet, p_opt);
			const float offset = static_cast<float>(samp_h / 16.0);
			const float samp_x = static_cast<float>(samp_w * 0.25);
			const float samp_y = static_cast<float>(samp_h * 0.5);
			//m_sample_shape->set_pos_anch(D2D1_POINT_2F{ -samp_x, samp_y - offset }, ANCH_TYPE::ANCH_P0, m_misc_vert_stick, false);
			//m_sample_shape->set_pos_anch(D2D1_POINT_2F{ samp_x, samp_y }, ANCH_TYPE::ANCH_P0 + 1, m_misc_vert_stick, false);
			//m_sample_shape->set_pos_anch(D2D1_POINT_2F{ -samp_x, samp_y + offset }, ANCH_TYPE::ANCH_P0 + 2, m_misc_vert_stick, false);
			s->set_pos_anch(D2D1_POINT_2F{ -samp_x, samp_y - offset }, ANCH_TYPE::ANCH_P0, m_misc_vert_stick, false);
			s->set_pos_anch(D2D1_POINT_2F{ samp_x, samp_y }, ANCH_TYPE::ANCH_P0 + 1, m_misc_vert_stick, false);
			s->set_pos_anch(D2D1_POINT_2F{ -samp_x, samp_y + offset }, ANCH_TYPE::ANCH_P0 + 2, m_misc_vert_stick, false);
			m_sample_sheet.m_shape_list.push_back(s);
#if defined(_DEBUG)
			debug_leak_cnt++;
#endif
			sample_draw();
		}
		else if (m_sample_type == SAMPLE_TYPE::IMAGE) {
			sample_image_load_async(samp_w, samp_h);
		}
		else if (m_sample_type == SAMPLE_TYPE::MISC) {
			constexpr uint32_t misc_min = 3;
			constexpr uint32_t misc_max = 12;
			static uint32_t misc_cnt = misc_min;
			const auto padd = samp_w * 0.125;
			const D2D1_POINT_2F samp_vec{ static_cast<FLOAT>(samp_w - 2.0 * padd), static_cast<FLOAT>(samp_h - 2.0 * padd) };
			POLY_OPTION p_opt{ m_drawing_poly_opt };
			p_opt.m_vertex_cnt = (misc_cnt >= misc_max ? misc_min : misc_cnt++);
			//m_sample_shape = new ShapePoly(D2D1_POINT_2F{ 0.0f, 0.0f }, samp_vec, &m_sample_sheet, p_opt);
			auto s = new ShapePoly(D2D1_POINT_2F{ 0.0f, 0.0f }, samp_vec, &m_sample_sheet, p_opt);
			D2D1_POINT_2F b_min;
			D2D1_POINT_2F b_max;
			D2D1_POINT_2F b_vec;
			//m_sample_shape->get_bound(D2D1_POINT_2F{ FLT_MAX, FLT_MAX }, D2D1_POINT_2F{ -FLT_MAX, -FLT_MAX }, b_min, b_max);
			s->get_bound(D2D1_POINT_2F{ FLT_MAX, FLT_MAX }, D2D1_POINT_2F{ -FLT_MAX, -FLT_MAX }, b_min, b_max);
			pt_sub(b_max, b_min, b_vec);
			//m_sample_shape->move(D2D1_POINT_2F{ static_cast<FLOAT>((samp_w - b_vec.x) * 0.5), static_cast<FLOAT>((samp_h - b_vec.y) * 0.5) });
			s->move(D2D1_POINT_2F{ static_cast<FLOAT>((samp_w - b_vec.x) * 0.5), static_cast<FLOAT>((samp_h - b_vec.y) * 0.5) });
			m_sample_sheet.m_shape_list.push_back(s);
#if defined(_DEBUG)
			debug_leak_cnt++;
#endif
			sample_draw();
		}
		else {
			throw winrt::hresult_invalid_argument();
		}

	}

}
