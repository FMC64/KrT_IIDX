#pragma once

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <optional>
#include <functional>
#include <set>
#include "../Shader.hpp"
#include "cpp_generator.hpp"
#include "util.hpp"
#include "util/string.hpp"
#include "util/sstream.hpp"

namespace Subtile {

class Shader::Compiler
{
	enum class BufType {
		Whitespace,
		Id,
		Operator
	};

	static char lower_char(char c)
	{
		if (c >= 'A' && c <= 'Z')
			return c + 32;
		else
			return c;
	}

	static BufType c_type(char c)
	{
		static const std::set<char> whitespace = {' ', '\n', '\t', '\r'};

		if (whitespace.find(c) != whitespace.end())
			return BufType::Whitespace;
		auto lower = lower_char(c);
		if (c == '_' || (lower >= 'a' && lower <= 'z') || (lower >= '0' && lower <= '9'))
			return BufType::Id;
		else
			return BufType::Operator;
	}

	class token_stream
	{
	public:
		token_stream(const std::vector<std::string> &tokens) :
			m_tokens(tokens),
			m_ndx(0)
		{
			m_tokens.emplace(m_tokens.begin(), "{");
			m_tokens.emplace_back("}");
		}

		auto& peek(void) const
		{
			return m_tokens.at(m_ndx);
		}

		auto peek2(void) const
		{
			return std::pair<const std::string&, const std::string&>(m_tokens.at(m_ndx), m_tokens.at(m_ndx + 1));
		}

		auto& poll(void)
		{
			auto &res = peek();

			m_ndx++;
			return res;
		}

		auto any_buf(void) const
		{
			return m_ndx < m_tokens.size();
		}

		auto is_end(void) const
		{
			return !any_buf();
		}

		size_t tokens_left(void) const
		{
			return m_tokens.size() - m_ndx;
		}

		void expect(const std::string &token)
		{
			auto got = poll();
			if (got != token)
				throw std::runtime_error(std::string("Expected '") + token + std::string("', but got '") + got + std::string("'"));
		}

	private:
		std::vector<std::string> m_tokens;
		size_t m_ndx;
	};

public:
	class token_output
	{
	public:
		token_output(void)
		{
		}

		template <typename S>
		auto& operator<<(S &&str)
		{
			if constexpr (std::is_same_v<std::string, std::remove_cv_t<std::remove_reference_t<S>>>)
				m_tokens.emplace_back(str);
			else
				m_tokens.emplace_back(util::sstream_str(std::forward<S>(str)));
			return *this;
		}

		void write(std::ostream &o) const
		{
			size_t indent = 0;
			auto endl = "\n";

			auto prev_type = BufType::Whitespace;
			bool new_lined = false;
			for (auto &t : m_tokens) {
				if (t == "{") {
					o << endl;
					write_tabs(o, indent);
					o << t << endl;
					new_lined = false;
					prev_type = BufType::Whitespace;
					indent++;
				} else if (t == "}") {
					indent--;
					write_tabs(o, indent);
					o << t << endl;
					new_lined = false;
					prev_type = BufType::Whitespace;
				} else if (t == ";") {
					o << t << endl;
					new_lined = false;
					prev_type = BufType::Whitespace;
				} else {
					if (!new_lined) {
						write_tabs(o, indent);
						new_lined = true;
					}
					auto type = c_type(t.at(0));
					if (type == BufType::Id && prev_type == BufType::Id)
						o << " ";
					o << t;
					prev_type = type;
				}
			}
		}

		const auto& getTokens(void)
		{
			return m_tokens;
		}

	private:
		std::vector<std::string> m_tokens;

		static void write_tabs(std::ostream &o, size_t n)
		{
			for (size_t i = 0; i < n; i++)
				o << "\t";
		}
	};

	using tstream = token_stream;

private:
	static auto read(const std::string &path)
	{
		std::stringstream res;
		std::ifstream in(path, std::ios::binary);

		res << in.rdbuf();
		return res.str();
	}

	static void flush_buffer(std::string &buf, BufType buf_type, std::vector<std::string> &res)
	{
		if (buf.size() > 0 && buf_type != BufType::Whitespace)
			res.emplace_back(buf);
		buf.clear();
	}

