#include <boost/program_options.hpp>
#include <boost/algorithm/string.hpp>

#include <algorithm>
#include <iterator>
#include <functional>
#include <iostream>
#include <iomanip>
#include <locale>
#include <string>
#include <vector>

#include "taglib/fileref.h"
#include "taglib/tag.h"
#include "taglib/tpropertymap.h"

// qt upgrade
#include <QApplication>
#include "main_window.hpp"


namespace po = boost::program_options;
namespace tl = TagLib;


static po::options_description description{"Available options"};
static po::variables_map options;


std::optional<tl::FileRef> make_fileref(const std::string& fp){
	tl::FileRef ref(fp.c_str(), true, tl::AudioProperties::Fast);

	if(ref.isNull()){
		std::cerr << "Error: Cannot open: [" << fp << "]\n";
		return {};
	}

	return std::make_optional(std::move(ref));
}


struct id3_entry{
	std::string name;
	std::vector<std::string> values;

	id3_entry(const std::string& n = "", const std::vector<std::string>& vals = {})
		: name{n}, values{vals} {}

	id3_entry(const std::pair<const tl::String, tl::StringList>& entry)
		: name{entry.first.to8Bit()}{

		values.reserve(entry.second.size());

		std::transform(entry.second.begin(), entry.second.end(), std::back_inserter(values), [](const tl::String& s){
			return s.to8Bit();
		});
	}

	void clear(){
		name.clear();
		values.clear();
	}

	std::string debug() const{
		std::string s = name + " =";

		for(const auto& value : values)
			s += " '" + boost::replace_all_copy(value, "'", "\\'") + '\'';

		return s;
	}

	tl::StringList get_string_list() const{
		tl::StringList list;

		for(const auto& value : values)
			list.append(value);

		return list;
	}
};



std::istream& parse_name(std::istream& in, std::string& name){
	name.clear();

	char c{};

	while(in.get(c) && c != '='){
		if(c == '\\' && in.peek() == '='){
			name += '=';
			in.get();
		}

		else
			name += c;
	}

	if(c == '='){
		in.putback(c);
		in.clear();
	}

	return in;
}


namespace detail{
	bool is_next(std::istream& stream, const std::string& any){
		auto old_state = stream.rdstate();
		auto old_pos = stream.tellg();

		char next;
		stream >> next;

		stream.seekg(old_pos, std::ios::beg);
		stream.setstate(old_state);

		return any.find(next) != any.npos;
	}
}


std::istream& parse_values(std::istream& in, std::vector<std::string>& values){
	values.clear();

	if(detail::is_next(in, "\"'")){
		std::string s;
		char delim;

		while(detail::is_next(in, "\"'") && in >> delim && in.putback(delim) >> std::quoted(s, delim))
			values.push_back(std::move(s));

		in.clear();
		return in;
	}

	values.push_back({});

	if(!std::getline(in, values.back(), ';'))
		return in;

	boost::trim_if(values.back(), std::bind(&std::isspace<char>, std::placeholders::_1, std::locale("")));
	return in;
}



std::istream& operator>> (std::istream& in, id3_entry& tag){
	tag.clear();

	if(!parse_name(in, tag.name))
		return in;

	boost::trim_if(tag.name, std::bind(&std::isspace<char>, std::placeholders::_1, std::locale("")));

	assert(in.peek() == '=');
	in.get();

	if(!parse_values(in, tag.values))
		return in;

	if(in.peek() == ';')
		in.get();

	in.clear();
	return in;
}


std::vector<id3_entry> make_id3_entries(const tl::PropertyMap& map){
	std::vector<id3_entry> results;
	results.reserve(map.size());

	for(const auto& it : map){
		results.push_back({it.first.to8Bit(), {}});
		results.back().values.reserve(it.second.size());

		std::transform(it.second.begin(), it.second.end(), std::back_inserter(results.back().values), [](const tl::String& s){
			return s.to8Bit();
		});
	}

	return results;
}


void show_all(const std::vector<std::string>& files){
	for(const auto& fp : files){
		if(auto ref = make_fileref(fp)){
			std::cout << "For [" << fp << "]:\n";

			for(const auto& entry : make_id3_entries(ref->tag()->properties()))
				std::cout << entry.debug() << '\n';
		}
	}
}



