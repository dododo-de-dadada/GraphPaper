#pragma once
//------------------------------
// Summary.h
//
// 一覧の項目
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
	using winrt::Windows::ApplicationModel::Resources::ResourceLoader;
	using winrt::Windows::UI::Xaml::ResourceDictionary;

	// 一覧の要素
	struct Summary : SummaryT<Summary>
	{
		// 一覧の要素を作成する.
		// s	表示する図形.
		// r	メインページのリソースディクショナリ.
		// 図形の名前と, パスアイコンの移動と描画のコマンド文字列を設定する.
		Summary(Shape* s, const ResourceDictionary& r) :
			m_shape(s)
		{
			auto const& t_id = typeid(*s);
			if (t_id == typeid(ShapeBezier)) {
				m_icon = unbox_value<winrt::hstring>(r.Lookup(box_value(L"data_bezier")));
				m_name = ResourceLoader::GetForCurrentView().GetString(L"rmfi_drawing_bezier/Text");
			}
			else if (t_id == typeid(ShapeEllipse)) {
				m_icon = unbox_value<winrt::hstring>(r.Lookup(box_value(L"data_elli")));
				m_name = ResourceLoader::GetForCurrentView().GetString(L"rmfi_drawing_ellipse/Text");
			}
			else if (t_id == typeid(ShapeGroup)) {
				m_icon = unbox_value<winrt::hstring>(r.Lookup(box_value(L"data_group")));
				m_name = ResourceLoader::GetForCurrentView().GetString(L"str_grouped");
			}
			else if (t_id == typeid(ShapeLine)) {
				m_icon = unbox_value<winrt::hstring>(r.Lookup(box_value(L"data_line")));
				m_name = ResourceLoader::GetForCurrentView().GetString(L"rmfi_drawing_line/Text");
			}
			else if (t_id == typeid(ShapePolygon)) {
				m_icon = unbox_value<winrt::hstring>(r.Lookup(box_value(L"data_polygon")));
				m_name = ResourceLoader::GetForCurrentView().GetString(L"rmfi_drawing_poly/Text");
			}
			else if (t_id == typeid(ShapeQEllipse)) {
				m_icon = unbox_value<winrt::hstring>(r.Lookup(box_value(L"data_qellipse")));
				m_name = ResourceLoader::GetForCurrentView().GetString(L"rmfi_drawing_qellipse/Text");
			}
			else if (t_id == typeid(ShapeRect)) {
				m_icon = unbox_value<winrt::hstring>(r.Lookup(box_value(L"data_rect")));
				m_name = ResourceLoader::GetForCurrentView().GetString(L"rmfi_drawing_rect/Text");
			}
			else if (t_id == typeid(ShapeRRect)) {
				m_icon = unbox_value<winrt::hstring>(r.Lookup(box_value(L"data_rrect")));
				m_name = ResourceLoader::GetForCurrentView().GetString(L"rmfi_drawing_rrect/Text");
			}
			else if (t_id == typeid(ShapeRuler)) {
				m_icon = unbox_value<winrt::hstring>(r.Lookup(box_value(L"data_ruler")));
				m_name = ResourceLoader::GetForCurrentView().GetString(L"rmfi_drawing_ruler/Text");
			}
			else if (t_id == typeid(ShapeText)) {
				m_icon = unbox_value<winrt::hstring>(r.Lookup(box_value(L"data_text")));
				m_name = ResourceLoader::GetForCurrentView().GetString(L"rmfi_drawing_text/Text");
			}
			else if (t_id == typeid(ShapeImage)) {
				m_icon = unbox_value<winrt::hstring>(r.Lookup(box_value(L"data_image")));
				m_name = ResourceLoader::GetForCurrentView().GetString(L"str_image");
			}
			else if (t_id == typeid(ShapeQEllipse)) {
				m_icon = unbox_value<winrt::hstring>(r.Lookup(box_value(L"data_qellipse")));
				m_name = ResourceLoader::GetForCurrentView().GetString(L"rmfi_drawing_qellipse/Text");
			}
			else {
				m_icon = unbox_value<winrt::hstring>(r.Lookup(box_value(L"data_select")));
				m_name = ResourceLoader::GetForCurrentView().GetString(L"str_unknown");
			}
		}

		Shape* const get_shape(void) const noexcept
		{
			return m_shape;
		}

		// 移動と描画のコマンド文字列をジオメトリに変換する.
		static winrt::Windows::UI::Xaml::Media::Geometry Geom(winrt::hstring const& move_and_draw_command)
		{
			using winrt::Windows::UI::Xaml::Markup::XamlReader;
			using winrt::Windows::UI::Xaml::Controls::PathIcon;

			constexpr auto START_TAG = L"<PathIcon xmlns=\"http://schemas.microsoft.com/winfx/2006/xaml/presentation\" Width=\"24\" Height=\"24\"><PathIcon.Data>";
			constexpr auto END_TAG = L"</PathIcon.Data></PathIcon>";
			winrt::hstring xaml = START_TAG + move_and_draw_command + END_TAG;
			auto icon = unbox_value<PathIcon>(XamlReader::Load(xaml));
			auto geom = icon.Data();
			icon.Data(nullptr);
			icon = nullptr;
			return geom;
		}

		// ジオメトリを得る.
		winrt::Windows::UI::Xaml::Media::Geometry Geom()
		{
			return Geom(m_icon);
		}

		// ジオメトリに格納する.
		void Geom(winrt::Windows::UI::Xaml::Media::Geometry const& /*geo*/)
		{
		}

		// 16 進文字列を得る.
		auto Hexa()
		{
			wchar_t buf[64];
			swprintf_s(buf, L"%p", m_shape);
			wchar_t* t = buf;
			while (*t == L'0') {
				t++;
			}
			return winrt::hstring(t);
		}

		// 16 進文字列に値を格納する.
		void Hexa(winrt::hstring const& val)
		{
			if (Hexa() == val) {
				return;
			}
			m_changed(*this, Windows::UI::Xaml::Data::PropertyChangedEventArgs{ L"Hexa" });
		}

		// 名前を得る.
		auto Name()
		{
			return m_name;
		}

		// 名前に値を格納する.
		void Name(winrt::hstring const& val)
		{
			if (Name() == val) {
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
		Shape* m_shape;	// 図形
		winrt::hstring m_icon;	// パスアイコンのデータ
		winrt::hstring m_name;	// 図形の名前
		winrt::event<Windows::UI::Xaml::Data::PropertyChangedEventHandler> m_changed;
	};
}