	static auto tokenize(const std::string &str)
	{
		static const std::set<std::string> ops = {
			"(", ")", ";", "{", "}", "[", "]"
		};

		std::vector<std::string> res;

		std::string buf;
		BufType buf_type = BufType::Whitespace;
		size_t next_i = 0;
		bool is_sl_comment = false;
		bool is_comment = false;
		size_t delay = 0;
		for (auto c : str) {
			next_i++;

			if (delay > 0) {
				delay--;
				continue;
			}

			std::optional<char> next;
			if (next_i < str.size())
				next = str.at(next_i);

			if (is_sl_comment) {
				if (c == '\n') {
					is_sl_comment = false;
					continue;
				}
			}
			if (is_comment) {
				if (c == '*' && next == '/') {
					is_comment = false;
					delay = 1;
					continue;
				}
			}

			if (c == '/' && next == '/') {
				flush_buffer(buf, buf_type, res);
				is_sl_comment = true;
			}
			if (c == '/' && next == '*') {
				flush_buffer(buf, buf_type, res);
				is_comment = true;
			}

			if (is_sl_comment || is_comment)
				continue;

			auto t = c_type(c);
			if (t != buf_type) {
				flush_buffer(buf, buf_type, res);
				buf_type = t;
			}
			buf.push_back(c);
			if ((buf_type == BufType::Operator) && (ops.find(buf) != ops.end()))
				flush_buffer(buf, buf_type, res);
		}
		flush_buffer(buf, buf_type, res);
		return res;
	}

	class Stages;
public:
	class Stage;

private:
	class Primitive
	{
	public:
		virtual ~Primitive(void) = default;

		virtual void write(token_output &o, Sbi sbi) const = 0;
	};

public:
	class Variable;
	class Struct;

private:
	class Counter
	{
	public:
		Counter(void) :
			m_ndx(0)
		{
		}

		size_t next(void)
		{
			return m_ndx++;
		}

	private:
		size_t m_ndx;
	};

	static auto sbiIsGlsl(Sbi sbi)
	{
		static const std::set<Sbi> table {Sbi::Vulkan};

		return table.find(sbi) != table.end();
	}

	Counter m_set_counter;

public:
	auto gen_set_ndx(void)
	{
		return m_set_counter.next();
	}

	class Set;

private:
	std::vector<std::reference_wrapper<Set>> m_sets;

	std::vector<std::reference_wrapper<Variable>> m_variables;
	std::vector<Struct> m_structs;

public:
	void addVariable(Variable &variable)
	{
		m_variables.emplace_back(variable);
	}

	template <typename ...Args>
	void addStruct(Args &&...args)
	{
		m_structs.emplace_back(std::forward<Args>(args)...);
	}

	auto& getStructs(void) const
	{
		return m_structs;
	}

	auto& getSets(void)
	{
		return m_sets;
	}

	void addSet(Set &set)
	{
		m_sets.emplace_back(set);
	}

	class Stage
	{
	public:
		class InterfaceInOut
		{
		public:
			enum class Dir {
				In,
				Out
			};

			InterfaceInOut(Variable &var, Dir dir, size_t loc) :
				m_var(var),
				m_dir(dir),
				m_loc(loc)
			{
			}

			void write(token_output &o) const
			{
				static const std::map<Dir, std::string> io_table {
					{Dir::In, "in"},
					{Dir::Out, "out"}
				};

				o << "layout" << "(" << "location" << "=" << m_loc << ")" << io_table.at(m_dir);
				m_var.declare(o);
			}

			auto& getVariable(void) const
			{
				return m_var;
			}

			auto getDir(void) const
			{
				return m_dir;
			}

			auto getLocation(void) const
			{
				return m_loc;
			}

		private:
			Variable &m_var;
			Dir m_dir;
			size_t m_loc;
		};

		Stage(Compiler &compiler, Shader::Stage stage) :
			m_compiler(compiler),
			m_stage(stage)
		{
		}

		void add(Primitive &prim)
		{
			m_primitives.emplace_back(prim);
		}

		void addIn(Variable &var)
		{
			m_in_variables.emplace_back(var);
		}
		void addOut(Variable &var)
		{
			m_out_variables.emplace_back(var);
		}

		auto& getInterface(void) const
		{
			return m_interface_ios;
		}

		void done(void) // called when all primitives are in stages (build in / out blocks)
		{
			if (m_in_variables.size() > 0) {
				if (&m_compiler.getStages().begin()->second == this) {
					size_t loc = 0;
					for (auto &var : m_in_variables) {
						m_interface_ios.emplace_back(var, InterfaceInOut::Dir::In, loc);
						loc += var.get().getType().parse("nolayout").loc_size;
					}
				} else if (m_stage == Shader::Stage::Fragment) {
					throw std::runtime_error("Input attachment not handled yet");
				} else
					throw std::runtime_error("In variable must be declared only in the first stage of the pipeline");
			}

			size_t loc = 0;
			for (auto &var : m_out_variables) {
				m_interface_ios.emplace_back(var, InterfaceInOut::Dir::Out, loc);
				auto after = m_compiler.getStages().stageAfter(m_stage);
				if (after)
					after->m_interface_ios.emplace_back(var, InterfaceInOut::Dir::In, loc);
				loc += var.get().getType().parse("nolayout").loc_size;
			}
		}

