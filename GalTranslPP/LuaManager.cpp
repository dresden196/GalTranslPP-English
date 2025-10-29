module;

#define _RANGES_
#include <toml.hpp>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>
#include <sol/sol.hpp>
#include <unicode/unistr.h>
#include <unicode/uchar.h>
#include <unicode/regex.h>
#define NESTED_CVT(className, memberName) sol::property([](className& self, sol::this_state s) \
{ \
	return sol::nested<decltype(className::memberName)>(self.memberName); \
}, [](className& self, sol::this_state s, decltype(className::memberName) table) { self.memberName = std::move(table); }) 

module LuaManager;

import PythonManager;

namespace fs = std::filesystem;
using json = nlohmann::json;

class LuaJson {
public:

	// 从 sol::object 转换到 json 的辅助函数
	static json solObj2JsonValue(sol::object obj) {
		sol::type type = obj.get_type();
		switch (type) {
		case sol::type::string:
			return obj.as<std::string>();
		case sol::type::number:
			if (obj.is<int64_t>()) {
				return obj.as<int64_t>();
			}
			return obj.as<double>();
		case sol::type::boolean:
			return obj.as<bool>();
		case sol::type::table: {
			sol::table luaTable = obj.as<sol::table>();
			bool arrayLike = true;
			if (luaTable.empty()) {
				// 如果是空表，我们默认它是一个数组。
				// 这是一个约定，你也可以选择默认为 table。
				// 对于 j 来说，空的 array [] 更常见。
				arrayLike = true;
			}
			else {
				// 遍历所有键，检查它们是否都是从1开始的连续整数
				size_t expectedKey = 1;
				for (auto& kv : luaTable) {
					if (!kv.first.is<lua_Integer>() || kv.first.as<lua_Integer>() != expectedKey) {
						arrayLike = false;
						break;
					}
					expectedKey++;
				}
				// 确保 Lua table 的 #size 和我们遍历的元素数量一致
				arrayLike &= (luaTable.size() == expectedKey - 1);
			}

			if (arrayLike) {
				json arr = json::array();
				for (auto& kv : luaTable) {
					arr.push_back(solObj2JsonValue(kv.second));
				}
				return arr;
			}
			else { // 否则，当作字典处理
				json tbl = json::object();
				for (auto& kv : luaTable) {
					// 确保键是字符串，因为 j 的键必须是字符串
					if (kv.first.is<std::string>()) {
						tbl[kv.first.as<std::string>()] = solObj2JsonValue(kv.second);
					}
					else {
						tbl["LuaJson: non-string key"] = solObj2JsonValue(kv.second);
					}
				}
				return tbl;
			}
		}
		default:
			return "LuaJson: unsupported type";
		}
	}

	// 递归转换函数：将 json 转换为 sol::object
	static sol::object jsonValue2SolObject(const json& value, sol::state_view lua) {
		// 检查节点类型并进行相应转换
		switch (value.type()) {
		case json::value_t::string:
			return sol::make_object(lua, value.get<std::string>());
		case json::value_t::number_integer:
			return sol::make_object(lua, value.get<int64_t>());
		case json::value_t::number_float:
			return sol::make_object(lua, value.get<double>());
		case json::value_t::boolean:
			return sol::make_object(lua, value.get<bool>());
		case json::value_t::array: {
			sol::table resultArray = lua.create_table();
			for (const auto& elem : value) {
				resultArray.add(jsonValue2SolObject(elem, lua));
			}
			return sol::make_object(lua, resultArray);
		}
		case json::value_t::object: {
			sol::table resultMap = lua.create_table();
			for (const auto& [key, val] : value.items()) {
				resultMap[key] = jsonValue2SolObject(val, lua);
			}
			return sol::make_object(lua, resultMap);
		}
		default:
			return sol::make_object(lua, sol::nil);
		}
	}
};

class LuaToml {
public:

