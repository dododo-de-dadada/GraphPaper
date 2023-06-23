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
	ID2D1RenderTarget* Shape::m_d2d_target = nullptr;	// D2D �`��Ώ�
	winrt::com_ptr<ID2D1DrawingStateBlock> Shape::m_state_block{ nullptr };	// �`���Ԃ�ێ�����u���b�N
	winrt::com_ptr<ID2D1StrokeStyle1> Shape::m_aux_style{ nullptr };	// �⏕���̌`��
	winrt::com_ptr<ID2D1SolidColorBrush> Shape::m_d2d_color_brush{ nullptr };	// �F�u���V (�^�[�Q�b�g�ˑ�)
	winrt::com_ptr<ID2D1SolidColorBrush> Shape::m_d2d_range_brush{ nullptr };	// �I�����ꂽ�����F�̃u���V (�^�[�Q�b�g�ˑ�)
	winrt::com_ptr<ID2D1BitmapBrush> Shape::m_d2d_bitmap_brush{ nullptr };	// �w�i�̉摜�u���V (�^�[�Q�b�g�ˑ�)
	winrt::com_ptr<IDWriteFactory> Shape::m_dwrite_factory{ create_dwrite_factory() };
	constexpr double LOCUS_LEN = 6.0;
	float Shape::m_aux_width = 1.0f;	// �⏕���̑���
	bool Shape::m_loc_show = true;	// ���ʂ̕\��/��\��.
	float Shape::m_loc_width = LOCUS_LEN;	// ���ʂ̑傫��
	float Shape::m_loc_square_inner = static_cast<float>(0.5 * LOCUS_LEN);	// ���� (�����`) �̓����̕ӂ̔����̒���
	float Shape::m_loc_square_outer = static_cast<float>(0.5 * (LOCUS_LEN + 4.0));	// ���� (�����`) �̊O���̕ӂ̔����̒���
	float Shape::m_loc_circle_inner = static_cast<float>(sqrt(LOCUS_LEN * LOCUS_LEN / M_PI));	// ���� (�~�`) �̓����̔��a
	float Shape::m_loc_circle_outer = static_cast<float>(sqrt(LOCUS_LEN * LOCUS_LEN / M_PI) + 2.0);	// ���� (�~�`) �̊O���̔��a
	float Shape::m_loc_rhombus_inner = static_cast<float>(sqrt(LOCUS_LEN * LOCUS_LEN * 0.5) * 0.5);	// ���� (�Ђ��^) �̒��S��������̒��_�܂ł̔����̒���
	float Shape::m_loc_rhombus_outer = static_cast<float>(sqrt((LOCUS_LEN + 4.0) * (LOCUS_LEN + 4.0) * 0.5) * 0.5);	// ���� (�Ђ��^) �̒��S����O���̒��_�܂ł̔����̒���

	// �`��O�ɕK�v�ȕϐ����i�[����.
	void Shape::begin_draw(
		ID2D1RenderTarget* target,	// D2D �`��Ώ�
		const bool located,	// ���ʂ̕\��/��\��
		IWICFormatConverter* const background,	// �w�i�p�^�[���̉摜 
		const double scale	// �\���{��
	) noexcept
	{
		HRESULT hr = S_OK;

		if (m_d2d_target != target) {
			m_d2d_target = target;
			m_d2d_color_brush = nullptr;
			m_d2d_range_brush = nullptr;
			m_d2d_bitmap_brush = nullptr;
		}
		if (target == nullptr) {
			return;
		}

		if (hr == S_OK && m_d2d_color_brush == nullptr) {
			hr = target->CreateSolidColorBrush({},
				Shape::m_d2d_color_brush.put());
		}

		if (hr == S_OK && m_d2d_range_brush == nullptr) {
			hr = target->CreateSolidColorBrush({},
				Shape::m_d2d_range_brush.put());
		}

		if (hr == S_OK && background == nullptr && m_d2d_bitmap_brush != nullptr) {
			m_d2d_bitmap_brush = nullptr;
		}
		else if (hr == S_OK && background != nullptr && m_d2d_bitmap_brush == nullptr) {
			winrt::com_ptr<ID2D1Bitmap> d2d_bitmap;
			if (hr == S_OK) {
				hr = target->CreateBitmapFromWicBitmap(background,
					d2d_bitmap.put());
			}
			if (hr == S_OK) {
				hr = target->CreateBitmapBrush(d2d_bitmap.get(),
					m_d2d_bitmap_brush.put());
			}
			if (hr == S_OK) {
				m_d2d_bitmap_brush->SetExtendModeX(D2D1_EXTEND_MODE_WRAP);
				m_d2d_bitmap_brush->SetExtendModeY(D2D1_EXTEND_MODE_WRAP);
			}
			d2d_bitmap = nullptr;
		}

		if (hr == S_OK && m_state_block == nullptr) {
			ID2D1Factory1* factory = nullptr;
			target->GetFactory(reinterpret_cast<ID2D1Factory**>(&factory));
			hr = factory->CreateDrawingStateBlock(
				m_state_block.put());
		}

		if (hr == S_OK && m_aux_style == nullptr) {
			ID2D1Factory1* factory = nullptr;
			target->GetFactory(reinterpret_cast<ID2D1Factory**>(&factory));
			hr = factory->CreateStrokeStyle(AUXILIARY_SEG_STYLE, AUXILIARY_SEG_DASHES, AUXILIARY_SEG_DASHES_CONT,
				m_aux_style.put());
		}

		if (hr == S_OK) {
			m_loc_show = located;
			m_aux_width = static_cast<float>(1.0 / scale);
			const double a_inner = LOCUS_LEN / scale;
			const double a_outer = (LOCUS_LEN + 4.0) / scale;
			m_loc_width = static_cast<float>(LOCUS_LEN / scale);
			m_loc_square_inner = static_cast<float>(0.5 * a_inner);
			m_loc_square_outer = static_cast<float>(0.5 * a_outer);
			const auto r = sqrt(LOCUS_LEN * LOCUS_LEN / M_PI);
			m_loc_circle_inner = static_cast<float>(r / scale);
			m_loc_circle_outer = static_cast<float>((r + 2.0) / scale);
			m_loc_rhombus_inner = static_cast<float>(sqrt(a_inner * a_inner * 0.5) * 0.5);
			m_loc_rhombus_outer = static_cast<float>(sqrt(a_outer * a_outer * 0.5) * 0.5);
		}
	}

}
