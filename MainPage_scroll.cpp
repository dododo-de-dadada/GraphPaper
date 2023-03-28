//-------------------------------
// MainPage_scroll.cpp
// �X�N���[���o�[
//-------------------------------
#include "pch.h"
#include "MainPage.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	//using winrt::Windows::UI::Xaml::Controls::Primitives::ScrollEventArgs;

	// �X�N���[���o�[�����삳�ꂽ.
	void MainPage::scroll(IInspectable const& sender, ScrollEventArgs const& args)
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

	// �X�N���[���o�[�̒l��ݒ肷��.
	// act_w	���ۂ̕�
	// act_h	���ۂ̍���
	void MainPage::scroll_set(const double act_w, const double act_h)
	{
		constexpr double SB_SIZE = 16.0;
		const double p_scale = m_main_page.m_page_scale;	// �y�[�W�̔{��
		const double view_w = act_w / p_scale;	// �����Ă��镔���̕�
		const double view_h = act_h / p_scale;	// �����Ă��镔���̍���
		const auto lt = m_main_bbox_lt;	// �y�[�W���܂ސ}�`�S�̂̋��E��`�̍���ʒu
		const auto rb = m_main_bbox_rb;	// �y�[�W���܂ސ}�`�S�̂̋��E��`�̉E���ʒu
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

	// �}�`���\�������悤�\�����X�N���[������.
	// s	�\�������}�`
	bool MainPage::scroll_to(Shape* const s)
	{
		// �X�N���[���r���[�A�̃r���[�|�[�g�̍��W��, �\�����W�ŋ��߂�.
		const double ox = m_main_bbox_lt.x;	// ���_ x
		const double oy = m_main_bbox_lt.y;	// ���_ y
		const double ho = sb_horz().Value();	// ���̃X�N���[���l
		const double vo = sb_vert().Value();	// �c�̃X�N���[���l
		const double vw = sb_horz().ViewportSize();	// �\���̕�
		const double vh = sb_vert().ViewportSize();	// �\���̍���
		const D2D1_POINT_2F v_lt{	// �\���̍���ʒu
			static_cast<FLOAT>(ox + ho),
			static_cast<FLOAT>(oy + vo)
		};
		const D2D1_POINT_2F v_rb{	// �\���̉E���ʒu
			static_cast<FLOAT>(v_lt.x + vw),
			static_cast<FLOAT>(v_lt.y + vh)
		};
		// �e�X�g�s��̕��`��, �r���[�|�[�g�Ɋ܂܂�邩���肵,
		// �܂܂����`���ЂƂł������ false ��Ԃ�.
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

		// �ŏ��̕��`�̐����ʒu�Ɛ����ʒu�ɂ���, �r���[�|�[�g�͈̔͊O�̏ꍇ, �X�N���[������.
		if (t_rb.x < v_lt.x || v_rb.x < t_lt.x) {
			sb_horz().Value(t_lt.x - ox);
		}
		if (t_rb.y < v_lt.y || v_rb.y < t_lt.y) {
			sb_vert().Value(t_lt.y - oy);
		}
		return true;
	}

}