		void write(token_output &o, Sbi sbi) const
		{
			for (auto &s : m_compiler.getStructs())
				s.write(o);
			for (auto &io : m_interface_ios)
				io.write(o);
			token_output inter_o;
			for (auto &p : m_primitives)
				p.get().write(inter_o, sbi);
			std::string last_token;
			for (auto &t : inter_o.getTokens()) {
				bool has_subs = false;
				if (last_token != ".") {
					/*for (auto &b : m_compiler.getDescriptorSets()) {
						auto sub = b.get().getBlock().substituate(t, sbi);
						if (sub) {
							o << *sub;
							has_subs = true;
							break;
						}
					}*/
				}
				if (!has_subs)
					o << t;
				last_token = t;
			}
		}

		void write(std::ostream &o, Sbi sbi) const
		{
			token_output toks;
			o << "#version 450\n";
			write(toks, sbi);
			toks.write(o);
		}

		auto getStage(void)
		{
			return m_stage;
		}

	private:
		Compiler &m_compiler;
		Shader::Stage m_stage;
		std::vector<std::reference_wrapper<Primitive>> m_primitives;
		std::vector<std::reference_wrapper<Variable>> m_in_variables;
		std::vector<std::reference_wrapper<Variable>> m_out_variables;

		std::vector<InterfaceInOut> m_interface_ios;
	};

private:
	class Stages : public std::map<Shader::Stage, Stage>
	{
	public:
		Stages(Compiler &compiler) :
			m_compiler(compiler)
		{
		}

		void add(Shader::Stage stage, Primitive &prim)
		{
			auto got = find(stage);
			if (got == end()) {
				auto [it, succ] = emplace(std::piecewise_construct, std::forward_as_tuple(stage), std::forward_as_tuple(m_compiler, stage));
				got = it;
			}
			got->second.add(prim);
		}

		auto getSet(void) const
		{
			std::set<Shader::Stage> res;

			for (auto &p : *this)
				res.emplace(p.first);
			return res;
		}

		Stage* stageAfter(Shader::Stage stage)
		{
			auto got = find(stage);
			if (got == end())
				throw std::runtime_error("Base stage not found for neighbor search");
			++got;
			if (got == end())
				return nullptr;
			else
				return &got->second;
		}

	private:
		Compiler &m_compiler;
	};

	Stages m_stages;

public:
	Stages& getStages(void)
	{
		return m_stages;
	}

	class Type
	{
		static std::vector<size_t> parseArray(tstream &s)
		{
			std::vector<size_t> res;

			while (s.peek() == "[") {
				s.poll();
				res.emplace_back(util::stot(std::stoull, s.poll()));
				s.expect("]");
			}
			return res;
		}

	public:
		struct Parsed {
			Parsed(const std::string &name, bool is_opaque = false, bool is_user_defined = false) :
				name(name),
				is_user_defined(is_user_defined),
				is_opaque(is_opaque)
			{
			}

			std::string name;
			bool is_user_defined;
			bool is_opaque;

			size_t salign = 0;
			size_t balign = 0;
			size_t ealign = 0;
			size_t loc_size = 0;

			size_t bsize = 0;
			size_t esize = 0;

			void set_all_sizes(size_t size)
			{
				bsize = size;
				esize = size;
			}

			void spec_array(const std::vector<size_t> &array, const std::string &layout)
			{
				if (array.size() == 0)
					return;

				for (auto it = array.crbegin(); it != array.crend(); it++) {
					auto &a = *it;
					std::stringstream ss;
					ss << typePrefix("Array") << "<" << name << ", " << a << ", " << layout << ">";
					name = ss.str();
				}
				size_t els = 1;
				for (auto &a : array)
					els *= a;
				loc_size *= els;
				ealign = util::align_dyn(ealign, 16);
				bsize *= els;
				esize *= els;
			}
		};

		Type(Compiler &compiler, tstream &s) :
			m_compiler(compiler),
			m_name(s.poll()),
			m_base_array(parseArray(s)),
			m_array(m_base_array)
		{
		}

		struct build_comma_t {};
		static inline constexpr build_comma_t build_comma {};

		Type(build_comma_t, const Type &other) :
			m_compiler(other.m_compiler),
			m_name(other.m_name),
			m_base_array(other.m_base_array),
			m_array(m_base_array)
		{
		}

		auto& getName(void) const
		{
			return m_name;
		}

		auto& getArray(void) const
		{
			return m_array;
		}

		auto getArrayElems(void) const
		{
			size_t res = 1;
			for (auto &a : m_array)
				res *= a;
			return res;
		}

		void extend(tstream &s)
		{
			auto array = parseArray(s);

			for (auto &e : m_array)
				array.emplace_back(e);
			m_array = array;
		}