	// 从 sol::object 转换到 toml::node 的辅助函数
	static toml::value solObj2TomlValue(sol::object obj) {
		sol::type type = obj.get_type();
		switch (type) {
		case sol::type::string:
			return toml::value(obj.as<std::string>());
		case sol::type::number:
			if (obj.is<int64_t>()) {
				return toml::value(obj.as<int64_t>());
			}
			return toml::value(obj.as<double>());
		case sol::type::boolean:
			return toml::value(obj.as<bool>());
		case sol::type::table: {
			sol::table luaTable = obj.as<sol::table>();
			bool arrayLike = true;
			if (luaTable.empty()) {
				// 如果是空表，我们默认它是一个数组。
				// 这是一个约定，你也可以选择默认为 table。
				// 对于 toml 来说，空的 array [] 更常见。
				arrayLike = true;
			}
			else {
				// 遍历所有键，检查它们是否都是从1开始的连续整数
				size_t expectedKey = 1;
				for (auto& kv : luaTable) {
					if (!kv.first.is<lua_Integer>() || kv.first.as<lua_Integer>() != expectedKey) {
						arrayLike = false;
						break;
					}
					expectedKey++;
				}
				// 确保 Lua table 的 #size 和我们遍历的元素数量一致
				arrayLike &= (luaTable.size() == expectedKey - 1);
			}

			if (arrayLike) {
				toml::array arr;
				for (auto& kv : luaTable) {
					arr.push_back(solObj2TomlValue(kv.second));
				}
				return arr;
			}
			else { // 否则，当作字典处理
				toml::table tbl;
				for (auto& kv : luaTable) {
					// 确保键是字符串，因为 TOML 的键必须是字符串
					if (kv.first.is<std::string>()) {
						tbl.insert({ kv.first.as<std::string>(), solObj2TomlValue(kv.second) });
					}
					else {
						tbl.insert({ "LuaToml: non-string key", solObj2TomlValue(kv.second) });
					}
				}
				return tbl;
			}
		}
		default:
			return toml::value{ "LuaToml: unsupported type" };
		}
	}

	// 递归转换函数：将 toml::value 转换为 sol::object
	static sol::object tomlValue2SolObject(const toml::value& value, sol::state_view lua) {
		// 检查节点类型并进行相应转换
		if (value.is_table()) {
			sol::table resultMap = lua.create_table();
			for (const auto& [key, val] : value.as_table()) {
				resultMap[key] = tomlValue2SolObject(val, lua);
			}
			return resultMap;
		}
		else if (value.is_array()) {
			sol::table resultVec = lua.create_table();
			for (const auto& elem : value.as_array()) {
				resultVec.add(tomlValue2SolObject(elem, lua));
			}
			return resultVec;
		}
		else if (value.is_string()) {
			return sol::make_object(lua, value.as_string());
		}
		else if (value.is_integer()) {
			return sol::make_object(lua, value.as_integer());
		}
		else if (value.is_floating()) {
			return sol::make_object(lua, value.as_floating());
		}
		else if (value.is_boolean()) {
			return sol::make_object(lua, value.as_boolean());
		}
		// 对于其他类型（如 toml::date, toml::time），我们返回 nil
		return sol::make_object(lua, sol::nil);
	}
};

std::optional<std::shared_ptr<LuaStateInstance>> LuaManager::registerFunction(const std::string& scriptPath, const std::string& functionName, bool& needReboot) {
	const fs::path stdScriptPath = fs::weakly_canonical(ascii2Wide(scriptPath));
	if (!fs::exists(stdScriptPath)) {
		m_logger->error("Script is not exist: {}", scriptPath);
		return std::nullopt;
	}
	auto it = m_scriptStates.find(stdScriptPath);
	if (it == m_scriptStates.end()) {
		try {
			auto& state = m_scriptStates[stdScriptPath];
			state = std::make_shared<LuaStateInstance>();
			state->lua->open_libraries();
			std::ifstream ifs(stdScriptPath);
			std::string script = std::string(std::istreambuf_iterator<char>(ifs), std::istreambuf_iterator<char>());
			state->lua->script(script);
			registerCustomTypes(state, scriptPath, needReboot);
			it = m_scriptStates.find(stdScriptPath);
		}
		catch (const sol::error& e) {
			m_logger->error("Failed to load script {}: {}", scriptPath, e.what());
			return std::nullopt;
		}
	}

	if (functionName.empty()) {
		return it->second;
	}

	if (!it->second->functions.contains(functionName)) {
		sol::function func = (*(it->second->lua))[functionName];
		if (!func.valid()) {
			m_logger->debug("Failed to find function {} in script {}", functionName, scriptPath);
			return std::nullopt;
		}
		it->second->functions[functionName] = func;
	}

	return it->second;
}

