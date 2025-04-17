#pragma once

#include "pubSysCls.h"
#include <mutex>
#include <atomic>

class MotorAxis {
public:
    MotorAxis(sFnd::INode& node, sFnd::SysManager& sysMgr);
    
    // 初始化电机
    bool Initialize();
    
    // 执行归零
    bool Home();
    
    // 移动到指定位置
    void MoveToPosition(int position);
    
    // 获取当前位置
    double GetCurrentPosition() const;
    
    // 检查电机是否就绪
    bool IsReady() const;
    
    // 检查电机是否正在运动
    bool IsMoving() const;
    
private:
    sFnd::INode& m_node;
    sFnd::SysManager& m_sysMgr;
    std::mutex m_mutex;
    std::atomic<bool> m_isMoving;
    
    // 检查电源状态
    bool IsBusPowerLow() const;
    
    // 等待电机就绪
    bool WaitForReady(double timeoutMs);
    
    // 等待归零完成
    bool WaitForHoming(double timeoutMs);
    
    // 设置运动参数
    void SetupMotionParameters();
}; 