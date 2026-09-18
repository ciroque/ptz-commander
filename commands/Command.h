#ifndef COMMAND_H
#define COMMAND_H

#include "../data/Context.h"
#include <string>

namespace commands {

class Command {
public:
    virtual ~Command() = default;
    virtual void execute(data::Context& ctx, const std::string& args) = 0;

    const std::string& family() const { return family_; }
    const std::string& verb() const { return verb_; }
    const std::string& usage() const { return usage_; }
    const std::string& summary() const { return summary_; }

protected:
    Command(std::string family, std::string verb, std::string usage, std::string summary)
        : family_(std::move(family)),
          verb_(std::move(verb)),
          usage_(std::move(usage)),
          summary_(std::move(summary)) {}

private:
    std::string family_;
    std::string verb_;
    std::string usage_;
    std::string summary_;
};

} // commands

#endif // COMMAND_H
