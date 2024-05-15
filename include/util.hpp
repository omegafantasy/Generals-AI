#pragma once
#include <cassert>
#include <cmath>
#include <set>
#include <vector>

#include "gamestate.hpp"

inline bool samepos(char *a, std::pair<int, int> b) { return a[0] == b.first && a[1] == b.second; }

/* ### `bool call_generals(GameState &gamestate, int player, std::pair<int, int> location)`

* 描述：在指定位置召唤副将。
* 参数：
  * `GameState &gamestate`：游戏状态对象的引用。
  * `int player`：召唤副将的玩家编号。
  * `std::pair<int, int> location`：召唤副将的位置。
* 返回值：如果召唤成功，返回 `true`；否则返回 `false`。
*/
bool call_generals(GameState &gamestate, int player, std::pair<int, int> location) {
    // 如果玩家的硬币少于50，返回false
    if (gamestate.coin[player] < 50) {
        return false;
    }
    // 如果棋盘上的位置不属于玩家，返回false
    else if (gamestate.board[location.first][location.second].player != player) {
        return false;
    }
    // 如果棋盘上的位置已经有将军，返回false
    else if (gamestate.board[location.first][location.second].generals.type != 0) {
        return false;
    }

    // 创建一个新的将军
    auto genPtr = SubGenerals(gamestate.next_generals_id, player, location);
    // 将新的将军放置在棋盘上的指定位置
    gamestate.board[location.first][location.second].generals = genPtr;
    // 将新的将军添加到游戏状态的将军列表中
    gamestate.general_pos.push_back(location.first * 15 + location.second);
    // 将军的ID增加1
    gamestate.next_generals_id += 1;
    // 玩家的硬币减少50
    gamestate.coin[player] -= 50;
    // 如果所有的条件都满足，返回true
    return true;
}

/*
### `float compute_attack(Cell cell, GameState gamestate)`

* 描述：计算指定格子的攻击力。
* 参数：
  * `Cell cell`：目标格子。
  * `GameState gamestate`：游戏状态对象。
* 返回值：攻击力值。
 */
float compute_attack(Cell& cell, GameState& gamestate) {
    float attack = 1.0;
    int cell_x = cell.position[0];
    int cell_y = cell.position[1];
    // 遍历cell周围至少5*5的区域，寻找里面是否有将军，他们是否使用了增益或减益技能
    for (int i = -2; i <= 2; ++i) {
        for (int j = -2; j <= 2; ++j) {
            int x = cell_x + i;
            int y = cell_y + j;
            if (0 <= x && x < Constant::row && 0 <= y && y < Constant::col) {
                Cell &neighbor_cell = gamestate.board[x][y];
                if (neighbor_cell.generals.type != 0) {
                    if (neighbor_cell.player == cell.player && neighbor_cell.generals.skill_duration[0] > 0) {
                        attack = attack * 1.5;
                    }
                    if (neighbor_cell.player != cell.player && neighbor_cell.generals.skill_duration[2] > 0) {
                        attack = attack * 0.75;
                    }
                }
            }
        }
    }
    // 考虑gamestate中的超级武器是否被激活，（可以获取到激活的位置）该位置的军队是否会被影响
    for (SuperWeapon &active_weapon : gamestate.active_super_weapon) {
        if (active_weapon.type == WeaponType::ATTACK_ENHANCE && cell_x - 1 <= active_weapon.position[0] &&
            active_weapon.position[0] <= cell_x + 1 && cell_y - 1 <= active_weapon.position[1] &&
            active_weapon.position[1] <= cell_y + 1 && active_weapon.player == cell.player) {
            attack = attack * 3;
            break;
        }
    }

    return attack;
}

/* ### `float compute_defence(Cell cell, GameState gamestate)`

* 描述：计算指定格子的防御力。
* 参数：
  * `Cell cell`：目标格子。
  * `GameState gamestate`：游戏状态对象。
* 返回值：防御力值。 */
float compute_defence(Cell& cell, GameState& gamestate) {
    float defence = 1.0;
    int cell_x = cell.position[0];
    int cell_y = cell.position[1];
    // 遍历cell周围至少5*5的区域，寻找里面是否有将军，他们是否使用了增益或减益技能
    for (int i = -2; i <= 2; ++i) {
        for (int j = -2; j <= 2; ++j) {
            int x = cell_x + i;
            int y = cell_y + j;
            if (0 <= x && x < Constant::row && 0 <= y && y < Constant::col) {
                Cell &neighbor_cell = gamestate.board[x][y];
                if (neighbor_cell.generals.type != 0) {
                    if (neighbor_cell.player == cell.player && neighbor_cell.generals.skill_duration[1] > 0) {
                        defence = defence * 1.5;
                    }
                    if (neighbor_cell.player != cell.player && neighbor_cell.generals.skill_duration[2] > 0) {
                        defence = defence * 0.75;
                    }
                }
            }
        }
    }
    // 考虑cell上是否有general，它的防御力是否被升级
    if (cell.generals.type != 0) {
        defence *= cell.generals.defence_level;
    }
    // 考虑gamestate中的超级武器是否被激活，（可以获取到激活的位置）该位置的军队是否会被影响
    for (SuperWeapon &active_weapon : gamestate.active_super_weapon) {
        if (active_weapon.type == WeaponType::ATTACK_ENHANCE && cell_x - 1 <= active_weapon.position[0] &&
            active_weapon.position[0] <= cell_x + 1 && cell_y - 1 <= active_weapon.position[1] &&
            active_weapon.position[1] <= cell_y + 1 && active_weapon.player == cell.player) {
            defence = defence * 3;
            break;
        }
    }

    return defence;
}

