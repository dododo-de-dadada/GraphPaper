//-------------------------------
// MainPage_scroll.cpp
// �X�N���[���o�[
//-------------------------------
#include "pch.h"
#include "MainPage.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	using winrt::Windows::UI::Xaml::Visibility;

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
		main_draw();
	}

	// �X�N���[���o�[�̒l��ݒ肷��.
	// act_w	���ۂ̕�
	// act_h	���ۂ̍���
	void MainPage::scroll_set(const double act_w, const double act_h)
	{
		constexpr double SB_SIZE = 16.0;
		const double view_w = act_w / m_main_scale;	// �����Ă��镔���̕�
		const double view_h = act_h / m_main_scale;	// �����Ă��镔���̍���
		const auto lt = m_main_bbox_lt;	// �p�����܂ސ}�`�S�̂̋��E��`�̍���ʒu
		const auto rb = m_main_bbox_rb;	// �p�����܂ސ}�`�S�̂̋��E��`�̉E���ʒu
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
	bool MainPage::scroll_to(const Shape* const s)
	{
		// �X�N���[���r���[�A�̃r���[�|�[�g�̍��W��, �\�����W�ŋ��߂�.
		const double ox = m_main_bbox_lt.x;	// ���_ x
		const double oy = m_main_bbox_lt.y;	// ���_ y
		const double ho = sb_horz().Value();	// ���̃X�N���[���l
		const double vo = sb_vert().Value();	// �c�̃X�N���[���l
		const double vw = sb_horz().ViewportSize();	// �\���̕�
		const double vh = sb_vert().ViewportSize();	// �\���̍���
		const D2D1_POINT_2F view_lt{	// �\����`�̍���ʒu
			static_cast<FLOAT>(ox + ho),
			static_cast<FLOAT>(oy + vo)
		};
		const D2D1_POINT_2F view_rb{	// �\����`�̉E���ʒu
			static_cast<FLOAT>(view_lt.x + vw),
			static_cast<FLOAT>(view_lt.y + vh)
		};

		// ���肳����`�𓾂�.
		D2D1_POINT_2F test_lt{};	// ���肳����`�̍���ʒu
		D2D1_POINT_2F test_rb{};	// ���肳����`�̉E���ʒu
		// �\�������}�`���ҏW�Ώۂ̐}�`�Ȃ�, ������̑I��͈͂𔻒肳����`�Ɋi�[����.
		if (static_cast<const ShapeText*>(s) == m_core_text_shape) {
			const ShapeText* t = m_core_text_shape;
			const auto end = m_main_sheet.m_select_trail ? m_main_sheet.m_select_end + 1 : m_main_sheet.m_select_end;
			if (m_main_sheet.m_select_start != end) {
				// ������̑I��͈͂̃L�����b�g�_�𓾂�, ����𔻒肷���`�Ɋi�[����.
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
		// ����ȊO�Ȃ�, �}�`�̋��E��`�𔻒肳����`�Ɋi�[����.
		else {
			s->get_bbox(D2D1_POINT_2F{ FLT_MAX, FLT_MAX }, D2D1_POINT_2F{ -FLT_MAX, -FLT_MAX }, test_lt, test_rb);
		}
		// ���肳����`���\����`�Ɋ܂܂�Ă���Ȃ� false ��Ԃ�.
		if (pt_in_rect(test_lt, view_lt, view_rb) && pt_in_rect(test_rb, view_lt, view_rb)) {
			return false;
		}

		// �ŏ��̕��`�̐����ʒu�Ɛ����ʒu�ɂ���, �\���͈̔͊O�̏ꍇ, �X�N���[������.
		if (test_rb.x < view_lt.x || view_rb.x < test_lt.x) {
			sb_horz().Value(test_lt.x - ox);
		}
		if (test_rb.y < view_lt.y || view_rb.y < test_lt.y) {
			sb_vert().Value(test_lt.y - oy);
		}
		return true;
	}

}