void LuaManager::registerCustomTypes(std::shared_ptr<LuaStateInstance> luaStateInstance, const std::string& scriptPath, bool& needReboot) {
	sol::state& lua = *(luaStateInstance->lua);
	// 绑定 NameType 枚举
	lua.new_enum("NameType",
		"None", NameType::None,
		"Single", NameType::Single,
		"Multiple", NameType::Multiple
	);

	lua.new_enum("TransEngine",
		"None", TransEngine::None,
		"ForGalJson", TransEngine::ForGalJson,
		"ForGalTsv", TransEngine::ForGalTsv,
		"ForNovelTsv", TransEngine::ForNovelTsv,
		"DeepseekJson", TransEngine::DeepseekJson,
		"Sakura", TransEngine::Sakura,
		"DumpName", TransEngine::DumpName,
		"GenDict", TransEngine::GenDict,
		"Rebuild", TransEngine::Rebuild,
		"ShowNormal", TransEngine::ShowNormal
	);

	// 绑定 Sentence 结构体
	lua.new_usertype<Sentence>("Sentence",
		sol::constructors<Sentence()>(), // 允许在 Lua 中创建 Sentence
		"index", &Sentence::index,
		"name", &Sentence::name,
		"names", &Sentence::names,
		"name_preview", &Sentence::name_preview,
		"names_preview", &Sentence::names_preview,
		"original_text", &Sentence::original_text,
		"pre_processed_text", &Sentence::pre_processed_text,
		"pre_translated_text", &Sentence::pre_translated_text,
		"problems", &Sentence::problems,
		"translated_by", &Sentence::translated_by,
		"translated_preview", &Sentence::translated_preview,
		"other_info", NESTED_CVT(Sentence, other_info),
		"problems_get_by_index", &Sentence::problems_get_by_index,
		"problems_set_by_index", &Sentence::problems_set_by_index,
		"complete", &Sentence::complete,
		"notAnalyzeProblem", &Sentence::notAnalyzeProblem,
		"nameType", &Sentence::nameType,
		"prev", &Sentence::prev,
		"next", &Sentence::next,
		"originalLinebreak", &Sentence::originalLinebreak
	);

	lua.new_usertype<fs::path>("Path",
		sol::meta_function::construct,
		sol::factories([](const std::string& str)
			{
				return fs::path(ascii2Wide(str));
			},
			[]()
			{
				return fs::path();
			},
			[](const fs::path& p)
			{
				return fs::path(p);
			}),
		"value", sol::property([](const fs::path& self)
			{
				return wide2Ascii(self);
			},
			[](fs::path& self, const std::string& str)
			{
				self = ascii2Wide(str);
			}),
		sol::meta_function::to_string,
		[](const fs::path& self)
		{
			return wide2Ascii(self);
		},
		sol::meta_function::division, sol::overload(
			[](const fs::path& self, const fs::path& other) { return self / other; },
			[](const fs::path& self, const std::string& other) { return self / ascii2Wide(other); },
			[](const std::string& self, const fs::path& other) { return ascii2Wide(self) / other; }),
		sol::meta_function::equal_to, [](const fs::path& self, const fs::path& other) { return self == other; },
		"filename", sol::property([](const fs::path& self) { return self.filename(); }),
		"stem", sol::property([](const fs::path& self) { return self.stem(); }),
		"extension", sol::property([](const fs::path& self) { return self.extension(); }),
		"parent_path", sol::property([](const fs::path& self) { return self.parent_path(); }),
		"empty", sol::property([](const fs::path& self) { return self.empty(); }),
		"is_absolute", sol::property([](const fs::path& self) { return self.is_absolute(); }),
		"is_relative", sol::property([](const fs::path& self) { return self.is_relative(); }),
		"equivalent", [](const fs::path& self, const fs::path& other) { return fs::equivalent(self, other); },
		"weakly_canonical", [](const fs::path& self) { return fs::weakly_canonical(self); },
		"canonical", [](const fs::path& self) { return fs::canonical(self); },
		"relative_to", [](const fs::path& self, const fs::path& base) { return fs::relative(self, base); }
	);

	sol::table luaTomlTable = lua.create_named_table("toml");
	auto luaTomlParseFunc = [](fs::path path, sol::this_state s)
		{
			sol::state_view lua = s;
			try {
				toml::value tomlValue = toml::parse(path);
				return LuaToml::tomlValue2SolObject(tomlValue, lua);
			}
			catch (const toml::exception&) {
				return sol::make_object(lua, sol::nil);
			}
		};
	luaTomlTable["parse"] = sol::overload(luaTomlParseFunc, [=](const std::string& str, sol::this_state s) { return luaTomlParseFunc(ascii2Wide(str), s); });
	auto luaTomlSaveFunc = [](const fs::path& path, sol::object obj) -> std::optional<std::string>
		{
			toml::value tomlValue = LuaToml::solObj2TomlValue(obj);
			try {
				std::ofstream ofs(path);
				if (!ofs.is_open()) {
					std::string err = std::format("Failed to open file for writing: {}", wide2Ascii(path));
					return err;
				}
				ofs << toml::format(tomlValue);
				ofs.close();
				return std::nullopt;
			}
			catch (const toml::exception& e) {
				std::string err = std::format("Failed to save toml: {}", e.what());
				return err;
			}
		};
	luaTomlTable["save"] = sol::overload(luaTomlSaveFunc, [=](const std::string& str, sol::object obj) { return luaTomlSaveFunc(ascii2Wide(str), obj); });

	auto luaJsonTable = lua.create_named_table("json");
	auto luaJsonParseFunc = [](const fs::path& path, sol::this_state s)
		{
			sol::state_view lua = s;
			try {
				std::ifstream ifs(path);
				json jsonValue = json::parse(ifs);
				return LuaJson::jsonValue2SolObject(jsonValue, lua);
			}
			catch (const std::exception&) {
				return sol::make_object(lua, sol::nil);
			}
		};
	luaJsonTable["parse"] = sol::overload(luaJsonParseFunc, [=](const std::string& str, sol::this_state s) { return luaJsonParseFunc(ascii2Wide(str), s); });
	auto luaJsonSaveFunc = [](const fs::path& path, sol::object obj, sol::optional<int> indent) -> std::optional<std::string>
		{
			json jsonValue = LuaJson::solObj2JsonValue(obj);
			try {
				std::ofstream ofs(path);
				if (!ofs.is_open()) {
					std::string err = std::format("Failed to open file for writing: {}", wide2Ascii(path));
					return err;
				}
				ofs << jsonValue.dump(indent.value_or(2));
				ofs.close();
				return std::nullopt;
			}
			catch (const std::exception& e) {
				std::string err = std::format("Failed to save json: {}", e.what());
				return err;
			}
		};
	luaJsonTable["save"] = sol::overload(luaJsonSaveFunc, [=](const std::string& str, sol::object obj, sol::optional<int> indent) { return luaJsonSaveFunc(ascii2Wide(str), obj, indent); });

	// 绑定 utils 库
	sol::table utilsTable = lua.create_named_table("utils");
	utilsTable["ascii2Ascii"] = [](const std::string& ascii, std::optional<int> src, std::optional<int> dst) -> std::tuple<std::string, bool>
		{
			BOOL usedDefaultChar = FALSE;
			std::string resultStr = ascii2Ascii(ascii, (UINT)src.value_or(65001), (UINT)dst.value_or(0), &usedDefaultChar);
			return std::make_tuple(resultStr, (bool)usedDefaultChar);
		};
	utilsTable["executeCommand"] = [](const std::string& program, const std::string& args, std::optional<bool> showWindow) -> bool
		{
			return executeCommand(ascii2Wide(program), ascii2Wide(args), showWindow.value_or(false));
		};
	utilsTable["getConsoleWidth"] = &getConsoleWidth;
	utilsTable["createParent"] = [](const std::string& path) -> bool
		{
			return createParent(ascii2Wide(path));
		};
	utilsTable["splitString"] = [](const std::string& str, const std::string& delimiter) -> std::vector<std::string>
		{
			return splitString(str, delimiter);
		};
	utilsTable["isSameExtension"] = [](const std::string& path, const std::string& ext) -> bool
		{
			return isSameExtension(ascii2Wide(path), ascii2Wide(ext));
		};
	utilsTable["removePunctuation"] = &removePunctuation;
	utilsTable["removeWhitespace"] = &removeWhitespace;
	utilsTable["getMostCommonChar"] = [](const std::string& str) -> std::tuple<std::string, int>
		{
			auto pair = getMostCommonChar(str);
			return std::make_tuple(pair.first, pair.second);
		};
	utilsTable["splitIntoTokens"] = &::splitIntoTokens;
	utilsTable["splitIntoGraphemes"] = &splitIntoGraphemes;
	utilsTable["countGraphemes"] = &countGraphemes;
	utilsTable["countSubstring"] = &countSubstring;
	utilsTable["getSubstringPositions"] = &getSubstringPositions;
	utilsTable["replaceStr"] = [](const std::string& str, const std::string& org, const std::string& rep) -> std::string
		{
			std::string result = str;
			return replaceStrInplace(result, org, rep);
		};
	utilsTable["extractKatakana"] = &extractKatakana;
	utilsTable["extractKana"] = &extractKana;
	utilsTable["extractLatin"] = &extractLatin;
	utilsTable["extractHangul"] = &extractHangul;
	utilsTable["traditionalChineseExtractor"] = getTraditionalChineseExtractor(m_logger);
	utilsTable["extractZip"] = [](const std::string& zipPath, const std::string& outputDir)
		{
			extractZip(ascii2Wide(zipPath), ascii2Wide(outputDir));
		};
	utilsTable["extractFileFromZip"] = [](const std::string& zipPath, const std::string& outputDir, const std::string& fileName)
		{
			extractFileFromZip(ascii2Wide(zipPath), ascii2Wide(outputDir), fileName);
		};
	utilsTable["extractZipExclude"] = [](const std::string& zipPath, const std::string& outputDir, std::vector<std::string> excludePrefixes)
		{
			std::set<std::string> excludeSet(excludePrefixes.begin(), excludePrefixes.end());
			extractZipExclude(ascii2Wide(zipPath), ascii2Wide(outputDir), excludeSet);
		};
	utilsTable["icuRegexMatch"] = [](const std::string& str, const std::string& pattern) -> bool
		{
			icu::UnicodeString ustr(icu::UnicodeString::fromUTF8(pattern));
			UErrorCode status = U_ZERO_ERROR;
			std::unique_ptr<icu::RegexPattern> regexPattern = std::unique_ptr<icu::RegexPattern>(icu::RegexPattern::compile(ustr, 0, status));
			if (U_FAILURE(status)) {
				throw std::runtime_error(std::format("Failed to compile regex pattern: {}", pattern));
			}
			std::unique_ptr<icu::RegexMatcher> matcher = std::unique_ptr<icu::RegexMatcher>(regexPattern->matcher(icu::UnicodeString::fromUTF8(str), status));
			if (U_FAILURE(status)) {
				throw std::runtime_error(std::format("Failed to create regex matcher: {}", pattern));
			}
			return (bool)matcher->matches(status);
		};
	utilsTable["icuRegexSearch"] = [](const std::string& str, const std::string& pattern) -> std::vector<std::vector<std::string>>
		{
			icu::UnicodeString ustr(icu::UnicodeString::fromUTF8(pattern));
			UErrorCode status = U_ZERO_ERROR;
			std::unique_ptr<icu::RegexPattern> regexPattern = std::unique_ptr<icu::RegexPattern>(icu::RegexPattern::compile(ustr, 0, status));
			if (U_FAILURE(status)) {
				throw std::runtime_error(std::format("Failed to compile regex pattern: {}", pattern));
			}
			std::unique_ptr<icu::RegexMatcher> matcher = std::unique_ptr<icu::RegexMatcher>(regexPattern->matcher(icu::UnicodeString::fromUTF8(str), status));
			if (U_FAILURE(status)) {
				throw std::runtime_error(std::format("Failed to create regex matcher: {}", pattern));
			}
			std::vector<std::vector<std::string>> result;
			while (matcher->find(status) && U_SUCCESS(status)) {
				std::vector<std::string> matches;
				for (int32_t i = 0; i <= matcher->groupCount(); i++) {
					icu::UnicodeString uGroup = matcher->group(i, status);
					if (U_FAILURE(status)) {
						throw std::runtime_error(std::format("Failed to get group {} of regex match: {}", i, pattern));
					}
					std::string group;
					matches.push_back(std::move(uGroup.toUTF8String(group)));
				}
				result.push_back(std::move(matches));
			}
			return result;
		};
	utilsTable["icuRegexReplace"] = [](const std::string& str, const std::string& pattern, const std::string& rep) -> std::string
		{
			icu::UnicodeString ustr(icu::UnicodeString::fromUTF8(str));
			UErrorCode status = U_ZERO_ERROR;
			std::unique_ptr<icu::RegexPattern> regexPattern = std::unique_ptr<icu::RegexPattern>(icu::RegexPattern::compile(icu::UnicodeString::fromUTF8(pattern), 0, status));
			if (U_FAILURE(status)) {
				throw std::runtime_error(std::format("Failed to compile regex pattern: {}", pattern));
			}
			std::unique_ptr<icu::RegexMatcher> matcher = std::unique_ptr<icu::RegexMatcher>(regexPattern->matcher(ustr, status));
			if (U_FAILURE(status)) {
				throw std::runtime_error(std::format("Failed to create regex matcher: {}", pattern));
			}
			std::string result;
			return matcher->replaceAll(icu::UnicodeString::fromUTF8(rep), status).toUTF8String(result);
		};

	// 绑定 spdlog::logger
	lua.new_enum("spdlogLevel",
		"trace", spdlog::level::trace,
		"debug", spdlog::level::debug,
		"info", spdlog::level::info,
		"warn", spdlog::level::warn,
		"err", spdlog::level::err,
		"critical", spdlog::level::critical
	);
	lua.new_usertype<spdlog::logger>(
		"spdlogLogger",
		sol::no_constructor,
		"name", &spdlog::logger::name,
		"level", &spdlog::logger::level,
		"set_level", &spdlog::logger::set_level,
		"set_pattern", [](spdlog::logger& logger, const std::string& pattern) { logger.set_pattern(pattern); },
		"flush", &spdlog::logger::flush,
		"trace", [](spdlog::logger& logger, const std::string& msg) { logger.trace(msg); },
		"debug", [](spdlog::logger& logger, const std::string& msg) { logger.debug(msg); },
		"info", [](spdlog::logger& logger, const std::string& msg) { logger.info(msg); },
		"warn", [](spdlog::logger& logger, const std::string& msg) { logger.warn(msg); },
		"error", [](spdlog::logger& logger, const std::string& msg) { logger.error(msg); },
		"critical", [](spdlog::logger& logger, const std::string& msg) { logger.critical(msg); }
	);
	utilsTable["logger"] = m_logger;

	auto supplyTokenizerFunc = [&](const std::string& mode)
		{
			if (lua[mode + "useTokenizer"].get_or(false)) {
				const std::string& tokenizerBackend = lua[mode + "tokenizerBackend"].get<std::string>();
				if (tokenizerBackend == "MeCab") {
					const std::string& mecabDictDir = lua[mode + "mecabDictDir"].get<std::string>();
					m_logger->info("{} 正在检查 MeCab 环境...", scriptPath);
					utilsTable[mode + "tokenizeFunc"] = getMeCabTokenizeFunc(mecabDictDir, m_logger);
					m_logger->info("{} MeCab 环境检查完毕。", scriptPath);
				}
				else if (tokenizerBackend == "spaCy") {
					const std::string& spaCyModelName = lua[mode + "spaCyModelName"].get<std::string>();
					m_logger->info("{} 正在检查 spaCy 环境...", scriptPath);
					utilsTable[mode + "tokenizeFunc"] = getNLPTokenizeFunc({ "spacy" }, "tokenizer_spacy", spaCyModelName, m_logger, needReboot);
					m_logger->info("{} spaCy 环境检查完毕。", scriptPath);
				}
				else if (tokenizerBackend == "Stanza") {
					const std::string& stanzaLang = lua[mode + "stanzaLang"].get<std::string>();
					m_logger->info("{} 正在检查 Stanza 环境...", scriptPath);
					utilsTable[mode + "tokenizeFunc"] = getNLPTokenizeFunc({ "stanza" }, "tokenizer_stanza", stanzaLang, m_logger, needReboot);
					m_logger->info("{} Stanza 环境检查完毕。", scriptPath);
				}
				else if (tokenizerBackend == "pkuseg") {
					m_logger->info("{} 正在检查 pkuseg 环境...", scriptPath);
					utilsTable[mode + "tokenizeFunc"] = getNLPTokenizeFunc({ "setuptools", "nes-py", "cython", "pkuseg" }, "tokenizer_pkuseg", "default", m_logger, needReboot);
					m_logger->info("{} pkuseg 环境检查完毕。", scriptPath);
				}
				else {
					throw std::invalid_argument(scriptPath + " 无效的 tokenizerBackend: " + tokenizerBackend);
				}
			}
		};
	supplyTokenizerFunc("sourceLang_");
	supplyTokenizerFunc("targetLang_");

}

