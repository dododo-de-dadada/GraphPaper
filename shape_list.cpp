//------------------------------
// Shape_list.cpp
// 図形リスト
//------------------------------
#include "pch.h"
#include "shape.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	// 図形をデータリーダーから読み込む.
	static Shape* s_list_read_shape(DataReader const& dt_reader);
	// 次の図形を得る.
	template <typename T> static Shape* s_list_next(T const& it_begin, T const& it_end, const Shape* s) noexcept;
	// 次の図形とその距離を得る.
	template <typename T> static Shape* s_list_next(T const& it_begin, T const& it_end, uint32_t& distance) noexcept;

	// データリーダーから図形を作成する.
	static Shape* s_list_read_shape(DataReader const& dt_reader)
	{
		if (dt_reader.UnconsumedBufferLength() < sizeof(uint32_t)) {
			return nullptr;
		}
		Shape* s = nullptr;
		auto s_type = dt_reader.ReadUInt32();
		if (s_type == SHAPE_NULL) {
		}
		else if (s_type == SHAPE_BEZI) {
			s = new ShapeBezi(dt_reader);
		}
		else if (s_type == SHAPE_ELLI) {
			s = new ShapeElli(dt_reader);
		}
		else if (s_type == SHAPE_LINE) {
			s = new ShapeLine(dt_reader);
		}
		else if (s_type == SHAPE_QUAD) {
			s = new ShapeQuad(dt_reader);
		}
		else if (s_type == SHAPE_RECT) {
			s = new ShapeRect(dt_reader);
		}
		else if (s_type == SHAPE_RRECT) {
			s = new ShapeRRect(dt_reader);
		}
		else if (s_type == SHAPE_TEXT) {
			s = new ShapeText(dt_reader);
		}
		else if (s_type == SHAPE_GROUP) {
			s = new ShapeGroup(dt_reader);
		}
		else if (s_type == SHAPE_SCALE) {
			s = new ShapeScale(dt_reader);
		}
		else {
			s = reinterpret_cast<Shape*>(-1);
		}
#if defined(_DEBUG)
		if (s != nullptr && s != reinterpret_cast<Shape*>(-1)) {
			debug_leak_cnt++;
		}
#endif
		return s;
	}

	// 次の図形をリストから得る.
	// it_begin	リストの始端
	// it_end	リストの終端
	// s	直前の図形
	// 戻り値	次の図形, ヌルならば次の図形はない.
	template <typename T>
	static Shape* s_list_next(T const& it_begin, T const& it_end, const Shape* s) noexcept
	{
		auto it{ std::find(it_begin, it_end, s) };
		if (it != it_end) {
			uint32_t _;
			return s_list_next(++it, it_end, _);
		}
		return static_cast<Shape*>(nullptr);
	}
	template Shape* winrt::GraphPaper::implementation::s_list_next(S_LIST_T::iterator const& it_begin, S_LIST_T::iterator const& it_end, const Shape* s) noexcept;
	template Shape* winrt::GraphPaper::implementation::s_list_next(S_LIST_T::reverse_iterator const& it_rbegin, S_LIST_T::reverse_iterator const& it_rend, const Shape* s) noexcept;

	// 次の図形とその長さをリストから得る.
	// it_begin	リストの始端
	// it_end	リストの終端
	// distance	次の図形との長さ
	// 戻り値	次の図形, ヌルならば次の図形はない.
	template <typename T>
	static Shape* s_list_next(T const& it_begin, T const& it_end, uint32_t& distance) noexcept
	{
		uint32_t i = 0;
		for (auto it = it_begin; it != it_end; it++) {
			auto s = *it;
			if (s->is_deleted() == false) {
				distance = i;
				return s;
			}
			i++;
		}
		return static_cast<Shape*>(nullptr);
	}
	template Shape* winrt::GraphPaper::implementation::s_list_next(S_LIST_T::iterator const& it_begin, S_LIST_T::iterator const& it_end, uint32_t& distance) noexcept;
	template Shape* winrt::GraphPaper::implementation::s_list_next(S_LIST_T::reverse_iterator const& it_rbegin, S_LIST_T::reverse_iterator const& it_rend, uint32_t& distance) noexcept;

	// 最後の図形をリストから得る.
	// s_list	図形リスト
	// 戻り値	得られた図形
	Shape* s_list_back(S_LIST_T const& s_list) noexcept
	{
		uint32_t _;
		return s_list_next(s_list.rbegin(), s_list.rend(), _);
	}

	// 図形リストを消去し, 含まれる図形を破棄する.
	// s_list	図形リスト
	void s_list_clear(S_LIST_T& s_list) noexcept
	{
		for (auto s : s_list) {
			delete s;
#if defined(_DEBUG)
			debug_leak_cnt--;
#endif
		}
		s_list.clear();
	}

	// 図形のリスト上での順番を得る.
	// s_list	図形リスト
	// s	図形
	// 戻り値	図形の順番
	// 消去フラグが立っている図形は順番に入れない.
	uint32_t s_list_distance(S_LIST_T const& s_list, const Shape* s) noexcept
	{
		uint32_t i = 0;
		for (auto t : s_list) {
			if (t->is_deleted()) {
				continue;
			}
			else if (t == s) {
				break;
			}
			i++;
		}
		return i;
	}

	// 最初の図形をリストから得る.
	// s_list	図形リスト
	// 戻り値	最初の図形
	// 消去フラグが立っている図形は勘定されない.
	Shape* s_list_front(S_LIST_T const& s_list) noexcept
	{
		uint32_t _;
		return s_list_next(s_list.begin(), s_list.end(), _);
	}

	// 図形全体の領域をリストから得る.
	// s_list	図形リスト
	// p_size	ページの寸法
	// b_min	領域の左上位置
	// b_max	領域の右下位置
	void s_list_bound(S_LIST_T const& s_list, const D2D1_SIZE_F p_size, D2D1_POINT_2F& b_min, D2D1_POINT_2F& b_max) noexcept
	{
		b_min = { 0.0F, 0.0F };	// 左上位置
		b_max = { p_size.width, p_size.height };	// 右下位置
		for (auto s : s_list) {
			if (s->is_deleted()) {
				continue;
			}
			s->get_bound(b_min, b_max);
		}
	}

	// 図形リストの中から, 位置を含む図形とその部位を調べる.
	// s_list	図形リスト
	// t_pos	調べる位置
	// a_len	部位の大きさ
	// s	位置を含む図形
	// 戻り値	位置を含む図形の部位
	ANCH_WHICH s_list_hit_test(S_LIST_T const& s_list, const D2D1_POINT_2F t_pos, const double a_len, Shape*& s) noexcept
	{
		// 前面にある図形が先にヒットするように, リストを逆順に検索する.
		for (auto it = s_list.rbegin(); it != s_list.rend(); it++) {
			auto t = *it;
			if (t->is_deleted()) {
				continue;
			}
			const auto a = t->hit_test(t_pos, a_len);
			if (a != ANCH_WHICH::ANCH_OUTSIDE) {
				s = t;
				return a;
			}
		}
		return ANCH_WHICH::ANCH_OUTSIDE;
	}

	// 図形をリストに挿入する.
	// s_list	図形リスト
	// s	挿入する図形
	// s_pos	挿入する位置にある図形
	void s_list_insert(S_LIST_T& s_list, Shape* s, const Shape* s_pos) noexcept
	{
		s_list.insert(std::find(s_list.begin(), s_list.end(), s_pos), s);
	}

	// リストの中の図形の順番を得る.
	// S	探索する型
	// T	得られた型
	// s_list	図形リスト
	// s	探索する値
	// t	得られた値
	template <typename S, typename T>
	bool s_list_match(S_LIST_T const& s_list, S s, T& t)
	{
		// 一致フラグ
		bool match = false;
		// 探索する順に図形を数える計数.
		uint32_t j = 0;
		// スタックカウンタ.
		uint32_t k = 0;
		// リストを深さ優先で探索するためのスタック
		std::list<S_LIST_T::const_iterator> stack;
		// 図形リストの開始と終端をスタックにプッシュする.
		stack.push_back(s_list.begin());
		stack.push_back(s_list.end());
		k++;
		while (k-- != 0) {
			// スタックカウンタが 0 でないかぎり以下を繰り返す.
			// 反復子と終端をスタックからポップする.
			auto it_end = stack.back();
			stack.pop_back();
			auto it = stack.back();
			stack.pop_back();
			while (it != it_end) {
				// 反復子が終端でないなら, 以下を繰り返す.
				// リストに含まれる図形を反復子から得る.
				// 反復子を次に進める.
				auto r = *it++;
				if constexpr (std::is_same<S, Shape*>::value) {
					// 引数 s が図形の場合,
					// リストに含まれる図形と引数 s が一致したなら,
					// 引数 t に添え字を格納し, スタックカウンタを 0 にする.
					// 反復子の繰り返しを中断する.
					match = (r == s);
					if (match) {
						t = j;
						k = 0;
						break;
					}
				}
				if constexpr (std::is_same<S, uint32_t>::value) {
					// 引数 s が添え字の場合,
					// 係数と引数 s が一致したなら,
					// 引数 t に図形を格納し, スタックカウンタを 0 にする.
					// 反復子の繰り返しを中断する.
					match = (j == s);
					if (match) {
						t = r;
						k = 0;
						break;
					}
				}
				j++;
				if (typeid(*r) == typeid(ShapeGroup)) {
					// リストに含まれる図形がグループ図形なら,
					// 次の図形を指している反復子と終端をスタックにプッシュする.
					// グループに含まれるリストの開始と終端をスタックにプッシュする.
					// 反復子の繰り返しを中断する.
					stack.push_back(it);
					stack.push_back(it_end);
					k++;
					auto& grp_list = static_cast<ShapeGroup*>(r)->m_list_grouped;
					stack.push_back(grp_list.begin());
					stack.push_back(grp_list.end());
					k++;
					break;
				}
			}
		}
		stack.clear();
		return match;
	}
	template bool winrt::GraphPaper::implementation::s_list_match<Shape*, uint32_t>(S_LIST_T const& s_list, Shape* s, uint32_t& t);
	template bool winrt::GraphPaper::implementation::s_list_match<uint32_t, Shape*>(S_LIST_T const& s_list, uint32_t s, Shape*& t);

	// 選択フラグの立つすべての図形を差分だけ移動する.
	// s_list	図形リスト
	// d_pos	移動する差分
	void s_list_move(S_LIST_T const& s_list, const D2D1_POINT_2F d_pos) noexcept
	{
		for (auto s : s_list) {
			if (s->is_deleted()) {
				continue;
			}
			if (s->is_selected() == false) {
				continue;
			}
			s->move(d_pos);
		}
	}

	// 次の図形をリストから得る.
	// s_list	図形リスト
	// s	直前の図形
	// 戻り値	得られた図形
	Shape* s_list_next(S_LIST_T const& s_list, const Shape* s) noexcept
	{
		return s_list_next(s_list.begin(), s_list.end(), s);
	}

	// 前の図形をリストから得る.
	Shape* s_list_prev(S_LIST_T const& s_list, const Shape* s) noexcept
	{
		return s_list_next(s_list.rbegin(), s_list.rend(), s);
	}

	// 図形リストをデータリーダーから読み込む.
	bool s_list_read(S_LIST_T& s_list, DataReader const& dt_reader)
	{
		Shape* s;
		while ((s = s_list_read_shape(dt_reader)) != static_cast<Shape*>(nullptr)) {
			if (s == reinterpret_cast<Shape*>(-1)) {
				return false;
			}
			s_list.push_back(s);
		}
		return true;
	}

	// 図形をリストから削除し, 削除した図形の次の図形を得る.
	// s_list	図形リスト.
	// s	削除する図形.
	// s_pos	図形を削除した位置.
	// 図形を削除した位置とは, 削除された図形の次の位置にあった図形へのポインター.
	// 削除された図形がリスト末尾にあったときはヌルポインターとなる.
	Shape* s_list_remove(S_LIST_T& s_list, const Shape* s) noexcept
	{
		auto it{ std::find(s_list.begin(), s_list.end(), s) };
		if (it != s_list.end()) {
			(*it)->set_delete(true);
			auto it_next = std::next(it, 1);
			if (it_next != s_list.end()) {
				return *it_next;
			}
		}
		return static_cast<Shape*>(nullptr);
	}

	// 選択された図形のリストを得る.
	// S	図形の型. Shape ならすべての種類, ShapeGroup ならグループ図形のみ.
	// s_list	図形リスト
	// t_list	得られたリスト
	template <typename S>
	void s_list_selected(S_LIST_T const& s_list, S_LIST_T& t_list) noexcept
	{
		for (auto s : s_list) {
			if (s->is_deleted()) {
				continue;
			}
			if (s->is_selected() == false) {
				continue;
			}
			if constexpr (std::is_same<S, ShapeGroup>::value) {
				if (typeid(*s) != typeid(S)) {
					continue;
				}
			}
			t_list.push_back(s);
		}
	}
	template void s_list_selected<Shape>(const S_LIST_T& s_list, S_LIST_T& t_list) noexcept;
	template void s_list_selected<ShapeGroup>(const S_LIST_T& s_list, S_LIST_T& t_list) noexcept;

	// 図形リストをデータライターに書き込む.
	// REDUCE	消去フラグが立っている図形を除く.
	// s_list	書き込む図形リスト
	// dt_writer	データライター
	template<bool REDUCE>
	void s_list_write(S_LIST_T const& s_list, DataWriter const& dt_writer)
	{
		for (const auto s : s_list) {
			if constexpr (REDUCE) {
				if (s->is_deleted()) {
					continue;
				}
			}
			// 図形の種類を得る.
			auto const& s_type = typeid(*s);
			uint32_t s_uint;
			if (s_type == typeid(ShapeBezi)) {
				s_uint = SHAPE_BEZI;
			}
			else if (s_type == typeid(ShapeElli)) {
				s_uint = SHAPE_ELLI;
			}
			else if (s_type == typeid(ShapeGroup)) {
				s_uint = SHAPE_GROUP;
			}
			else if (s_type == typeid(ShapeLine)) {
				s_uint = SHAPE_LINE;
			}
			else if (s_type == typeid(ShapeQuad)) {
				s_uint = SHAPE_QUAD;
			}
			else if (s_type == typeid(ShapeRect)) {
				s_uint = SHAPE_RECT;
			}
			else if (s_type == typeid(ShapeRRect)) {
				s_uint = SHAPE_RRECT;
			}
			else if (s_type == typeid(ShapeScale)) {
				s_uint = SHAPE_SCALE;
			}
			else if (s_type == typeid(ShapeText)) {
				s_uint = SHAPE_TEXT;
			}
			else {
				continue;
			}
			// 図形の種類と図形を書き込む.
			dt_writer.WriteUInt32(s_uint);
			s->write(dt_writer);
		}
		// 終端を書き込む.
		dt_writer.WriteInt32(static_cast<int32_t>(SHAPE_NULL));
	}
	constexpr auto REDUCE = true;
	template void s_list_write<!REDUCE>(S_LIST_T const& s_list, DataWriter const& dt_writer);
	template void s_list_write<REDUCE>(S_LIST_T const& s_list, DataWriter const& dt_writer);

	// 図形リストから文字列を得る.
	winrt::hstring s_list_text(S_LIST_T const& s_list) noexcept
	{
		winrt::hstring text;
		for (auto s : s_list) {
			if (s->is_deleted()) {
				continue;
			}
			if (s->is_selected() == false) {
				continue;
			}
			wchar_t* w;
			if (s->get_text(w) == false) {
				continue;
			}
			if (text.empty()) {
				text = w;
			}
			else {
				text = text + L"\n" + w;
			}
		}
		return text;
	}
}