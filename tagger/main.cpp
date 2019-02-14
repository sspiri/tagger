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



namespace po = boost::program_options;
namespace tl = TagLib;


static po::options_description description{"Available options"};
static po::variables_map options;


std::wstring towstr(const std::string& s){
	std::wstring result(s.size(), L'\0');
	std::size_t size = std::mbstowcs(&result[0], s.data(), s.size());

	if(result.size() > size)
		result.resize(size);

	return result;
}


std::optional<tl::FileRef> make_fileref(const std::string& fp){
	tl::FileRef ref(fp.c_str(), true, tl::AudioProperties::Fast);

	if(ref.isNull()){
		std::cerr << "Error: Cannot open: [" << fp << "]\n";
		return {};
	}

	return std::make_optional(std::move(ref));
}


struct id3_entry{
	std::wstring name;
	std::vector<std::wstring> values;

	id3_entry(const std::wstring& n = L"", const std::vector<std::wstring>& vals = {})
		: name{n}, values{vals} {}

	id3_entry(const std::pair<const tl::String, tl::StringList>& entry)
		: name{entry.first.toWString()}{

		values.reserve(entry.second.size());

		std::transform(entry.second.begin(), entry.second.end(), std::back_inserter(values), [](const tl::String& s){
			return s.toWString();
		});
	}

	void clear(){
		name.clear();
		values.clear();
	}

	std::wstring debug() const{
		std::wstring s = name + L" =";

		for(const auto& value : values)
			s += L" '" + boost::replace_all_copy(value, "'", "\\'") + L'\'';

		return s;
	}

	tl::StringList get_string_list() const{
		tl::StringList list;

		for(const auto& value : values)
			list.append(value);

		return list;
	}
};



std::wistream& parse_name(std::wistream& in, std::wstring& name){
	name.clear();

	wchar_t c{};

	while(in.get(c) && c != L'='){
		if(c == L'\\' && in.peek() == L'='){
			name += L'=';
			in.get();
		}

		else
			name += c;
	}

	if(c == L'='){
		in.putback(c);
		in.clear();
	}

	return in;
}


namespace detail{
	bool is_next(std::wistream& stream, const std::wstring& any){
		auto old_state = stream.rdstate();
		auto old_pos = stream.tellg();

		wchar_t next;
		stream >> next;

		stream.seekg(old_pos, std::ios::beg);
		stream.setstate(old_state);

		return any.find(next) != any.npos;
	}
}


std::wistream& parse_values(std::wistream& in, std::vector<std::wstring>& values){
	values.clear();

	if(detail::is_next(in, L"\"'")){
		std::wstring s;
		wchar_t delim;

		while(detail::is_next(in, L"\"'") && in >> delim && in.putback(delim) >> std::quoted(s, delim))
			values.push_back(std::move(s));

		in.clear();
		return in;
	}

	values.push_back({});

	if(!std::getline(in, values.back(), L';'))
		return in;

	boost::trim_if(values.back(), std::bind(&std::isspace<wchar_t>, std::placeholders::_1, std::locale("")));
	return in;
}



std::wistream& operator>> (std::wistream& in, id3_entry& tag){
	tag.clear();

	if(!parse_name(in, tag.name))
		return in;

	boost::trim_if(tag.name, std::bind(&std::isspace<wchar_t>, std::placeholders::_1, std::locale("")));

	assert(in.peek() == L'=');
	in.get();

	if(!parse_values(in, tag.values))
		return in;

	if(in.peek() == L';')
		in.get();

	in.clear();
	return in;
}


std::vector<id3_entry> make_id3_entries(const tl::PropertyMap& map){
	std::vector<id3_entry> results;
	results.reserve(map.size());

	for(const auto& it : map){
		results.push_back({it.first.toWString(), {}});
		results.back().values.reserve(it.second.size());

		std::transform(it.second.begin(), it.second.end(), std::back_inserter(results.back().values), [](const tl::String& s){
			return s.toWString();
		});
	}

	return results;
}


void show_all(const std::vector<std::string>& files){
	for(const auto& fp : files){
		if(auto ref = make_fileref(fp)){
			std::cout << "For [" << fp << "]:\n";

			for(const auto& entry : make_id3_entries(ref->tag()->properties()))
				std::wcout << entry.debug() << '\n';
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


std::vector<std::wstring> get_ids(const std::wstring& line){
	std::wstringstream wss(line);
	std::vector<std::wstring> res;
	std::wstring id;

	while(std::getline(wss, id, L';'))
		res.push_back(boost::trim_copy_if(id, std::bind(&std::isspace<wchar_t>, std::placeholders::_1, std::locale(""))));

	return res;
}


void get_tags(const std::vector<std::string>& files, const std::wstring& line){
	auto ids = get_ids(line);

	for(const auto& fp : files){
		if(auto ref = make_fileref(fp)){
			auto map = ref->tag()->properties();

			for(const auto& id : ids){
				auto it = map.find(id);

				if(it == map.end())
					std::wcerr << "No tag found for '" << id << "': [" << towstr(fp) << "]\n";
				else
					std::wcout << id3_entry{*it}.debug() << '\n';
			}
		}
	}
}


void set_tags(const std::vector<std::string>& files, const std::wstring& line){
	std::wstringstream wss{line};
	std::vector<id3_entry> entries;
	id3_entry item;

	while(wss >> item)
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



void setup_options_description(){
	description.add_options()
		("help,h", "produce this help message")
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


int process_options(const std::vector<std::string>& unknown){
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

	if(!options.count("get") && !options.count("set") && !options.count("clear")){
		show_all(files);
		return 0;
	}

	if(options.count("get"))
		get_tags(files, towstr(options["get"].as<std::string>()));

	if(options.count("set"))
		set_tags(files, towstr(options["set"].as<std::string>()));

	if(options.count("clear"))
		clear_tags(files);

	return 0;
}


int main(int argc, char** argv) try{
	std::locale::global(std::locale(""));
	std::wcout.imbue(std::locale(""));
	std::wcerr.imbue(std::locale(""));

	setup_options_description();
	return process_options(parse_command_line(argc, argv));
}


catch(const std::exception& error){
	std::cerr << "Error: " << error.what() << '\n';
	return -1;
}
