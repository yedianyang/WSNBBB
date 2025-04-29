#include "json2Route.hpp"
#include <iostream>
#include <iomanip>
#include <fstream>
#include <chrono>
#include <thread>
#include <ctime>

// 测试用的JSON数据
std::string TEST_JSON_FILE;

void initTestJsonFile(int argc, char* argv[]) {
    if (argc > 1) {
        TEST_JSON_FILE = argv[1];
    } else {
        std::cout << "Please enter the path to the test JSON file: ";
        std::getline(std::cin, TEST_JSON_FILE);
    }
}

// 创建临时测试文件
void createTestFile(const std::string& filename, const std::string& content) {
    std::ofstream file(filename);
    file << content;
    file.close();
}

// 获取当前时间戳字符串
std::string getCurrentTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;
    
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time), "%H:%M:%S");
    ss << '.' << std::setfill('0') << std::setw(3) << ms.count();
    return ss.str();
}

int main(int argc, char* argv[]) {
    // 初始化测试JSON文件路径
    initTestJsonFile(argc, argv);
    // 创建Json2Route对象
    Json2Route pattern;

    // 直接从指定的JSON文件加载
    if (!pattern.loadFromFile(TEST_JSON_FILE)) {
        std::cerr << "Failed to load pattern file: " << TEST_JSON_FILE << std::endl;
        return 1;
    }

    // 打印画布尺寸
    std::cout << "\n画布尺寸:" << std::endl;
    std::cout << "宽度: " << pattern.getCanvasWidth() << std::endl;
    std::cout << "高度: " << pattern.getCanvasHeight() << std::endl;

    // 打印所有路径的信息
    std::cout << "\n路径信息:" << std::endl;
    std::cout << "路径数量: " << pattern.getPaths().size() << std::endl;
    
    // 获取所有命令的连续序列
    auto allCommands = pattern.getAllCommands();
    std::cout << "\n所有命令总数: " << allCommands.size() << std::endl;
    
    std::cout << "\n命令列表:" << std::endl;
    std::cout << std::setw(15) << "时间戳" 
              << std::setw(10) << "类型" 
              << std::setw(10) << "X坐标" 
              << std::setw(10) << "Y坐标" << std::endl;
    std::cout << std::string(45, '-') << std::endl;
    
    // 每秒打印一个命令
    for (const auto& cmd : allCommands) {
        std::cout << std::setw(15) << getCurrentTimestamp() << " ";
        printCommand(cmd);
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    return 0;
} 