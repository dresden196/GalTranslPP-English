module;

#include <pybind11/stl.h>
#include <pybind11/complex.h>
#include <pybind11/functional.h>
#include <pybind11/stl_bind.h>
#include <pybind11/pybind11.h>
#include <pybind11/embed.h>
#include <mecab/mecab.h>
#include <toml.hpp>
#include <spdlog/spdlog.h>
#pragma comment(lib, "../lib/python3.lib")
#pragma comment(lib, "../lib/python312.lib")

export module PythonManager;

import Tool;
export import IPlugin;

namespace fs = std::filesystem;
namespace py = pybind11;

export {

    template <typename T>
    class SafeQueue {
    public:
        void push(T value) {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_queue.push(std::move(value));
            m_cond.notify_one();
        }

        std::optional<T> pop() {
            std::unique_lock<std::mutex> lock(m_mutex);
            m_cond.wait(lock, [this] { return !m_queue.empty() || m_stopped; });
            if (m_stopped && m_queue.empty()) {
                return std::nullopt;
            }
            T value = std::move(m_queue.front());
            m_queue.pop();
            return value;
        }

        void stop() {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_stopped = true;
            m_cond.notify_all();
        }

    private:
        std::queue<T> m_queue;
        std::mutex m_mutex;
        std::condition_variable m_cond;
        bool m_stopped = false;
    };

    struct PythonTask {
        std::function<void()> taskFunc;
        std::promise<void> promise; // 用于返回结果
    };

    struct PythonModule {
        std::shared_ptr<py::module_> module_;
        std::map<std::string, std::shared_ptr<py::object>> processors_;
    };

    class PythonManager {

    public:
        static PythonManager& getInstance();
        ~PythonManager();

        void stop();

        PythonManager(PythonManager&) = delete;
        PythonManager(PythonManager&&) = delete;

        std::future<void> submitTask(std::function<void()> taskFunc); // 提交任务到队列

        void checkDependency(const std::vector<std::string>& dependencies, std::shared_ptr<spdlog::logger> logger);
        std::shared_ptr<PythonModule> registerNLPFunction(const std::string& moduleName, const std::string& modelName, std::shared_ptr<spdlog::logger> logger, bool& needReboot);
        std::optional<std::shared_ptr<PythonModule>> registerFunction(const std::string& modulePath, const std::string& functionName, std::shared_ptr<spdlog::logger> logger, bool& needReboot);

    private:
        std::thread::id m_threadId;
        PythonManager();
        void run(); // 守护线程的执行函数
        void registerCustomTypes(std::shared_ptr<PythonModule> pythonModule, const std::string& modulePath, std::shared_ptr<spdlog::logger> logger, bool& needReboot);

        // e.g. { "tokenize_spacy", <pybind11::module, { {"ja_core_news_trf", <pybind11::object>}, {"en_core_web_trf", <pybind11::object> } }> }
        std::map<std::string, std::weak_ptr<PythonModule>> m_pyModules;
        std::mutex m_mutex;
        std::thread m_pyThread; // 守护线程
        SafeQueue<std::unique_ptr<PythonTask>> m_taskQueue;
    };

    template <typename T>
    void pythonDeleter(T* ptr) {
        auto deleteTaskFunc = [ptr]()
            {
                delete ptr;
            };
        PythonManager::getInstance().submitTask(std::move(deleteTaskFunc));
    }

    std::function<NLPResult(const std::string&)> getMeCabTokenizeFunc(const std::string& mecabDictDir, std::shared_ptr<spdlog::logger> logger);
    std::function<NLPResult(const std::string&)> getNLPTokenizeFunc(const std::vector<std::string>& dependencies, const std::string& moduleName,
        const std::string& modelName, std::shared_ptr<spdlog::logger> logger, bool& needReboot);
    std::optional<CheckSeCondFunc> getPythonCheckSeCondFunc(const std::string& modulePath, const std::string& functionName, std::shared_ptr<spdlog::logger> logger, bool& needReboot);

    std::tuple<bool, std::string> extractPDF(const fs::path& pdfPath, const fs::path& jsonPath, bool showProgress = false);
    std::tuple<bool, std::string> rejectPDF(const fs::path& orgPDFPath, const fs::path& translatedJsonPath, const fs::path& outputPDFPath,
        bool noMono = false, bool noDual = false, bool showProgress = false);


    class PythonTextPlugin : public IPlugin {
    private:
        std::vector<std::function<NLPResult(const std::string&)>> m_tokenizeFuncs;
        std::shared_ptr<PythonModule> m_pyModule;
        std::shared_ptr<py::object> m_pyRunFunc;
        std::string m_modulePath;
        bool m_needReboot = false;

    public:
        PythonTextPlugin(const fs::path& projectDir, const std::string& modulePath, std::shared_ptr<spdlog::logger> logger);
        virtual bool needReboot() override { return m_needReboot; }
        virtual void run(Sentence* se) override;
        virtual ~PythonTextPlugin() override;
    };

}
