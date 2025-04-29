#pragma once

#include <string>
#include <vector>
#include <nlohmann/json.hpp>
#include <fstream>
#include <memory>
#include <iostream>
#include <iomanip>

using json = nlohmann::json;

/**
 * @brief 表示单个绘图命令
 */
struct Command {
    enum class Type {
        PEN_DOWN,
        MOVE,
        PEN_UP
    };

    Type type;
    double x;
    double y;

    Command(Type t, double x_pos, double y_pos) 
        : type(t), x(x_pos), y(y_pos) {}
};

/**
 * @brief 打印命令信息到控制台
 * @param cmd 要打印的命令
 * @details 该方法会将命令的类型和坐标信息打印到控制台，主要用于调试
 */
void printCommand(const Command& cmd);

/**
 * @brief 表示单个路径
 */
class Path {
public:
    Path(const std::string& id, const std::string& style) 
        : id_(id), style_(style) {}

    void addCommand(Command cmd) {
        commands_.push_back(cmd);
    }

    const std::vector<Command>& getCommands() const { return commands_; }
    const std::string& getId() const { return id_; }
    const std::string& getStyle() const { return style_; }

private:
    std::string id_;
    std::string style_;
    std::vector<Command> commands_;
};

/**
 * @brief 用于管理所有路径的主类
 */
class Json2Route {
public:
    Json2Route() = default;
    ~Json2Route() = default;

    // 从JSON文件加载图案
    bool loadFromFile(const std::string& filename);

    // 获取所有命令的连续序列
    std::vector<Command> getAllCommands() const;

    // 获取特定路径的命令
    const std::vector<Command>& getPathCommands(const std::string& pathId) const;

    // 获取所有路径
    const std::vector<Path>& getPaths() const { return paths_; }

    // 获取画布尺寸
    double getCanvasWidth() const { return canvasWidth_; }
    double getCanvasHeight() const { return canvasHeight_; }

private:
    bool parseJson(const json& j);

    std::vector<Path> paths_;
    double canvasWidth_ = 0.0;
    double canvasHeight_ = 0.0;
    static const std::vector<Command> emptyCommands_;  // 只声明，不定义 
};
