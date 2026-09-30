#ifndef MESSAGE_H
#define MESSAGE_H
#include <string>
#include <variant>
// 这个不准裸用
struct BaseMessage {
  BaseMessage() = default;
  BaseMessage(const std::string &role) : role(role) {}
  BaseMessage(const std::string &role, const std::string &content)
      : role(role), content(content) {}
  std::string GetRole();
  std::string GetContent();
  bool setRole(std::string &role);
  bool setContent(std::string &content);
  std::string role;
  std::string content;
};
class SystemMessage : public BaseMessage {
public:
  SystemMessage() : BaseMessage("system") {}
};
class HumanMessage : public BaseMessage {
public:
  HumanMessage() : BaseMessage("user") {}
};
class AIMessage : public BaseMessage {
public:
  AIMessage() : BaseMessage("assistant") {}
};
class ToolMessage : public BaseMessage {
public:
  ToolMessage() : BaseMessage("tool") {}
};
using Message =
    std::variant<SystemMessage, HumanMessage, AIMessage, ToolMessage>;
#endif