/* ### `bool outrange(std::pair<int, int> location)`

* 描述：检查位置是否越界。
* 参数：
  * `std::pair<int, int> location`：待检查的位置。
* 返回值：如果越界，返回 `true`；否则返回 `false`。 */
inline bool outrange(std::pair<int, int> location) {
    return location.first < 0 || location.first >= Constant::row || location.second < 0 ||
           location.second >= Constant::col;
}

/* ### `std::pair<int, int> calculate_new_pos(std::pair<int, int> location, Direction direction)`

* 描述：根据移动方向计算新的位置。
* 参数：
  * `std::pair<int, int> location`：当前位置。
  * `Direction direction`：移动方向。
* 返回值：新的位置坐标。 */
std::pair<int, int> calculate_new_pos(std::pair<int, int> location, Direction direction) {
    std::pair<int, int> new_location = {0, 0};
    if (direction == Direction::UP) {
        new_location.first = location.first - 1;
        new_location.second = location.second;
    } else if (direction == Direction::DOWN) {
        new_location.first = location.first + 1;
        new_location.second = location.second;
    } else if (direction == Direction::LEFT) {
        new_location.first = location.first;
        new_location.second = location.second - 1;
    } else if (direction == Direction::RIGHT) {
        new_location.first = location.first;
        new_location.second = location.second + 1;
    }
    if (outrange(new_location)) {
        return {-1, -1};
    }
    return new_location;
}

