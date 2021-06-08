//-------------------------------
// MainPage_scroll.cpp
// スクロールバー
//-------------------------------
#include "pch.h"
#include "MainPage.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
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
		sheet_draw();
	}

	// スクロールバーの値を設定する.
	// aw	実際の幅
	// ah	実際の高さ
	void MainPage::scroll_set(const double aw, const double ah)
	{
		constexpr double SB_SIZE = 16.0;
		const double ss = m_sheet_main.m_sheet_scale;	// 用紙の表示倍率
		const double vw = aw / ss;	// 見えている部分の幅
		const double vh = ah / ss;	// 見えている部分の高さ
		const auto s_min = m_sheet_min;
		const auto s_max = m_sheet_max;
		const auto mw = static_cast<double>(s_max.x) - static_cast<double>(s_min.x) - vw;
		const auto mh = static_cast<double>(s_max.y) - static_cast<double>(s_min.y) - vh;
		const auto wgt0 = mw > 0.0;
		const auto hgt0 = mh > 0.0;
		sb_horz().ViewportSize(vw);
		sb_horz().Maximum(wgt0 ? (hgt0 ? mw + SB_SIZE : mw) : 0.0);
		sb_horz().Visibility(wgt0 ? UI_VISIBLE : UI_COLLAPSED);
		sb_horz().Margin({ 0, 0, hgt0 ? SB_SIZE : 0.0, 0 });
		sb_vert().ViewportSize(vh);
		sb_vert().Maximum(hgt0 ? (wgt0 ? mh + SB_SIZE : mh) : 0.0);
		sb_vert().Visibility(hgt0 ? UI_VISIBLE : UI_COLLAPSED);
		sb_vert().Margin({ 0, 0, 0, wgt0 ? SB_SIZE : 0.0 });
		//sb_horz().ViewportSize(vw);
		//if (pw > vw) {
			//if (ph > vh) {
			// sb_horz().Maximum(pw + SB_SIZE - vw);
			//}
			//else {
			// sb_horz().Maximum(pw - vw);
			//}
			//sb_horz().Visibility(UI_VISIBLE);
			//sb_vert().Margin({ 0, 0, 0, SB_SIZE });
		//}
		//else {
			//sb_horz().Maximum(0.0);
			//sb_horz().Visibility(UI_COLLAPSED);
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
			//sb_vert().Visibility(UI_VISIBLE);
			//sb_horz().Margin({ 0, 0, SB_SIZE, 0 });
		//}
		//else {
			//sb_vert().Maximum(0.0);
			//sb_vert().Visibility(UI_COLLAPSED);
			//sb_horz().Margin({ 0, 0, 0, 0 });
		//}
	}

	// 図形が表示されるよう用紙をスクロールする.
	// s	表示される図形
	bool MainPage::scroll_to(const Shape* s)
	{
		// スクロールビューアのビューポートの座標を, 用紙座標で求める.
		const double ox = m_sheet_min.x;	// 原点 x
		const double oy = m_sheet_min.y;	// 原点 y
		const double ho = sb_horz().Value();	// 横のスクロール値
		const double vo = sb_vert().Value();	// 縦のスクロール値
		const double vw = sb_horz().ViewportSize();	// 用紙の幅
		const double vh = sb_vert().ViewportSize();	// 用紙の高さ
		const D2D1_POINT_2F v_min{
			static_cast<FLOAT>(ox + ho),
			static_cast<FLOAT>(oy + vo)
		};
		const D2D1_POINT_2F v_max{
			static_cast<FLOAT>(v_min.x + vw),
			static_cast<FLOAT>(v_min.y + vh)
		};
		// テスト行列の方形が, ビューポートに含まれるか判定し,
		// 含まれる方形がひとつでもあれば false を返す.
		D2D1_POINT_2F r_min{};
		D2D1_POINT_2F r_max{};
		DWRITE_TEXT_RANGE t_range;
		if (s->get_text_range(t_range) && t_range.length > 0) {
			const auto s_text = static_cast<const ShapeText*>(s);
			const auto cnt = s_text->m_dw_selected_cnt;
			const auto mtx = s_text->m_dw_selected_metrics;
			D2D1_POINT_2F t_pos;
			s->get_start_pos(t_pos);
			for (auto i = cnt; i > 0; i--) {
				r_min.x = t_pos.x + mtx[i - 1].left;
				r_min.y = t_pos.y + mtx[i - 1].top;
				r_max.x = r_min.x + mtx[i - 1].width;
				r_max.y = r_min.y + mtx[i - 1].height;
				if (pt_in_rect(r_min, v_min, v_max)
					|| pt_in_rect(r_max, v_min, v_max)) {
					return false;
				}
			}
		}
		else {
			if (s->in_area(v_min, v_max)) {
				return false;
			}
			s->get_bound(D2D1_POINT_2F{ FLT_MAX, FLT_MAX }, D2D1_POINT_2F{ -FLT_MAX, -FLT_MAX }, r_min, r_max);
		}

		// 最初の方形の水平位置と垂直位置について, ビューポートの範囲外の場合, スクロールする.
		if (r_max.x < v_min.x || v_max.x < r_min.x) {
			sb_horz().Value(r_min.x - ox);
		}
		if (r_max.y < v_min.y || v_max.y < r_min.y) {
			sb_vert().Value(r_min.y - oy);
		}
		return true;
	}

}