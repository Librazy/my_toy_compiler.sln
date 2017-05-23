#pragma once
//https://github.com/0x00A/debug

#include <sstream>
#include <functional>


class debug_stream : public std::ostream
{
public:
	using level = enum
	{
		quiet,
		error,
		warn,
		info,
		verbose
	};

	struct indent
	{
		const int value;
		const int mode;
		constexpr indent(int value, int mode): value(value), mode(mode) {}
	};

private:
	using stream_formatter = std::function<
		std::string (
			const std::string& name,
			const std::string& level,
			const int indent,
			const std::string& data)
	>;

	stream_formatter formatter;
	bool has_formatter = false;

	class buffer : public std::stringbuf
	{
		std::ostream& output;
		int indent;
	public:
		debug_stream* debug;
		buffer(std::ostream& output, debug_stream* debug) : output(output), indent(0), debug(debug) {}

		int sync() override
		{
			static const char* labels[4] = {"error  ", "warn   ", "info   ", "verbose"};
			if (debug->current_level <= debug->max_level) {
				if (debug->has_formatter) {
					auto name = debug->name;
					auto level = labels[debug->current_level - 1];
					output << debug->formatter(name, level, indent, str());
				} else {
					output << debug->name << " "
							<< labels[debug->current_level - 1] << " "
							<< std::string(indent, ' ')
							<< str();
				}
			}
			str("");
			output.flush();
			return 0;
		}

		friend debug_stream& operator <<(debug_stream&, const debug_stream::indent&);
	};

	buffer buffer;

public:

	level current_level = verbose;
	level max_level = verbose;
	std::string name;

	void format(const stream_formatter& f)
	{
		has_formatter = true;
		formatter = f;
	}

	debug_stream(const std::string& name, level maxLevel, std::ostream& stream) :
		std::ostream(&buffer),
		buffer(stream, this),
		max_level(maxLevel),
		name(name)
	{ }

	friend debug_stream& operator <<(debug_stream&, const indent&);
};

inline debug_stream& operator <<(debug_stream& d, const debug_stream::level& level)
{
	d.current_level = level;
	return d;
}

inline debug_stream& operator <<(debug_stream& d, const debug_stream::indent& indent)
{
	switch (indent.mode) {
	case 0: d.buffer.indent = indent.value;
		break;
	case -1: d.buffer.indent -= indent.value;
		break;
	default: d.buffer.indent += indent.value;
		break;
	}
	return d;
}

template <class _Elem,
          class _Traits>
debug_stream& operator <<(debug_stream& d, std::function<std::basic_ostream<_Elem, _Traits>&(std::basic_ostream<_Elem, _Traits>&)> func)
{
	return func(d);
}
