module;

#define PYBIND11_HEADERS
#include "GPPMacros.hpp"
#include <mecab/mecab.h>
#include <spdlog/spdlog.h>

export module NLPTool;

import Tool;
import PythonManager;

namespace fs = std::filesystem;
namespace py = pybind11;

export {

    std::function<NLPResult(const std::string&)> getMeCabTokenizeFunc(const std::string& mecabDictDir, std::shared_ptr<spdlog::logger> logger);

    std::function<NLPResult(const std::string&)> getNLPTokenizeFunc(const std::vector<std::string>& dependencies, const std::string& moduleName,
        const std::string& modelName, std::shared_ptr<spdlog::logger> logger, bool& needReboot);
}


module :private;

std::function<NLPResult(const std::string&)> getMeCabTokenizeFunc(const std::string& mecabDictDir, std::shared_ptr<spdlog::logger> logger)
{
    std::shared_ptr<MeCab::Model> mecabModel = std::shared_ptr<MeCab::Model>(
        MeCab::Model::create(("-r BaseConfig/mecabDict/mecabrc -d " + ascii2Ascii(mecabDictDir)).c_str())
    );
    if (!mecabModel) {
        throw std::runtime_error("无法初始化 MeCab Model。请确保 BaseConfig/mecabDict/mecabrc 和 " + mecabDictDir + " 存在且无特殊字符\n"
            "错误信息: " + MeCab::getLastError());
    }
    std::shared_ptr<MeCab::Tagger> mecabTagger = std::shared_ptr<MeCab::Tagger>(mecabModel->createTagger());
    if (!mecabTagger) {
        throw std::runtime_error("无法初始化 MeCab Tagger。请确保 BaseConfig/mecabDict/mecabrc 和 " + mecabDictDir + " 存在且无特殊字符\n"
            "错误信息: " + MeCab::getLastError());
    }
    std::function<NLPResult(const std::string&)> resultFunc = [=](const std::string& text) -> NLPResult
        {
            WordPosVec wordPosList;
            EntityVec entityList;
            std::unique_ptr<MeCab::Lattice> lattice(mecabModel->createLattice());
            lattice->set_sentence(text.c_str());
            if (!mecabTagger->parse(lattice.get())) {
                throw std::runtime_error(std::format("分词器解析失败，错误信息: {}", MeCab::getLastError()));
            }
            for (const MeCab::Node* node = lattice->bos_node(); node; node = node->next) {
                if (node->stat == MECAB_BOS_NODE || node->stat == MECAB_EOS_NODE) continue;

                std::string surface(node->surface, node->length);
                std::string feature = node->feature;
                //logger->trace("分词结果：{} ({})", surface, feature);
                wordPosList.emplace_back(std::vector<std::string>{ surface, feature });
                if (feature.find("固有名詞") != std::string::npos || !extractKatakana(surface).empty()) {
                    entityList.emplace_back(std::vector<std::string>{ surface, feature });
                }
            }
            return NLPResult{ std::move(wordPosList), std::move(entityList) };
        };
    return resultFunc;
}

std::function<NLPResult(const std::string&)> getNLPTokenizeFunc(const std::vector<std::string>& dependencies, const std::string& moduleName,
    const std::string& modelName, std::shared_ptr<spdlog::logger> logger, bool& needReboot)
{
    PythonManager::getInstance().checkDependency(dependencies, logger);
    std::shared_ptr<PythonModule> pythonModule = PythonManager::getInstance().registerNLPFunction(moduleName, modelName, logger, needReboot);
    std::shared_ptr<py::object> processorClass = pythonModule->processors_[modelName];
    std::function<NLPResult(const std::string&)> resultFunc;
    if (processorClass) {
        std::shared_ptr<py::object> processorFunc;
        auto getFuncTaskFunc = [&]()
            {
                processorFunc = std::shared_ptr<py::object>(new py::object{ processorClass->attr("process_text") }, pythonDeleter<py::object>);
            };
        PythonManager::getInstance().submitTask(std::move(getFuncTaskFunc)).get();
        resultFunc = [pythonModule, processorFunc](const std::string& text) -> NLPResult
            {
                NLPResult result;
                auto nlpTaskFunc = [&]()
                    {
                        result = (*processorFunc)(text).cast<NLPResult>();
                    };
                PythonManager::getInstance().submitTask(std::move(nlpTaskFunc)).get();
                return result;
            };
    }
    else {
        resultFunc = [](const std::string& text) -> NLPResult
            {
                return NLPResult{ { {text, "ORIG"} }, EntityVec{} };
            };
    }
    return resultFunc;
}