/* ### `bool army_move(const std::pair<int, int> location, GameState &gamestate, int player, Direction direction, int
num)`

* 描述：执行军队移动操作。
* 参数：
  * `std::pair<int, int> location`：起始位置。
  * `GameState &gamestate`：游戏状态对象的引用。
  * `int player`：执行移动的玩家编号。
  * `Direction direction`：移动方向。
  * `int num`：移动的兵力数量。
* 返回值：如果移动成功，返回 `true`；否则返回 `false`。 */
bool army_move(const std::pair<int, int> location, GameState &gamestate, int player, Direction direction, int num) {
    int x = location.first, y = location.second;
    if (outrange(location)) return false;                      // 越界
    if (player != 0 && player != 1) return false;              // 玩家参数非法
    if (gamestate.board[x][y].player != player) return false;  // 操作格子非法
    if (gamestate.rest_move_step[player] == 0) return false;
    if (gamestate.board[x][y].army <= 1) return false;

    if (num <= 0) return false;                   // 移动数目非法
    if (num >= gamestate.board[x][y].army - 1) {  // 超过最多移动兵力
        num = gamestate.board[x][y].army - 1;
    }

    for (SuperWeapon &sw : gamestate.active_super_weapon) {  // 超级武器效果
        if (samepos(sw.position, location) && sw.rest && sw.type == WeaponType::TRANSMISSION &&
            sw.player == gamestate.board[x][y].player) {  // 超时空传送眩晕
            return false;
        }
        if (std::abs(sw.position[0] - x) <= 1 && std::abs(sw.position[1] - y) <= 1 && sw.rest &&
            sw.type == WeaponType::TIME_STOP) {  // 时间暂停效果
            return false;
        }
    }
    std::pair<int, int> new_position = calculate_new_pos(location, direction);
    int newX = new_position.first, newY = new_position.second;
    if (newX < 0) return false;                                                                         // 越界
    if (gamestate.board[newX][newY].type == CellType::SWAMP && gamestate.tech_level[player][1] == 0) {  // 不能经过沼泽
        return false;
    }
    // gamestate.changed_cells.push_back(std::make_pair(x, y));
    // gamestate.changed_cells.push_back(std::make_pair(newX, newY));

    if (gamestate.board[newX][newY].player == player) {  // 目的地格子己方所有
        gamestate.board[newX][newY].army += num;
        gamestate.board[x][y].army -= num;
    } else if (gamestate.board[newX][newY].player == 1 - player ||
               gamestate.board[newX][newY].player == -1) {  // 攻击敌方或无主格子
        float attack = compute_attack(gamestate.board[x][y], gamestate);
        float defence = compute_defence(gamestate.board[newX][newY], gamestate);
        float vs = num * attack - gamestate.board[newX][newY].army * defence;
        if (vs > 0.001) {  // 攻下
            gamestate.board[newX][newY].player = player;
            gamestate.board[newX][newY].army = (int)(std::ceil(vs / attack));
            gamestate.board[x][y].army -= num;
            if (gamestate.board[newX][newY].generals.type != 0) {  // 将军易主
                gamestate.board[newX][newY].generals.player = player;
            }
        } else if (vs < -0.001) {  // 防住
            gamestate.board[newX][newY].army = (int)(std::ceil((-vs) / defence));
            gamestate.board[x][y].army -= num;
        } else {  // 中立
            if (gamestate.board[newX][newY].generals.type == 0) {
                gamestate.board[newX][newY].player = -1;
            }
            gamestate.board[newX][newY].army = 0;
            gamestate.board[x][y].army -= num;
        }
    }
    gamestate.rest_move_step[player] -= 1;
    return true;
}
std::pair<bool, int> check_general_movement(const std::pair<int, int> location, GameState &gamestate, int player,
                                            const std::pair<int, int> destination) {
    int x = location.first, y = location.second;
    int newX = destination.first, newY = destination.second;
    if(newX==x&&newY==y) return std::make_pair(false, -1);

    if (outrange(location)) {
        return std::make_pair(false, -1);  // 越界
    }

    if (player != 0 && player != 1) {
        return std::make_pair(false, -1);  // 玩家非法
    }

    if (gamestate.board[x][y].player != player || gamestate.board[newX][newY].player != player||gamestate.board[x][y].generals.type == 0) {
        return std::make_pair(false, -1);  
    }
    // 油井不能移动
    if (gamestate.board[x][y].generals.type == 3) {
        return std::make_pair(false, -1);
    }
    if(gamestate.board[newX][newY].generals.type!=0) return std::make_pair(false, -1);

    for (SuperWeapon &sw : gamestate.active_super_weapon) {
        if (samepos(sw.position, location) && sw.rest && sw.type == WeaponType::TRANSMISSION &&
            sw.player == gamestate.board[x][y].player) {
            return std::make_pair(false, -1);  // 超时空传送眩晕
        }
        if (std::abs(sw.position[0] - x) <= 1 && std::abs(sw.position[1] - y) <= 1 && sw.rest &&
            sw.type == WeaponType::TIME_STOP) {
            return std::make_pair(false, -1);  // 时间暂停效果
        }
    }

    return std::make_pair(true, 1);
    // // bfs检查可移动性
    // int op = -1, cl = 0;
    // std::vector<std::pair<int, int>> queue;
    // std::vector<int> steps;
    // std::vector<std::vector<bool>> check(25, std::vector<bool>(25, false));
    // queue.push_back(std::make_pair(x, y));
    // steps.push_back(0);
    // check[x][y] = true;

    // std::vector<std::pair<int, int>> directions = {{-1, 0}, {1, 0}, {0, -1}, {0, 1}};

    // while (op < cl) {
    //     op += 1;
    //     if (steps[op] > gamestate.board[x][y].generals.rest_move) {
    //         break;  // 步数超限
    //     }
    //     if (queue[op] == std::make_pair(newX, newY)) {
    //         return std::make_pair(true, steps[op]);  // 到达目的地
    //     }
    //     int p = queue[op].first, q = queue[op].second;
    //     for (const auto &direction : directions) {
    //         int newP = p + direction.first, newQ = q + direction.second;
    //         if (outrange({newP, newQ})) {
    //             continue;  // 越界
    //         }
    //         if (check[newP][newQ]) {
    //             continue;  // 已入队
    //         }
    //         if (gamestate.board[newP][newQ].type == CellType::SWAMP && gamestate.tech_level[player][1] == 0) {
    //             continue;  // 无法经过沼泽
    //         }
    //         if (gamestate.board[newP][newQ].player != player || gamestate.board[newP][newQ].generals.type != 0) {
    //             continue;  // 目的地格子非法
    //         }
    //         queue.push_back(std::make_pair(newP, newQ));  // 入队
    //         cl += 1;
    //         steps.push_back(steps[op] + 1);
    //         check[newP][newQ] = true;
    //     }
    // }

    // // bfs结束，没到达目的地
    // return std::make_pair(false, -1);
}

