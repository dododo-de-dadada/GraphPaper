//-------------------------------
// MainPage_scroll.cpp
// スクロールバー
//-------------------------------
#include "pch.h"
#include "MainPage.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	//using winrt::Windows::UI::Xaml::Controls::Primitives::ScrollEventArgs;

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
		page_draw();
	}

	// スクロールバーの値を設定する.
	// act_w	実際の幅
	// act_h	実際の高さ
	void MainPage::scroll_set(const double act_w, const double act_h)
	{
		constexpr double SB_SIZE = 16.0;
		const double p_scale = m_main_page.m_page_scale;	// ページの倍率
		const double view_w = act_w / p_scale;	// 見えている部分の幅
		const double view_h = act_h / p_scale;	// 見えている部分の高さ
		const auto lt = m_main_bbox_lt;	// ページを含む図形全体の境界矩形の左上位置
		const auto rb = m_main_bbox_rb;	// ページを含む図形全体の境界矩形の右下位置
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
	bool MainPage::scroll_to(Shape* const s)
	{
		// スクロールビューアのビューポートの座標を, 表示座標で求める.
		const double ox = m_main_bbox_lt.x;	// 原点 x
		const double oy = m_main_bbox_lt.y;	// 原点 y
		const double ho = sb_horz().Value();	// 横のスクロール値
		const double vo = sb_vert().Value();	// 縦のスクロール値
		const double vw = sb_horz().ViewportSize();	// 表示の幅
		const double vh = sb_vert().ViewportSize();	// 表示の高さ
		const D2D1_POINT_2F v_lt{	// 表示の左上位置
			static_cast<FLOAT>(ox + ho),
			static_cast<FLOAT>(oy + vo)
		};
		const D2D1_POINT_2F v_rb{	// 表示の右下位置
			static_cast<FLOAT>(v_lt.x + vw),
			static_cast<FLOAT>(v_lt.y + vh)
		};
		// テスト行列の方形が, ビューポートに含まれるか判定し,
		// 含まれる方形がひとつでもあれば false を返す.
		D2D1_POINT_2F t_lt{};
		D2D1_POINT_2F t_rb{};
		DWRITE_TEXT_RANGE t_range;
		if (s->get_text_selected(t_range) && t_range.length > 0) {
			const auto s_text = static_cast<ShapeText*>(s);
			s_text->create_text_layout();
			const auto cnt = s_text->m_dwrite_selected_cnt;
			const auto met = s_text->m_dwrite_selected_metrics;
			D2D1_POINT_2F start;
			s->get_pos_start(start);
			for (auto i = cnt; i > 0; i--) {
				t_lt.x = start.x + met[i - 1].left;
				t_lt.y = start.y + met[i - 1].top;
				t_rb.x = t_lt.x + met[i - 1].width;
				t_rb.y = t_lt.y + met[i - 1].height;
				if (pt_in_rect(t_lt, v_lt, v_rb)
					|| pt_in_rect(t_rb, v_lt, v_rb)) {
					return false;
				}
			}
		}
		else {
			if (s->is_inside(v_lt, v_rb)) {
				return false;
			}
			s->get_bound(
				D2D1_POINT_2F{ FLT_MAX, FLT_MAX }, D2D1_POINT_2F{ -FLT_MAX, -FLT_MAX },
				t_lt, t_rb);
		}

		// 最初の方形の水平位置と垂直位置について, ビューポートの範囲外の場合, スクロールする.
		if (t_rb.x < v_lt.x || v_rb.x < t_lt.x) {
			sb_horz().Value(t_lt.x - ox);
		}
		if (t_rb.y < v_lt.y || v_rb.y < t_lt.y) {
			sb_vert().Value(t_lt.y - oy);
		}
		return true;
	}

}