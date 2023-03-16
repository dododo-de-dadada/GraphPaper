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
	/*
	float Shape::s_anc_len = 6.0f;	// �A���J�[�|�C���g�̑傫��
	D2D1_COLOR_F Shape::s_background_color = COLOR_WHITE;	// �O�i�F (�A���J�[�̔w�i�F)
	D2D1_COLOR_F Shape::s_foreground_color = COLOR_BLACK;	// �w�i�F (�A���J�[�̑O�i�F)
	winrt::com_ptr<ID2D1StrokeStyle1> Shape::m_aux_style = nullptr;	// �⏕���̌`��
	ID2D1Factory3* Shape::s_d2d_factory = nullptr;
	ID2D1RenderTarget* Shape::s_d2d_target = nullptr;
	ID2D1SolidColorBrush* Shape::s_d2d_color_brush = nullptr;
	ID2D1SolidColorBrush* Shape::s_d2d_range_brush = nullptr;
	IDWriteFactory* Shape::s_dwrite_factory = nullptr;
	*/
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
	constexpr double ANC_LEN = 6.0;
	float Shape::m_aux_width = 1.0f;	// �⏕���̑���
	bool Shape::m_anc_show = true;	// �}�`�̕��ʂ�\������.
	float Shape::m_anc_width = ANC_LEN;	// �}�`�̕��ʂ̑傫��
	float Shape::m_anc_square_inner = static_cast<FLOAT>(0.5 * ANC_LEN);	// �}�`�̕��� (�����`) �̓����̕ӂ̔����̒���
	float Shape::m_anc_square_outer = static_cast<FLOAT>(0.5 * (ANC_LEN + 4.0));	// �}�`�̕��� (�����`) �̊O���̕ӂ̔����̒���
	float Shape::m_anc_circle_inner = static_cast<FLOAT>(sqrt(ANC_LEN * ANC_LEN / M_PI));	// �}�`�̕��� (�~�`) �̓����̔��a
	float Shape::m_anc_circle_outer = static_cast<FLOAT>(sqrt(ANC_LEN * ANC_LEN / M_PI) + 2.0);	// �}�`�̕��� (�~�`) �̊O���̔��a
	float Shape::m_anc_rhombus_inner = static_cast<FLOAT>(sqrt(ANC_LEN * ANC_LEN * 0.5) * 0.5);	// �}�`�̕��� (�Ђ��^) �̒��S��������̒��_�܂ł̔����̒���
	float Shape::m_anc_rhombus_outer =	// �}�`�̕��� (�Ђ��^) �̒��S����O���̒��_�܂ł̔����̒���
		static_cast<FLOAT>(sqrt((ANC_LEN + 4.0) * (ANC_LEN + 4.0) * 0.5) * 0.5);

	void Shape::begin_draw(
		ID2D1RenderTarget* target, const bool anc_show, IWICFormatConverter* const background, 
		const double scale)
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
			ID2D1Factory1* factory = nullptr;
			m_d2d_target->GetFactory(reinterpret_cast<ID2D1Factory**>(&factory));
			winrt::check_hresult(
				factory->CreateDrawingStateBlock(m_state_block.put())
			);
		}

		if (m_aux_style == nullptr) {
			ID2D1Factory1* factory = nullptr;
			m_d2d_target->GetFactory(reinterpret_cast<ID2D1Factory**>(&factory));
			winrt::check_hresult(
				factory->CreateStrokeStyle(
					AUXILIARY_SEG_STYLE, AUXILIARY_SEG_DASHES, AUXILIARY_SEG_DASHES_CONT,
					m_aux_style.put())
			);
		}

		m_anc_show = anc_show;
		m_aux_width = static_cast<float>(1.0 / scale);
		const double a_inner = ANC_LEN / scale;
		const double a_outer = (ANC_LEN + 4.0) / scale;
		m_anc_width = static_cast<float>(ANC_LEN / scale);
		m_anc_square_inner = static_cast<float>(0.5 * a_inner);
		m_anc_square_outer = static_cast<float>(0.5 * a_outer);
		const auto r = sqrt(ANC_LEN * ANC_LEN / M_PI);
		m_anc_circle_inner = static_cast<float>(r / scale);
		m_anc_circle_outer = static_cast<float>((r + 2.0) / scale);
		m_anc_rhombus_inner = static_cast<float>(sqrt(a_inner * a_inner * 0.5) * 0.5);
		m_anc_rhombus_outer = static_cast<float>(sqrt(a_outer * a_outer * 0.5) * 0.5);
	}

}
