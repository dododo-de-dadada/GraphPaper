//-------------------------------
// MainPage_core_text.cpp
// ��������
//-------------------------------
#include "pch.h"
#include "MainPage.h"
#include "undo.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	using winrt::Windows::Foundation::Rect;
	using winrt::Windows::UI::Text::Core::CoreTextInputScope;
	using winrt::Windows::UI::Text::Core::CoreTextInputPaneDisplayPolicy;
	using winrt::Windows::UI::Text::Core::CoreTextLayoutRequest;
	using winrt::Windows::UI::Text::Core::CoreTextRange;
	using winrt::Windows::UI::Text::Core::CoreTextSelectionRequest;
	using winrt::Windows::UI::Text::Core::CoreTextTextRequest;
	using winrt::Windows::UI::Xaml::Media::GeneralTransform;
	using winrt::Windows::UI::Xaml::Window;

	// ��������͂̂��߂̃n���h���[��ݒ肷��.
	void MainPage::core_text_setup_handler(void) noexcept
	{
		// �e�L�X�g����
		// CoreTextEditContext::NotifyFocusEnter
		//	- SelectionRequested
		//	- TextRequested
		// ���p��������
		//	- TextUpdating
		//	- TextRequested
		// �S�p��������
		//	- CompositionStarted
		//	- TextUpdating
		//	- TextRequested
		//	- FormatUpdating
		//	- CompositionCompleted
		m_core_text.InputPaneDisplayPolicy(CoreTextInputPaneDisplayPolicy::Manual);
		m_core_text.InputScope(CoreTextInputScope::Text);
		m_core_text.TextRequested([this](auto const&, auto const& args) {
#ifdef _DEBUG
			winrt::hstring debug_text{ status_bar_debug().Text().size() > 32 ? status_bar_debug().Text().data() + status_bar_debug().Text().size() - 16 : status_bar_debug().Text().data() };
			status_bar_debug().Text(debug_text + L" TextRequested");
#endif
			if (m_core_text_focused == nullptr) {
				return;
			}
			CoreTextTextRequest req{ args.Request() };
			const CoreTextRange ran{ req.Range() };
			const auto end = static_cast<uint32_t>(ran.EndCaretPosition);
			const auto text = (m_core_text_focused->m_text == nullptr ? L"" : m_core_text_focused->m_text);
			const auto len = min(end, m_core_text_focused->get_text_len()) - ran.StartCaretPosition;	// ����������̒���
			winrt::hstring sub_text{	// ����������
				text + ran.StartCaretPosition, static_cast<winrt::hstring::size_type>(len)
			};
			req.Text(sub_text);
			});
		m_core_text.SelectionRequested([this](auto const&, auto const& args) {
#ifdef _DEBUG
			winrt::hstring debug_text{ status_bar_debug().Text().size() > 32 ? status_bar_debug().Text().data() + status_bar_debug().Text().size() - 16 : status_bar_debug().Text().data() };
			status_bar_debug().Text(debug_text + L" SelectionRequested");
#endif
			if (m_core_text_focused == nullptr) {
				return;
			}
			CoreTextSelectionRequest req{ args.Request() };
			const ShapeText* t = m_core_text_focused;
			const auto len = t->get_text_len();
			const auto end = min(m_main_sheet.m_core_text_range.m_trail ? m_main_sheet.m_core_text_range.m_end + 1 : m_main_sheet.m_core_text_range.m_end, len);
			const auto start = min(m_main_sheet.m_core_text_range.m_start, len);
			CoreTextRange ran{};
			ran.StartCaretPosition = min(start, end);
			ran.EndCaretPosition = max(start, end);
			req.Selection(ran);
			});
		m_core_text.FocusRemoved([this](auto const&, auto const& args) {
#ifdef _DEBUG
			winrt::hstring debug_text{ status_bar_debug().Text().size() > 32 ? status_bar_debug().Text().data() + status_bar_debug().Text().size() - 16 : status_bar_debug().Text().data() };
			status_bar_debug().Text(debug_text + L" FocusRemoved");
#endif
			if (m_core_text_focused == nullptr) {
				return;
			}
			__debugbreak();
			m_core_text.NotifyFocusLeave();
			undo_push_text_unselect(m_core_text_focused);
			m_core_text_focused = nullptr;
			m_core_text_comp = false;
			main_sheet_draw();
			});
		// ���������͂����
		m_core_text.TextUpdating([this](auto const&, auto const& args) {
#ifdef _DEBUG
			winrt::hstring debug_text{ status_bar_debug().Text().size() > 32 ? status_bar_debug().Text().data() + status_bar_debug().Text().size() - 16 : status_bar_debug().Text().data() };
			status_bar_debug().Text(debug_text + L" TextUpdating");
#endif
			//__debugbreak();
			CoreTextRange ran{ args.Range() };
			const winrt::hstring ins_text{ args.Text() };
			core_text_insert(ins_text.data(), static_cast<uint32_t>(ins_text.size()));
			});
		// �ϊ���, �L�����b�g���ړ�����
		m_core_text.SelectionUpdating([this](auto const&, auto const& args) {
#ifdef _DEBUG
			winrt::hstring debug_text{ status_bar_debug().Text().size() > 32 ? status_bar_debug().Text().data() + status_bar_debug().Text().size() - 16 : status_bar_debug().Text().data() };
			status_bar_debug().Text(debug_text + L" SelectionUpdating");
#endif
			CoreTextRange ran{ args.Selection() };
			undo_push_text_select(m_core_text_focused, ran.StartCaretPosition, ran.EndCaretPosition, false);
			main_sheet_draw();
			});
		// �ϊ���₪�\������钼�O�ɌĂ΂�, ���Ԃ�ϊ����̏��̂Ȃǂ�ݒ肷����.
#ifdef _DEBUG
		m_core_text.FormatUpdating([this](auto const&, auto const&) {
			winrt::hstring debug_text{ status_bar_debug().Text().size() > 32 ? status_bar_debug().Text().data() + status_bar_debug().Text().size() - 16 : status_bar_debug().Text().data() };
			status_bar_debug().Text(debug_text + L" FormatUpdating");
#else
		m_core_text.FormatUpdating([](auto const&, auto const&) {
#endif
			//__debugbreak();
			});
		// �ϊ�����\�����邽�߂̋�`��ݒ肷��.
		m_core_text.LayoutRequested([this](auto const&, auto const& args) {
#ifdef _DEBUG
			winrt::hstring debug_text{ status_bar_debug().Text().size() > 32 ? status_bar_debug().Text().data() + status_bar_debug().Text().size() - 16 : status_bar_debug().Text().data() };
			status_bar_debug().Text(debug_text + L" LayoutRequested");
#endif
			if (m_core_text_focused == nullptr) {
				return;
			}
			CoreTextLayoutRequest req{ args.Request() };
			Rect con_rect;	// �e�L�X�g�̋�`
			Rect sel_rect;	// �I��͈͂̋�`
			D2D1_POINT_2F con_start, con_end;	// �e�L�X�g�̒[
			D2D1_POINT_2F sel_start, sel_end;	// �I��͈͂̒[
			// �L�����b�g������s�𓾂�.
			// �L�����b�g�͑I��͈͂� end �̈ʒu�ɂ���.
			const ShapeText* t = m_core_text_focused;
			if (t->m_dwrite_text_layout == nullptr) {
				m_core_text_focused->create_text_layout();
			}

			float height = t->m_font_size;
			for (uint32_t i = 0; i + 1 < t->m_dwrite_line_cnt; i++) {
				height += t->m_dwrite_line_metrics[i].height;
			}
			height = max(height, t->m_dwrite_text_layout->GetMaxHeight());
			float width = t->m_dwrite_text_layout->GetMaxWidth();
			float left = (t->m_lineto.x < 0.0 ? t->m_start.x + t->m_lineto.x : t->m_start.x);
			float top = (t->m_lineto.y < 0.0 ? t->m_start.y + t->m_lineto.y : t->m_start.y);
			GeneralTransform tran{	// �ϊ��q
				scp_main_panel().TransformToVisual(nullptr)
			};
			const Point panel{	// �X���b�v�`�F�[���p�l���̍���_
				tran.TransformPoint(Point{ 0.0f, 0.0f })
			};
			Rect win_rect{	// �E�B���h�E�̃N���C�A���g��`
				Window::Current().CoreWindow().Bounds()
			};
			con_rect.X = win_rect.X + panel.X + left / m_main_scale;
			con_rect.Y = win_rect.Y + panel.Y + top / m_main_scale;
			con_rect.Width = width / m_main_scale;
			con_rect.Height = height / m_main_scale;

			const auto end = m_main_sheet.m_core_text_range.m_trail ? m_main_sheet.m_core_text_range.m_end + 1 : m_main_sheet.m_core_text_range.m_end;
			const auto start = m_main_sheet.m_core_text_range.m_start;
			const auto car_row = t->get_text_row(end);	// �L�����b�g�s
			const auto row_start = t->m_dwrite_test_metrics[car_row].textPosition;	// �L�����b�g�s�̊J�n�ʒu
			const auto row_end = row_start + t->m_dwrite_test_metrics[car_row].length;	// �L�����b�g�s�̏I���ʒu

			// �I��͈͂������s�̏ꍇ, �I��͈͂̋�`���ǂ��ݒ肷��ΓK�؂Ȃ̂�������Ȃ�.
			// �Ƃ肠�����I��͈͂̓L�����b�g������s�Ɍ��肷��.
			t->get_text_caret(max(min(start, end), row_start), car_row, false, sel_start);
			t->get_text_caret(min(max(start, end), row_end), car_row, false, sel_end);
			sel_rect.X = win_rect.X + panel.X + sel_start.x / m_main_scale;
			sel_rect.Y = win_rect.Y + panel.Y + sel_end.y / m_main_scale;
			sel_rect.Width = (sel_end.x - sel_start.x) / m_main_scale;
			sel_rect.Height = t->m_font_size / m_main_scale;
			const auto disp_scale = DisplayInformation::GetForCurrentView().RawPixelsPerViewPixel();
			con_rect.X = static_cast<FLOAT>(con_rect.X * disp_scale);
			con_rect.Y = static_cast<FLOAT>(con_rect.Y * disp_scale);
			con_rect.Width = static_cast<FLOAT>(con_rect.Width * disp_scale);
			con_rect.Height = static_cast<FLOAT>(con_rect.Height * disp_scale);
			sel_rect.X = static_cast<FLOAT>(sel_rect.X * disp_scale);
			sel_rect.Y = static_cast<FLOAT>(sel_rect.Y * disp_scale);
			sel_rect.Width = static_cast<FLOAT>(sel_rect.Width * disp_scale);
			sel_rect.Height = static_cast<FLOAT>(sel_rect.Height * disp_scale);
			req.LayoutBounds().ControlBounds(con_rect);
			req.LayoutBounds().TextBounds(sel_rect);
			});
		// ���͕ϊ����J�n���ꂽ�Ƃ��Ăяo�����.
		// ���͕ϊ��t���O�𗧂�, �ϊ��J�n���̕�����̑I��͈͂�ۑ�����.
		// �I��͈͂�ۑ�����̂�, �ϊ��I�����ĕ������}������Ƃ��K�v�ƂȂ邽��.
		// ���͕ϊ��t���O�������ĂȂ��Ƃ��͂��̎��_�̑I��͈͂�u������,
		// ���͕ϊ��t���O�������Ă���Ȃ炠�炩���ߕۑ������I��͈͂Œu��������.
		m_core_text.CompositionStarted([this](auto const&, auto const& args) {
#ifdef _DEBUG
			winrt::hstring debug_text{ status_bar_debug().Text().size() > 32 ? status_bar_debug().Text().data() + status_bar_debug().Text().size() - 16 : status_bar_debug().Text().data() };
			status_bar_debug().Text(debug_text + L" CompositionStarted");
#endif
			m_core_text_comp = true;
			m_core_text_comp_start = m_main_sheet.m_core_text_range.m_start;
			//m_core_text_end = m_main_sheet.m_core_text_range.m_end;
			//m_core_text_trail = m_main_sheet.m_core_text_range.m_trail;
			undo_push_null();

			m_core_text.NotifyLayoutChanged();
			});
		// �ϊ��I�� (���f) �̂Ƃ��Ăяo�����.
		// ��ރL�[�Ȃǂŕϊ����̕����񂪋�ɂ��ꂽ�ꍇ.
		// ���s�L�[�����⊿���ϊ��L�[, �G�X�P�[�v�L�[�Ȃǂŕϊ����I���ꍇ.
		// LayoutRequested �Őݒ肵���R���e�L�X�g��`�ȊO�Ń}�E�X�{�^�����g���� (�����̂��Ɨ�������ɌĂяo�����) �ꍇ.
		// NotifyFocusLeave ���Ăяo���ꂽ�ꍇ.
		m_core_text.CompositionCompleted([this](auto const&, auto const& args) {
#ifdef _DEBUG
			winrt::hstring debug_text{ status_bar_debug().Text().size() > 32 ? status_bar_debug().Text().data() + status_bar_debug().Text().size() - 16 : status_bar_debug().Text().data() };
			status_bar_debug().Text(debug_text + L" CompositionCompleted");
#endif
			// ���͕ϊ��t���O�����낷.
			m_core_text_comp = false;
			});
	}

	// ������̒����𓾂�.
	uint32_t MainPage::core_text_len(void) const noexcept
	{
		if (m_core_text_focused != nullptr) {
			return m_core_text_focused->get_text_len();
		}
		return 0;
	}

	// ������̃L�����b�g�ʒu�𓾂�.
	uint32_t MainPage::core_text_pos(void) const noexcept
	{
		if (m_core_text_focused != nullptr) {
			const auto len = m_core_text_focused->get_text_len();
			const auto end = min(m_main_sheet.m_core_text_range.m_trail ? m_main_sheet.m_core_text_range.m_end + 1 : m_main_sheet.m_core_text_range.m_end, len);
			return end;
		}
		return 0;
	}

	// ������̑I��͈͂̒����𓾂�.
	uint32_t MainPage::core_text_selected_len(void) const noexcept
	{
		if (m_core_text_focused != nullptr) {
			const auto len = m_core_text_focused->get_text_len();
			const auto end = min(m_main_sheet.m_core_text_range.m_trail ? m_main_sheet.m_core_text_range.m_end + 1 : m_main_sheet.m_core_text_range.m_end, len);
			const auto start = min(m_main_sheet.m_core_text_range.m_start, len);
			const auto s = min(start, end);
			const auto e = max(start, end);
			return e - s;
		}
		return 0;
	}

	// ������̑I��͈͂̕�����𓾂�.
	winrt::hstring MainPage::core_text_substr(void) const noexcept
	{
		const auto len = m_core_text_focused->get_text_len();
		const auto end = min(m_main_sheet.m_core_text_range.m_trail ? m_main_sheet.m_core_text_range.m_end + 1 : m_main_sheet.m_core_text_range.m_end, len);
		const auto start = min(m_main_sheet.m_core_text_range.m_start, len);
		const auto s = min(start, end);
		const auto e = max(start, end);
		return winrt::hstring{ m_core_text_focused->m_text + s, e - s };
	}

	// ������̑I��͈͂̕������폜����.
	void MainPage::core_text_delete_selection(void) noexcept
	{
		const auto len = m_core_text_focused->get_text_len();
		const auto end = min(m_main_sheet.m_core_text_range.m_trail ? m_main_sheet.m_core_text_range.m_end + 1 : m_main_sheet.m_core_text_range.m_end, len);
		const auto start = min(m_main_sheet.m_core_text_range.m_start, len);
		// ������̑I��͈͂�����Ȃ炻����폜����.
		if (end != start) {
			undo_push_null();
			m_undo_stack.push_back(new UndoText2(m_core_text_focused, nullptr));
			main_sheet_draw();
		}
	}

	// ������̑I��͈͂ɕ������}������.
	void MainPage::core_text_insert(const wchar_t* ins_text, const uint32_t ins_len) noexcept
	{
		const auto len = m_core_text_focused->get_text_len();
		const auto end = min(m_main_sheet.m_core_text_range.m_trail ? m_main_sheet.m_core_text_range.m_end + 1 : m_main_sheet.m_core_text_range.m_end, len);
		const auto start = min(m_core_text_comp ? m_core_text_comp_start : m_main_sheet.m_core_text_range.m_start, len);
		const auto s = min(start, end);
		const auto e = max(start, end);
		if (s < e || ins_len > 0) {
			// ���͕ϊ����t���O������Ă���Ȃ�, ���ɖ߂��X�^�b�N�ɋ�؂��ς�ŕ������}������.
			if (!m_core_text_comp) {
				undo_push_null();
			}
			// ���͕ϊ����t���O�������Ă���Ȃ�, ���O�̕ϊ����ʂ��̂Ă�, ���炽�ȕϊ����ʂ��X�^�b�N�ɐς݂Ȃ���.
			else {
				Undo* u;
				while (!m_undo_stack.empty() && (u = m_undo_stack.back()) != nullptr) {
					u->exec();
					delete u;
					m_undo_stack.pop_back();
				}
			}
			m_undo_stack.push_back(new UndoText2(m_core_text_focused, ins_text));
			undo_push_text_select(m_core_text_focused, s + ins_len, s + ins_len, false);
			main_sheet_draw();
		}
	}

	template <bool SHIFT_KEY> void MainPage::core_text_up_key(void) noexcept
	{
		if (m_core_text_focused == nullptr) {
			return;
		}
		if constexpr (SHIFT_KEY) {
			if (m_core_text_comp) {
				return;
			}
		}
		const auto len = m_core_text_focused->get_text_len();
		const auto end = min(m_main_sheet.m_core_text_range.m_trail ? m_main_sheet.m_core_text_range.m_end + 1 : m_main_sheet.m_core_text_range.m_end, len);
		const auto start = min(m_main_sheet.m_core_text_range.m_start, len);
		const auto row = m_core_text_focused->get_text_row(m_main_sheet.m_core_text_range.m_end);
		if (end != start && row == 0) {
			if constexpr (SHIFT_KEY) {
				undo_push_text_select(m_core_text_focused, start, m_main_sheet.m_core_text_range.m_end, m_main_sheet.m_core_text_range.m_trail);
				main_sheet_draw();
			}
			else {
				undo_push_text_select(m_core_text_focused, end, m_main_sheet.m_core_text_range.m_end, m_main_sheet.m_core_text_range.m_trail);
				main_sheet_draw();
			}
		}
		else if (row != 0) {
			const auto h = m_core_text_focused->m_dwrite_test_metrics[row].top - m_core_text_focused->m_dwrite_test_metrics[row - 1].top;
			D2D1_POINT_2F car;
			m_core_text_focused->get_text_caret(end, row, m_main_sheet.m_core_text_range.m_trail, car);
			const D2D1_POINT_2F new_car{ car.x, car.y - h };
			bool new_trail;
			const auto new_end = m_core_text_focused->get_text_pos(new_car, new_trail);
			if constexpr (SHIFT_KEY) {
				undo_push_text_select(m_core_text_focused, start, new_end, new_trail);
				main_sheet_draw();
			}
			else {
				const auto new_start = new_trail ? new_end + 1 : new_end;
				undo_push_text_select(m_core_text_focused, new_start, new_end, new_trail);
				main_sheet_draw();
			}
		}
		const auto new_start = m_main_sheet.m_core_text_range.m_start;
		const auto new_end = m_main_sheet.m_core_text_range.m_trail ? m_main_sheet.m_core_text_range.m_end + 1 : m_main_sheet.m_core_text_range.m_end;
		CoreTextRange new_ran{
			static_cast<int32_t>(min(new_start, new_end)), static_cast<int32_t>(max(new_start, new_end))
		};
		m_core_text.NotifySelectionChanged(new_ran);
	}

	template <bool SHIFT_KEY> void MainPage::core_text_right_key(void) noexcept
	{
		if (m_core_text_focused == nullptr) {
			return;
		}
		if constexpr (SHIFT_KEY) {
			if (m_core_text_comp) {
				return;
			}
		}
		const auto len = m_core_text_focused->get_text_len();
		const auto end = min(m_main_sheet.m_core_text_range.m_trail ? m_main_sheet.m_core_text_range.m_end + 1 : m_main_sheet.m_core_text_range.m_end, len);
		const auto start = min(m_main_sheet.m_core_text_range.m_start, len);
		if constexpr (SHIFT_KEY) {
			if (end < len) {
				undo_push_text_select(m_core_text_focused, start, end + 1, false);
				main_sheet_draw();
			}
		}
		else {
			if (end == start && end < len) {
				undo_push_text_select(m_core_text_focused, end + 1, end + 1, false);
				main_sheet_draw();
			}
			else if (end != start) {
				const auto new_end = max(start, end);
				undo_push_text_select(m_core_text_focused, new_end, new_end, false);
				main_sheet_draw();
			}
		}
		const auto new_start = m_main_sheet.m_core_text_range.m_start;
		const auto new_end = m_main_sheet.m_core_text_range.m_trail ? m_main_sheet.m_core_text_range.m_end + 1 : m_main_sheet.m_core_text_range.m_end;
		CoreTextRange new_ran{
			static_cast<int32_t>(min(new_start, new_end)), static_cast<int32_t>(max(new_start, new_end))
		};
		m_core_text.NotifySelectionChanged(new_ran);
	}

	void MainPage::core_text_backspace_key(void) noexcept
	{
		if (m_core_text_focused == nullptr) {
			return;
		}
		// �I��͈͂��Ȃ��L�����b�g�ʒu�������łȂ��Ȃ�
		const auto len = m_core_text_focused->get_text_len();
		const auto end = min(m_main_sheet.m_core_text_range.m_trail ? m_main_sheet.m_core_text_range.m_end + 1 : m_main_sheet.m_core_text_range.m_end, len);
		const auto start = min(m_main_sheet.m_core_text_range.m_start, len);
		if (end == start && end > 0) {
			undo_push_null();
			undo_push_text_select(m_core_text_focused, end - 1, end, false);
			m_undo_stack.push_back(new UndoText2(m_core_text_focused, nullptr));
			main_sheet_draw();
		}
		// �I��͈͂�����Ȃ�
		else if (end != start) {
			undo_push_null();
			m_undo_stack.push_back(new UndoText2(m_core_text_focused, nullptr));
			main_sheet_draw();
		}
		CoreTextRange modified_ran{
			static_cast<int32_t>(min(start, end)), static_cast<int32_t>(max(start, end))
		};
		const auto new_start = m_main_sheet.m_core_text_range.m_start;
		const auto new_end = m_main_sheet.m_core_text_range.m_trail ? m_main_sheet.m_core_text_range.m_end + 1 : m_main_sheet.m_core_text_range.m_end;
		CoreTextRange new_ran{
			static_cast<int32_t>(min(new_start, new_end)), static_cast<int32_t>(max(new_start, new_end))
		};
		m_core_text.NotifyTextChanged(modified_ran, 0, new_ran);
	}

	template <bool SHIFT_KEY> void MainPage::core_text_left_key(void) noexcept
	{
		if (m_core_text_focused == nullptr) {
			return;
		}
		if constexpr (SHIFT_KEY) {
			if (m_core_text_comp) {
				return;
			}
		}
		const auto len = m_core_text_focused->get_text_len();
		const auto end = min(m_main_sheet.m_core_text_range.m_trail ? m_main_sheet.m_core_text_range.m_end + 1 : m_main_sheet.m_core_text_range.m_end, len);
		const auto start = min(m_main_sheet.m_core_text_range.m_start, len);
		if constexpr (SHIFT_KEY) {
			if (end > 0) {
				undo_push_text_select(m_core_text_focused, start, end - 1, false);
				main_sheet_draw();
			}
		}
		else {
			if (end == start && end > 0) {
				undo_push_text_select(m_core_text_focused, end - 1, end - 1, false);
				main_sheet_draw();
			}
			else if (end != start) {
				const auto new_end = min(start, end);
				undo_push_text_select(m_core_text_focused, new_end, new_end, false);
				main_sheet_draw();
			}
		}
		const auto new_start = m_main_sheet.m_core_text_range.m_start;
		const auto new_end = m_main_sheet.m_core_text_range.m_trail ? m_main_sheet.m_core_text_range.m_end + 1 : m_main_sheet.m_core_text_range.m_end;
		CoreTextRange new_ran{
			static_cast<int32_t>(min(new_start, new_end)), static_cast<int32_t>(max(new_start, new_end))
		};
		m_core_text.NotifySelectionChanged(new_ran);
	}

	void MainPage::core_text_enter_key(void) noexcept
	{
		if (m_core_text_focused == nullptr) {
			return;
		}
		const auto len = m_core_text_focused->get_text_len();
		const auto end = min(m_main_sheet.m_core_text_range.m_trail ? m_main_sheet.m_core_text_range.m_end + 1 : m_main_sheet.m_core_text_range.m_end, len);
		const auto start = min(m_main_sheet.m_core_text_range.m_start, len);
		const auto s = min(start, end);
		undo_push_null();
		// ���s��}������.
		m_undo_stack.push_back(new UndoText2(m_core_text_focused, L"\r"));
		undo_push_text_select(m_core_text_focused, s + 1, s + 1, false);
		main_sheet_draw();

		CoreTextRange modified_ran{
			static_cast<int32_t>(min(start, end)), static_cast<int32_t>(max(start, end))
		};
		const auto new_start = m_main_sheet.m_core_text_range.m_start;
		const auto new_end = m_main_sheet.m_core_text_range.m_trail ? m_main_sheet.m_core_text_range.m_end + 1 : m_main_sheet.m_core_text_range.m_end;
		CoreTextRange new_ran{
			static_cast<int32_t>(min(new_start, new_end)), static_cast<int32_t>(max(new_start, new_end))
		};
		m_core_text.NotifyTextChanged(modified_ran, 1, new_ran);
	}

	// 1 �����폜.
	template <bool SHIFT_KEY> void MainPage::core_text_delete_key(void) noexcept
	{
		// �V�t�g�L�[�����łȂ��I��͈͂��Ȃ��L�����b�g�ʒu�������łȂ��Ȃ�
		// �L�����b�g�ʒu�̕������폜����.
		const auto len = m_core_text_focused->get_text_len();
		const auto end = min(m_main_sheet.m_core_text_range.m_trail ? m_main_sheet.m_core_text_range.m_end + 1 : m_main_sheet.m_core_text_range.m_end, len);
		const auto start = min(m_main_sheet.m_core_text_range.m_start, len);
		if (end == start && end < len) {
			if constexpr (!SHIFT_KEY) {
				undo_push_null();
				undo_push_text_select(m_core_text_focused, end, end + 1, false);
				m_undo_stack.push_back(new UndoText2(m_core_text_focused, nullptr));
				main_sheet_draw();
			}
		}
		// �I��͈͂�����Ȃ�I��͈͂̕�������폜����.
		else if (end != start) {
			undo_push_null();
			m_undo_stack.push_back(new UndoText2(m_core_text_focused, nullptr));
			main_sheet_draw();
		}
		CoreTextRange modified_ran{
			static_cast<const int32_t>(start), static_cast<const int32_t>(end)
		};
		CoreTextRange new_ran{
			static_cast<int32_t>(m_main_sheet.m_core_text_range.m_start),
				static_cast<int32_t>(m_main_sheet.m_core_text_range.m_trail ? m_main_sheet.m_core_text_range.m_end + 1 : m_main_sheet.m_core_text_range.m_end)
		};
		m_core_text.NotifyTextChanged(modified_ran, 0, new_ran);
	}

	template <bool SHIFT_KEY> void MainPage::core_text_down_key(void) noexcept
	{
		if (m_core_text_focused == nullptr) {
			return;
		}
		if constexpr (SHIFT_KEY) {
			if (m_core_text_comp) {
				return;
			}
		}
		const auto len = m_core_text_focused->get_text_len();
		const auto end = min(m_main_sheet.m_core_text_range.m_trail ? m_main_sheet.m_core_text_range.m_end + 1 : m_main_sheet.m_core_text_range.m_end, len);
		const auto start = min(m_main_sheet.m_core_text_range.m_start, len);
		const auto row = m_core_text_focused->get_text_row(m_main_sheet.m_core_text_range.m_end);	// �L�����b�g������s
		const auto last = m_core_text_focused->m_dwrite_test_cnt - 1;	// �ŏI�s
		if (end != start && row == last) {
			if constexpr (SHIFT_KEY) {
				undo_push_text_select(m_core_text_focused, start, m_main_sheet.m_core_text_range.m_end, m_main_sheet.m_core_text_range.m_trail);
				main_sheet_draw();
			}
			else {
				undo_push_text_select(m_core_text_focused, end, m_main_sheet.m_core_text_range.m_end, m_main_sheet.m_core_text_range.m_trail);
				main_sheet_draw();
			}
		}
		else if (row != last) {
			const auto h = m_core_text_focused->m_dwrite_test_metrics[row + 1].top - m_core_text_focused->m_dwrite_test_metrics[row].top;
			D2D1_POINT_2F car;
			m_core_text_focused->get_text_caret(end, row, m_main_sheet.m_core_text_range.m_trail, car);
			const D2D1_POINT_2F new_car{ car.x, car.y + h };
			bool new_trail;
			const auto new_end = m_core_text_focused->get_text_pos(new_car, new_trail);
			if constexpr (SHIFT_KEY) {
				undo_push_text_select(m_core_text_focused, start, new_end, new_trail);
				main_sheet_draw();
			}
			else {
				const auto new_start = new_trail ? new_end + 1 : new_end;
				undo_push_text_select(m_core_text_focused, new_start, new_end, new_trail);
				main_sheet_draw();
			}
		}
		const auto new_start = m_main_sheet.m_core_text_range.m_start;
		const auto new_end = m_main_sheet.m_core_text_range.m_trail ? m_main_sheet.m_core_text_range.m_end + 1 : m_main_sheet.m_core_text_range.m_end;
		CoreTextRange new_ran{
			static_cast<int32_t>(min(new_start, new_end)), static_cast<int32_t>(max(new_start, new_end))
		};
		m_core_text.NotifySelectionChanged(new_ran);
	}
	template void MainPage::core_text_delete_key<true>(void) noexcept;
	template void MainPage::core_text_delete_key<false>(void) noexcept;
	template void MainPage::core_text_down_key<true>(void) noexcept;
	template void MainPage::core_text_down_key<false>(void) noexcept;
	template void MainPage::core_text_left_key<true>(void) noexcept;
	template void MainPage::core_text_left_key<false>(void) noexcept;
	template void MainPage::core_text_right_key<true>(void) noexcept;
	template void MainPage::core_text_right_key<false>(void) noexcept;
	template void MainPage::core_text_up_key<true>(void) noexcept;
	template void MainPage::core_text_up_key<false>(void) noexcept;
}