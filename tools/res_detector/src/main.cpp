#include <iostream>
#include <filesystem>
#include <map>
#include <vector>
#include <string>
#include <sstream>
#include <fstream>
#include <stdexcept>
#include <optional>

class FileIO
{
public:
	static std::string read(const std::string &path)
	{
		std::stringstream res;
		std::ifstream in(path, std::ios::binary);

		res << in.rdbuf();
		return res.str();
	}

	static void write(const std::string &outpath, const std::string &data)
	{
		auto got = read(outpath);
		if (got == data)
			return;

		std::cout << "Update " << outpath << std::endl;
		std::ofstream out(outpath, std::ios::trunc | std::ios::binary);

		out << data;
	}
};

class CodeFormatter
{
	static bool is_whitespace(char in)
	{
		return in == ' ' || in == '\r' || in == '\n' || in == '\t';
	}

	static bool upcoming_char(const std::string &str, size_t ndx, char ch)
	{
		for (size_t i = ndx; i < str.size(); i++) {
			auto &c = str.at(i);
			if (c == ch)
				return true;
			if (is_whitespace(c))
				continue;
			return false;
		}
		return false;
	}

	static void str_add_tabs(std::string &str, size_t count)
	{
		for (size_t i = 0; i < count; i++)
			str.push_back('\t');
	}

	static void format_decorate(std::string &out, const std::string &in, size_t i, size_t d, bool has_changed)
	{
		auto &c = in.at(i);

		if ((has_changed && !upcoming_char(in, i, ';')) || c == ';') {
			out.push_back('\n');
			str_add_tabs(out, d);
		}
		if (c == '\n')
			str_add_tabs(out, d);
	}

	static std::string format_code_first(const std::string &in)
	{
		std::string out;
		size_t d = 0;

		for (size_t i = 0; i < in.size(); i++) {
			auto &c = in.at(i);

			if (c == '\r')
				continue;
			auto deco = false;
			auto d_p = d;
			if (c == '{')
				d++;
			if (c == '}') {
				d--;
				format_decorate(out, in, i, d, d != d_p);
				deco = true;
			}
			out.push_back(c);
			if (!deco)
				format_decorate(out, in, i, d, d != d_p);
		}
		return out;
	}

	static size_t blank_line(const std::string &in, size_t ndx)
	{
		for (size_t i = ndx; i < in.size(); i++) {
			auto &c = in.at(i);
			if (c == '\n')
				return i;
			if (!is_whitespace(c))
				return ndx;
		}
		return in.size();
	}

	static std::string format_code_second(const std::string &in)
	{
		std::string out;

		for (size_t i = 0; i < in.size(); i++) {
			auto next = blank_line(in, i);
			if (next != i)
				i = next;
			else
				for (; i < in.size(); i++) {
					auto &c = in.at(i);
					out.push_back(c);
					if (c == '\n')
						break;
				}
		}
		return out;
	}

public:
	static std::string format(const std::string &in)
	{
		return format_code_second(format_code_first(in));
	}
};

namespace std {
namespace fs = filesystem;
}

class FolderPrinter
{
	static std::string id_storage(const std::string &id)
	{
		return id + std::string("_storage");
	}

	template <typename T>
	static std::optional<std::tuple<std::string, std::string>> getMember(const T &e)
	{
		static const std::map<std::string, std::string> exts = {
			{".obj", "sb::rs::Model"},
			{".png", "sb::rs::Texture"},
		};

		auto ext = e.path().extension().string();
		auto got = exts.find(ext);

		if (got == exts.end())
			return std::nullopt;
		auto &type = got->second;
		auto id = e.path().stem().string();
		return std::make_tuple(type, id);
	}

	static std::string class_name(const std::string &str)
	{
		return str + std::string("_class");
	}

	static std::string scope_append(const std::string &base, const std::string &ap)
	{
		return base + std::string("::") + ap;
	}

	static void output_impl(std::ostream &out, const std::string &scope, const std::string &type, const std::string &id)
	{
		out << type << "& " << scope << "::" << id << "(void)" << std::endl;
		out << "{" << std::endl;
		out << "return " << id_storage(id) << ";" << std::endl;
		out << "}" << std::endl;
	}

