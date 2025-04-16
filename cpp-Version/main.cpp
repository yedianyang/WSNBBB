// main.cpp
#include "./WacomInkling.hpp"
#include "./Library/ClearPath-SC-Development/sFoundation.h"
#include <iostream>
#include <vector>
#include <chrono>
#include <thread>

using namespace sFnd;

class MotionController {
private:
    // 系统管理器
    SysManager* myMgr = nullptr;
    std::vector<INode*> xAxisNodes;
    std::vector<INode*> yAxisNodes;
    
    // Inkling控制器
    WacomInkling inkling;
    
    // 运动参数
    const double ACCEL_LIMIT = 100000;  // 加速度限制
    const double VEL_LIMIT = 50000;     // 速度限制
    const int HOMING_TIMEOUT = 10000;   // 归零超时时间(ms)
    
    // 运行状态
    std::atomic<bool> running{false};
    std::thread motionThread;
    
    // 轴控制开关
    const bool enableXAxis;  // X轴使能开关
    const bool enableYAxis;  // Y轴使能开关
    
    bool initializeSCHub() {
        try {
            myMgr = SysManager::Instance();
            
            // 查找SC Hub端口
            std::vector<std::string> comHubPorts;
            SysManager::FindComHubPorts(comHubPorts);
            if (comHubPorts.empty()) {
                std::cerr << "未找到SC Hub端口" << std::endl;
                return false;
            }
            
            // 配置端口
            for (size_t i = 0; i < comHubPorts.size() && i < NET_CONTROLLER_MAX; i++) {
                myMgr->ComHubPort(i, comHubPorts[i].c_str());
            }
            
            // 打开端口
            myMgr->PortsOpen(comHubPorts.size());
            
            // 初始化节点
            for (size_t i = 0; i < myMgr->PortCount(); i++) {
                IPort& port = myMgr->Ports(i);
                for (size_t j = 0; j < port.NodeCount(); j++) {
                    INode& node = port.Nodes(j);
                    
                    // 检查节点类型
                    if (node.Info.NodeType() != IInfo::CLEARPATH_SC_ADV) {
                        std::cerr << "节点 " << j << " 不是ClearPath-SC高级电机" << std::endl;
                        return false;
                    }
                    
                    // 根据索引分配X轴和Y轴
                    if (j == 0) xAxisNodes.push_back(&node);
                    else if (j == 1) yAxisNodes.push_back(&node);
                }
            }
            
            return true;
        }
        catch (mnErr& err) {
            std::cerr << "SC Hub初始化错误: " << err.ErrorMsg << std::endl;
            return false;
        }
    }
    
    bool homeAxes() {
        try {
            // 为每个节点执行归零操作
            auto homeNode = [this](INode* node) {
                // 清除警报和停止状态
                node->Status.AlertsClear();
                node->Motion.NodeStopClear();
                
                // 使能节点
                node->EnableReq(true);
                
                // 等待就绪
                auto timeout = myMgr->TimeStampMsec() + HOMING_TIMEOUT;
                while (!node->Motion.IsReady()) {
                    if (myMgr->TimeStampMsec() > timeout) {
                        throw std::runtime_error("节点使能超时");
                    }
                }
                
                // 检查是否可以归零
                if (!node->Motion.Homing.HomingValid()) {
                    throw std::runtime_error("节点未设置归零参数");
                }
                
                // 执行归零
                node->Motion.Homing.Initiate();
                
                // 等待归零完成
                timeout = myMgr->TimeStampMsec() + HOMING_TIMEOUT;
                while (!node->Motion.Homing.WasHomed()) {
                    if (myMgr->TimeStampMsec() > timeout) {
                        throw std::runtime_error("归零操作超时");
                    }
                }
                
                // 设置运动参数
                node->Motion.AccLimit = ACCEL_LIMIT;
                node->Motion.VelLimit = VEL_LIMIT;
                node->Info.Ex.Parameter(98, 1);  // 启用中断移动
            };
            
            // 执行X轴归零
            for (auto node : xAxisNodes) {
                homeNode(node);
            }
            
            // 执行Y轴归零
            for (auto node : yAxisNodes) {
                homeNode(node);
            }
            
            return true;
        }
        catch (const std::exception& e) {
            std::cerr << "归零操作失败: " << e.what() << std::endl;
            return false;
        }
    }
    
    void motionLoop() {
        InklingData data;
        while (running) {
            if (inkling.getData(data)) {
                try {
                    // 映射Inkling坐标到电机位置
                    double xPos = data.x * (193.0 / 1920.0);  // 映射比例需要根据实际情况调整
                    double yPos = data.y * (145.0 / 1920.0);
                    
                    // 移动X轴
                    for (auto node : xAxisNodes) {
                        node->Motion.MoveWentDone();
                        node->Motion.MovePosnStart(xPos);
                    }
                    
                    // 移动Y轴
                    for (auto node : yAxisNodes) {
                        node->Motion.MoveWentDone();
                        node->Motion.MovePosnStart(yPos);
                    }
                }
                catch (mnErr& err) {
                    std::cerr << "运动控制错误: " << err.ErrorMsg << std::endl;
                }
            }
            
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }

public:
    // 构造函数中设置轴使能状态
    MotionController(bool xEnable = true, bool yEnable = true)
        : enableXAxis(xEnable), enableYAxis(yEnable) {
    }

    // 获取轴使能状态（仅用于显示信息）
    void getAxisEnable(bool& xEnable, bool& yEnable) const {
        xEnable = enableXAxis;
        yEnable = enableYAxis;
    }

    bool initialize() {
        // 初始化Inkling设备
        if (!inkling.initialize()) {
            std::cerr << "Inkling初始化失败: " << inkling.getLastError() << std::endl;
            return false;
        }
        
        // 初始化SC Hub
        if (!initializeSCHub()) {
            return false;
        }
        
        // 执行归零操作
        if (!homeAxes()) {
            return false;
        }
        
        return true;
    }
    
    void start() {
        if (running) return;
        
        running = true;
        inkling.start();
        motionThread = std::thread(&MotionController::motionLoop, this);
    }
    
    void stop() {
        running = false;
        inkling.stop();
        if (motionThread.joinable()) {
            motionThread.join();
        }
        
        // 关闭所有节点
        for (auto node : xAxisNodes) {
            node->EnableReq(false);
        }
        for (auto node : yAxisNodes) {
            node->EnableReq(false);
        }
        
        // 关闭端口
        if (myMgr) {
            myMgr->PortsClose();
        }
    }
    
    ~MotionController() {
        stop();
    }
};

int main() {
    std::cout << "Wacom Inkling - ClearPath-SC 控制系统启动..." << std::endl;
    
    // 创建控制器时设置轴使能状态
    MotionController controller(true, false);  // 例如：只启用X轴
    
    // 显示当前轴使能状态
    bool xEnabled, yEnabled;
    controller.getAxisEnable(xEnabled, yEnabled);
    std::cout << "X轴状态: " << (xEnabled ? "启用" : "禁用") << std::endl;
    std::cout << "Y轴状态: " << (yEnabled ? "启用" : "禁用") << std::endl;
    
    if (!controller.initialize()) {
        std::cerr << "系统初始化失败" << std::endl;
        return 1;
    }
    
    std::cout << "初始化成功，开始运行..." << std::endl;
    controller.start();
    
    std::cout << "按回车键退出..." << std::endl;
    std::cin.get();
    
    std::cout << "正在停止系统..." << std::endl;
    controller.stop();
    
    std::cout << "系统已关闭" << std::endl;
    return 0;
}