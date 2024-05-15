#pragma once

#include <functional>
#include <vector>

#include "controller.hpp"
/**
 * @brief 执行AI的函数。将你的决策过程封装在一个函数中，我们帮你处理各类通讯相关的繁琐事项。
 * @param ai
 * 你的决策函数，其第一个参数为`my_seat`，第二个参数为当前的`GameState`对象，返回值为操作序列`std::vector<Operation>`。
 */
void run_ai(std::function<std::vector<Operation>(int, const GameState &)> ai) {
    GameController c;
    c.init();
    while (true) {
        if (c.my_seat == 0)  // 先手
        {
            // AI 给出操作
            std::vector<Operation> ops = ai(c.my_seat, c.game_state);
            // 尝试执行操作
            c.try_apply_our_ops(ops);
            // 向judger发送操作
            c.finish_and_send_our_ops();
            // 读取并应用敌方操作
            c.read_and_apply_enemy_ops();
            // 更新回合
            c.update_round_info();
        } else  // 后手
        {
            // 读取并应用敌方操作
            c.read_and_apply_enemy_ops();
            // AI 给出操作
            std::vector<Operation> ops = ai(c.my_seat, c.game_state);
            // 尝试执行操作
            c.try_apply_our_ops(ops);
            // 向judger发送操作
            c.finish_and_send_our_ops();
            // 更新回合
            c.update_round_info();
        }
    }
}