	template <typename T>
	static void it_dir(std::ostream &out, std::ostream &outimpl, const T &itbase, const std::string &scope)
	{
		for (auto &e : std::fs::directory_iterator(itbase)) {
			auto name = e.path().filename().string();
			if (name.at(0) == '.')
				continue;

			if (e.is_directory()) {
				out << "class " << class_name(name) << std::endl << "{";
				it_dir(out, outimpl, e, scope_append(scope, class_name(name)));
				out << "};";
			} else {
			}
		}

		for (auto &e : std::fs::directory_iterator(itbase)) {
			auto name = e.path().filename().string();
			if (name.at(0) == '.')
				continue;

			if (e.is_directory()) {
				out << class_name(name) << " " << id_storage(name) << ";";
				output_impl(outimpl, scope, scope_append(scope, class_name(name)), name);
			} else {
				auto got = getMember(e);
				if (!got)
					continue;
				auto &[type, id] = *got;
				out << type << " " << id_storage(id) << ";";
				output_impl(outimpl, scope, type, id);
			}
		}

		out << "public:\n";
		for (auto &e : std::fs::directory_iterator(itbase)) {
			auto name = e.path().filename().string();
			if (name.at(0) == '.')
				continue;

			if (e.is_directory()) {
				out << class_name(name) << "& " << name << "(void);";
			} else {
				auto got = getMember(e);
				if (!got)
					continue;
				auto &[type, id] = *got;
				out << type << "& " << id << "(void);";
			}
		}
	}

public:
	static void print(std::ostream &out, std::ostream &impl_out, const std::string &root)
	{
		auto scope = class_name(std::fs::path(root).filename().string());
		out << "class " << scope << std::endl << "{";
		it_dir(out, impl_out, root, scope);
		out << "};";
	}
};

class ResourceCompiler
{
	static void namespace_prologue(std::ostream &out, const std::vector<std::string> &ns)
	{
		for (auto &n : ns)
			out << "namespace " << n << " {" << std::endl;
		if (ns.size())
			out << std::endl;
	}

	static void namespace_epilogue(std::ostream &out, const std::vector<std::string> &ns)
	{
		if (ns.size())
			out << std::endl;
		for (auto &n : ns) {
			static_cast<void>(n);
			out << "}" << std::endl;
		}
	}

public:
	static void run(const std::string &root, const std::vector<std::string> &ns, const std::string &implpath, const std::string &hpath)
	{
		std::stringstream out;

		out << "#pragma once" << std::endl << std::endl;
		out << "#include \"Subtile/Resource/Model.hpp\"" << std::endl;
		out << "#include \"Subtile/Resource/Texture.hpp\"" << std::endl;
		out << std::endl;

		std::stringstream outimpl;

		auto hp = std::fs::path(hpath).filename().string();

		outimpl << "#include \"" << hp << "\"" << std::endl << std::endl;

		namespace_prologue(out, ns);
		namespace_prologue(outimpl, ns);

		FolderPrinter::print(out, outimpl, root);

		namespace_epilogue(out, ns);
		namespace_epilogue(outimpl, ns);

		FileIO::write(hpath, CodeFormatter::format(out.str()));
		FileIO::write(implpath, CodeFormatter::format(outimpl.str()));
	}
};

static auto getOutpath(const std::string &root, const std::string &output, const std::string &ext)
{
	std::stringstream ss;

	ss << output << "/" << std::fs::path(root).filename().string() << ext;
	return ss.str();
}

static auto getArgs(int argc, char **argv)
{
	std::vector<std::string> res;

	for (int i = 1; i < argc; i++)
		res.emplace_back(argv[i]);
	return res;
}

static auto getNs(const std::vector<std::string> &args)
{
	std::vector<std::string> res;

	for (size_t i = 2; i < args.size(); i++)
		res.emplace_back(args.at(i));
	return res;
}

int main(int argc, char **argv)
{
	auto args = getArgs(argc, argv);
	auto &input = args.at(0);
	auto &output = args.at(1);
	auto ns = getNs(args);

	auto in = getOutpath(input, output, ".resdecl.cpp");
	auto out = getOutpath(input, output, ".resdecl.hpp");
	ResourceCompiler::run(input, ns, in, out);

	return 0;
}