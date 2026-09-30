#include "json/reader.h"
#include <config/ConfigLoader.h>
#include <optional>
#include <string>
#include <fstream>
#include <json/json.h>
#include <spdlog/spdlog.h>
bool ConfigLoader::LoadConfigFile(const std::string &configFilePath)  {
  _configFilePath = configFilePath;
  if(_configFilePath.empty()) {
    spdlog::error("Config file path is empty");
    return false;
  }
  std::ifstream configFile(configFilePath); 
  if(not configFile.is_open()){
    spdlog::error("Failed to open config file: {}", configFilePath);
    return false;
  }
  //建造者模式
  Json::CharReaderBuilder builder;
  builder["collectComments"] = false;
  builder["allowComments"] = false;
  builder["failIfExtra"] = true;
  builder["rejectDupKeys"] = true;
  std::string error;
  if(not Json::parseFromStream(builder,  configFile, &(this->_jsonConfig),&error)){
    spdlog::error("Json格式错误: " + error);
    return false;
  }
  if (not this->_jsonConfig.isObject()){
    spdlog::error("配置文件必须对象");
    return false;
  }
  return true;
}
std::optional<std::string> ConfigLoader::GetConfigFilePath() {
  if(_configFilePath.empty()){
    return std::nullopt;
  }
  return _configFilePath;
}
std::optional<std::string> ConfigLoader::GetConfigValue(const std::string &key) {
  if(key.empty() || _configFilePath.empty()){
    return std::nullopt;
  }
  std::string res = (this->_jsonConfig)[key].asString();
  return std::optional<std::string>(res);
}