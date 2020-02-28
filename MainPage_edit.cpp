//-------------------------------
// MainPage_edit.cpp
// ������̕ҏW
//-------------------------------
#include "pch.h"
#include "MainPage.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	// �}�`�����������ҏW����.
	// s	������}�`
	void MainPage::edit_text_of_shape(ShapeText* s)
	{
		static winrt::event_token primary_token;
		static winrt::event_token closed_token;

		tx_edit().Text(s->m_text == nullptr ? L"" : s->m_text);
		tx_edit().SelectAll();
		primary_token = cd_edit_text().PrimaryButtonClick(
			[this, s](auto, auto)
			{
				auto text = wchar_cpy(tx_edit().Text().c_str());
				undo_push_set<UNDO_OP::TEXT>(s, text);
				if (ck_ignore_blank().IsChecked().GetBoolean()) {
					s->delete_bottom_blank();
				}
				undo_push_null();
				enable_undo_menu();
				enable_edit_menu();
			}
		);
		closed_token = cd_edit_text().Closed(
			[this](auto, auto)
			{
				cd_edit_text().PrimaryButtonClick(primary_token);
				cd_edit_text().Closed(closed_token);
				draw_page();
			}
		);
		auto _{ cd_edit_text().ShowAsync() };
	}

	// �ҏW���j���[�́u������̕ҏW�v���I�����ꂽ.
	void MainPage::mfi_edit_text_click(IInspectable const& /*sender*/, RoutedEventArgs const& /*args*/)
	{
		ShapeText* s = nullptr;

		if (m_press_shape_prev != nullptr && typeid(*m_press_shape_prev) == typeid(ShapeText)) {
			//	�O��|�C���^�[�������ꂽ�̂�������}�`�̏ꍇ,
			//	���̐}�`�𓾂�.
			s = static_cast<ShapeText*>(m_press_shape_prev);
		}
		else {
			// �I�����ꂽ�}�`�̂����őO�ʂɂ��镶����}�`�𓾂�.
			for (auto it = m_list_shapes.rbegin(); it != m_list_shapes.rend(); it++) {
				auto t = *it;
				if (t->is_deleted()) {
					continue;
				}
				if (t->is_selected() == false) {
					continue;
				}
				if (typeid(*t) == typeid(ShapeText)) {
					s = static_cast<ShapeText*>(t);
					break;
				}
			}
		}
		if (s != nullptr) {
			edit_text_of_shape(s);
		}

	}

}