bool general_move(const std::pair<int, int> location, GameState &gamestate, int player,
                  const std::pair<int, int> destination) {  // 将军移动（参数须保证合法）
    std::pair<bool, int> able = check_general_movement(location, gamestate, player, destination);
    if (!able.first) return false;
    int x = location.first, y = location.second;
    int newX = destination.first, newY = destination.second;
    // gamestate.changed_cells.push_back(location);
    // gamestate.changed_cells.push_back(destination);
    gamestate.board[newX][newY].generals = gamestate.board[x][y].generals;
    gamestate.board[newX][newY].generals.position[0] = newX;
    gamestate.board[newX][newY].generals.position[1] = newY;
    gamestate.board[newX][newY].generals.rest_move -= able.second;
    auto it = std::find(gamestate.general_pos.begin(), gamestate.general_pos.end(), x * 15 + y);
    gamestate.general_pos[std::distance(gamestate.general_pos.begin(), it)] = newX * 15 + newY;
    gamestate.board[x][y].generals = NullGenerals();

    return true;
}

bool army_rush(std::pair<int, int> location, GameState &gamestate, int player, std::pair<int, int> destination) {
    int x = location.first, y = location.second;                // 获取当前位置
    int new_x = destination.first, new_y = destination.second;  // 获取目标位置
    int num = gamestate.board[x][y].army - 1;                   // 计算移动的军队数量

    // 如果目标位置没有玩家
    if (gamestate.board[new_x][new_y].player == -1) {
        gamestate.board[new_x][new_y].army += num;
        gamestate.board[x][y].army -= num;
        gamestate.board[new_x][new_y].player = player;  // 设置目标位置的玩家
    }
    // 如果目标位置是当前玩家
    else if (gamestate.board[new_x][new_y].player == player) {
        gamestate.board[x][y].army -= num;          // 减少当前位置的军队数量
        gamestate.board[new_x][new_y].army += num;  // 增加目标位置的军队数量
    }
    // 如果目标位置是对手玩家
    else if (gamestate.board[new_x][new_y].player == 1 - player) {
        float attack = compute_attack(gamestate.board[x][y], gamestate);            // 计算攻击力
        float defence = compute_defence(gamestate.board[new_x][new_y], gamestate);  // 计算防御力
        float vs = num * attack - gamestate.board[new_x][new_y].army * defence;     // 计算战斗结果
        assert(vs > 0.001);                                                         // 确保战斗结果为正
        gamestate.board[new_x][new_y].player = player;                              // 设置目标位置的玩家
        gamestate.board[new_x][new_y].army = (int)(std::ceil(vs / attack));         // 设置目标位置的军队数量
        gamestate.board[x][y].army -= num;                                          // 减少当前位置的军队数量
    }
    return true;  // 返回成功
}


bool check_rush_param(int player, std::pair<int, int> destination, std::pair<int, int> location, GameState &gamestate) {
    int x = location.first, y = location.second;                // 获取当前位置
    int x_new = destination.first, y_new = destination.second;  // 获取目标位置

    // 检查参数合理性
    if (gamestate.board[x][y].generals.type == 0) return false;  // 如果当前位置没有将军，返回失败
    if (gamestate.board[x_new][y_new].generals.type != 0) return false;  // 如果目标位置有将军，返回失败
    if (gamestate.board[x][y].army < 2) return false;
    if (static_cast<int>(gamestate.board[x_new][y_new].type) == 2 && gamestate.tech_level[player][1] == 0) {
        return false;
    }

    if (gamestate.board[x_new][y_new].player == 1 - player) {                       // 如果目标位置是对手玩家
        int num = gamestate.board[x][y].army - 1;                                   // 计算移动的军队数量
        float attack = compute_attack(gamestate.board[x][y], gamestate);            // 计算攻击力
        float defence = compute_defence(gamestate.board[x_new][y_new], gamestate);  // 计算防御力
        float vs = num * attack - gamestate.board[x_new][y_new].army * defence;     // 计算战斗结果
        if (vs < 0.001) return false;  // 如果战斗结果为负，返回失败
    }
    return true;  // 返回成功
}

// 处理突破
/* ### `bool handle_breakthrough(std::pair<int, int> destination, GameState &gamestate)`

* 描述：处理突破技能（注，此函数不检查合法性）。
* 参数：
  * `std::pair<int, int> destination`：目标位置。
  * `GameState &gamestate`：游戏状态对象的引用。
* 返回值：如果处理成功，返回 `true`；否则返回 `false`。 */
bool handle_breakthrough(std::pair<int, int> destination, GameState &gamestate) {
    int x = destination.first, y = destination.second;  // 获取目标位置

    // 如果目标位置的军队数量大于20
    if (gamestate.board[x][y].army > 20) {
        gamestate.board[x][y].army -= 20;  // 减少目标位置的军队数量
    } else {
        gamestate.board[x][y].army = 0;                  // 设置目标位置的军队数量为0
        if (gamestate.board[x][y].generals.type == 0) {  // 如果目标位置没有将军
            gamestate.board[x][y].player = -1;           // 设置目标位置没有玩家
        }
    }
    return true;  // 返回成功
}

