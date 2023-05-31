//-------------------------------
// MainPage_scroll.cpp
// スクロールバー
//-------------------------------
#include "pch.h"
#include "MainPage.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	using winrt::Windows::UI::Xaml::Visibility;

	// スクロールバーが操作された.
	void MainPage::scroll(IInspectable const& sender, ScrollEventArgs const& args)
	{
		using winrt::Windows::UI::Xaml::Controls::Primitives::ScrollBar;
		using winrt::Windows::UI::Xaml::Controls::Primitives::ScrollEventType;
		auto sb = unbox_value<ScrollBar>(sender);	// スクロールバー
		auto sv = sb.Value();	// スクロールバーの値
		auto vs = sb.ViewportSize();	// スクロールバーの内容の見えている大きさ
		auto mi = sb.Minimum();	// スクロールバーの最小値
		auto mx = sb.Maximum();	// スクロールバーの最大値

		switch (args.ScrollEventType()) {
		case ScrollEventType::SmallDecrement:
			vs = 16.0;
			[[fallthrough]];
		case ScrollEventType::LargeDecrement:
			sb.Value(max(sv - vs, mi));
			break;
		case ScrollEventType::SmallIncrement:
			vs = 16.0;
			[[fallthrough]];
		case ScrollEventType::LargeIncrement:
			sb.Value(min(sv + vs, mx));
			break;
		}
		main_draw();
	}

	// スクロールバーの値を設定する.
	// act_w	実際の幅
	// act_h	実際の高さ
	void MainPage::scroll_set(const double act_w, const double act_h)
	{
		constexpr double SB_SIZE = 16.0;
		const double view_w = act_w / m_main_scale;	// 見えている部分の幅
		const double view_h = act_h / m_main_scale;	// 見えている部分の高さ
		const auto lt = m_main_bbox_lt;	// 用紙を含む図形全体の境界矩形の左上位置
		const auto rb = m_main_bbox_rb;	// 用紙を含む図形全体の境界矩形の右下位置
		const auto mw = static_cast<double>(rb.x) - static_cast<double>(lt.x) - view_w;
		const auto mh = static_cast<double>(rb.y) - static_cast<double>(lt.y) - view_h;
		sb_horz().ViewportSize(view_w);
		sb_horz().Maximum(mw >= 1.0 ? (mh >= 1.0 ? mw + SB_SIZE : mw) : 0.0);
		sb_horz().Visibility(mw >= 1.0 ? Visibility::Visible : Visibility::Collapsed);
		sb_horz().Margin({ 0, 0, mh >= 1.0 ? SB_SIZE : 0.0, 0 });
		sb_vert().ViewportSize(view_h);
		sb_vert().Maximum(mh >= 1.0 ? (mw >= 1.0 ? mh + SB_SIZE : mh) : 0.0);
		sb_vert().Visibility(mh >= 1.0 ? Visibility::Visible : Visibility::Collapsed);
		sb_vert().Margin({ 0, 0, 0, mw >= 1.0 ? SB_SIZE : 0.0 });
		//sb_horz().ViewportSize(vw);
		//if (pw > vw) {
			//if (ph > vh) {
			// sb_horz().Maximum(pw + SB_SIZE - vw);
			//}
			//else {
			// sb_horz().Maximum(pw - vw);
			//}
			//sb_horz().Visibility(Visibility::Visible);
			//sb_vert().Margin({ 0, 0, 0, SB_SIZE });
		//}
		//else {
			//sb_horz().Maximum(0.0);
			//sb_horz().Visibility(Visibility::Collapsed);
			//sb_vert().Margin({ 0, 0, 0, 0 });
		//}
		//sb_vert().ViewportSize(vh);
		//if (ph > vh) {
			//if (pw > vw) {
			// sb_vert().Maximum(ph + SB_SIZE - vh);
			//}
			//else {
			// sb_vert().Maximum(ph - vh);
			//}
			//sb_vert().Visibility(Visibility::Visible);
			//sb_horz().Margin({ 0, 0, SB_SIZE, 0 });
		//}
		//else {
			//sb_vert().Maximum(0.0);
			//sb_vert().Visibility(Visibility::Collapsed);
			//sb_horz().Margin({ 0, 0, 0, 0 });
		//}
	}

	// 図形が表示されるよう表示をスクロールする.
	// s	表示される図形
	bool MainPage::scroll_to(const Shape* const s)
	{
		// スクロールビューアのビューポートの座標を, 表示座標で求める.
		const double ox = m_main_bbox_lt.x;	// 原点 x
		const double oy = m_main_bbox_lt.y;	// 原点 y
		const double ho = sb_horz().Value();	// 横のスクロール値
		const double vo = sb_vert().Value();	// 縦のスクロール値
		const double vw = sb_horz().ViewportSize();	// 表示の幅
		const double vh = sb_vert().ViewportSize();	// 表示の高さ
		const D2D1_POINT_2F view_lt{	// 表示矩形の左上位置
			static_cast<FLOAT>(ox + ho),
			static_cast<FLOAT>(oy + vo)
		};
		const D2D1_POINT_2F view_rb{	// 表示矩形の右下位置
			static_cast<FLOAT>(view_lt.x + vw),
			static_cast<FLOAT>(view_lt.y + vh)
		};

		// 判定される矩形を得る.
		D2D1_POINT_2F test_lt{};	// 判定される矩形の左上位置
		D2D1_POINT_2F test_rb{};	// 判定される矩形の右下位置
		// 表示される図形が編集対象の図形なら, 文字列の選択範囲を判定される矩形に格納する.
		if (static_cast<const ShapeText*>(s) == m_core_text_shape) {
			const ShapeText* t = m_core_text_shape;
			const auto end = m_main_sheet.m_select_trail ? m_main_sheet.m_select_end + 1 : m_main_sheet.m_select_end;
			if (m_main_sheet.m_select_start != end) {
				// 文字列の選択範囲のキャレット点を得て, これを判定する矩形に格納する.
				D2D1_POINT_2F car_start;
				D2D1_POINT_2F car_end;
				t->get_text_caret(m_main_sheet.m_select_start, t->get_text_row(m_main_sheet.m_select_start), false, car_start);
				t->get_text_caret(m_main_sheet.m_select_end, t->get_text_row(m_main_sheet.m_select_end), m_main_sheet.m_select_trail, car_end);
				test_lt.x = min(car_start.x, car_end.x);
				test_lt.y = min(car_start.y, car_end.y);
				test_rb.x = max(car_start.x, car_end.x);
				test_rb.y = max(car_start.y, car_end.y) + t->m_font_size;
			}
		}
		// それ以外なら, 図形の境界矩形を判定される矩形に格納する.
		else {
			s->get_bbox(D2D1_POINT_2F{ FLT_MAX, FLT_MAX }, D2D1_POINT_2F{ -FLT_MAX, -FLT_MAX }, test_lt, test_rb);
		}
		// 判定される矩形が表示矩形に含まれているなら false を返す.
		if (pt_in_rect(test_lt, view_lt, view_rb) && pt_in_rect(test_rb, view_lt, view_rb)) {
			return false;
		}

		// 最初の方形の水平位置と垂直位置について, 表示の範囲外の場合, スクロールする.
		if (test_rb.x < view_lt.x || view_rb.x < test_lt.x) {
			sb_horz().Value(test_lt.x - ox);
		}
		if (test_rb.y < view_lt.y || view_rb.y < test_lt.y) {
			sb_vert().Value(test_lt.y - oy);
		}
		return true;
	}

}