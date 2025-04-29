#include "json2Route.hpp"
#include <iostream>

// 静态成员初始化
const std::vector<Command> Json2Route::emptyCommands_;

/**
 * @brief 从JSON文件加载图案数据
 * @param filename JSON文件的路径
 * @return 如果成功加载返回true，否则返回false
 * @details 该方法会尝试打开并解析指定的JSON文件，如果文件不存在或格式错误则返回false
 */
bool Json2Route::loadFromFile(const std::string& filename) {
    try {
        std::ifstream file(filename);
        if (!file.is_open()) {
            return false;
        }
        json j;
        file >> j;
        return parseJson(j);
    } catch (const std::exception& e) {
        return false;
    }
}

/**
 * @brief 获取所有路径中的所有命令
 * @return 包含所有命令的vector
 * @details 该方法会遍历所有路径，并将每个路径中的命令合并到一个vector中返回
 */
std::vector<Command> Json2Route::getAllCommands() const {
    std::vector<Command> allCommands;
    for (const auto& path : paths_) {
        allCommands.insert(allCommands.end(), 
                         path.getCommands().begin(), 
                         path.getCommands().end());
    }
    return allCommands;
}

/**
 * @brief 获取指定路径ID的所有命令
 * @param pathId 要查询的路径ID
 * @return 如果找到指定路径，返回该路径的所有命令；否则返回空vector
 * @details 该方法会遍历所有路径，查找匹配的路径ID，并返回该路径的所有命令
 */
const std::vector<Command>& Json2Route::getPathCommands(const std::string& pathId) const {
    for (const auto& path : paths_) {
        if (path.getId() == pathId) {
            return path.getCommands();
        }
    }
    return emptyCommands_;
}

/**
 * @brief 解析JSON数据
 * @param j 要解析的JSON对象
 * @return 如果成功解析返回true，否则返回false
 * @details 该方法会解析JSON对象中的canvas_size和paths数据，并更新相应的成员变量
 */
bool Json2Route::parseJson(const json& j) {
    try {
        // 解析canvas_size
        if (j.contains("canvas_size")) {
            auto& canvas = j["canvas_size"][0];
            canvasWidth_ = canvas["width"];
            canvasHeight_ = canvas["height"];
        }

        // 解析paths
        if (j.contains("paths")) {
            for (const auto& pathJson : j["paths"]) {
                Path path(pathJson["id"], pathJson["style"]);
                
                for (const auto& cmdJson : pathJson["commands"]) {
                    Command::Type type;
                    std::string cmdType = cmdJson["type"];
                    
                    if (cmdType == "pen_down") type = Command::Type::PEN_DOWN;
                    else if (cmdType == "move") type = Command::Type::MOVE;
                    else if (cmdType == "pen_up") type = Command::Type::PEN_UP;
                    else continue;

                    path.addCommand(Command(type, cmdJson["x"], cmdJson["y"]));
                }
                
                paths_.push_back(path);
            }
        }
        return true;
    } catch (const std::exception& e) {
        return false;
    }
}

/**
 * @brief 打印命令信息到控制台
 * @param cmd 要打印的命令
 * @details 该方法会将命令的类型和坐标信息打印到控制台，主要用于调试
 */
void printCommand(const Command& cmd) {
    std::string typeStr;
    switch (cmd.type) {
        case Command::Type::PEN_DOWN:
            typeStr = "PEN_DOWN";
            break;
        case Command::Type::MOVE:
            typeStr = "MOVE";
            break;
        case Command::Type::PEN_UP:
            typeStr = "PEN_UP";
            break;
    }
    std::cout << "Type: " << typeStr << ", X: " << cmd.x << ", Y: " << cmd.y << std::endl;
} 