/* ### `bool skill_activate(int player, std::pair<int, int> location, std::pair<int, int> destination, GameState
&gamestate, SkillType skillType)`

* 描述：激活将军技能。
* 参数：
  * `int player`：执行技能的玩家编号。
  * `std::pair<int, int> location`：将军当前位置。
  * `std::pair<int, int> destination`：目标位置。
  * `GameState &gamestate`：游戏状态对象的引用。
  * `SkillType skillType`：要激活的技能类型。
* 返回值：如果激活成功，返回 `true`；否则返回 `false`。 */
bool skill_activate(int player, std::pair<int, int> location, std::pair<int, int> destination, GameState &gamestate,
                    SkillType skillType) {
    // 检查参数范围
    if (player != 0 && player != 1) {
        return false;  // 如果玩家参数不是0或1，则返回false
    }
    int x = location.first, y = location.second;
    if (x < 0 || x > Constant::row || y < 0 || y > Constant::col) {
        return false;  // 如果位置坐标超出范围，则返回false
    }

    // 检查参数合理性
    if (gamestate.board[x][y].player != player) {
        return false;  // 如果指定位置上的玩家不是当前玩家，则返回false
    }
    int coin = gamestate.coin[player];
    Generals &general = gamestate.board[location.first][location.second].generals;
    if (general.type == 0) return false;                   // 如果指定位置上没有将领，则返回false
    for (SuperWeapon &sw : gamestate.active_super_weapon)  // 超级武器效果
    {
        if (samepos(sw.position, location) && sw.rest && sw.type == WeaponType::TRANSMISSION &&
            sw.player == gamestate.board[x][y].player) {
            return false;  // 超时空传送眩晕
        }
        if (std::abs(sw.position[0] - x) <= 1 && std::abs(sw.position[1] - y) <= 1 && sw.rest &&
            sw.type == WeaponType::TIME_STOP) {
            return false;  // 时间暂停效果
        }
    }

    if (skillType == SkillType::SURPRISE_ATTACK) {
        bool is_param_legal = check_rush_param(player, destination, location, gamestate);
        if (is_param_legal == false) {
            return false;  // 如果突袭技能的参数不合法，则返回false
        }
        if (coin >= Constant::tactical_strike && general.skills_cd[0] == 0) {
            // general_move(location, gamestate, player, destination);
            general.skills_cd[0] = 5;
            army_rush(location, gamestate, player, destination);
            gamestate.board[destination.first][destination.second].generals = general;
            gamestate.board[destination.first][destination.second].generals.position[0] = destination.first;
            gamestate.board[destination.first][destination.second].generals.position[1] = destination.second;
            auto it = std::find(gamestate.general_pos.begin(), gamestate.general_pos.end(), location.first * 15 + location.second);
            gamestate.general_pos[std::distance(gamestate.general_pos.begin(), it)] = destination.first * 15 + destination.second;
            gamestate.board[location.first][location.second].generals = NullGenerals();
            gamestate.coin[player] -= Constant::tactical_strike;
            return true;  // 如果满足使用突袭技能的条件，则返回true
        } else {
            return false;  // 否则返回false
        }
    } else if (skillType == SkillType::ROUT) {
        if (destination != std::pair<int, int>{-1, -1}) {
            int x_new = destination.first, y_new = destination.second;
            if (x_new < 0 || x_new > Constant::row || y_new < 0 || y_new > Constant::col) {
                return false;  // 如果目的地坐标超出范围，则返回false
            }
            int d1 = std::abs(x_new - x);
            int d2 = std::abs(y_new - y);
            if (d1 > 2 || d2 > 2) {
                return false;  // 如果目的地与当前位置之间的距离超过2，则返回false
            }
        }
        if (coin >= Constant::breakthrough && general.skills_cd[1] == 0) {
            handle_breakthrough(destination, gamestate);
            general.skills_cd[1] = 10;
            gamestate.coin[player] -= Constant::breakthrough;
            gamestate.board[location.first][location.second].generals = general;
            return true;  // 如果满足使用突围技能的条件，则返回true
        } else {
            return false;  // 否则返回false
        }
    } else if (skillType == SkillType::COMMAND) {
        if (coin >= Constant::leadership && general.skills_cd[2] == 0) {
            general.skills_cd[2] = 10;
            general.skill_duration[0] = 10;
            gamestate.board[location.first][location.second].generals = general;
            gamestate.coin[player] -= Constant::leadership;
            return true;  // 如果满足使用指挥技能的条件，则返回true
        } else {
            return false;  // 否则返回false
        }
    } else if (skillType == SkillType::DEFENCE) {
        if (coin >= Constant::fortification && general.skills_cd[3] == 0) {
            general.skills_cd[3] = 10;
            general.skill_duration[1] = 10;
            gamestate.board[location.first][location.second].generals = general;
            gamestate.coin[player] -= Constant::fortification;
            return true;  // 如果满足使用防御技能的条件，则返回true
        } else {
            return false;  // 否则返回false
        }
    } else {
        if (coin >= Constant::weakening && general.skills_cd[4] == 0) {
            general.skills_cd[4] = 10;
            general.skill_duration[2] = 10;
            gamestate.board[location.first][location.second].generals = general;
            gamestate.coin[player] -= Constant::weakening;
            return true;  // 如果满足使用削弱技能的条件，则返回true
        } else {
            return false;  // 否则返回false
        }
    }
}

