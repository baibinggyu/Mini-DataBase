#include <agent/Agent.h>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/regex.hpp>
#include <memory>
#include <optional>
#include <sstream>
#include <spdlog/spdlog.h>
// LLM implementation
LLM::LLM(boost::asio::io_context &ioc, std::string apiKey,
         std::string anthropicApiUrl, std::string modelName)
    : _ioc(ioc), _apiKey(apiKey), _anthropicApiUrl(anthropicApiUrl),
      _modelName(modelName) {
  (this->_jsonSend)["model"] = "deepseek-flash";
  (this->_jsonSend)["stream"] = false;
  (this->_jsonSend)["thinking"]["type"] = "enabled";
  (this->_jsonSend)["reasoning_effort"] = "high";
}
LLM &LLM::setApiKey(std::string apiKey) {
  this->_apiKey = apiKey;
  return *this;
}
LLM &LLM::setAnthropicApiUrl(std::string anthropicApiUrl) {
  this->_anthropicApiUrl = anthropicApiUrl;
  return *this;
}
LLM &LLM::setModelName(std::string modelName) {
  this->_modelName = modelName;
  return *this;
}
std::optional<std::string> LLM::Chat(std::vector<std::string> prompt,
                                     const std::string role) {
  if (prompt.empty()) {
    spdlog::info("LLM::Chat prompt is empty.");
    return std::nullopt;
  }

  // tls版本设置
  boost::asio::ssl::context ctx{boost::asio::ssl::context::tls_client};
  ctx.set_default_verify_paths();
  ctx.set_verify_mode(boost::asio::ssl::verify_peer);
  
  //dns
  boost::asio::ip::tcp::resolver resolver{this->_ioc};
  //tcp + tls 
  boost::beast::ssl_stream<boost::beast::tcp_stream> stream{this->_ioc,ctx};
  //DNS
  std::string domain, path, port;
  {
    auto x = this->parseAnthropicApiUrl(this->_anthropicApiUrl);
    if(x == std::nullopt){return std::nullopt;}
    std::tie(domain,path,port) = x.value();
  }
  auto result = resolver.resolve(domain,port);
  //TLS connect 
  boost::beast::get_lowest_layer(stream).connect(result);  
  //TLS handshake
  stream.handshake(boost::asio::ssl::stream_base::client);
  boost::beast::http::request<boost::beast::http::string_body> req{
    boost::beast::http::verb::post,path,11
  } ;
  req.set(boost::beast::http::field::host,domain);
  // req.set(boost::beast::http::field::content_type,"application/json");
  
  //设置对话记录
  Json::Value arr(Json::arrayValue);
  for(const auto& x : prompt){
    arr.append(x);
  }
  (this->_jsonSend)["messages"] = arr;
  Json::StreamWriterBuilder builder;
  builder["indentation"] = "";      // 紧凑，无缩进无换行
  builder["commentStyle"] = "None"; // 不输出注释
  builder["emitUTF8"] = true;       // 中文原样输出，不转义
  std::unique_ptr<Json::StreamWriter> writer(builder.newStreamWriter()); 
  std::stringstream ss;
  writer->write(this->_jsonSend, &ss);
  req.body() = ss.str();
  req.prepare_payload();
  boost::beast::http::write(stream,req);
  boost::beast::flat_buffer buffer;
  boost::beast::http::response<boost::beast::http::string_body> response;
  boost::beast::http::read(stream, buffer, response);
  std::string ret = response.body();
  boost::beast::error_code ec;
  const auto shutdown_error = stream.shutdown(ec);
  if (shutdown_error &&
      shutdown_error != boost::asio::ssl::error::stream_truncated) {
    spdlog::warn("TLS shutdown failed: {}", shutdown_error.message());
  }
  return ret;
}


std::optional<std::tuple<std::string,std::string,std::string>> LLM::parseAnthropicApiUrl(std::string url){
  using opt = std::optional<std::tuple<std::string,std::string,std::string>>;  
  using trip =std::tuple<std::string,std::string,std::string>; 
  boost::regex sre(R"(^https://[A-Za-z0-9.-]+(/[A-Za-z0-9._~:/?#\[\]@!$&'()*+,;=%-]*)?$)");
  if (not boost::regex_match(url,sre)){
    spdlog::info("parseAnthropicApiUrl's url is uncorrect. line = {}",__LINE__) ;
    return std::nullopt;
  }
  boost::regex re(R"(^([^:]+)://([^/]+)(/.*)$)");
  boost::smatch match;
  if (not boost::regex_match(url, match, re)) {
    spdlog::info("parseAnthropicApiUrl error. line = {}",__LINE__);
    return std::nullopt;
  }
  auto res = opt(trip(match[2],match[3],"443"));
  return res;
}



// Agent implementation
Agent::Agent(boost::asio::io_context &ioc, std::string apiKey,
             std::string anthropicApiUrl, std::string modelName)
    : _llm(ioc, apiKey, anthropicApiUrl, modelName) {}
