#include <cmath>
#include <algorithm>

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

