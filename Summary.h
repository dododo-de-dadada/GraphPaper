#pragma once
//------------------------------
// Summary.h
//
// �ꗗ�̍���
//------------------------------
#include "Summary.g.h"
#include "shape.h"
#include <winrt/Windows.ApplicationModel.Resources.h>
#include <winrt/Windows.UI.Xaml.Media.h>
#include <winrt/Windows.UI.Xaml.Controls.h>
#include <MainPage.g.h>
#include <typeinfo>

namespace winrt::GraphPaper::implementation
{
	using winrt::Windows::UI::Xaml::ResourceDictionary;

	// �ꗗ�̗v�f
	struct Summary : SummaryT<Summary>
	{
		// �ꗗ�̗v�f���쐬����.
		// s	�\������}�`.
		// r	���C���y�[�W�̃��\�[�X�f�B�N�V���i��.
		// �}�`�̖��O��, �p�X�A�C�R���̈ړ��ƕ`��̃R�}���h�������ݒ肷��.
		Summary(Shape* s, ResourceDictionary const& r) :
			m_shape(s)
		{
			using winrt::Windows::ApplicationModel::Resources::ResourceLoader;

			auto const& t_id = typeid(*s);
			if (t_id == typeid(ShapeBezi)) {
				m_data = unbox_value<winrt::hstring>(r.Lookup(box_value(L"data_bezi")));
				m_name = ResourceLoader::GetForCurrentView().GetString(L"rmfi_tool_bezi/Text");
			}
			else if (t_id == typeid(ShapeElli)) {
				m_data = unbox_value<winrt::hstring>(r.Lookup(box_value(L"data_elli")));
				m_name = ResourceLoader::GetForCurrentView().GetString(L"rmfi_tool_elli/Text");
			}
			else if (t_id == typeid(ShapeGroup)) {
				m_data = unbox_value<winrt::hstring>(r.Lookup(box_value(L"data_group")));
				m_name = ResourceLoader::GetForCurrentView().GetString(L"str_grouped");
			}
			else if (t_id == typeid(ShapeLine)) {
				m_data = unbox_value<winrt::hstring>(r.Lookup(box_value(L"data_line")));
				m_name = ResourceLoader::GetForCurrentView().GetString(L"rmfi_tool_line/Text");
			}
			else if (t_id == typeid(ShapeQuad)) {
				m_data = unbox_value<winrt::hstring>(r.Lookup(box_value(L"data_quad")));
				m_name = ResourceLoader::GetForCurrentView().GetString(L"rmfi_tool_quad/Text");
			}
			else if (t_id == typeid(ShapeRect)) {
				m_data = unbox_value<winrt::hstring>(r.Lookup(box_value(L"data_rect")));
				m_name = ResourceLoader::GetForCurrentView().GetString(L"rmfi_tool_rect/Text");
			}
			else if (t_id == typeid(ShapeRRect)) {
				m_data = unbox_value<winrt::hstring>(r.Lookup(box_value(L"data_rrect")));
				m_name = ResourceLoader::GetForCurrentView().GetString(L"rmfi_tool_rrect/Text");
			}
			else if (t_id == typeid(ShapeScale)) {
				m_data = unbox_value<winrt::hstring>(r.Lookup(box_value(L"data_scale")));
				m_name = ResourceLoader::GetForCurrentView().GetString(L"rmfi_tool_scale/Text");
			}
			else if (t_id == typeid(ShapeText)) {
				m_data = unbox_value<winrt::hstring>(r.Lookup(box_value(L"data_text")));
				m_name = ResourceLoader::GetForCurrentView().GetString(L"rmfi_tool_text/Text");
			}
			else {
				m_data = unbox_value<winrt::hstring>(r.Lookup(box_value(L"data_select")));
				m_name = ResourceLoader::GetForCurrentView().GetString(L"str_unknown");
			}
		}

		Shape* get_shape(void) const noexcept
		{
			return m_shape;
		}

		// �ړ��ƕ`��̃R�}���h��������W�I���g���ɕϊ�����.
		static winrt::Windows::UI::Xaml::Media::Geometry Data(winrt::hstring const& move_and_draw_command)
		{
			using winrt::Windows::UI::Xaml::Markup::XamlReader;
			using winrt::Windows::UI::Xaml::Controls::PathIcon;

			constexpr auto PATH_ICON = L"<PathIcon xmlns=\"http://schemas.microsoft.com/winfx/2006/xaml/presentation\"><PathIcon.Data>";
			constexpr auto END_TAG = L"</PathIcon.Data></PathIcon>";
			winrt::hstring xaml = PATH_ICON + move_and_draw_command + END_TAG;
			auto icon = unbox_value<PathIcon>(XamlReader::Load(xaml));
			auto geom = icon.Data();
			icon.Data(nullptr);
			icon = nullptr;
			return geom;
		}

		// �W�I���g���𓾂�.
		winrt::Windows::UI::Xaml::Media::Geometry Data()
		{
			return Data(m_data);
		}

		void Data(winrt::Windows::UI::Xaml::Media::Geometry const& /*geo*/)
		{
		}

		// 16 �i������𓾂�.
		auto Hex()
		{
			wchar_t buf[64];
			swprintf_s(buf, L"%p", m_shape);
			wchar_t* t = buf;
			while (*t == L'0') {
				t++;
			}
			return winrt::hstring(t);
		}

		// 16 �i������ɒl���i�[����.
		void Hex(winrt::hstring const& value)
		{
			if (Hex() == value) {
				return;
			}
			m_changed(*this, Windows::UI::Xaml::Data::PropertyChangedEventArgs{ L"Hex" });
		}

		// ���O�𓾂�.
		auto Name()
		{
			return m_name;
		}

		// ���O�ɒl���i�[����.
		void Name(winrt::hstring const& value)
		{
			if (Name() == value) {
				return;
			}
			m_changed(*this, Windows::UI::Xaml::Data::PropertyChangedEventArgs{ L"Name" });
		}

		winrt::event_token PropertyChanged(Windows::UI::Xaml::Data::PropertyChangedEventHandler const& handler)
		{
			return m_changed.add(handler);
		}

		void PropertyChanged(winrt::event_token const& token) noexcept
		{
			m_changed.remove(token);
		}

	private:
		Shape* m_shape;	// �}�`
		winrt::hstring m_data;	// �ړ��ƕ`��̃R�}���h������
		winrt::hstring m_name;	// �}�`�̖��O
		winrt::event<Windows::UI::Xaml::Data::PropertyChangedEventHandler> m_changed;
	};
}
