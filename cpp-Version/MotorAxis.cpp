#include "MotorAxis.h"
#include <iostream>

#define ACC_LIM_RPM_PER_SEC 10000
#define VEL_LIM_RPM 1000
#define TIME_TILL_TIMEOUT 10000 // The timeout used for homing(ms)

MotorAxis::MotorAxis(sFnd::INode& node, sFnd::SysManager& sysMgr)
    : m_node(node)
    , m_sysMgr(sysMgr)
    , m_isMoving(false)
{
}

bool MotorAxis::Initialize() {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // 禁用节点
    m_node.EnableReq(false);
    m_sysMgr.Delay(200);
    
    // 清除警报和停止状态
    m_node.Status.AlertsClear();
    m_node.Motion.NodeStopClear();
    
    // 启用节点
    m_node.EnableReq(true);
    
    // 等待节点就绪
    if (!WaitForReady(TIME_TILL_TIMEOUT)) {
        return false;
    }
    
    // 启用运动中断功能
    m_node.Info.Ex.Parameter(98, 1);
    m_node.Motion.MoveWentDone();
    
    // 设置运动参数
    SetupMotionParameters();
    
    return true;
}

bool MotorAxis::Home() {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (!m_node.Motion.Homing.HomingValid()) {
        std::cout << "Homing not configured for this node." << std::endl;
        return false;
    }
    
    if (m_node.Motion.Homing.WasHomed()) {
        std::cout << "Node already homed, current position: " << m_node.Motion.PosnMeasured.Value() << std::endl;
        std::cout << "Rehoming..." << std::endl;
    } else {
        std::cout << "Node not homed. Starting homing..." << std::endl;
    }
    
    m_node.Motion.Homing.Initiate();
    
    return WaitForHoming(TIME_TILL_TIMEOUT);
}

void MotorAxis::MoveToPosition(int position) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_isMoving = true;
    
    std::cout << "Moving to position: " << position << std::endl;
    m_node.Motion.MovePosnStart(position, true);
    
    m_isMoving = false;
}

double MotorAxis::GetCurrentPosition() const {
    return m_node.Motion.PosnMeasured.Value();
}

bool MotorAxis::IsReady() const {
    return m_node.Motion.IsReady();
}

bool MotorAxis::IsMoving() const {
    return m_isMoving;
}

bool MotorAxis::IsBusPowerLow() const {
    return m_node.Status.Power.Value().fld.InBusLoss;
}

bool MotorAxis::WaitForReady(double timeoutMs) {
    double timeout = m_sysMgr.TimeStampMsec() + timeoutMs;
    
    while (!m_node.Motion.IsReady()) {
        if (m_sysMgr.TimeStampMsec() > timeout) {
            if (IsBusPowerLow()) {
                std::cout << "Error: Bus Power low. Make sure 75V supply is powered on." << std::endl;
                return false;
            }
            std::cout << "Error: Timed out waiting for node to enable" << std::endl;
            return false;
        }
    }
    return true;
}

bool MotorAxis::WaitForHoming(double timeoutMs) {
    double timeout = m_sysMgr.TimeStampMsec() + timeoutMs;
    
    while (!m_node.Motion.Homing.WasHomed()) {
        if (m_sysMgr.TimeStampMsec() > timeout) {
            if (IsBusPowerLow()) {
                std::cout << "Error: Bus Power low. Make sure 75V supply is powered on." << std::endl;
                return false;
            }
            std::cout << "Error: Homing timeout. Check homing settings and alerts." << std::endl;
            return false;
        }
    }
    std::cout << "Homing completed successfully." << std::endl;
    return true;
}

void MotorAxis::SetupMotionParameters() {
    m_node.AccUnit(sFnd::INode::RPM_PER_SEC);
    m_node.VelUnit(sFnd::INode::RPM);
    m_node.Motion.AccLimit = ACC_LIM_RPM_PER_SEC;
    m_node.Motion.VelLimit = VEL_LIM_RPM;
} 