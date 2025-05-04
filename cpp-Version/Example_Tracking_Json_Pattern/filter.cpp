#include "filter.hpp"
#include <cmath>
#include <algorithm>
#include <cstring>
#include <nlohmann/json.hpp>
using json = nlohmann::json;

// 低通滤波器实现
int lowPassFilter(int currentValue, int newValue, float alpha) {
    return static_cast<int>(alpha * newValue + (1 - alpha) * currentValue);
} 

// 自适应低通滤波器实现
int adaptiveLowPassFilter(int currentValue, int newValue, float baseAlpha) {
    float delta = std::abs(newValue - currentValue);
    
    // 动态调整平滑因子
    float alpha = baseAlpha + (1.0f - baseAlpha) * (delta / 12000);
    alpha = std::min(1.0f, std::max(baseAlpha, alpha)); // 确保alpha在合理范围内

    return static_cast<int>(alpha * newValue + (1 - alpha) * currentValue);
} 

// KalmanFilter2D构造函数实现
KalmanFilter2D::KalmanFilter2D(double dt) : dt(dt) {
    // 初始化参数
    process_noise = 1e-3;  // 过程噪声强度
    measurement_noise = 1e-1;  // 观测噪声强度
    
    // 初始化状态协方差矩阵为单位矩阵
    std::memset(covariance, 0, sizeof(covariance));
    for(int i = 0; i < 6; i++) {
        covariance[i][i] = 1.0;
    }
    
    // 初始化状态向量为0
    std::memset(position_pred, 0, sizeof(position_pred));
    std::memset(velocity_pred, 0, sizeof(velocity_pred));
    std::memset(acceleration_pred, 0, sizeof(acceleration_pred));
}

// KalmanFilter2D的step函数实现
std::array<double, 6> KalmanFilter2D::step(const std::array<double, 2>& measured_pos) {
    // 1. 预测步骤
    // 更新位置预测
    double dt_squared = dt * dt;
    position_pred[X] += velocity_pred[X] * dt + 0.5 * acceleration_pred[X] * dt_squared;
    position_pred[Y] += velocity_pred[Y] * dt + 0.5 * acceleration_pred[Y] * dt_squared;
    
    // 更新速度预测
    velocity_pred[X] += acceleration_pred[X] * dt;
    velocity_pred[Y] += acceleration_pred[Y] * dt;
    // 加速度预测保持不变
    
    // 更新协方差矩阵
    for(int i = 0; i < 6; i++) {
        covariance[i][i] += process_noise;
    }
    
    // 2. 更新步骤
    // 计算观测增益
    double innovation_x = covariance[0][0] + measurement_noise;
    double innovation_y = covariance[1][1] + measurement_noise;
    
    // 计算卡尔曼增益
    double kalman_gain_x = covariance[0][0] / innovation_x;
    double kalman_gain_y = covariance[1][1] / innovation_y;
    
    // 计算观测误差
    double measurement_error_x = measured_pos[X] - position_pred[X];
    double measurement_error_y = measured_pos[Y] - position_pred[Y];
    
    // 更新状态估计
    position_pred[X] += kalman_gain_x * measurement_error_x;
    position_pred[Y] += kalman_gain_y * measurement_error_y;
    
    // 更新协方差矩阵
    covariance[0][0] *= (1 - kalman_gain_x);
    covariance[1][1] *= (1 - kalman_gain_y);
    
    // 返回完整状态向量
    return {
        position_pred[X],      // 滤波后的X位置
        position_pred[Y],      // 滤波后的Y位置
        velocity_pred[X],      // X方向速度
        velocity_pred[Y],      // Y方向速度
        acceleration_pred[X],  // X方向加速度
        acceleration_pred[Y]   // Y方向加速度
    };
}

bool KalmanFilter2D::saveConfig(const std::string& filename) const {
    try {
        json config;
        config["sampling_time"] = dt;
        config["process_noise"] = process_noise;
        config["measurement_noise"] = measurement_noise;
        
        // 保存协方差矩阵
        json covariance_json;
        for(int i = 0; i < 6; i++) {
            for(int j = 0; j < 6; j++) {
                covariance_json.push_back(covariance[i][j]);
            }
        }
        config["covariance"] = covariance_json;

        // 写入文件
        std::ofstream file(filename);
        file << config.dump(4);
        return true;
    } catch(...) {
        return false;
    }
}

bool KalmanFilter2D::loadConfig(const std::string& filename) {
    try {
        std::ifstream file(filename);
        json config = json::parse(file);
        
        dt = config["sampling_time"];
        process_noise = config["process_noise"];
        measurement_noise = config["measurement_noise"];
        
        // 加载协方差矩阵
        auto covariance_json = config["covariance"];
        int idx = 0;
        for(int i = 0; i < 6; i++) {
            for(int j = 0; j < 6; j++) {
                covariance[i][j] = covariance_json[idx++];
            }
        }
        return true;
    } catch(...) {
        return false;
    }
}