		void write(token_output &o) const
		{
			o << m_name;
			for (auto &a : m_array)
				o << "[" << a << "]";
		}

		enum class Layout
		{
			Std140,
			Std430,
			Irrelevant = Std140
		};

		static std::string layoutToType(Layout layout)
		{
			static const std::map<Layout, std::string> table {
				{Layout::Std140, "Std140"},
				{Layout::Std430, "Std430"}
			};

			return typePrefix(table.at(layout));
		}

		Parsed parse(const std::string &layout) const
		{
			auto nopq = parseNOpq(layout);
			if (nopq)
				return *nopq;
			auto opq = parseOpq();
			if (opq)
				return Parsed(*opq, true);
			for (auto &s : m_compiler.getStructs()) {
				if (s.getName() == m_name) {
					Parsed res(m_name, false, true);

					res.salign = s.getSalign();
					res.balign = s.getBalign();
					res.ealign = s.getEalign();
					res.loc_size = s.getLocSize();
					res.bsize = s.getBsize();
					res.esize = s.getEsize();
					return res;
				}
			}
			throw std::runtime_error(std::string("Unknown type '") + m_name + std::string("'"));
		}

	private:
		Compiler &m_compiler;
		std::string m_name;
		std::vector<size_t> m_base_array;
		std::vector<size_t> m_array;

		static std::string typePrefix(const std::string &name)
		{
			static const std::string add("sb::Shader::Type::");

			return add + name;
		}

		static bool is_vec_num(char num)
		{
			static const std::set<char> table {
				'2', '3', '4'
			};

			return table.find(num) != table.end();
		}

		class Scalar
		{
		public:
			Scalar(const std::string &name, size_t size) :
				name(name),
				size(size)
			{
			}

			const std::string name;
			const size_t size;
		};

		static auto& Bool(void)
		{
			static const Scalar res("Bool", sizeof(sb::Shader::Type::Bool));
			return res;
		}
		static auto& Int(void)
		{
			static const Scalar res("Int", sizeof(sb::Shader::Type::Int));
			return res;
		}
		static auto& Uint(void)
		{
			static const Scalar res("Uint", sizeof(sb::Shader::Type::Uint));
			return res;
		}
		static auto& Float(void)
		{
			static const Scalar res("Float", sizeof(sb::Shader::Type::Float));
			return res;
		}
		static auto& Double(void)
		{
			static const Scalar res("Double", sizeof(sb::Shader::Type::Double));
			return res;
		}

		static Parsed contructVec(const Scalar &base, size_t size)
		{
			std::stringstream ss;

			ss << typePrefix("Vec") << "<" << typePrefix(base.name) << ", " << size << ">";

			Parsed res(ss.str());
			res.salign = base.size;
			res.balign = res.salign * (size == 2 ? 2 : 4);
			res.ealign = res.balign;
			res.loc_size = base.size == sizeof(double) && size > 2 ? 2 : 1;
			res.set_all_sizes(base.size * size);
			return res;
		}

		std::optional<Parsed> parseVec(void) const
		{
			static const std::string vecn("vecn");
			static const std::string tvecn("tvecn");
			static const std::map<char, Scalar> tvec_table {
				{'b', Bool()},
				{'i', Int()},
				{'u', Uint()},
				{'d', Double()}
			};

			if (m_name.size() == vecn.size()) {
				auto &num = m_name.at(3);
				if (m_name.substr(0, 3) == "vec" && is_vec_num(num))
					return contructVec(Float(), num - '0');
			}
			if (m_name.size() == tvecn.size()) {
				auto &num = m_name.at(4);
				auto got = tvec_table.find(m_name.at(0));
				if (got != tvec_table.end() && m_name.substr(1, 3) == "vec" && is_vec_num(num))
					return contructVec(got->second, num - '0');
			}
			return std::nullopt;
		}


		static Parsed constructMat(const Scalar &basetype, size_t c, size_t r, const std::string &layout)
		{
			std::stringstream ss;
			ss << typePrefix("Mat") << "<" << typePrefix(basetype.name) <<", " << c << ", " << r << ", " << layout << ">";

			auto vec = contructVec(basetype, c);
			Parsed res(ss.str());
			res.salign = vec.salign;
			res.balign = vec.balign;
			res.ealign = res.balign;
			res.bsize = res.balign * r;
			res.esize = res.ealign * r;
			return res;
		}

		std::optional<Parsed> try_construct_mat(const Scalar &basetype, const std::string &rest, const std::string &layout) const
		{
			if (rest.size() == 1) {
				auto num = rest.at(0);
				if (is_vec_num(num))
					return constructMat(basetype, num - '0', num - '0', layout);
			}
			if (rest.size() == 3) {
				auto c = rest.at(0);
				auto r = rest.at(2);
				if (rest.at(1) == 'x' && is_vec_num(c) && is_vec_num(r))
					return constructMat(basetype, c - '0', r - '0', layout);
			}
			return std::nullopt;
		}

