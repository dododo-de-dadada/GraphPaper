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
	void ShapeGroup::draw(void)
	{
		// �I���t���O�������Ă邩���肷��.
		if (m_anc_show && is_selected()) {
			D2D1_POINT_2F b_lt { FLT_MAX, FLT_MAX };
			D2D1_POINT_2F b_rb{ -FLT_MAX, -FLT_MAX };
			// �O���[�v�����ꂽ�e�}�`�ɂ��Ĉȉ����J��Ԃ�.
			for (const auto s : m_list_grouped) {
				// �����t���O�������Ă��邩���肷��.
				if (s->is_deleted()) {
					continue;
				}
				s->draw();
				s->get_bound(b_lt, b_rb, b_lt, b_rb);
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

	//------------------------------
	// �}�`���͂ޗ̈�𓾂�.
	// a_lt	���̗̈�̍���ʒu.
	// a_rb	���̗̈�̉E���ʒu.
	// b_lt	����ꂽ�̈�̍���ʒu.
	// b_rb	����ꂽ�̈�̉E���ʒu.
	//------------------------------
	void ShapeGroup::get_bound(
		const D2D1_POINT_2F a_lt, const D2D1_POINT_2F a_rb, D2D1_POINT_2F& b_lt,
		D2D1_POINT_2F& b_rb) const noexcept
	{
		b_lt = a_lt;
		b_rb = a_rb;
		for (const auto s : m_list_grouped) {
			if (s->is_deleted()) {
				continue;
			}
			s->get_bound(b_lt, b_rb, b_lt, b_rb);
		}
	}

	//------------------------------
	// �}�`���͂ޗ̈�̍���ʒu�𓾂�.
	// val	�̈�̍���ʒu
	//------------------------------
	void ShapeGroup::get_bound_lt(D2D1_POINT_2F& val) const noexcept
	{
		get_pos_start(val);
	}

	//------------------------------
	// �J�n�ʒu�𓾂�.
	// val	�J�n�ʒu
	// �O���[�v�}�`�̏ꍇ, �J�n�ʒu�͐}�`���͂ޗ̈�̍���ʒu.
	//------------------------------
	bool ShapeGroup::get_pos_start(D2D1_POINT_2F& val) const noexcept
	{
		auto flag = false;
		for (const auto s : m_list_grouped) {
			if (s->is_deleted()) {
				continue;
			}
			D2D1_POINT_2F lt;
			s->get_bound_lt(lt);
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

	//------------------------------
	// �ʒu���܂ނ����肷��.
	// test	���肷��ʒu
	// �߂�l	�ʒu���܂ސ}�`�̕���
	//------------------------------
	uint32_t ShapeGroup::hit_test(const D2D1_POINT_2F test) const noexcept
	{
		for (const Shape* s : m_list_grouped) {
			if (s->is_deleted()) {
				continue;
			}
			if (s->hit_test(test) != ANC_TYPE::ANC_PAGE) {
				return ANC_TYPE::ANC_FILL;
			}
		}
		return ANC_TYPE::ANC_PAGE;
	}

	//------------------------------
	// �͈͂Ɋ܂܂�邩���肷��.
	// area_lt	�͈͂̍���ʒu
	// area_rb	�͈͂̉E���ʒu
	// �߂�l	�܂܂��Ȃ� true
	// ���̑����͍l������Ȃ�.
	//------------------------------
	bool ShapeGroup::in_area(const D2D1_POINT_2F area_lt, const D2D1_POINT_2F area_rb) const noexcept
	{
		for (const Shape* s : m_list_grouped) {
			if (!s->in_area(area_lt, area_rb)) {
				return false;
			}
		}
		return true;
	}

	//------------------------------
	// �ʒu���ړ�����
	// pos	�ړ�����ʒu�x�N�g��
	//------------------------------
	bool ShapeGroup::move(const D2D1_POINT_2F pos) noexcept
	{
		return slist_move_selected(m_list_grouped, pos);
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
	// �l���J�n�ʒu�Ɋi�[����. ���̕��ʂ̈ʒu������.
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
	ShapeGroup::ShapeGroup(const Shape& page, DataReader const& dt_reader)
	{
		slist_read(m_list_grouped, page, dt_reader);
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