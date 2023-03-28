//------------------------------
// shape_slist.cpp
// 図形リスト
//------------------------------
#include "pch.h"
#include "shape.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	// 図形の種類
	// ファイルへの書き込みで使用する.
	enum SHAPE_T : uint32_t {
		SHAPE_NULL,	// ヌル
		SHAPE_ARC,	// 円弧
		SHAPE_BEZIER,	// 曲線
		SHAPE_ELLIPSE,	// だ円
		SHAPE_GROUP,	// グループ
		SHAPE_IMAGE,	// 画像
		SHAPE_LINE,	// 線分
		SHAPE_POLY,	// 多角形
		SHAPE_RECT,	// 方形
		SHAPE_RRECT,	// 角丸方形
		SHAPE_RULER,	// 定規
		SHAPE_TEXT,	// 文字列
	};

	// データリーダーから図形を読み込む.
	static Shape* slist_read_shape(DataReader const& dt_reader);
	// リスト中の図形のその次の図形を得る.
	template <typename T>
	static Shape* slist_next(T const& it_beg, T const& it_end, const Shape* s) noexcept;
	// リスト中の図形のその次の図形と, その間隔をリストから得る.
	template <typename T>
	static Shape* slist_next(T const& it_beg, T const& it_end, uint32_t& distance) noexcept;

	// リストの中の文字列図形に, 利用できない書体があったならばその書体名を得る.
	// slist	図形リスト
	// unavailable_font	利用できない書体名
	// 戻り値	利用できない書体があったなら true
	bool slist_check_avaiable_font(const SHAPE_LIST& slist, wchar_t*& unavailable_font) noexcept
	{
		for (const Shape* s : slist) {
			if (s->is_deleted()) {
				continue;
			}
			wchar_t* fam;	// 書体名
			if (!s->get_font_family(fam)) {
				continue;
			}
			// 空文字列は既定の書体名としてうけつける.
			if (wchar_len(fam) > 0 && !ShapeText::is_available_font(fam)) {
				unavailable_font = fam;
				return false;
			}
		}
		return true;
	}

	// リスト中の最後の図形を得る.
	// slist	図形リスト
	// 戻り値	得られた図形
	Shape* slist_back(SHAPE_LIST const& slist) noexcept
	{
		uint32_t _;
		return slist_next(slist.rbegin(), slist.rend(), _);
	}

	// リスト中の図形とページを囲む領域を得る.
	// slist	図形リスト
	// p_size	ページの大きさ
	// b_lt	領域の左上位置
	// b_rb	領域の右下位置
	void slist_bound_page(SHAPE_LIST const& slist, const D2D1_SIZE_F p_size, D2D1_POINT_2F& b_lt, D2D1_POINT_2F& b_rb) noexcept
	{
		b_lt = { 0.0f, 0.0f };
		b_rb = { p_size.width, p_size.height };
		for (const Shape* s : slist) {
			if (s->is_deleted()) {
				continue;
			}
			s->get_bound(b_lt, b_rb, b_lt, b_rb);
		}
	}

	// リスト中の図形を囲む領域を得る.
	// slist	図形リスト
	// b_lt	領域の左上位置
	// b_rb	領域の右下位置
	void slist_bound_shape(SHAPE_LIST const& slist, D2D1_POINT_2F& b_lt, D2D1_POINT_2F& b_rb) noexcept
	{
		b_lt = { FLT_MAX, FLT_MAX };	// 左上位置
		b_rb = { -FLT_MAX, -FLT_MAX };	// 右下位置
		for (const Shape* s : slist) {
			if (s->is_deleted()) {
				continue;
			}
			s->get_bound(b_lt, b_rb, b_lt, b_rb);
		}
	}

	// リスト中の選択された図形を囲む領域を得る.
	// slist	図形リスト
	// b_lt	領域の左上位置
	// b_rb	領域の右下位置
	// 戻り値	選択された図形があったなら true.
	bool slist_bound_selected(SHAPE_LIST const& slist, D2D1_POINT_2F& b_lt, D2D1_POINT_2F& b_rb) noexcept
	{
		bool flag = false;
		b_lt = D2D1_POINT_2F{ FLT_MAX, FLT_MAX };	// 左上位置
		b_rb = D2D1_POINT_2F{ -FLT_MAX, -FLT_MAX };	// 右下位置
		for (const Shape* s : slist) {
			if (s->is_deleted() || !s->is_selected()) {
				continue;
			}
			s->get_bound(b_lt, b_rb, b_lt, b_rb);
			flag = true;
		}
		return flag;
	}

	// 図形リストを消去し, 含まれる図形を破棄する.
	// slist	図形リスト
	void slist_clear(SHAPE_LIST& slist) noexcept
	{
		for (auto s : slist) {
			delete s;
#if defined(_DEBUG)
			debug_leak_cnt--;
#endif
		}
		slist.clear();
	}

	// 図形を種類別に数える.
	// undeleted_cnt	消去フラグがない図形の数
	// selected_cnt	選択された図形の数
	// selected_group_cnt	選択されたグループ図形の数
	// runlength_cnt	選択された図形のランレングスの数
	// selected_text_cnt	選択された文字列図形の数
	// selected_image_cnt	選択された画像図形の数
	// selected_arc_cnt	選択された円弧図形の数
	// text_cnt	文字列図形の数
	// fore_selected	最前面の図形の選択フラグ
	// back_selected	最背面の図形の選択フラグ
	// prev_selected	ひとつ背面の図形の選択フラグ
	void slist_count(
		const SHAPE_LIST& slist, uint32_t& undeleted_cnt, uint32_t& selected_cnt,
		uint32_t& selected_group_cnt, uint32_t& runlength_cnt, uint32_t& selected_text_cnt,
		uint32_t& text_cnt, uint32_t& selected_image_cnt, uint32_t& selected_arc_cnt,
		uint32_t& selected_poly_open_cnt, uint32_t& selected_poly_close_cnt, bool& fore_selected,
		bool& back_selected, bool& prev_selected) noexcept
	{
		undeleted_cnt = 0;	// 消去フラグがない図形の数
		selected_cnt = 0;	// 選択された図形の数
		selected_group_cnt = 0;	// 選択されたグループ図形の数
		runlength_cnt = 0;	// 選択された図形のランレングスの数
		selected_text_cnt = 0;	// 選択された文字列図形の数
		selected_image_cnt = 0;	// 選択された画像図形の数
		selected_poly_open_cnt = 0;	// 選択された開いた多角形図形の数
		selected_poly_close_cnt = 0;	// 選択された閉じた多角形図形の数
		selected_arc_cnt = 0;	// 選択された円弧図形の数
		text_cnt = 0;	// 文字列図形の数
		fore_selected = false;	// 最前面の図形の選択フラグ
		back_selected = false;	// 最背面の図形の選択フラグ
		prev_selected = false;	// ひとつ背面の図形の選択フラグ

		// 図形リストの各図形について以下を繰り返す.
		for (auto s : slist) {
			// 図形の消去フラグを判定する.
			if (s->is_deleted()) {
				// 以下を無視する.
				continue;
			}
			// 消去フラグがない図形の数をインクリメントする.
			undeleted_cnt++;
			// 図形の動的な型を得る.
			auto const& s_tid = typeid(*s);
			if (s_tid == typeid(ShapeText)) {
				// 型が文字列図形の場合,
				// 文字列図形の数をインクリメントする.
				text_cnt++;
			}
			else if (s_tid == typeid(ShapeGroup)) {
				if (static_cast<ShapeGroup*>(s)->has_text()) {
					// 型が文字列図形の場合,
					// 文字列図形の数をインクリメントする.
					text_cnt++;
				}
			}
			// 図形の選択フラグを最前面の図形の選択フラグに格納する.
			fore_selected = s->is_selected();
			if (fore_selected) {
				// 最前面の図形の選択フラグが立っている場合,
				// 選択された図形の数をインクリメントする.
				selected_cnt++;
				if (undeleted_cnt == 1) {
					// 消去フラグがない図形の数が 1 の場合,
					// 最背面の図形の選択フラグを立てる.
					back_selected = true;
				}
				// 図形の型が画像か判定する.,
				if (s_tid == typeid(ShapeImage)) {
					selected_image_cnt++;
				}
				// 図形の型が画像か判定する.,
				if (s_tid == typeid(ShapeArc)) {
					selected_arc_cnt++;
				}
				// 図形の型がグループ図形か判定する.,
				else if (s_tid == typeid(ShapeGroup)) {
					// 図形の型がグループ図形の場合,
					// 選択されたグループ図形の数をインクリメントする.
					selected_group_cnt++;
				}
				else if (s_tid == typeid(ShapePoly)) {
					// 図形の型が多角形図形の場合,
					bool closed;
					if (s->get_poly_closed(closed)) {
						if (closed) {
							// 選択された閉じた多角形図形の数をインクリメントする.
							selected_poly_close_cnt++;
						}
						else {
							// 選択された開いた多角形図形の数をインクリメントする.
							selected_poly_open_cnt++;
						}
					}
				}
				else if (s_tid == typeid(ShapeText)) {
					// 図形の型が文字列図形の場合,
					// 選択された文字列図形の数をインクリメントする.
					selected_text_cnt++;
				}
				if (!prev_selected) {
					// ひとつ背面の図形がヌル
					// またはひとつ背面の図形の選択フラグがない場合,
					// 選択された図形のランレングスの数をインクリメントする.
					runlength_cnt++;
				}
			}
			prev_selected = fore_selected;
		}

	}

	// 先頭から図形まで数える.
	// slist	図形リスト
	// s	図形
	// 戻り値	図形の順番
	// 消去フラグが立っている図形は順番に入れない.
	uint32_t slist_count(SHAPE_LIST const& slist, const Shape* s) noexcept
	{
		uint32_t i = 0;
		for (auto t : slist) {
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

	// 最初の図形を得る.
	// slist	図形リスト
	// 戻り値	最初の図形
	// 消去フラグが立っている図形は勘定されない.
	Shape* slist_front(SHAPE_LIST const& slist) noexcept
	{
		uint32_t _;
		return slist_next(slist.begin(), slist.end(), _);
	}

	// リスト中の図形が点を含むか判定する.
	// slist	図形リスト
	// t	判定される点
	// s	点を含む図形
	// 戻り値	点を含む図形の部位
	uint32_t slist_hit_test(SHAPE_LIST const& slist, const D2D1_POINT_2F t, Shape*& s) noexcept
	{
		// 前面にある図形が先にヒットするように, リストを逆順に検索する.
		for (auto it = slist.rbegin(); it != slist.rend(); it++) {
			const auto i = *it;
			if (i->is_deleted()) {
				continue;
			}
			const uint32_t anc = i->hit_test(t);
			if (anc != ANC_TYPE::ANC_PAGE) {
				s = i;
				return anc;
			}
		}
		return ANC_TYPE::ANC_PAGE;
	}

	// リストに図形を挿入する.
	// slist	図形リスト
	// s_ins	挿入される図形
	// s_at	挿入する位置にある図形
	void slist_insert(SHAPE_LIST& slist, Shape* const s_ins, const Shape* s_at) noexcept
	{
		slist.insert(std::find(slist.begin(), slist.end(), s_at), s_ins);
	}

	// リストの中の図形の順番を得る.
	// S	探索する型
	// T	得られた型
	// slist	図形リスト
	// s	探索する値
	// t	得られた値
	template <typename S, typename T> bool slist_match(SHAPE_LIST const& slist, S s, T& t)
	{
		bool f = false;	// 一致フラグ
		uint32_t j = 0;	// 探索する順に図形を数える添え字.
		uint32_t k = 0;	// スタックカウンタ.
		std::list<SHAPE_LIST::const_iterator> stack;	// リストを深さ優先で探索するためのスタック
		// 図形リストの開始と終端をスタックにプッシュする.
		stack.push_back(slist.begin());
		stack.push_back(slist.end());
		k++;
		while (k-- != 0) {
			// スタックカウンタが 0 でないかぎり以下を繰り返す.
			// 反復子と終端をスタックからポップする.
			SHAPE_LIST::const_iterator it_end = stack.back();
			stack.pop_back();
			SHAPE_LIST::const_iterator it = stack.back();
			stack.pop_back();
			while (it != it_end) {
				// 反復子が終端でないなら, 以下を繰り返す.
				// リストに含まれる図形を反復子から得る.
				// 反復子を次に進める.
				auto r = *it++;	// リストに含まれる図形
				if constexpr (std::is_same<S, Shape* const>::value) {
					// 引数 s が図形の場合,
					// リストに含まれる図形と引数 s が一致したなら,
					// フラグを立てる.
					// 引数 t に添え字を格納し, スタックカウンタを 0 にする.
					// 反復子の繰り返しを中断する.
					f = (r == s);
					if (f) {
						t = j;
						k = 0;
						break;
					}
				}
				if constexpr (std::is_same<S, const uint32_t>::value) {
					// 引数 s が添え字の場合,
					// 係数と引数 s が一致したなら, フラグを立てる.
					// 引数 t に図形を格納し, スタックカウンタを 0 にする.
					// 反復子の繰り返しを中断する.
					f = (j == s);
					if (f) {
						t = r;
						k = 0;
						break;
					}
				}
				// 添え字をひとつ加算.
				j++;
				if (typeid(*r) == typeid(ShapeGroup)) {
					// リストに含まれる図形がグループ図形なら,
					// 次の図形を指している反復子と終端をスタックにプッシュする.
					// グループに含まれるリストの開始と終端をスタックにプッシュする.
					// 反復子の繰り返しを中断する.
					stack.push_back(it);
					stack.push_back(it_end);
					k++;
					auto& glist = static_cast<ShapeGroup*>(r)->m_list_grouped;
					stack.push_back(glist.begin());
					stack.push_back(glist.end());
					k++;
					break;
				}
			}
		}
		stack.clear();
		return f;
	}
	template bool winrt::GraphPaper::implementation::slist_match<Shape* const, uint32_t>(
		SHAPE_LIST const& slist, Shape* const s, uint32_t& t);
	template bool winrt::GraphPaper::implementation::slist_match<const uint32_t, Shape*>(
		SHAPE_LIST const& slist, const uint32_t s, Shape*& t);

	// 選択された図形を移動する.
	// slist	図形リスト
	// pos	位置ベクトル
	// 戻り値	移動した図形があるなら true
	bool slist_move_selected(SHAPE_LIST const& slist, const D2D1_POINT_2F pos) noexcept
	{
		bool flag = false;
		for (auto s : slist) {
			if (s->is_deleted() || !s->is_selected()) {
				continue;
			}
			if (s->move(pos) && !flag) {
				flag = true;
			}
		}
		return flag;
	}

	// リスト中の図形のその次の図形を得る.
	// slist	図形リスト
	// s	図形
	// 戻り値	その次の図形
	Shape* slist_next(SHAPE_LIST const& slist, const Shape* s) noexcept
	{
		return slist_next(slist.begin(), slist.end(), s);
	}

	// リスト中の図形のその次の図形を得る.
	// it_beg	リストの始端
	// it_end	リストの終端
	// s	図形
	// 戻り値	次の図形. ヌルならば次の図形はない.
	template <typename T>
	static Shape* slist_next(T const& it_beg, T const& it_end, const Shape* s) noexcept
	{
		auto it{ std::find(it_beg, it_end, s) };
		if (it != it_end) {
			uint32_t _;
			return slist_next(++it, it_end, _);
		}
		return static_cast<Shape*>(nullptr);
	}
	template Shape* winrt::GraphPaper::implementation::slist_next(
		SHAPE_LIST::iterator const& it_beg, SHAPE_LIST::iterator const& it_end, const Shape* s)
		noexcept;
	template Shape* winrt::GraphPaper::implementation::slist_next(
		SHAPE_LIST::reverse_iterator const& it_rbeg, SHAPE_LIST::reverse_iterator const& it_rend,
		const Shape* s) noexcept;

	// リスト中の図形のその次の図形と, その間隔をリストから得る.
	// it_beg	リストの始端
	// it_end	リストの終端
	// distance	次の図形との間隔
	// 戻り値	次の図形, ヌルならば次の図形はない.
	template <typename T>
	static Shape* slist_next(T const& it_beg, T const& it_end, uint32_t& distance) noexcept
	{
		uint32_t i = 0;
		for (auto it = it_beg; it != it_end; it++) {
			auto s = *it;
			if (!s->is_deleted()) {
				distance = i;
				return s;
			}
			i++;
		}
		return static_cast<Shape*>(nullptr);
	}
	template Shape* winrt::GraphPaper::implementation::slist_next(
		SHAPE_LIST::iterator const& it_beg, SHAPE_LIST::iterator const& it_end, uint32_t& distance)
		noexcept;
	template Shape* winrt::GraphPaper::implementation::slist_next(
		SHAPE_LIST::reverse_iterator const& it_rbeg, SHAPE_LIST::reverse_iterator const& it_rend,
		uint32_t& distance) noexcept;

	// 前の図形をリストから得る.
	Shape* slist_prev(SHAPE_LIST const& slist, const Shape* s) noexcept
	{
		return slist_next(slist.rbegin(), slist.rend(), s);
	}

	// データリーダーから図形リストを読み込む.
	bool slist_read(SHAPE_LIST& slist, DataReader const& dt_reader)
	{
		Shape* s;
		while ((s = slist_read_shape(dt_reader)) != static_cast<Shape*>(nullptr)) {
			if (s == reinterpret_cast<Shape*>(-1)) {
				return false;
			}
			slist.push_back(s);
		}
		return true;
	}

	// データリーダーから図形を読み込む.
	static Shape* slist_read_shape(DataReader const& dt_reader)
	{
		if (dt_reader.UnconsumedBufferLength() < sizeof(uint32_t)) {
			return nullptr;
		}
		Shape* s = nullptr;
		auto s_tid = dt_reader.ReadUInt32();
		if (s_tid == SHAPE_T::SHAPE_NULL) {
		}
		else if (s_tid == SHAPE_T::SHAPE_BEZIER) {
			s = new ShapeBezier(dt_reader);
		}
		else if (s_tid == SHAPE_T::SHAPE_IMAGE) {
			s = new ShapeImage(dt_reader);
		}
		else if (s_tid == SHAPE_T::SHAPE_ELLIPSE) {
			s = new ShapeEllipse(dt_reader);
		}
		else if (s_tid == SHAPE_T::SHAPE_LINE) {
			s = new ShapeLine(dt_reader);
		}
		else if (s_tid == SHAPE_T::SHAPE_POLY) {
			s = new ShapePoly(dt_reader);
		}
		else if (s_tid == SHAPE_T::SHAPE_RECT) {
			s = new ShapeRect(dt_reader);
		}
		else if (s_tid == SHAPE_T::SHAPE_RRECT) {
			s = new ShapeRRect(dt_reader);
		}
		else if (s_tid == SHAPE_T::SHAPE_TEXT) {
			s = new ShapeText(dt_reader);
		}
		else if (s_tid == SHAPE_T::SHAPE_GROUP) {
			s = new ShapeGroup(dt_reader);
		}
		else if (s_tid == SHAPE_T::SHAPE_RULER) {
			s = new ShapeRuler(dt_reader);
		}
		else if (s_tid == SHAPE_T::SHAPE_ARC) {
			s = new ShapeArc(dt_reader);
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

	// 図形をリストから削除し, 削除した図形の次の図形を得る.
	// slist	図形リスト.
	// s	図形.
	// 戻り値	図形の次にあった図形. 図形がリスト末尾にあったときはヌルポインター.
	Shape* slist_remove(SHAPE_LIST& slist, const Shape* s) noexcept
	{
		auto it{ std::find(slist.begin(), slist.end(), s) };
		if (it != slist.end()) {
			(*it)->set_delete(true);
			auto it_next = std::next(it, 1);
			if (it_next != slist.end()) {
				return *it_next;
			}
		}
		return static_cast<Shape*>(nullptr);
	}

	// 選択された図形のリストを得る.
	// T	図形の型. Shape ならすべての種類, ShapeGroup ならグループ図形のみ.
	// slist	図形リスト
	// t_list	得られたリスト
	template <typename T> void slist_get_selected(const SHAPE_LIST& slist, SHAPE_LIST& t_list) noexcept
	{
		for (auto s : slist) {
			if (s->is_deleted() || !s->is_selected()) {
				continue;
			}
			if constexpr (std::is_same<T, ShapeGroup>::value) {
				if (typeid(*s) != typeid(T)) {
					continue;
				}
			}
			t_list.push_back(s);
		}
	}
	template void slist_get_selected<Shape>(const SHAPE_LIST& slist, SHAPE_LIST& t_list) noexcept;
	template void slist_get_selected<ShapeGroup>(const SHAPE_LIST& slist, SHAPE_LIST& t_list) noexcept;

	// データライターに図形リストを書き込む.
	// REDUCE	消去された図形は省く.
	// slist	図形リスト
	// dt_writer	データライター
	template<bool REDUCE> void slist_write(SHAPE_LIST const& slist, DataWriter const& dt_writer)
	{
		for (const auto s : slist) {
			if constexpr (REDUCE) {
				if (s->is_deleted()) {
					continue;
				}
			}
			auto const& s_tid = typeid(*s);
			if (s_tid == typeid(ShapeArc)) {
				dt_writer.WriteUInt32(SHAPE_T::SHAPE_ARC);
				s->write(dt_writer);
			}
			else if (s_tid == typeid(ShapeBezier)) {
				dt_writer.WriteUInt32(SHAPE_T::SHAPE_BEZIER);
				s->write(dt_writer);
			}
			else if (s_tid == typeid(ShapeEllipse)) {
				dt_writer.WriteUInt32(SHAPE_T::SHAPE_ELLIPSE);
				s->write(dt_writer);
			}
			else if (s_tid == typeid(ShapeGroup)) {
				dt_writer.WriteUInt32(SHAPE_T::SHAPE_GROUP);
				s->write(dt_writer);
			}
			else if (s_tid == typeid(ShapeImage)) {
				dt_writer.WriteUInt32(SHAPE_T::SHAPE_IMAGE);
				s->write(dt_writer);
			}
			else if (s_tid == typeid(ShapeLine)) {
				dt_writer.WriteUInt32(SHAPE_T::SHAPE_LINE);
				s->write(dt_writer);
			}
			else if (s_tid == typeid(ShapePoly)) {
				dt_writer.WriteUInt32(SHAPE_T::SHAPE_POLY);
				s->write(dt_writer);
			}
			else if (s_tid == typeid(ShapeRect)) {
				dt_writer.WriteUInt32(SHAPE_T::SHAPE_RECT);
				s->write(dt_writer);
			}
			else if (s_tid == typeid(ShapeRRect)) {
				dt_writer.WriteUInt32(SHAPE_T::SHAPE_RRECT);
				s->write(dt_writer);
			}
			else if (s_tid == typeid(ShapeRuler)) {
				dt_writer.WriteUInt32(SHAPE_T::SHAPE_RULER);
				s->write(dt_writer);
			}
			else if (s_tid == typeid(ShapeText)) {
				dt_writer.WriteUInt32(SHAPE_T::SHAPE_TEXT);
				s->write(dt_writer);
			}
		}
		// 終端を書き込む.
		dt_writer.WriteUInt32(static_cast<uint32_t>(SHAPE_NULL));
	}
	constexpr auto REDUCE = true;
	// 消去された図形も含めて, 図形リストを書き込む.
	template void slist_write<!REDUCE>(SHAPE_LIST const& slist, DataWriter const& dt_writer);
	// 消去された図形は省いて, 図形リストを書き込む.
	template void slist_write<REDUCE>(SHAPE_LIST const& slist, DataWriter const& dt_writer);

	// 選択されてない図形から, 指定した距離以下で, 指定した位置に最も近い頂点を得る.
	// slist	図形リスト
	// p	位置
	// d	距離
	// v	最も近い頂点
	bool slist_find_vertex_closest(
		const SHAPE_LIST& slist, const D2D1_POINT_2F& p, const float d, D2D1_POINT_2F& v) noexcept
	{
		bool flag = false;	// 頂点があったかどうかのフラグ
		float dd = d * d;	// 距離の二乗
		for (const auto s : slist) {
			if (s->is_deleted() || s->is_selected()) {
				continue;
			}
			if (s->get_pos_nearest(p, dd, v) && !flag) {
				flag = true;
			}
		}
		return flag;
	}

}