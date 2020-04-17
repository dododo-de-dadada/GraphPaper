//-------------------------------
// MainPage_scroll.cpp
// �X�N���[���o�[�̐ݒ�
//-------------------------------
#include "pch.h"
#include "MainPage.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	// �X�N���[���o�[�����삳�ꂽ.
	void MainPage::sb_scroll(IInspectable const& sender, ScrollEventArgs const& args)
	{
		using winrt::Windows::UI::Xaml::Controls::Primitives::ScrollBar;
		using winrt::Windows::UI::Xaml::Controls::Primitives::ScrollEventType;
		auto sb = unbox_value<ScrollBar>(sender);	// �X�N���[���o�[
		auto sv = sb.Value();	// �X�N���[���o�[�̒l
		auto vs = sb.ViewportSize();	// �X�N���[���o�[�̓��e�̌����Ă���傫��
		auto mi = sb.Minimum();	// �X�N���[���o�[�̍ŏ��l
		auto mx = sb.Maximum();	// �X�N���[���o�[�̍ő�l

		switch (args.ScrollEventType()) {
		case ScrollEventType::SmallDecrement:
			vs = 16.0;
			/* no break; */
		case ScrollEventType::LargeDecrement:
			sb.Value(max(sv - vs, mi));
			break;
		case ScrollEventType::SmallIncrement:
			vs = 16.0;
			/* no break; */
		case ScrollEventType::LargeIncrement:
			sb.Value(min(sv + vs, mx));
			break;
		}
		page_draw();
	}

	// �X�N���[���o�[�̒l��ݒ肷��.
	// aw	���ۂ̕�
	// ah	���ۂ̍���
	void MainPage::scroll_set(const double aw, const double ah)
	{
		constexpr double SB_SIZE = 16.0;
		const auto ps = m_page_layout.m_page_scale;	// �y�[�W�̕\���{��
		const double vw = aw / ps;	// �����Ă��镔���̕�
		const double vh = ah / ps;	// �����Ă��镔���̍���
		const auto p_min = page_min();
		const auto p_max = page_max();
		const auto mw = static_cast<double>(p_max.x) - static_cast<double>(p_min.x) - vw;
		const auto mh = static_cast<double>(p_max.y) - static_cast<double>(p_min.y) - vh;
		const auto wgt0 = mw > 0.0;
		const auto hgt0 = mh > 0.0;
		sb_horz().ViewportSize(vw);
		sb_horz().Maximum(wgt0 ? (hgt0 ? mw + SB_SIZE : mw) : 0.0);
		sb_horz().Visibility(wgt0 ? VISIBLE : COLLAPSED);
		sb_horz().Margin({ 0, 0, hgt0 ? SB_SIZE : 0.0, 0 });
		sb_vert().ViewportSize(vh);
		sb_vert().Maximum(hgt0 ? (wgt0 ? mh + SB_SIZE : mh) : 0.0);
		sb_vert().Visibility(hgt0 ? VISIBLE : COLLAPSED);
		sb_vert().Margin({ 0, 0, 0, wgt0 ? SB_SIZE : 0.0 });
		//sb_horz().ViewportSize(vw);
		//if (pw > vw) {
			//if (ph > vh) {
			// sb_horz().Maximum(pw + SB_SIZE - vw);
			//}
			//else {
			// sb_horz().Maximum(pw - vw);
			//}
			//sb_horz().Visibility(VISIBLE);
			//sb_vert().Margin({ 0, 0, 0, SB_SIZE });
		//}
		//else {
			//sb_horz().Maximum(0.0);
			//sb_horz().Visibility(COLLAPSED);
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
			//sb_vert().Visibility(VISIBLE);
			//sb_horz().Margin({ 0, 0, SB_SIZE, 0 });
		//}
		//else {
			//sb_vert().Maximum(0.0);
			//sb_vert().Visibility(COLLAPSED);
			//sb_horz().Margin({ 0, 0, 0, 0 });
		//}
	}

	// �}�`���\�������悤�y�[�W���X�N���[������.
	// s	�\�������}�`
	bool MainPage::scroll_to(Shape* s)
	{
		// �X�N���[���r���[�A�̃r���[�|�[�g�̍��W��, 
		// �y�[�W���W�ŋ��߂�.
		const double ox = m_page_min.x;
		const double oy = m_page_min.y;
		const double ho = sb_horz().Value();
		const double vo = sb_vert().Value();
		const double vw = sb_horz().ViewportSize();
		const double vh = sb_vert().ViewportSize();
		const D2D1_POINT_2F v_min{
			static_cast<FLOAT>(ox + ho),
			static_cast<FLOAT>(oy + vo)
		};
		const D2D1_POINT_2F v_max{
			static_cast<FLOAT>(v_min.x + vw),
			static_cast<FLOAT>(v_min.y + vh)
		};
		// �e�X�g�s��̕��`��, �r���[�|�[�g�Ɋ܂܂�邩����,
		// �܂܂����`���ЂƂł������ false ��Ԃ�.
		D2D1_POINT_2F r_min{};
		D2D1_POINT_2F r_max{};
		DWRITE_TEXT_RANGE t_range;
		if (s->get_text_range(t_range) && t_range.length > 0) {
			const auto s_text = static_cast<ShapeText*>(s);
			const auto cnt = s_text->m_dw_range_cnt;
			const auto mat = s_text->m_dw_range_metrics;
			D2D1_POINT_2F t_pos;
			s->get_start_pos(t_pos);
			for (auto i = cnt; i > 0; i--) {
				r_min.x = t_pos.x + mat[i - 1].left;
				r_min.y = t_pos.y + mat[i - 1].top;
				r_max.x = r_min.x + mat[i - 1].width;
				r_max.y = r_min.y + mat[i - 1].height;
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
			s->get_min_pos(r_min);
			r_max = r_min;
			s->get_bound(r_min, r_max);
		}

		// �ŏ��̕��`�̐����ʒu�Ɛ����ʒu�ɂ���,
		// �r���[�|�[�g�͈̔͊O�̏ꍇ, �X�N���[������.
		if (r_max.x < v_min.x || v_max.x < r_min.x) {
			sb_horz().Value(r_min.x - ox);
		}
		if (r_max.y < v_min.y || v_max.y < r_min.y) {
			sb_vert().Value(r_min.y - oy);
		}
		return true;
	}

}