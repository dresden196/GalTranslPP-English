export module GPPDefines;

export import std;

namespace fs = std::filesystem;

export {

    std::string GPPVERSION = "1.0.1P";

    fs::path pluginConfigsPath = L"BaseConfig/pluginConfigs";

    struct Sentence {
        int index;
        std::string name;
        std::string name_preview;
        std::string original_text;
        std::string pre_processed_text;
        std::string pre_translated_text;
        std::string problem;
        std::string translated_by;
        std::string translated_preview;
        std::map<std::string, std::string> other_info;

        bool complete = false;
        bool hasName = false;
        Sentence* prev = nullptr;
        Sentence* next = nullptr;
        std::string originalLinebreak;
    };

    enum  class TransEngine
    {
        ForGalJson, ForGalTsv, ForNovelTsv, DeepseekJson, Sakura, DumpName, GenDict, Rebuild, ShowNormal
    };



}