LuaTextPlugin::LuaTextPlugin(const fs::path& projectDir, const std::string& scriptPath, LuaManager& luaManager, std::shared_ptr<spdlog::logger> logger)
	: IPlugin(projectDir, logger), m_scriptPath(scriptPath)
{
	m_logger->info("正在初始化 Lua 插件 {}", scriptPath);
	std::optional<std::shared_ptr<LuaStateInstance>> luaStateOpt = luaManager.registerFunction(scriptPath, "init", m_needReboot);
	if (!luaStateOpt.has_value()) {
		throw std::runtime_error(scriptPath + " init函数 初始化失败");
	}
	luaStateOpt = luaManager.registerFunction(scriptPath, "run", m_needReboot);
	if (!luaStateOpt.has_value()) {
		throw std::runtime_error(scriptPath + " run函数 初始化失败");
	}
	m_luaState = luaStateOpt.value();
	m_luaRunFunc = m_luaState->functions["run"];
	luaManager.registerFunction(scriptPath, "unload", m_needReboot);

	try {
		m_luaState->functions["init"](wide2Ascii(projectDir));
	}
	catch (const sol::error& e) {
		m_logger->error("{} init函数 执行失败", scriptPath);
		throw std::runtime_error(e.what());
	}

	m_logger->info("{} 初始化成功", scriptPath);
}

LuaTextPlugin::~LuaTextPlugin()
{
	sol::function unloadFunc = m_luaState->functions["unload"];
	if (unloadFunc.valid()) {
		try {
			std::lock_guard<std::mutex> lock(m_luaState->executionMutex);
			unloadFunc();
		}
		catch (const sol::error& e) {
			m_logger->error("Error while unloading script: {}", e.what());
		}
	}
}

void LuaTextPlugin::run(Sentence* se) {
	std::lock_guard<std::mutex> lock(m_luaState->executionMutex);
	try {
		m_luaRunFunc(se);
	}
	catch (const sol::error& e) {
		m_logger->error("{} run函数 执行失败", m_scriptPath);
		throw std::runtime_error(e.what());
	}
}