// 处理炸弹单元格的函数
/* ### `bool handle_bomb_cell(GameState &gamestate, int x, int y)`

* 描述：处理炸弹对单元格的影响。
* 参数：
  * `GameState &gamestate`：游戏状态对象的引用。
  * `int x`：单元格的 x 坐标。
  * `int y`：单元格的 y 坐标。
* 返回值：如果处理成功，返回 `true`；否则返回 `false`。 */
bool handle_bomb_cell(GameState &gamestate, int x, int y) {
    Cell &cell = gamestate.board[x][y];
    // 如果单元格中有主将军，将军队数量减半
    if (cell.generals.type == 1) {
        cell.army = (int)(cell.army / 2);
    } else {
        // 否则，清空单元格
        cell.army = 0;
        cell.player = -1;
        cell.generals = NullGenerals();
        auto it = std::find(gamestate.general_pos.begin(), gamestate.general_pos.end(), x * 15 + y);
        if (it != gamestate.general_pos.end()) {
            gamestate.general_pos.erase(it);
        }
    }
    gamestate.board[x][y] = cell;
    return true;
}
// 处理炸弹的函数
/* ### `bool handle_bomb(GameState &gamestate, const std::pair<int, int> &location)`

* 描述：处理使用炸弹的操作，触发炸弹效果。
* 参数：
  * `GameState &gamestate`：游戏状态对象的引用。
  * `const std::pair<int, int> &location`：炸弹爆炸的位置。
* 返回值：如果处理成功，返回 `true`；否则返回 `false`。 */
bool handle_bomb(GameState &gamestate, const std::pair<int, int> &location) {
    // 遍历目标位置周围的单元格
    for (int i = -1; i <= 1; i++) {
        for (int j = -1; j <= 1; j++) {
            int x = location.first + i;
            int y = location.second + j;
            // 如果单元格在棋盘内，处理该单元格
            if (x >= 0 && x < Constant::row && y >= 0 && y < Constant::col) {
                handle_bomb_cell(gamestate, x, y);
            }
        }
    }
    return true;
}

// 使用炸弹的函数
/* ### `bool bomb(GameState &gamestate, const std::pair<int, int> location, int player)`

* 描述：激活炸弹超级武器。
* 参数：
  * `GameState &gamestate`：游戏状态对象的引用。
  * `const std::pair<int, int> location`：炸弹爆炸的位置。
  * `int player`：执行操作的玩家编号。
* 返回值：如果激活成功，返回 `true`；否则返回 `false`。 */
bool bomb(GameState &gamestate, const std::pair<int, int> location, int player) {
    // 检查玩家和位置的有效性
    if (player != 0 && player != 1) {
        return false;
    }
    if (location.first < 0 || location.first > Constant::row) {
        return false;
    }
    if (location.second < 0 || location.second > Constant::col) {
        return false;
    }
    // 检查超级武器是否解锁并且冷却时间为0
    bool is_super_weapon_unlocked = gamestate.super_weapon_unlocked[player];
    int cd = gamestate.super_weapon_cd[player];
    if (is_super_weapon_unlocked && cd == 0) {
        // 激活超级武器
        gamestate.active_super_weapon.push_back(SuperWeapon(WeaponType::NUCLEAR_BOOM, player, 0, 5, location));
        // 设置超级武器的冷却时间
        gamestate.super_weapon_cd[player] = 50;
        // 处理炸弹
        handle_bomb(gamestate, location);
        return true;
    } else {
        return false;
    }
}