		std::optional<Parsed> parseMat(const std::string &layout) const
		{
			static const std::string mat("mat");
			static const std::string dmat("dmat");

			if (m_name.substr(0, mat.size()) == mat) {
				auto got = try_construct_mat(Float(), m_name.substr(mat.size()), layout);
				if (got)
					return *got;
			}
			if (m_name.substr(0, dmat.size()) == dmat) {
				auto got = try_construct_mat(Double(), m_name.substr(dmat.size()), layout);
				if (got)
					return *got;
			}
			return std::nullopt;
		}

		std::optional<Parsed> parseNOpq(const std::string &layout) const
		{
			static const std::map<std::string, Scalar> scalars {
				{"bool", Bool()},
				{"int", Int()},
				{"uint", Uint()},
				{"float", Float()},
				{"double", Double()}
			};

			auto sgot = scalars.find(m_name);
			if (sgot != scalars.end()) {
				auto &scalar = sgot->second;
				Parsed res(typePrefix(sgot->second.name));
				res.salign = scalar.size;
				res.balign = res.salign;
				res.ealign = res.balign;
				res.loc_size = 1;
				res.set_all_sizes(scalar.size);
				res.spec_array(m_array, layout);
				return res;
			}
			auto vec = parseVec();
			if (vec) {
				vec->spec_array(m_array, layout);
				return *vec;
			}
			auto mat = parseMat(layout);
			if (mat) {
				mat->spec_array(m_array, layout);
				return *mat;
			}
			return std::nullopt;
		}

		//static std::map<std::string, s>

		std::optional<std::string> parseOpq(void) const
		{
			//static const std::map<std::string, std::string> sampler_table;
			//static const std::map<std::string, std::string> image_table = getImageTable("image");

			if (m_name == "atomic_uint")
				return typePrefix("AtomicUint");
			return std::nullopt;
		}
	};

	class Variable : public Primitive
	{
	public:
		enum class Storage {
			Inline,
			Const,
			In,
			Out
		};

	private:
		using StorageTable = std::map<std::string, Storage>;

		static auto& storageTable(void)
		{
			static const StorageTable table {
				{"const", Storage::Const},
				{"in", Storage::In},
				{"out", Storage::Out}
			};

			return table;
		}

		std::string getId(tstream &s)
		{
			auto res = s.poll();

			m_type.extend(s);
			return res;
		}

	public:
		Variable(Compiler &compiler, tstream &s, Storage storage) :
			m_compiler(compiler),
			m_storage(storage),
			m_type(compiler, s),
			m_id(getId(s)),
			m_value(getValue(s))
		{
		}

		Variable(Compiler &compiler, tstream &s) :
			Variable(compiler, s, storageTable().at(s.poll()))
		{
		}

		Variable(tstream &s, Variable &first) :
			m_compiler(first.m_compiler),
			m_storage(first.m_storage),
			m_type(Type::build_comma, first.m_type),
			m_id(getId(s)),
			m_value(getValue(s))
		{
		}

		static bool isComingUp(tstream &s)
		{
			return storageTable().find(s.peek()) != storageTable().end();
		}

		void declare(token_output &o) const
		{
			m_type.write(o);
			o << m_id;
			if (m_value) {
				o << "=";
				for (auto &v : *m_value)
					o << v;
			}
			o << ";";
		}

		void write(token_output &o, Sbi sbi) const override
		{
			if (sbi == Sbi::Vulkan) {
				write_vulkan(o);
			} else
				throw std::runtime_error("Can't output variable for such interface");
		}

		void addStage(Shader::Stage stage)
		{
			m_stages.emplace(stage);
		}

		void addToStage(Stage &stage)
		{
			if (m_storage == Storage::In) {
				stage.addIn(*this);
			} else if (m_storage == Storage::Out) {
				stage.addOut(*this);
			} else
				stage.add(*this);
		}

		const std::set<Shader::Stage>& getStages(void) const
		{
			return m_stages;
		}

		const std::string& getName(void) const
		{
			return m_id;
		}

		const Type& getType(void) const
		{
			return m_type;
		}

	private:
		Compiler &m_compiler;
		Storage m_storage;
		Type m_type;
		std::string m_id;
		std::optional<std::vector<std::string>> m_value;

		std::set<Shader::Stage> m_stages;

		std::optional<std::vector<std::string>> getValue(tstream &s)
		{
			if (s.peek() == "=") {
				s.poll();

				std::vector<std::string> res;

				while (s.peek() != ";")
					res.emplace_back(s.poll());
				return res;
			} else
				return std::nullopt;
		}

