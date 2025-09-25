export module GPPDefines;

export import std;

namespace fs = std::filesystem;

export {

    std::string GPPVERSION = "1.0.3";

    fs::path pluginConfigsPath = L"BaseConfig/pluginConfigs";

    enum  class NameType
    {
        None, Single, Multiple
    };

    struct Sentence {
        int index;
        std::string name;
        std::vector<std::string> names;
        std::string name_preview;
        std::vector<std::string> names_preview;
        std::string original_text;
        std::string pre_processed_text;
        std::string pre_translated_text;
        std::string problem;
        std::string translated_by;
        std::string translated_preview;
        std::map<std::string, std::string> other_info;

        bool complete = false;
        NameType nameType = NameType::None;
        Sentence* prev = nullptr;
        Sentence* next = nullptr;
        std::string originalLinebreak;
    };

    enum  class TransEngine
    {
        None, ForGalJson, ForGalTsv, ForNovelTsv, DeepseekJson, Sakura, DumpName, GenDict, Rebuild, ShowNormal
    };

    enum class CachePart { None, Name, NamePreview, OrigText, PreprocText, PretransText, TransPreview };



}