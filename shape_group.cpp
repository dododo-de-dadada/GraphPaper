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
	// sh	�\������p��
	//------------------------------
	void ShapeGroup::draw(void)
	{
		//ID2D1DeviceContext* const context = sheet.m_d2d.m_d2d_context.get();
		ID2D1RenderTarget* const context = Shape::s_target;
		ID2D1SolidColorBrush* const brush = Shape::s_color_brush;

		// �I���t���O�������Ă邩���肷��.
		if (is_selected()) {
			D2D1_POINT_2F b_min{ FLT_MAX, FLT_MAX };
			D2D1_POINT_2F b_max{ -FLT_MAX, -FLT_MAX };
			// �O���[�v�����ꂽ�e�}�`�ɂ��Ĉȉ����J��Ԃ�.
			for (const auto s : m_list_grouped) {
				// �����t���O�������Ă��邩���肷��.
				if (s->is_deleted()) {
					continue;
				}
				s->draw();
				s->get_bound(b_min, b_max, b_min, b_max);
			}
			context->DrawRectangle(D2D1_RECT_F{ b_min.x, b_min.y, b_max.x, b_max.y }, brush, 1.0f, Shape::m_aux_style.get());
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
	// a_min	���̗̈�̍���ʒu.
	// a_man	���̗̈�̉E���ʒu.
	// b_min	����ꂽ�̈�̍���ʒu.
	// b_max	����ꂽ�̈�̉E���ʒu.
	//------------------------------
	void ShapeGroup::get_bound(const D2D1_POINT_2F a_min, const D2D1_POINT_2F a_max, D2D1_POINT_2F& b_min, D2D1_POINT_2F& b_max) const noexcept
	{
		b_min = a_min;
		b_max = a_max;
		for (const auto s : m_list_grouped) {
			if (s->is_deleted()) {
				continue;
			}
			s->get_bound(b_min, b_max, b_min, b_max);
		}
	}

	//------------------------------
	// �}�`���͂ޗ̈�̍���ʒu�𓾂�.
	// val	�̈�̍���ʒu
	//------------------------------
	void ShapeGroup::get_pos_min(D2D1_POINT_2F& val) const noexcept
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
		//if (m_list_grouped.empty()) {
		//	return false;
		//}
		auto flag = false;
		for (const auto s : m_list_grouped) {
			if (s->is_deleted()) {
				continue;
			}
			D2D1_POINT_2F pos;
			s->get_pos_min(pos);
			if (!flag) {
				val = pos;
				flag = true;
			}
			else {
				val.x = pos.x < val.x ? pos.x : val.x;
				val.y = pos.y < val.y ? pos.y : val.y;
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
	// t_pos	���肷��ʒu
	// �߂�l	�ʒu���܂ސ}�`�̕���
	//------------------------------
	uint32_t ShapeGroup::hit_test(const D2D1_POINT_2F t_pos) const noexcept
	{
		for (const Shape* s : m_list_grouped) {
			if (s->is_deleted()) {
				continue;
			}
			if (s->hit_test(t_pos) != ANC_TYPE::ANC_SHEET) {
				return ANC_TYPE::ANC_FILL;
			}
		}
		return ANC_TYPE::ANC_SHEET;
	}

	//------------------------------
	// �͈͂Ɋ܂܂�邩���肷��.
	// area_min	�͈͂̍���ʒu
	// area_max	�͈͂̉E���ʒu
	// �߂�l	�܂܂��Ȃ� true
	// ���̑����͍l������Ȃ�.
	//------------------------------
	bool ShapeGroup::in_area(const D2D1_POINT_2F area_min, const D2D1_POINT_2F area_max) const noexcept
	{
		for (const Shape* s : m_list_grouped) {
			if (!s->in_area(area_min, area_max)) {
				return false;
			}
		}
		return true;
	}

	//------------------------------
	// ���������ړ�����
	// d_vec	�����x�N�g��
	//------------------------------
	bool ShapeGroup::move(const D2D1_POINT_2F d_vec) noexcept
	{
		return slist_move(m_list_grouped, d_vec);
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
		D2D1_POINT_2F b_min;
		if (get_pos_start(b_min) && !equal(val, b_min)) {
			D2D1_POINT_2F d_vec;
			pt_sub(val, b_min, d_vec);
			move(d_vec);
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