		void write_vulkan(token_output &o) const
		{
			/*static const std::map<Storage, size_t> storage_to_set {
				{Storage::Material, 0},
				{Storage::Object, 1}
			};*/

			if (m_storage == Storage::In || m_storage == Storage::Out)
				return;

			if (m_storage == Storage::Const)
				o << "const";
			/*else
				o << "layout" << "(" << "set" << "=" << (storage_to_set.at(m_storage)) << "," << "binding" << "=" << 0 << ")" << "uniform";*/
			declare(o);
		}
	};

	class Struct
	{
	public:
		Struct(tstream &s, Compiler &compiler) :
			m_compiler(compiler),
			m_name(getName(s)),
			m_variables(getVariables(s)),
			m_variables_parsed(getVariablesParsed()),
			m_salign(computeAlign(&Type::Parsed::salign)),
			m_balign(computeAlign(&Type::Parsed::balign)),
			m_ealign(util::align_dyn(computeAlign(&Type::Parsed::ealign), 16)),
			m_loc_size(computeLocSize()),
			m_bsize(computeSize(&Type::Parsed::bsize, &Type::Parsed::balign)),
			m_esize(computeSize(&Type::Parsed::esize, &Type::Parsed::ealign))
		{
			s.expect(";");
		}

		static bool isComingUp(tstream &s)
		{
			return s.peek() == "struct";
		}

		void write(token_output &o) const
		{
			o << "struct" << m_name << "{";
			for (auto &v : m_variables)
				v.declare(o);
			o << "}" << ";";
		}

		const std::string& getName(void) const
		{
			return m_name;
		}

		auto& getVariables(void) const
		{
			return m_variables;
		}

		size_t getSalign(void) const { return m_salign; }
		size_t getBalign(void) const { return m_balign; }
		size_t getEalign(void) const { return m_ealign; }
		size_t getLocSize(void) const { return m_loc_size; }
		size_t getBsize(void) const { return m_bsize; }
		size_t getEsize(void) const { return m_esize; }

	private:
		Compiler &m_compiler;
		std::string m_name;
		util::unique_vector<Variable> m_variables;
		std::vector<Type::Parsed> m_variables_parsed;
		size_t m_salign;
		size_t m_balign;
		size_t m_ealign;
		size_t m_loc_size;

		size_t m_bsize;
		size_t m_esize;

		std::string getName(tstream &s)
		{
			s.expect("struct");
			return s.poll();
		}

		util::unique_vector<Variable> getVariables(tstream &s)
		{
			util::unique_vector<Variable> res;

			s.expect("{");
			while (s.peek() != "}") {
				auto &first = res.emplace(m_compiler, s, Variable::Storage::Inline);
				while (s.peek() == ",") {
					s.poll();
					res.emplace(s, first);
				}
				s.expect(";");
			}
			s.poll();

			return res;
		}

		std::vector<Type::Parsed> getVariablesParsed(void)
		{
			std::vector<Type::Parsed> res;

			for (auto &v : m_variables)
				res.emplace_back(v.getType().parse("nolayout"));
			return res;
		}

		size_t computeAlign(size_t Type::Parsed::*resolver)
		{
			size_t res = 0;

			for (auto &p : m_variables_parsed)
				res = std::max(res, p.*resolver);
			return res;
		}

		size_t computeLocSize(void)
		{
			size_t res = 0;

			for (auto &p : m_variables_parsed)
				res += p.loc_size;
			return res;
		}

		size_t computeSize(size_t Type::Parsed::*size_resolver, size_t Type::Parsed::*align_resolver)
		{
			size_t res = 0;

			for (auto &p : m_variables_parsed) {
				auto size = p.*size_resolver;
				auto align = p.*align_resolver;
				res = util::align_dyn(res, align);
				res += size;
			}
			return res;
		}
	};

	class GlslMappedBlock
	{
	public:
		GlslMappedBlock(Set &set, size_t binding, const std::string &name) :
			m_set(set),
			m_binding(binding),
			m_name(name)
		{
		}

		void write(token_output &o, Sbi sbi) const
		{
			if (sbi == Sbi::Vulkan)
				write_vulkan(o);
			else
				throw std::runtime_error("Sbi not supported");
		}

		void add(Variable &var)
		{
			m_variables.emplace_back(var);
		}

		auto getLayoutBinding(void) const
		{
			sb::Shader::DescriptorSet::Layout::DescriptionBinding res;

			res.binding = m_binding;
			res.descriptorCount = 0;
			res.descriptorType = sb::Shader::DescriptorType::UniformBuffer;
			res.stages = m_set.getStages();
			return res;
		}

	private:
		Set &m_set;
		size_t m_binding;
		const std::string &m_name;
		std::vector<std::reference_wrapper<Variable>> m_variables;

