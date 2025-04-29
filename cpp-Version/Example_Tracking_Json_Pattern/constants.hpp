#ifndef CONSTANTS_HPP


// 备忘参数
const int INKLING_RESOLUTION_X = 1920; //1920 resolution
const int INKLING_RESOLUTION_Y = 1920; //1920 resolution
const int INKLING_LENGTH_X = 193; // mm
const int INKLING_LENGTH_Y = 145; // mm
const int MOTOR_CNT_PER_REVOLUTION = 6400; //6400 counts
const int MOTOR_LENGTH_PER_REVOLUTION = 75; //75mm

// X轴每个分辨率点对应8.57个电机计数(193mm/1920分辨率*6400计数/75mm)
//const float CNT_PER_RESOLUTION_X = (float)MOTOR_CNT_PER_REVOLUTION * INKLING_LENGTH_X / (MOTOR_LENGTH_PER_REVOLUTION * INKLING_RESOLUTION_X); // counts/resolution
// Y轴每个分辨率点对应6.44个电机计数(145mm/1920分辨率*6400计数/75mm)
//const float CNT_PER_RESOLUTION_Y = (float)MOTOR_CNT_PER_REVOLUTION * INKLING_LENGTH_Y / (MOTOR_LENGTH_PER_REVOLUTION * INKLING_RESOLUTION_Y); // counts/resolution
// 同上 - 坐标转换系数
const float CNT_PER_RESOLUTION_X = 386.0f/45.0f;  // X轴位置转换系数
const float CNT_PER_RESOLUTION_Y = 58.0f/9.0f;    // Y轴位置转换系数


// 电机运动控制相关常量
const int ACC_LIM_RPM_PER_SEC = 5000;    // 加速度限制
const int VEL_LIM_RPM = 800;             // 速度限制
const int TIME_TILL_TIMEOUT = 10000;      // 归零超时时间(ms)

// 目标位置相关常量
const int DEFAULT_TARGET_POSITION_X = 960;   // 默认目标位置
const int DEFAULT_TARGET_POSITION_Y = 960;   // 默认目标位置
const int MAX_MOTOR_POSITION_X = 45000;      // 电机最大位置限制
const int MIN_MOTOR_POSITION_X = 0;          // 电机最小位置限制
const int MAX_MOTOR_POSITION_Y = 45000;      // 电机最大位置限制
const int MIN_MOTOR_POSITION_Y = 0;          // 电机最小位置限制


// 自适应滤波器参数
const float DEFAULT_FILTER_ALPHA_X = 0.2f;   // 默认滤波系数
const float DEFAULT_FILTER_ALPHA_Y = 0.2f;   // 默认滤波系数
const float MAX_DELTA_X = 1000.0f;          // 最大变化幅度
const float MAX_DELTA_Y = 1000.0f;          // 最大变化幅度


#endif // CONSTANTS_HPP 