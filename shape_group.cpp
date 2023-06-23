//------------------------------
// Shape_group.cpp
// �O���[�v�}�`
//------------------------------
#include "pch.h"
#include "shape.h"

namespace winrt::GraphPaper::implementation
{
	//using winrt::Windows::Storage::Streams::DataReader;
	//using winrt::Windows::Storage::Streams::DataWriter;

	//------------------------------
	// �}�`��\������.
	//------------------------------
	void ShapeGroup::draw(void) noexcept
	{
		// �I���t���O�������Ă邩���肷��.
		if (m_loc_show && is_selected()) {
			D2D1_POINT_2F b_lt { FLT_MAX, FLT_MAX };
			D2D1_POINT_2F b_rb{ -FLT_MAX, -FLT_MAX };
			// �O���[�v�����ꂽ�e�}�`�ɂ��Ĉȉ����J��Ԃ�.
			for (const auto s : m_list_grouped) {
				// �����t���O�������Ă��邩���肷��.
				if (s->is_deleted()) {
					continue;
				}
				s->draw();
				s->get_bbox(b_lt, b_rb, b_lt, b_rb);
			}
			ID2D1RenderTarget* const target = Shape::m_d2d_target;
			ID2D1SolidColorBrush* const brush = Shape::m_d2d_color_brush.get();
			target->DrawRectangle(D2D1_RECT_F{ b_lt.x, b_lt.y, b_rb.x, b_rb.y }, brush,
				m_aux_width, m_aux_style.get());
		}
		else {
			for (const auto s : m_list_grouped) {
				if (s->is_deleted()) {
					continue;
				}
				s->draw();
			}
		}
	}

	// ���E��`�𓾂�.
	void ShapeGroup::get_bbox(
		const D2D1_POINT_2F a_lt,	// ���̋�`�̍���ʒu.
		const D2D1_POINT_2F a_rb,	// ���̋�`�̉E���ʒu.
		D2D1_POINT_2F& b_lt,	// ����ꂽ��`�̍���ʒu.
		D2D1_POINT_2F& b_rb	// ����ꂽ��`�̉E���ʒu.
	) const noexcept
	{
		b_lt = a_lt;
		b_rb = a_rb;
		for (const auto s : m_list_grouped) {
			if (s->is_deleted()) {
				continue;
			}
			s->get_bbox(b_lt, b_rb, b_lt, b_rb);
		}
	}

	// ���E��`�̍���_�𓾂�.
	void ShapeGroup::get_bbox_lt(D2D1_POINT_2F& val) const noexcept
	{
		get_pos_start(val);
	}

	//------------------------------
	// �n�_�𓾂�.
	// val	�n�_
	// �O���[�v�}�`�̏ꍇ, �n�_�͐}�`���͂ދ�`�̍���_.
	//------------------------------
	bool ShapeGroup::get_pos_start(D2D1_POINT_2F& val) const noexcept
	{
		auto flag = false;
		for (const auto s : m_list_grouped) {
			if (s->is_deleted()) {
				continue;
			}
			D2D1_POINT_2F lt;
			s->get_bbox_lt(lt);
			if (!flag) {
				val = lt;
				flag = true;
			}
			else {
				val.x = lt.x < val.x ? lt.x : val.x;
				val.y = lt.y < val.y ? lt.y : val.y;
			}
		}
		return flag;
	}

	//------------------------------
	// ������}�`���܂ނ����肷��.
	//------------------------------
	bool ShapeGroup::has_text(void) noexcept
	{
		std::list<SHAPE_LIST::iterator> stack;
		stack.push_back(m_list_grouped.begin());
		stack.push_back(m_list_grouped.end());
		while (!stack.empty()) {
			auto j = stack.back();
			stack.pop_back();
			auto i = stack.back();
			stack.pop_back();
			while (i != j) {
				auto s = *i++;
				if (s == nullptr || s->is_deleted()) {
					continue;
				}
				if (typeid(*s) == typeid(ShapeText)) {
					return true;
				}
				else if (typeid(*s) == typeid(ShapeGroup)) {
					stack.push_back(i);
					stack.push_back(j);
					i = static_cast<ShapeGroup*>(s)->m_list_grouped.begin();
					j = static_cast<ShapeGroup*>(s)->m_list_grouped.end();
					continue;
				}
			}
		}
		return false;
	}

