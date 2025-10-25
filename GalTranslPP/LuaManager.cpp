module;

#include <toml.hpp>
#include <spdlog/spdlog.h>
#include <sol/sol.hpp>
#include <unicode/unistr.h>
#include <unicode/uchar.h>
#include <unicode/regex.h>

module LuaManager;

namespace fs = std::filesystem;

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
		"other_info_contains", &Sentence::other_info_contains,
		"other_info_get", &Sentence::other_info_get,
		"other_info_set", &Sentence::other_info_set,
		"other_info_get_all", &Sentence::other_info_get_all,
		"other_info_set_all", &Sentence::other_info_set_all,
		"other_info_erase", &Sentence::other_info_erase,
		"other_info_clear", &Sentence::other_info_clear,
		"problems_get_by_index", &Sentence::problems_get_by_index,
		"problems_set_by_index", &Sentence::problems_set_by_index,
		"complete", &Sentence::complete,
		"nameType", &Sentence::nameType,
		"prev", &Sentence::prev,
		"next", &Sentence::next,
		"originalLinebreak", &Sentence::originalLinebreak
	);

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
	auto loggerUsertype = lua.new_usertype<spdlog::logger>(
		"spdlog_logger",
		sol::no_constructor
	);
	loggerUsertype["trace"] = [](spdlog::logger& logger, const std::string& msg)
		{
			logger.trace(msg);
		};
	loggerUsertype["debug"] = [](spdlog::logger& logger, const std::string& msg)
		{
			logger.debug(msg);
		};
	loggerUsertype["info"] = [](spdlog::logger& logger, const std::string& msg)
		{
			logger.info(msg);
		};
	loggerUsertype["warn"] = [](spdlog::logger& logger, const std::string& msg)
		{
			logger.warn(msg);
		};
	loggerUsertype["error"] = [](spdlog::logger& logger, const std::string& msg)
		{
			logger.error(msg);
		};
	loggerUsertype["critical"] = [](spdlog::logger& logger, const std::string& msg)
		{
			logger.critical(msg);
		};
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
