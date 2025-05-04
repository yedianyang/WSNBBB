#pragma once

#include <vector>
#include <chrono>
#include <cmath>
#include "./json2Route.hpp"

class DynamicAdvancer {
public:
    struct AdvancePoint {
        int x;
        int y;
        bool is_corner;
        Command::Type type;
    };

    DynamicAdvancer(const std::vector<Command>& commands, 
                    float distance_threshold = 0.5,
                    float time_interval_ms = 20) 
        : commands_(interpolate_path(commands)),
          distance_threshold_(distance_threshold),
          time_interval_ms_(time_interval_ms) {}

    // 获取下一个点，如果返回false表示已经结束
    bool getNextPoint(AdvancePoint& point) {
        auto now = std::chrono::high_resolution_clock::now();
        
        if (current_index_ >= commands_.size()) {
            return false;
        }

        // 检查时间间隔
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            now - last_advance_time_).count();
        
        if (elapsed < time_interval_ms_) {
            return false;
        }

        // 获取当前点
        const auto& current = commands_[current_index_];
        
        // 计算与上一个点的距离
        if (current_index_ > 0) {
            const auto& prev = commands_[current_index_ - 1];
            float distance = std::sqrt(
                std::pow(current.x - prev.x, 2) + 
                std::pow(current.y - prev.y, 2));
            
            if (distance < distance_threshold_) {
                return false;
            }
        }

        // 检查是否是拐角
        bool is_corner = false;
        if (current_index_ > 0 && current_index_ < commands_.size() - 1) {
            is_corner = check_corner(
                commands_[current_index_ - 1],
                current,
                commands_[current_index_ + 1]
            );
        }

        // 更新输出点
        point.x = current.x;
        point.y = current.y;
        point.type = current.type;
        point.is_corner = is_corner;

        // 更新状态
        current_index_++;
        last_advance_time_ = now;
        
        return true;
    }

private:
    std::vector<Command> commands_;
    float distance_threshold_;
    float time_interval_ms_;
    size_t current_index_ = 0;
    std::chrono::time_point<std::chrono::high_resolution_clock> last_advance_time_ 
        = std::chrono::high_resolution_clock::now();

    // 路径插值（简化版，可以根据需要扩展）
    std::vector<Command> interpolate_path(const std::vector<Command>& input) {
        std::vector<Command> result;
        const float step = distance_threshold_;  // 使用构造函数传入的距离阈值
        
        for (size_t i = 0; i < input.size() - 1; i++) {
            const auto& p1 = input[i];
            const auto& p2 = input[i + 1];
            
            // 计算两点间距离
            float dx = p2.x - p1.x;
            float dy = p2.y - p1.y;
            float distance = std::sqrt(dx*dx + dy*dy);
            
            // 计算需要插入的点数
            int steps = std::ceil(distance / step);
            
            // 插值
            for (int j = 0; j <= steps; j++) {
                float t = static_cast<float>(j) / steps;
                Command cmd(p1.type, p1.x + dx * t, p1.y + dy * t);
                result.push_back(cmd);
            }
        }
        
        // 确保添加最后一个点
        if (!input.empty()) {
            result.push_back(input.back());
        }
        
        return result;
    }

    // 检查是否是拐角
    bool check_corner(const Command& prev, const Command& curr, const Command& next) {
        // 计算两个向量
        float dx1 = curr.x - prev.x;
        float dy1 = curr.y - prev.y;
        float dx2 = next.x - curr.x;
        float dy2 = next.y - curr.y;
        
        // 计算向量夹角
        float dot = dx1 * dx2 + dy1 * dy2;
        float mag1 = std::sqrt(dx1 * dx1 + dy1 * dy1);
        float mag2 = std::sqrt(dx2 * dx2 + dy2 * dy2);
        
        if (mag1 * mag2 == 0) return false;
        
        float cos_angle = dot / (mag1 * mag2);
        // 角度大于45度认为是拐角
        return cos_angle < 0.7071; // cos(45°)
    }
};