//------------------------------
// shape.cpp
// �}�`�̂ЂȌ^, ���̑�
//------------------------------
#include "pch.h"
#include "shape.h"

namespace winrt::GraphPaper::implementation
{
#if defined(_DEBUG)
	uint32_t debug_deleted_cnt = 0;
	uint32_t debug_leak_cnt = 0;
	uint32_t debug_shape_cnt = 0;
#endif

	static winrt::com_ptr<IDWriteFactory3> create_dwrite_factory(void)
	{
		winrt::com_ptr<IDWriteFactory3> dwrite_factory;
		// �t�@�N�g���^�C�v�ɂ� DWRITE_FACTORY_TYPE_SHARED ���w�肷��.
		winrt::check_hresult(
			DWriteCreateFactory(
				DWRITE_FACTORY_TYPE_SHARED,
				__uuidof(IDWriteFactory3),
				reinterpret_cast<::IUnknown**>(dwrite_factory.put())
			)
		);
		return dwrite_factory;
	};
	ID2D1RenderTarget* Shape::m_d2d_target = nullptr;
	winrt::com_ptr<ID2D1DrawingStateBlock> Shape::m_state_block{ nullptr };	// �`���Ԃ�ێ�����u���b�N
	winrt::com_ptr<ID2D1StrokeStyle1> Shape::m_aux_style{ nullptr };	// �⏕���̌`��
	winrt::com_ptr<ID2D1SolidColorBrush> Shape::m_d2d_color_brush{ nullptr };	// �F�u���V (�^�[�Q�b�g�ˑ�)
	winrt::com_ptr<ID2D1SolidColorBrush> Shape::m_d2d_range_brush{ nullptr };	// �I�����ꂽ�����F�̃u���V (�^�[�Q�b�g�ˑ�)
	winrt::com_ptr<ID2D1BitmapBrush> Shape::m_d2d_bitmap_brush{ nullptr };	// �w�i�̉摜�u���V (�^�[�Q�b�g�ˑ�)
	winrt::com_ptr<IDWriteFactory> Shape::m_dwrite_factory{ create_dwrite_factory() };
	constexpr double LOC_LEN = 6.0;
	float Shape::m_aux_width = 1.0f;	// �⏕���̑���
	bool Shape::m_loc_show = true;	// ���ʂ̕\��/��\��.
	float Shape::m_loc_width = LOC_LEN;	// ���ʂ̑傫��
	float Shape::m_loc_square_inner = static_cast<FLOAT>(0.5 * LOC_LEN);	// ���� (�����`) �̓����̕ӂ̔����̒���
	float Shape::m_loc_square_outer = static_cast<FLOAT>(0.5 * (LOC_LEN + 4.0));	// ���� (�����`) �̊O���̕ӂ̔����̒���
	float Shape::m_loc_circle_inner = static_cast<FLOAT>(sqrt(LOC_LEN * LOC_LEN / M_PI));	// ���� (�~�`) �̓����̔��a
	float Shape::m_loc_circle_outer = static_cast<FLOAT>(sqrt(LOC_LEN * LOC_LEN / M_PI) + 2.0);	// ���� (�~�`) �̊O���̔��a
	float Shape::m_loc_rhombus_inner = static_cast<FLOAT>(sqrt(LOC_LEN * LOC_LEN * 0.5) * 0.5);	// ���� (�Ђ��^) �̒��S��������̒��_�܂ł̔����̒���
	float Shape::m_loc_rhombus_outer =	// ���� (�Ђ��^) �̒��S����O���̒��_�܂ł̔����̒���
		static_cast<FLOAT>(sqrt((LOC_LEN + 4.0) * (LOC_LEN + 4.0) * 0.5) * 0.5);

	// �}�`��`�悷��O�ɐݒ肷��.
	void Shape::begin_draw(
		ID2D1RenderTarget* target,	// D2D �`��Ώ�
		const bool located,	// ���ʂ̕\��/��\��
		IWICFormatConverter* const background,	// �w�i�p�^�[���̉摜 
		const double scale	// �\���{��
	)
	{
		// �`��Ώۂ��ύX�����Ȃ�, �^�[�Q�b�g�ˑ��̕`��I�u�W�F�N�g���쐬����.
		if (m_d2d_target != target) {
			m_d2d_target = target;

			// �����_�[�^�[�Q�b�g�ˑ��̐F�u���V���쐬����.
			m_d2d_color_brush = nullptr;
			winrt::check_hresult(
				m_d2d_target->CreateSolidColorBrush({}, Shape::m_d2d_color_brush.put())
			);
			m_d2d_range_brush = nullptr;
			winrt::check_hresult(
				m_d2d_target->CreateSolidColorBrush({}, Shape::m_d2d_range_brush.put())
			);

			// �����_�[�^�[�Q�b�g�ˑ��̉摜�u���V���쐬����.
			m_d2d_bitmap_brush = nullptr;
			if (background != nullptr) {
				winrt::com_ptr<ID2D1Bitmap> d2d_bitmap;
				winrt::check_hresult(
					target->CreateBitmapFromWicBitmap(background, d2d_bitmap.put())
				);
				winrt::check_hresult(
					target->CreateBitmapBrush(d2d_bitmap.get(), m_d2d_bitmap_brush.put())
				);
				d2d_bitmap = nullptr;
				m_d2d_bitmap_brush->SetExtendModeX(D2D1_EXTEND_MODE_WRAP);
				m_d2d_bitmap_brush->SetExtendModeY(D2D1_EXTEND_MODE_WRAP);
			}
		}

		if (m_state_block == nullptr) {

			// �`���Ԃ�ێ�����u���b�N���쐬����.
			ID2D1Factory1* factory = nullptr;
			m_d2d_target->GetFactory(reinterpret_cast<ID2D1Factory**>(&factory));
			winrt::check_hresult(
				factory->CreateDrawingStateBlock(m_state_block.put())
			);
		}

		if (m_aux_style == nullptr) {

			// �⏕���̃X�g���[�N�`�����쐬����.
			ID2D1Factory1* factory = nullptr;
			m_d2d_target->GetFactory(reinterpret_cast<ID2D1Factory**>(&factory));
			winrt::check_hresult(
				factory->CreateStrokeStyle(
					AUXILIARY_SEG_STYLE, AUXILIARY_SEG_DASHES, AUXILIARY_SEG_DASHES_CONT,
					m_aux_style.put())
			);
		}

		m_loc_show = located;
		m_aux_width = static_cast<float>(1.0 / scale);
		const double a_inner = LOC_LEN / scale;
		const double a_outer = (LOC_LEN + 4.0) / scale;
		m_loc_width = static_cast<float>(LOC_LEN / scale);
		m_loc_square_inner = static_cast<float>(0.5 * a_inner);
		m_loc_square_outer = static_cast<float>(0.5 * a_outer);
		const auto r = sqrt(LOC_LEN * LOC_LEN / M_PI);
		m_loc_circle_inner = static_cast<float>(r / scale);
		m_loc_circle_outer = static_cast<float>((r + 2.0) / scale);
		m_loc_rhombus_inner = static_cast<float>(sqrt(a_inner * a_inner * 0.5) * 0.5);
		m_loc_rhombus_outer = static_cast<float>(sqrt(a_outer * a_outer * 0.5) * 0.5);
	}

}