		void write_vulkan(token_output &o) const
		{
			o << "layout" << "(" << "std140" << "," << "set" << "=" << m_set.getNdx() << "," << "binding" << "=" << m_binding << ")" << "uniform" << (std::string("_") + m_name + std::string("_t_")) << "{";
			for (auto &v : m_variables)
				v.get().declare(o);
			o << "}" << m_name << ";";
		}
	};

	class GlslOpaqueVar
	{
	public:
		GlslOpaqueVar(Set &set, size_t binding, Variable &variable) :
			m_set(set),
			m_binding(binding),
			m_variable(variable)
		{
		}

		void write(token_output &o, Sbi sbi) const
		{
			if (sbi == Sbi::Vulkan)
				write_vulkan(o);
			else
				throw std::runtime_error("Sbi not supported");
		}

		auto getLayoutBinding(void) const
		{
			sb::Shader::DescriptorSet::Layout::DescriptionBinding res;

			res.binding = m_binding;
			res.descriptorCount = m_variable.getType().getArrayElems();
			res.descriptorType = sb::Shader::DescriptorType::CombinedImageSampler;
			res.stages = m_set.getStages();
			return res;
		}

	private:
		Set &m_set;
		size_t m_binding;
		Variable &m_variable;

		void write_vulkan(token_output &o) const
		{
			o << "layout" << "(" << "std140" << "," << "set" << "=" << m_set.getNdx() << "," << "binding" << "=" << m_binding << ")" << "uniform";
			m_variable.declare(o);
		}
	};

	class Set : public Primitive
	{
	public:
		Set(tstream &s, Compiler &compiler) :
			m_set_ndx(compiler.gen_set_ndx()),
			m_name(getName(s)),
			m_variables(getVariables(s, compiler))
		{
			s.expect(";");
			compiler.addSet(*this);
		}

		static bool isComingUp(tstream &s)
		{
			return s.peek() == "set";
		}

		size_t getNdx(void) const
		{
			return m_set_ndx;
		}

		const std::set<sb::Shader::Stage>& getStages(void) const
		{
			return m_stages;
		}

		void write(token_output &o, Sbi sbi) const override
		{
			if (sbiIsGlsl(sbi)) {
				if (m_mapped_block)
					m_mapped_block->write(o, sbi);
				for (auto &opq : m_opaque_vars)
					opq.write(o, sbi);
			} else
				throw std::runtime_error("Unsupported SBI");
		}

		void addToStage(Stage &stage)
		{
			m_stages.emplace(stage.getStage());
			stage.add(*this);
		};

		auto& getName(void) const { return m_name; }
		auto& getVariables(void) const { return m_variables; }
		auto getLayout(void) const
		{
			sb::Shader::DescriptorSet::Layout::Description res;

			if (m_mapped_block)
				res.emplace_back(m_mapped_block->getLayoutBinding());
			for (auto &o : m_opaque_vars)
				res.emplace_back(o.getLayoutBinding());
			return res;
		}

	private:
		size_t m_set_ndx;
		Counter m_binding_counter;
		std::string m_name;
		std::optional<GlslMappedBlock> m_mapped_block;
		std::vector<GlslOpaqueVar> m_opaque_vars;
		std::set<sb::Shader::Stage> m_stages;

		util::unique_vector<Variable> m_variables;

		std::string getName(tstream &s)
		{
			s.expect("set");
			return s.poll();
		}

		util::unique_vector<Variable> getVariables(tstream &s, Compiler &compiler)
		{
			util::unique_vector<Variable> res;

			s.expect("{");
			while (s.peek() != "}") {
				auto &first = res.emplace(compiler, s, Variable::Storage::Inline);
				varAdded(first);
				while (s.peek() == ",") {
					s.poll();
					varAdded(res.emplace(s, first));
				}
				s.expect(";");
			}
			s.poll();
			return res;
		}

		void varAdded(Variable &var)
		{
			auto parsed = var.getType().parse("nolayout");

			if (parsed.is_opaque) {
				m_opaque_vars.emplace_back(*this, m_binding_counter.next(), var);
			} else {
				if (!m_mapped_block)
					m_mapped_block.emplace(*this, m_binding_counter.next(), m_name);
				m_mapped_block->add(var);
			}
		}
	};

private:
	class Section : public Primitive
	{
		using StageTable = std::map<std::string, Shader::Stage>;

		static auto& stageTable(void)
		{
			static const StageTable table {
				{"tesselation_control", Shader::Stage::TesselationControl},
				{"tesselation_evaluation", Shader::Stage::TesselationEvaluation},
				{"geometry", Shader::Stage::Geometry},
				{"vertex", Shader::Stage::Vertex},
				{"fragment", Shader::Stage::Fragment}
			};

			return table;
		}

