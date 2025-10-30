module;

#include <spdlog/spdlog.h>
#include <boost/regex.hpp>
#include <cpr/cpr.h>

export module API_Tool;

export import Tool;
export import ITranslator;

using json = nlohmann::json;

export {
    struct TranslationApi {
        std::string apikey;
        std::string apiurl;
        std::string modelName;
        std::chrono::steady_clock::time_point lastReportTime = std::chrono::steady_clock::now();
        int reportCount = 0;
        bool stream;
    };

    struct ApiResponse {
        bool success = false;
        std::string content; // 成功时的内容 或 失败时的错误信息
        long statusCode = 0;   // HTTP 状态码
    };

    ApiResponse performApiRequest(json& payload, const TranslationApi& api, int threadId,
        std::shared_ptr<IController> controller, std::shared_ptr<spdlog::logger> logger, int apiTimeOutMs);

    std::string cvt2StdApiUrl(const std::string& url);
}


module :private;

ApiResponse performApiRequest(json& payload, const TranslationApi& api, int threadId,
    std::shared_ptr<IController> controller, std::shared_ptr<spdlog::logger> logger, const int apiTimeOutMs) {
    ApiResponse apiResponse;

    if (api.stream) {
        // =================================================
        // ===========   流式请求处理路径   ================
        // =================================================
        payload["stream"] = true;
        std::string concatenatedContent;
        std::string sseBuffer;

        // 1. 定义一个符合 cpr::WriteCallback 构造函数要求的 lambda
        auto callbackLambda = [&](const std::string_view& data, intptr_t userdata) -> bool
            {
                // 将接收到的数据块(string_view)追加到缓冲区(string)
                sseBuffer.append(data);
                size_t pos;
                while ((pos = sseBuffer.find('\n')) != std::string::npos) {
                    std::string line = sseBuffer.substr(0, pos);
                    sseBuffer.erase(0, pos + 1);

                    if (line.rfind("data: ", 0) == 0) {
                        std::string jsonDataStr = line.substr(6);
                        if (jsonDataStr == "[DONE]") {
                            return true;
                        }
                        try {
                            json chunk = json::parse(jsonDataStr);
                            if (!chunk["choices"].empty() && chunk["choices"][0].contains("delta") && chunk["choices"][0]["delta"].contains("content")) {
                                auto content_node = chunk["choices"][0]["delta"]["content"];
                                if (content_node.is_string()) {
                                    concatenatedContent += content_node.get<std::string>();
                                }
                            }
                        }
                        catch (const json::exception&) {

                        }
                    }
                }
                // 继续接收数据
                return !controller->shouldStop();
            };

        // 2. 使用上面定义的 lambda 来构造一个 cpr::WriteCallback 类的实例
        cpr::WriteCallback writeCallbackInstance(callbackLambda);

        // 3. 将该实例传递给 cpr::Post
        cpr::Response response = cpr::Post(
            cpr::Url{ api.apiurl },
            cpr::Body{ payload.dump() },
            cpr::Header{ {"Content-Type", "application/json"}, {"Authorization", "Bearer " + api.apikey} },
            cpr::Timeout{ apiTimeOutMs },
            writeCallbackInstance // 传递类的实例
        );

        apiResponse.statusCode = response.status_code;
        if (response.status_code == 200) {
            apiResponse.success = true;
            apiResponse.content = concatenatedContent;
        }
        else {
            apiResponse.success = false;
            apiResponse.content = response.text;
            logger->error("[线程 {}] API 流式请求失败，状态码: {}, 错误: {}", threadId, response.status_code, response.text);
        }
    }
    else {
        // =================================================
        // =========   非流式请求处理路径   =========
        // =================================================
        cpr::Response response = cpr::Post(
            cpr::Url{ api.apiurl },
            cpr::Body{ payload.dump() },
            cpr::Header{ {"Content-Type", "application/json"}, {"Authorization", "Bearer " + api.apikey} },
            cpr::Timeout{ apiTimeOutMs }
        );

        apiResponse.statusCode = response.status_code;
        apiResponse.content = response.text; // 先记录原始响应体

        if (response.status_code == 200) {
            try {
                // 解析完整的JSON响应
                apiResponse.content = json::parse(response.text)["choices"][0]["message"]["content"];
                apiResponse.success = true;
            }
            catch (const json::exception& e) {
                logger->error("[线程 {}] 成功响应但JSON解析失败: {}, 错误: {}", threadId, response.text, e.what());
                apiResponse.success = false;
            }
        }
        else {
            logger->error("[线程 {}] API 非流请求失败，状态码: {}, 错误: {}", threadId, response.status_code, response.text);
            apiResponse.success = false;
        }
    }

    return apiResponse;
}

std::string cvt2StdApiUrl(const std::string& url) {
    std::string ret = url;
    if (ret.ends_with("/")) {
        ret.pop_back();
    }
    if (ret.ends_with("/chat/completions")) {
        return ret;
    }
    if (ret.ends_with("/chat")) {
        return ret + "/completions";
    }
    if (boost::regex_search(ret, boost::regex(R"(/v\d+$)"))) {
        return ret + "/chat/completions";
    }
    return ret + "/v1/chat/completions";
}