// 强化的函数
/* ### `bool strengthen(GameState &gamestate, std::pair<int, int> location, int player)`

* 描述：激活强化超级武器。
* 参数：
  * `GameState &gamestate`：游戏状态对象的引用。
  * `std::pair<int, int> location`：超级武器激活的位置。
  * `int player`：执行操作的玩家编号。
* 返回值：如果激活成功，返回 `true`；否则返回 `false`。 */
bool strengthen(GameState &gamestate, std::pair<int, int> location, int player) {
    // 检查玩家和位置的有效性
    if (player != 0 && player != 1) {
        return false;
    }
    if (location.first < 0 || location.first > Constant::row) {
        return false;
    }
    if (location.second < 0 || location.second > Constant::col) {
        return false;
    }
    // 检查超级武器是否解锁并且冷却时间为0
    bool is_super_weapon_unlocked = gamestate.super_weapon_unlocked[player];
    int cd = gamestate.super_weapon_cd[player];
    if (is_super_weapon_unlocked && cd == 0) {
        // 激活超级武器
        gamestate.active_super_weapon.push_back(SuperWeapon(WeaponType::ATTACK_ENHANCE, player, 5, 5, location));
        // 设置超级武器的冷却时间
        gamestate.super_weapon_cd[player] = 50;
        return true;
    } else {
        return false;
    }
}

/**
 * 在游戏状态中执行传送操作，将指定位置的军队传送到目标位置。
 *
 * @param gamestate 游戏状态对象
 * @param start 起始位置的坐标（x，y）
 * @param to 目标位置的坐标（x，y）
 * @param player 玩家编号
 * @return 如果传送操作成功，则返回true；否则返回false。
 */
bool tp(GameState &gamestate, const std::pair<int, int> start, const std::pair<int, int> to, int player) {
    // 检查玩家编号是否有效
    if (player != 0 && player != 1) {
        return false;
    }

    // 检查起始位置的坐标是否有效
    if (start.first < 0 || start.first > Constant::row || start.second < 0 || start.second > Constant::col) {
        return false;
    }

    // 检查目标位置的坐标是否有效
    if (to.first < 0 || to.first > Constant::row || to.second < 0 || to.second > Constant::col) {
        return false;
    }

    // 检查是否已解锁超级武器并且冷却时间为0
    bool is_super_weapon_unlocked = gamestate.super_weapon_unlocked[player];
    int cd = gamestate.super_weapon_cd[player];
    if (is_super_weapon_unlocked && cd == 0) {
        int x_st = start.first;
        int y_st = start.second;
        int x_to = to.first;
        int y_to = to.second;
        Cell cell_st = gamestate.board[x_st][y_st];
        Cell cell_to = gamestate.board[x_to][y_to];

        // 检查起始位置的cell是否属于当前玩家
        if (cell_st.player != player) {
            return false;
        }

        // 检查目标位置是否已被占据
        if (cell_to.generals.type != 0) {
            return false;
        }
        if (static_cast<int>(cell_to.type) == 2 && gamestate.tech_level[player][1] == 0) {
            return false;
        }

        int num = 0;
        if (cell_st.army == 0 || cell_st.army == 1) {
            return false;
        } else {
            num = cell_st.army - 1;
            cell_st.army = 1;
        }

        cell_to.army = num;
        cell_to.player = player;
        gamestate.board[x_st][y_st] = cell_st;
        gamestate.board[x_to][y_to] = cell_to;
        gamestate.super_weapon_cd[player] = 50;
        gamestate.active_super_weapon.push_back(SuperWeapon(WeaponType::TRANSMISSION, player, 2, 2, to));
        return true;
    } else {
        return false;
    }
}

/**
 * 在游戏状态中执行时间停止操作，冻结指定位置。
 *
 * @param gamestate 游戏状态对象
 * @param location 冻结位置的坐标（x，y）
 * @param player 玩家编号
 * @return 如果时间停止操作成功，则返回true；否则返回false。
 */
bool timestop(GameState &gamestate, const std::pair<int, int> location, int player) {
    // 检查玩家编号是否有效
    if (player != 0 && player != 1) {
        return false;
    }

    int x = location.first;
    int y = location.second;

    // 检查冻结位置的坐标是否有效
    if (x < 0 || x > Constant::row || y < 0 || y > Constant::col) {
        return false;
    }

    // 检查是否已解锁超级武器并且冷却时间为0
    bool is_super_weapon_unlocked = gamestate.super_weapon_unlocked[player];
    int cd = gamestate.super_weapon_cd[player];
    if (is_super_weapon_unlocked && cd == 0) {
        gamestate.active_super_weapon.push_back(SuperWeapon(WeaponType::TIME_STOP, player, 10, 10, location));
        gamestate.super_weapon_cd[player] = 50;
        return true;
    } else {
        return false;
    }
}

