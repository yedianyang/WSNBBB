#include <gtest/gtest.h>
#include "json2Route.hpp"
#include <fstream>
#include <sstream>

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

// 测试类
class Json2RouteTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 创建测试文件
        createTestFile("test_pattern.json", TEST_JSON);
    }

    void TearDown() override {
        // 清理测试文件
        std::remove("test_pattern.json");
    }
};

// 测试加载文件
TEST_F(Json2RouteTest, LoadFromFile) {
    Json2Route pattern;
    EXPECT_TRUE(pattern.loadFromFile("test_pattern.json"));
    EXPECT_EQ(pattern.getCanvasWidth(), 1000);
    EXPECT_EQ(pattern.getCanvasHeight(), 1000);
}

// 测试获取所有命令
TEST_F(Json2RouteTest, GetAllCommands) {
    Json2Route pattern;
    pattern.loadFromFile("test_pattern.json");
    auto commands = pattern.getAllCommands();
    EXPECT_EQ(commands.size(), 6); // 3 commands per path, 2 paths
}

// 测试获取特定路径的命令
TEST_F(Json2RouteTest, GetPathCommands) {
    Json2Route pattern;
    pattern.loadFromFile("test_pattern.json");
    
    // 测试存在的路径
    auto path1Commands = pattern.getPathCommands("path1");
    EXPECT_EQ(path1Commands.size(), 3);
    EXPECT_EQ(path1Commands[0].type, Command::Type::PEN_DOWN);
    EXPECT_EQ(path1Commands[0].x, 100);
    EXPECT_EQ(path1Commands[0].y, 100);

    // 测试不存在的路径
    auto nonExistentPathCommands = pattern.getPathCommands("nonexistent");
    EXPECT_TRUE(nonExistentPathCommands.empty());
}

// 测试获取所有路径
TEST_F(Json2RouteTest, GetPaths) {
    Json2Route pattern;
    pattern.loadFromFile("test_pattern.json");
    auto paths = pattern.getPaths();
    EXPECT_EQ(paths.size(), 2);
    EXPECT_EQ(paths[0].getId(), "path1");
    EXPECT_EQ(paths[1].getId(), "path2");
}

// 测试无效的JSON文件
TEST_F(Json2RouteTest, InvalidJsonFile) {
    Json2Route pattern;
    // 创建无效的JSON文件
    createTestFile("invalid.json", "{invalid json}");
    EXPECT_FALSE(pattern.loadFromFile("invalid.json"));
    std::remove("invalid.json");
}

// 测试不存在的文件
TEST_F(Json2RouteTest, NonExistentFile) {
    Json2Route pattern;
    EXPECT_FALSE(pattern.loadFromFile("nonexistent.json"));
}

int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
} 