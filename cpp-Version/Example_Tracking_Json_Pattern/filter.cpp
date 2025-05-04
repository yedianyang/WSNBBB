#include "filter.hpp"
#include <cmath>
#include <algorithm>
#include <cstring>

// Define the low-pass filter function

int lowPassFilter(int currentValue, int newValue, float alpha) {
    return static_cast<int>(alpha * newValue + (1 - alpha) * currentValue);
} 

// 自适应低通滤波器函数
int adaptiveLowPassFilter(int currentValue, int newValue, float baseAlpha) {
    // 自适应低通滤波器函数
    // currentValue: 当前值
    // newValue: 新的输入值 
    // baseAlpha: 基础平滑因子(0-1之间),值越小滤波效果越强
    // 返回值: 经过滤波后的值
    // 该函数会根据输入信号的变化幅度自动调整滤波强度
    // 当信号变化剧烈时,滤波强度减弱以保持响应速度
    // 当信号变化平缓时,滤波强度增强以获得更好的平滑效果
    // 计算输入信号的变化幅度
    float delta = std::abs(newValue - currentValue);
    
    // 动态调整平滑因子
    // X轴: 1920 resolution = 193mm = 6400*2.57 counts
    // Y轴: 1920 resolution = 145mm = 6400*1.93 counts
    // 取较大值作为基准，X轴1个resolution约等于8.57个counts (386/45)
    // 因此对于counts单位，设置变化幅度基准为1920 * 8.57 ≈ 16454
    
    float alpha = baseAlpha + (1.0f - baseAlpha) * (delta / 12000);
    alpha = std::min(1.0f, std::max(baseAlpha, alpha)); // 确保alpha在合理范围内

    return static_cast<int>(alpha * newValue + (1 - alpha) * currentValue);
} 

class KalmanFilter2D {
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
    
public:
    KalmanFilter2D(double dt) : dt(dt) {
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

    std::array<double, 6> step(const std::array<double, 2>& measured_pos) {
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
};

