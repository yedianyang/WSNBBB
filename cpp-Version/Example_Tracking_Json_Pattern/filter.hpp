#pragma once
#include <array>
#include <string>
#include <fstream>
#include <json.hpp> // 使用nlohmann/json库

class KalmanFilter2D {
public:
    // 构造函数，支持从文件加载配置
    KalmanFilter2D(double dt = 0.02);
    
    // 执行一步卡尔曼滤波，输入测量值，返回完整状态
    std::array<double, 6> step(const std::array<double, 2>& measured_pos);

    // 参数调整方法
    void setProcessNoise(double q) { process_noise = q; }
    void setMeasurementNoise(double r) { measurement_noise = r; }
    void setSamplingTime(double new_dt) { dt = new_dt; }
    
    // 获取当前参数
    double getProcessNoise() const { return process_noise; }
    double getMeasurementNoise() const { return measurement_noise; }
    double getSamplingTime() const { return dt; }

    // 配置保存和加载
    bool saveConfig(const std::string& filename) const;
    bool loadConfig(const std::string& filename);

private:
    const int X = 0;  // 数组索引常量
    const int Y = 1;
    
    double dt;                    // 采样周期
    double process_noise;         // 过程噪声强度 (Q)
    double measurement_noise;     // 观测噪声强度 (R)
    double covariance[6][6];      // 状态协方差矩阵 (P)
    
    // 状态预测向量，分开存储更清晰
    double position_pred[2];      // [x, y]
    double velocity_pred[2];      // [vx, vy]
    double acceleration_pred[2];  // [ax, ay]
};

// 声明低通滤波函数
int lowPassFilter(int currentValue, int newValue, float alpha);
int adaptiveLowPassFilter(int currentValue, int newValue, float baseAlpha);