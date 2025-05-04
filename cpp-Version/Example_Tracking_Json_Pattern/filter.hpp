#pragma once
#include <array>

class KalmanFilter2D {
public:
    // 构造函数，默认采样周期0.02s
    KalmanFilter2D(double dt = 0.02);
    
    // 执行一步卡尔曼滤波，输入测量值，返回完整状态
    std::array<double, 6> step(const std::array<double, 2>& measured_pos);

private:
    const int X = 0;  // 数组索引常量
    const int Y = 1;
    
    double dt;                    // 采样周期
    double process_noise;         // 过程噪声强度 (原Q)
    double measurement_noise;     // 观测噪声强度 (原R)
    double covariance[6][6];      // 状态协方差矩阵 (原P)
    
    // 状态预测向量，分开存储更清晰
    double position_pred[2];      // [x, y]
    double velocity_pred[2];      // [vx, vy]
    double acceleration_pred[2];  // [ax, ay]
};

// 声明低通滤波函数
int lowPassFilter(int currentValue, int newValue, float alpha);
int adaptiveLowPassFilter(int currentValue, int newValue, float baseAlpha);