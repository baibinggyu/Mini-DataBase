#ifndef AGENT_H
#define AGENT_H
#include <agent/Message.h>
#include <boost/asio.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/beast.hpp>
#include <json/json.h>
#include <optional>
#include <string>
#include <vector>
#include <tuple>
// use Anthropic API
// 建造者模式
// 单次对话
class LLM {
public:
  LLM() = delete;
  // 默认走deepseek-flash模型
  LLM(boost::asio::io_context &ioc, std::string apiKey,
      std::string anthropicApiUrl = "https://api.deepseek.com/chat/completions",
      std::string modelName = "deepseek-flash");
  LLM &setApiKey(std::string apiKey);
  LLM &setAnthropicApiUrl(std::string anthropicApiUrl);
  LLM &setModelName(std::string modelName);
  // Chat不会append
  // message字段，单纯只修改message然后序列化发送给anthropicApiUrl
  std::optional<std::string> Chat(std::vector<std::string> prompt,
                                  const std::string role);
private:
  // domain router port 
  // 先使用optional去接，然后判空，不空再结构化绑定
  std::optional<std::tuple<std::string,std::string,std::string>> parseAnthropicApiUrl(std::string url);
private:
  std::string _apiKey;
  std::string _anthropicApiUrl;
  std::string _modelName;
  boost::asio::io_context &_ioc;
  // 这里的json，不append message字段
  Json::Value _jsonSend;
};
class Agent final {
public:
  Agent() = delete;
  Agent(
      boost::asio::io_context &ioc, std::string apiKey,
      std::string anthropicApiUrl = "https://api.deepseek.com/chat/completions",
      std::string modelName = "deepseek-flash");
  Agent &setApiKey(std::string apiKey);
  Agent &setAnthropicApiUrl(std::string anthropicApiUrl);
  Agent &setModelName(std::string modelName);
  std::string Chat(std::vector<std::string> prompt,
                   const std::string &context = "");

private:
  LLM _llm;
  std::vector<Message> _messages;
};
#endif