	public:
		Section(tstream &s, Compiler &compiler) :
			m_compiler(compiler),
			m_stage(getStage(s))
		{
			while (!poll_end(s))
				poll(s, compiler);
		}

		static bool isComingUp(tstream &s)
		{
			if (s.peek() == "{")
				return true;
			if (s.tokens_left() >= 2) {
				auto [first, second] = s.peek2();
				return (stageTable().find(first) != stageTable().end()) && (second == "{");
			}
			return false;
		}

		void write(token_output &o, Sbi sbi) const override
		{
			for (auto &p : m_primitives)
				p.write(o, sbi);
		}

		void poll(tstream &s, Compiler &compiler);

		void recurAdd(Primitive &prim)
		{
			if (m_stage) {
				auto var = dynamic_cast<Variable*>(&prim);
				if (var) {
					var->addToStage(*m_stage);
				} else {
					auto set = dynamic_cast<Set*>(&prim);
					if (set)
						set->addToStage(*m_stage);
					else
						m_stage->add(prim);
				}
			}
			for (auto &p : m_primitives) {
				auto s = dynamic_cast<Section*>(&p);
				if (s)
					s->recurAdd(prim);
			}
		}

		void dispatch(void)
		{
			for (auto &p : m_primitives) {
				auto s = dynamic_cast<Section*>(&p);
				if (s)
					s->dispatch();
				else
					recurAdd(p);
			}
		}

	private:
		Compiler &m_compiler;
		Stage *m_stage;
		util::unique_vector<Primitive> m_primitives;

		Stage* getStage(tstream &s)
		{
			auto [first, second] = s.peek2();

			if (first == "{") {
				s.poll();
				return nullptr;
			} else {
				auto name = s.poll();
				s.poll();
				auto stage = stageTable().at(name);
				auto &stages = m_compiler.getStages();
				auto got = stages.find(stage);
				if (got == stages.end()) {
					auto [it, suc] = stages.emplace(std::piecewise_construct, std::forward_as_tuple(stage), std::forward_as_tuple(m_compiler, stage));
					if (!suc)
						throw std::runtime_error("Can't emplace stage");
					got = it;
				}
				return &got->second;
			}
		}

		bool poll_end(tstream &s)
		{
			auto &p = s.peek();

			if (p == "}") {
				s.poll();
				return true;
			} else
				return false;
		}
	};

	class Function : public Primitive
	{
	public:
		Function(tstream &s) :
			m_tokens(getTokens(s))
		{
		}

		void write(token_output &o, Sbi) const override
		{
			for (auto &t : m_tokens)
				o << t;
		}

	private:
		std::vector<std::string> m_tokens;

		std::vector<std::string> getTokens(tstream &s)
		{
			std::vector<std::string> res;

			while (true) {
				auto &cur = s.poll();
				res.emplace_back(cur);
				if (cur == "{")
					break;
			}
			size_t lev = 1;
			while (lev > 0) {
				auto &cur = s.poll();
				res.emplace_back(cur);
				if (cur == "{")
					lev++;
				else if (cur == "}")
					lev--;
			}
			return res;
		}
	};

	token_stream m_stream;
	Section m_collec;

public:	
	Compiler(const std::string &path);
};

inline void Shader::Compiler::Section::poll(tstream &s, Compiler &compiler)
{
	if (s.peek() == ";") {
		s.poll();
		return;
	}
	if (Section::isComingUp(s))
		m_primitives.emplace<Section>(s, compiler);
	else if (Struct::isComingUp(s)) {
		compiler.addStruct(s, compiler);
	} else if (Set::isComingUp(s)) {
		m_primitives.emplace<Set>(s, compiler);
	} else if (Variable::isComingUp(s)) {
		auto &first = m_primitives.emplace<Variable>(compiler, s);
		compiler.addVariable(first);
		while (s.peek() == ",") {
			s.poll();
			compiler.addVariable(m_primitives.emplace<Variable>(s, first));
		}
		s.expect(";");
	} else
		m_primitives.emplace<Function>(s);
}

inline Shader::Compiler::Compiler(const std::string &path) :
	m_stages(*this),
	m_stream(token_stream(tokenize(read(path)))),
	m_collec(m_stream, *this)
{
	m_collec.dispatch();

	/*for (auto &v : m_variables) {
		auto var = v.get();
		auto set = var.getSet();
		if (set)
			m_blocks.forSet(*set).add(v);
	}*/

	for (auto &s : m_stages)
		s.second.done();

	/*std::cout << "STAGES:" << std::endl;
	for (auto &s : m_stages) {
		std::cout << static_cast<std::underlying_type_t<decltype(s.first)>>(s.first) << std::endl;
		token_output o;
		s.second.write(o, Sbi::Vulkan);
		o.write(std::cout);
	}*/
}

}