void clear_tags(const std::vector<std::string>& files){
	for(const auto& fp : files){
		if(auto ref = make_fileref(fp)){
			ref->tag()->setProperties({});

			if(!ref->save())
				std::cerr << "Error: Cannot write contents: [" << fp << "]\n";
		}
	}
}


std::vector<std::string> get_ids(const std::string& line){
	std::stringstream wss(line);
	std::vector<std::string> res;
	std::string id;

	while(std::getline(wss, id, ';'))
		res.push_back(boost::trim_copy_if(id, std::bind(&std::isspace<char>, std::placeholders::_1, std::locale(""))));

	return res;
}


void get_tags(const std::vector<std::string>& files, const std::string& line){
	auto ids = get_ids(line);

	for(const auto& fp : files){
		if(auto ref = make_fileref(fp)){
			auto map = ref->tag()->properties();

			for(const auto& id : ids){
				auto it = map.find(id);

				if(it == map.end())
					std::cerr << "No tag found for '" << id << "': [" << fp << "]\n";
				else
					std::cout << id3_entry{*it}.debug() << '\n';
			}
		}
	}
}


void set_tags(const std::vector<std::string>& files, const std::string& line){
	std::stringstream ss{line};
	std::vector<id3_entry> entries;
	id3_entry item;

	while(ss >> item)
		entries.push_back(std::move(item));

	for(const auto& fp : files){
		if(auto ref = make_fileref(fp)){
			auto map = ref->tag()->properties();

			for(const auto& item : entries){
				auto it = map.find(item.name);

				if(it == map.end())
					map.insert(item.name, item.get_string_list());
				else
					it->second = item.get_string_list();
			}

			ref->tag()->setProperties(map);

			if(!ref->save())
				std::cerr << "Error: Cannot write contents: [" << fp << "]\n";
		}
	}
}


int launch_qt_session(const std::vector<std::string>& file_paths, int argc, char** argv){
	QApplication ui{argc, argv};

	main_window window{file_paths};
	window.setMinimumWidth(500);
	window.show();

	return ui.exec();
}



void setup_options_description(){
	description.add_options()
		("help,h", "produce this help message")
		("qt,q", po::value<bool>()->zero_tokens()->default_value(false), "launch a graphical user interface")
		("file,i", po::value<std::vector<std::string>>()->multitoken(), "specify one or more files to operate on")
		("get,g", po::value<std::string>(), "get IDx tag information")
		("set,s", po::value<std::string>(), "set IDx tag information")
		("clear,c", "clear all tags")
	;
}


std::vector<std::string> parse_command_line(int argc, char** argv){
	auto parsed = po::command_line_parser(argc, argv).options(description).allow_unregistered().run();
	po::store(parsed, options);
	return po::collect_unrecognized(parsed.options, po::include_positional);
}


int process_options(const std::vector<std::string>& unknown, int argc, char** argv){
	if(argc == 1)	// no option specified
		return launch_qt_session({}, argc, argv);

	if(!options.count("file") && !unknown.size()){
		std::cerr << description;
		return -2;
	}

	if(options.count("help")){
		std::cerr << description;
		return 0;
	}

	std::vector<std::string> files;

	if(options.count("file"))
		files = options["file"].as<std::vector<std::string>>();

	files.reserve(files.size() + unknown.size());
	std::copy(unknown.begin(), unknown.end(), std::back_inserter(files));

	if(options["qt"].as<bool>())
		return launch_qt_session(files, argc, argv);

	if(!options.count("get") && !options.count("set") && !options.count("clear")){
		show_all(files);
		return 0;
	}

	if(options.count("get"))
		get_tags(files, options["get"].as<std::string>());

	if(options.count("set"))
		set_tags(files, options["set"].as<std::string>());

	if(options.count("clear"))
		clear_tags(files);

	return 0;
}


int main(int argc, char** argv) try{
	std::locale::global(std::locale(""));
	std::cout.imbue(std::locale(""));
	std::cerr.imbue(std::locale(""));

	setup_options_description();
	return process_options(parse_command_line(argc, argv), argc, argv);
}


catch(const std::exception& error){
	std::cerr << "Error: " << error.what() << '\n';
	return -1;
}