/* ### `bool production_up(std::pair<int, int> location, GameState &gamestate, int player)`

* 描述：执行将军生产力升级。
* 参数：
  * `std::pair<int, int> location`：升级将军位置的坐标。
  * `GameState &gamestate`：游戏状态对象的引用。
  * `int player`：执行操作的玩家编号。
* 返回值：如果技术升级成功，返回 `true`；否则返回 `false`。 */
bool production_up(std::pair<int, int> location, GameState &gamestate, int player) {
    if (gamestate.board[location.first][location.second].generals.type != 0) {
        if (gamestate.board[location.first][location.second].player == player) {
            return gamestate.board[location.first][location.second].generals.production_up(location, gamestate, player);
        } else {
            return false;
        }
    } else {
        return false;
    }
}

/* ### `bool defence_up(std::pair<int, int> location, GameState &gamestate, int player)`

* 描述：执行将军防御力升级。
* 参数：
  * `std::pair<int, int> location`：升级将军位置的坐标。
  * `GameState &gamestate`：游戏状态对象的引用。
  * `int player`：执行操作的玩家编号。
* 返回值：如果技术升级成功，返回 `true`；否则返回 `false`。 */
bool defence_up(std::pair<int, int> location, GameState &gamestate, int player) {
    if (gamestate.board[location.first][location.second].generals.type != 0) {
        if (gamestate.board[location.first][location.second].player == player) {
            return gamestate.board[location.first][location.second].generals.defence_up(location, gamestate, player);
        } else {
            return false;
        }
    } else {
        return false;
    }
}

/* ### `bool movement_up(std::pair<int, int> location, GameState &gamestate, int player)`

* 描述：执行将军移动力升级。
* 参数：
  * `std::pair<int, int> location`：升级将军位置的坐标。
  * `GameState &gamestate`：游戏状态对象的引用。
  * `int player`：执行操作的玩家编号。
* 返回值：如果技术升级成功，返回 `true`；否则返回 `false`。 */
bool movement_up(std::pair<int, int> location, GameState &gamestate, int player) {
    if (gamestate.board[location.first][location.second].generals.type != 0) {
        if (gamestate.board[location.first][location.second].player == player) {
            return gamestate.board[location.first][location.second].generals.movement_up(location, gamestate, player);
        } else {
            return false;
        }
    } else {
        return false;
    }
}

/* ### `bool tech_update(int tech_type, GameState &gamestate, int player)`

* 描述：科技升级。
* 参数：
  * `int tech_type`：升级的科技类型。
  * `GameState &gamestate`：游戏状态对象的引用。
  * `int player`：执行操作的玩家编号。
* 返回值：如果技术升级成功，返回 `true`；否则返回 `false`。
 */
bool tech_update(int tech_type, GameState &gamestate, int player) {
    if (tech_type == 0) {
        if (gamestate.tech_level[player][0] == 2) {
            if (gamestate.coin[player] < Constant::army_movement_T1) {
                return false;
            } else {
                gamestate.tech_level[player][0] = 3;
                gamestate.rest_move_step[player] += 1;
                gamestate.coin[player] -= Constant::army_movement_T1;
                return true;
            }
        } else if (gamestate.tech_level[player][0] == 3) {
            if (gamestate.coin[player] < Constant::army_movement_T2) {
                return false;
            } else {
                gamestate.tech_level[player][0] = 5;
                gamestate.rest_move_step[player] += 2;
                gamestate.coin[player] -= Constant::army_movement_T2;
                return true;
            }
        } else {
            return false;
        }
    } else if (tech_type == 1) {
        if (gamestate.tech_level[player][1] == 0) {
            if (gamestate.coin[player] < Constant::swamp_immunity) {
                return false;
            } else {
                gamestate.tech_level[player][1] = 1;
                gamestate.coin[player] -= Constant::swamp_immunity;
                return true;
            }
        } else {
            return false;
        }
    } else if (tech_type == 2) {
        if (gamestate.tech_level[player][2] == 0) {
            if (gamestate.coin[player] < Constant::sand_immunity) {
                return false;
            } else {
                gamestate.tech_level[player][2] = 1;
                gamestate.coin[player] -= Constant::sand_immunity;
                return true;
            }
        } else {
            return false;
        }
    } else if (tech_type == 3) {
        if (gamestate.tech_level[player][3] == 0) {
            if (gamestate.coin[player] < Constant::unlock_super_weapon) {
                return false;
            } else {
                gamestate.tech_level[player][3] = 1;
                gamestate.super_weapon_cd[player] = 10;
                gamestate.super_weapon_unlocked[player] = true;
                gamestate.coin[player] -= Constant::unlock_super_weapon;
                return true;
            }
        } else {
            return false;
        }
    } else {
        return false;
    }
}