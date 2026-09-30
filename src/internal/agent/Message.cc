#include <agent/Message.h>

std::string BaseMessage::GetRole() { return this->role; }
std::string BaseMessage::GetContent() { return this->content; }
bool BaseMessage::setRole(std::string &role) {
  if (role.empty())
    return false;
  this->role = role;
  return true;
}
bool BaseMessage::setContent(std::string &content) {
    if(content.empty())return false;
    this->content = content;
    return true;
}