	// �}�`���_���܂ނ����肷��.
	// test_pt	���肳���_
	// �߂�l	�_���܂ޕ���
	uint32_t ShapeGroup::hit_test(const D2D1_POINT_2F test_pt, const bool/*ctrl_key*/) const noexcept
	{
		for (const Shape* s : m_list_grouped) {
			if (s->is_deleted()) {
				continue;
			}
			if (s->hit_test(test_pt, false) != LOCUS_TYPE::LOCUS_SHEET) {
				return LOCUS_TYPE::LOCUS_FILL;
			}
		}
		return LOCUS_TYPE::LOCUS_SHEET;
	}

	//------------------------------
	// ��`�Ɋ܂܂�邩���肷��.
	// lt	��`�̍���ʒu
	// rb	��`�̉E���ʒu
	// �߂�l	�܂܂��Ȃ� true
	// ���̑����͍l������Ȃ�.
	//------------------------------
	bool ShapeGroup::is_inside(const D2D1_POINT_2F lt, const D2D1_POINT_2F rb) const noexcept
	{
		for (const Shape* s : m_list_grouped) {
			if (!s->is_inside(lt, rb)) {
				return false;
			}
		}
		return true;
	}

	//------------------------------
	// �ʒu���ړ�����.
	// to	�ړ���ւ̃x�N�g��
	//------------------------------
	bool ShapeGroup::move(const D2D1_POINT_2F to) noexcept
	{
		return slist_move_selected(m_list_grouped, to);
	}

	//------------------------------
	// �l�������t���O�Ɋi�[����.
	// val	�i�[����l
	// �߂�l	�ύX���ꂽ�Ȃ� true
	//------------------------------
	bool ShapeGroup::set_delete(const bool val) noexcept
	{
		bool flag = false;
		for (Shape* s : m_list_grouped) {
			if (s->set_delete(val) && !flag) {
				flag = true;
			}
		}
		return flag;
	}

	//------------------------------
	// �l���n�_�Ɋi�[����. ���̕��ʂ̈ʒu������.
	// val	�i�[����l
	// �߂�l	�ύX���ꂽ�Ȃ� true
	//------------------------------
	bool ShapeGroup::set_pos_start(const D2D1_POINT_2F val) noexcept
	{
		D2D1_POINT_2F old_val;
		if (get_pos_start(old_val) && !equal(val, old_val)) {
			D2D1_POINT_2F pos;
			pt_sub(val, old_val, pos);
			move(pos);
			return true;
		}
		return false;
	}

	//------------------------------
	// �l��I���t���O�Ɋi�[����.
	// val	�i�[����l
	// �߂�l	�ύX���ꂽ�Ȃ� true
	//------------------------------
	bool ShapeGroup::set_select(const bool val) noexcept
	{
		bool flag = false;
		for (Shape* s : m_list_grouped) {
			if (s->set_select(val) && !flag) {
				flag = true;
			}
		}
		return flag;
	}

	//------------------------------
	// �}�`���f�[�^���[�_�[����ǂݍ���.
	//------------------------------
	ShapeGroup::ShapeGroup(DataReader const& dt_reader)
	{
		slist_read(m_list_grouped, dt_reader);
	}

	//------------------------------
	// �}�`���f�[�^���C�^�[�ɏ�������.
	//------------------------------
	void ShapeGroup::write(DataWriter const& dt_writer) const
	{
		constexpr bool REDUCED = true;
		slist_write<!REDUCED>(m_list_grouped, dt_writer);
	}

}