#pragma once
#include <iostream>
#include <vector>

#include "gamestate.hpp"
#include "operation.hpp"
#include "protocol.hpp"
#include "util.hpp"
/* ## `class GameController`

游戏控制器。可以帮助选手处理游戏流程有关的繁琐操作。

### 成员变量

#### `int my_seat`

* 描述：标识玩家的先后手。
* 可取值：0 或 1，其中 0 代表先手，1 代表后手。

#### `GameState game_state`

* 描述：游戏状态对象。
* 类型：`GameState`。

#### `std::vector<Operation> my_operation_list`

* 描述：存储我方未发送的操作列表。
* 类型：`std::vector<Operation>`。 */
class GameController {
   public:
    int my_seat = 0;                           // 您的位置。0代表您是先手，1代表您是后手。
    GameState game_state;                      // 游戏状态，一个 GameState 类的对象。
    std::vector<Operation> my_operation_list;  // 我方未发送的操作列表。

    void init();
    bool execute_single_command(int player, const Operation &op);
    std::vector<Operation> read_enemy_ops();
    bool apply_enemy_ops(const std::vector<Operation> &ops);
    bool read_and_apply_enemy_ops();
    bool try_apply_our_op(const Operation &op);
    bool try_apply_our_ops(const std::vector<Operation> &ops);
    void finish_and_send_our_ops();
    void update_round_info();
};

/* #### `bool execute_single_command(int player, const Operation &op)`

* 描述：执行单个操作。
* 参数：
  * `int player`：操作所属玩家，取值为 0 或 1。
  * `const Operation &op`：待执行的操作。
* 返回值：操作是否成功执行，成功返回 true，否则返回 false */
bool GameController::execute_single_command(int player, const Operation &op) {
    // 获取操作码和操作数
    OperationType command = op.opcode;
    std::vector<int> params = op.operand;

    // 根据操作码执行相应的操作
    switch (command) {
        case OperationType::MOVE_ARMY: {
            // 移动军队
            return army_move({params[0], params[1]}, game_state, player, static_cast<Direction>(params[2] - 1),
                             params[3]);
        }
        case OperationType::MOVE_GENERALS: {
            // 移动将军
            int id = params[0];
            auto pos = game_state.find_general_position_by_id(id);
            if (pos.first == -1) break;
            return general_move(pos, game_state, player, {params[1], params[2]});
        }
        case OperationType::UPDATE_GENERALS: {
            // 更新将军
            int id = params[0];
            auto pos = game_state.find_general_position_by_id(id);
            if (pos.first == -1) break;
            switch (params[1]) {
                case 1:
                    // 提升生产力
                    return production_up(pos, game_state, player);
                case 2:
                    // 提升防御力
                    return defence_up(pos, game_state, player);
                case 3:
                    // 提升移动力
                    return movement_up(pos, game_state, player);
            }

            break;
        }
        case OperationType::USE_GENERAL_SKILLS: {
            // 使用将军技能
            int id = params[0];
            auto pos = game_state.find_general_position_by_id(id);
            if (pos.first == -1) break;
            if (params[1] == 1 || params[1] == 2)
                return skill_activate(player, pos, {params[2], params[3]}, game_state,
                                      static_cast<SkillType>(params[1] - 1));
            else
                return skill_activate(player, pos, {-1, -1}, game_state, static_cast<SkillType>(params[1] - 1));
        }
        case OperationType::UPDATE_TECH: {
            // 更新科技
            return tech_update(params[0] - 1, game_state, player);
        }
        case OperationType::USE_SUPERWEAPON: {
            // 使用超级武器
            switch (params[0]) {
                case 1:
                    // 炸弹
                    return bomb(game_state, {params[1], params[2]}, player);
                case 2:
                    // 强化
                    return strengthen(game_state, {params[1], params[2]}, player);
                case 3:
                    // 传送
                    return tp(game_state, {params[3], params[4]}, {params[1], params[2]}, player);
                case 4:
                    // 停止时间
                    return timestop(game_state, {params[1], params[2]}, player);
            }
            break;
        }
        case OperationType::CALL_GENERAL: {
            // 召唤将军
            return call_generals(game_state, player, {params[0], params[1]});
        }
        default: {
            // 如果没有匹配的操作码，返回false
            return false;
        }
    }
    return false;
}

/* #### `void init()`

* 描述：初始化游戏。
* 参数：无。
* 返回值：无。 */
void GameController::init() {
    // 初始化游戏。
    std::tie(my_seat, game_state) = read_init_map();
}

/* #### `std::vector<Operation> read_enemy_ops()`

* 描述：读取敌方操作列表。
* 参数：无。
* 返回值：包含敌方操作的操作列表，类型为 `std::vector<Operation>`。 */
std::vector<Operation> GameController::read_enemy_ops() {
    // 读取敌方操作列表。
    return read_enemy_operations();
}

/* #### `bool apply_enemy_ops(const std::vector<Operation> &ops)`

* 描述：应用敌方操作。
* 参数：
  * `const std::vector<Operation> &ops`：待应用的敌方操作列表。
* 返回值：如果所有敌方操作合法且成功应用，则返回 true，否则返回 false。 */
bool GameController::apply_enemy_ops(const std::vector<Operation> &ops) {
    // 应用敌方操作。如果返回false说明敌方操作不合法。
    for (const auto &op : ops) {
        if (!execute_single_command(1 - my_seat, op)) {
            return false;
        }
    }
    return true;
}

/* #### `bool read_and_apply_enemy_ops()`

* 描述：读取并应用敌方操作。
* 参数：无。
* 返回值：如果所有敌方操作合法且成功应用，则返回 true，否则返回 false。 */
bool GameController::read_and_apply_enemy_ops() {
    // 读取并应用敌方操作。如果返回false说明敌方操作不合法。
    return apply_enemy_ops(read_enemy_ops());
}

/* #### `bool try_apply_our_op(const Operation &op)`

* 描述：尝试执行我方单个操作。
* 参数：
  * `const Operation &op`：待尝试执行的我方单个操作。
* 返回值：如果操作合法且成功执行，则返回 true，否则返回 false。 */
bool GameController::try_apply_our_op(const Operation &op) {
    // 尝试执行我方操作。如果返回false说明我方操作不合法。
    if (execute_single_command(my_seat, op)) {
        my_operation_list.push_back(op);
        return true;
    }
    return false;
}

/* #### `bool try_apply_our_ops(const std::vector<Operation> &ops)`

* 描述：尝试执行我方若干操作。
* 参数：
  * `const std::vector<Operation> &ops`：待尝试执行的我方操作列表。
* 返回值：如果所有我方操作合法且成功执行，则返回 true，否则返回 false。 */
bool GameController::try_apply_our_ops(const std::vector<Operation> &ops) {
    // 尝试执行我方若干操作。如果返回false说明我方操作不合法，并停止执行后面的操作。
    for (const auto &op : ops) {
        if (!try_apply_our_op(op)) {
            return false;
        }
    }
    return true;
}

/* #### `void finish_and_send_our_ops()`

* 描述：结束我方操作回合，将操作列表打包发送并清空。
* 参数：无。
* 返回值：无。 */
void GameController::finish_and_send_our_ops() {
    // 结束我方操作回合，将操作列表打包发送并清空。
    std::string msg = "";
    for (const auto &op : my_operation_list) {
        msg += op.stringize();
    }
    msg += "8\n";
    write_to_judger(msg);
    my_operation_list.clear();
}

/* #### `void update_round_info()`

* 描述：更新游戏回合信息。
* 参数：无。
* 返回值：无。
 */
void GameController::update_round_info() { game_state.update_round(); }