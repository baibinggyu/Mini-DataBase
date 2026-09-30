#ifndef CONFIG_LOADER_H
#define CONFIG_LOADER_H
#include <json/json.h>
#include <pattern/SingleTon.hpp>
#include <string>
#include <optional>
// 这里的json单纯是一对一，意思就是说string:string, 也就是key-value的形式
// 不准写json数组，json数组是一个json对象的集合，json对象是一个key-value的集合
class ConfigLoader final : public SingleTon<ConfigLoader> {
  friend SingleTon<ConfigLoader>;
public:
  ConfigLoader(const ConfigLoader &) = delete;
  //true is success, false is fail
  bool LoadConfigFile(const std::string &configFilePath) ;
  std::optional<std::string> GetConfigFilePath() ;
  //没有直接返回"""
  std::optional<std::string>GetConfigValue(const std::string &key);
private:
  ConfigLoader() = default;
  ~ConfigLoader() = default;
  std::string _configFilePath = "";
  Json::Value _jsonConfig;
};
#endif