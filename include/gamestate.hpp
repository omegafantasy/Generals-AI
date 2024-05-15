#pragma once
#include <algorithm>
#include <cmath>
#include <iostream>
#include <random>
#include <string>
#include <vector>

#include "constant.hpp"

class GameState;
class Generals;

// 将军技能类型
enum class SkillType { SURPRISE_ATTACK = 0, ROUT = 1, COMMAND = 2, DEFENCE = 3, WEAKEN = 4 };

// 将军属性类型
enum class QualityType { PRODUCTION = 0, DEFENCE = 1, MOBILITY = 2 };

// 超级武器类型
enum class WeaponType { NUCLEAR_BOOM = 0, ATTACK_ENHANCE = 1, TRANSMISSION = 2, TIME_STOP = 3 };

// 格子类型
enum class CellType { PLAIN = 0, SAND = 1, SWAMP = 2 };

// 科技类型
enum class TechType { MOBILITY = 0, IMMUNE_SWAMP = 1, IMMUNE_SAND = 2, UNLOCK = 3 };

// 方向类型
enum class Direction { UP = 0, DOWN = 1, LEFT = 2, RIGHT = 3 };

// 将军技能结构体
struct Skill {
    SkillType type = SkillType::SURPRISE_ATTACK;  // 技能类型
    int cd = 0;                                   // 冷却回合数
};

// 超级武器结构体
struct SuperWeapon {
    char player = -1;                            // 所属玩家
    char cd = 0;                                 // 冷却回合数
    char rest = 0;                               // 效果剩余回合数
    char position[2] = {0, 0};                   // 位置坐标
    WeaponType type = WeaponType::NUCLEAR_BOOM;  // 武器类型
    SuperWeapon(WeaponType type, int player, int cd, int rest, std::pair<int, int> position)
        : type(type), player(player), cd(cd), rest(rest) {
        this->position[0] = position.first;
        this->position[1] = position.second;
    }
};

// 将军基类
class Generals {
   public:
    char player = -1;                                                                    // 所属玩家
    char produce_level = 1;                                                              // 生产力等级
    char mobility_level = 1;                                                             // 移动力等级
    char type = -1;                                                                      // 将军类型
    char position[2] = {0, 0};                                                           // 位置坐标
    char skill_duration[3] = {0, 0, 0};                                                  // 技能持续回合数列表
    char skills_cd[5] = {0, 0, 0, 0, 0};                                                 // 技能冷却回合数列表
    char rest_move = 1;                                                                  // 剩余移动步数
    short id = 0;                                                                        // 将军编号
    float defence_level = 1;                                                             // 防御力等级
    bool production_up(std::pair<int, int> location, GameState &gamestate, int player);  // 提升生产力
    bool defence_up(std::pair<int, int> location, GameState &gamestate, int player);     // 提升防御力
    bool movement_up(std::pair<int, int> location, GameState &gamestate, int player);    // 提升移动力
    Generals(int id, int player, std::pair<int, int> position) : id(id), player(player) {
        this->position[0] = position.first;
        this->position[1] = position.second;
    };
    Generals() = default;
};

// 油井类，继承自将军基类
class OilWell : public Generals {
   public:
    // virtual bool production_up(std::pair<int, int> location, GameState &gamestate, int player) override;
    // virtual bool defence_up(std::pair<int, int> location, GameState &gamestate, int player) override;
    // virtual bool movement_up(std::pair<int, int> location, GameState &gamestate, int player) override;
    OilWell(int id, int player, std::pair<int, int> position) : Generals(id, player, position) {
        type = 3;
        mobility_level = 0;
    };
};

// 主将类，继承自将军基类
class MainGenerals : public Generals {
   public:
    std::vector<Skill> skills = {{SkillType::SURPRISE_ATTACK, 5},
                                 {SkillType::ROUT, 10},
                                 {SkillType::COMMAND, 10},
                                 {SkillType::DEFENCE, 10},
                                 {SkillType::WEAKEN, 10}};
    // virtual bool production_up(std::pair<int, int> location, GameState &gamestate, int player) override;
    // virtual bool defence_up(std::pair<int, int> location, GameState &gamestate, int player) override;
    // virtual bool movement_up(std::pair<int, int> location, GameState &gamestate, int player) override;
    MainGenerals(int id, int player, std::pair<int, int> position) : Generals(id, player, position) { type = 1; };
    // copy constructor
    // MainGenerals(const MainGenerals &main_generals) {
    //     this->id = main_generals.id;
    //     this->player = main_generals.player;
    //     this->produce_level = main_generals.produce_level;
    //     this->defence_level = main_generals.defence_level;
    //     this->mobility_level = main_generals.mobility_level;
    //     this->type = main_generals.type;
    //     this->position[0] = main_generals.position[0];
    //     this->position[1] = main_generals.position[1];
    //     for (int i = 0; i < 3; ++i) {
    //         this->skill_duration[i] = main_generals.skill_duration[i];
    //     }
    //     for (int i = 0; i < 5; ++i) {
    //         this->skills_cd[i] = main_generals.skills_cd[i];
    //     }
    //     this->rest_move = main_generals.rest_move;
    //     this->skills = main_generals.skills;
    // }
    MainGenerals() = default;
};

// 副将类，继承自将军基类
class SubGenerals : public Generals {
   public:
    std::vector<Skill> skills = {{SkillType::SURPRISE_ATTACK, 5},
                                 {SkillType::ROUT, 10},
                                 {SkillType::COMMAND, 10},
                                 {SkillType::DEFENCE, 10},
                                 {SkillType::WEAKEN, 10}};
    // virtual bool production_up(std::pair<int, int> location, GameState &gamestate, int player) override;
    // virtual bool defence_up(std::pair<int, int> location, GameState &gamestate, int player) override;
    // virtual bool movement_up(std::pair<int, int> location, GameState &gamestate, int player) override;
    SubGenerals(int id, int player, std::pair<int, int> position) : Generals(id, player, position) { type = 2; };
    // copy constructor
    // SubGenerals(const SubGenerals &sub_generals) {
    //     this->id = sub_generals.id;
    //     this->player = sub_generals.player;
    //     this->produce_level = sub_generals.produce_level;
    //     this->defence_level = sub_generals.defence_level;
    //     this->mobility_level = sub_generals.mobility_level;
    //     this->type = sub_generals.type;
    //     this->position = sub_generals.position;
    //     this->skills_cd = sub_generals.skills_cd;
    //     this->skill_duration = sub_generals.skill_duration;
    //     this->rest_move = sub_generals.rest_move;
    //     this->skills = sub_generals.skills;
    // }
};

class NullGenerals : public Generals {
   public:
    NullGenerals() {
        this->type = 0;
        this->id == -1;
    }
};

// 格子类
class Cell {
   public:
    char position[2] = {0, 0};  // 格子的位置坐标
    char player = -1;
    int army = 0;                        // 格子里的军队数量
    CellType type = CellType::PLAIN;     // 格子的类型
                                         // 控制格子的玩家编号
    Generals generals = NullGenerals();  // 格子上的将军

    Cell() {}  // 默认构造函数，初始化格子对象
    // copy constructor
    // Cell(const Cell &cell) {
    //     this->position[0] = cell.position[0];
    //     this->position[1] = cell.position[1];
    //     this->type = cell.type;
    //     this->player = cell.player;
    //     this->generals = cell.generals;
    //     this->army = cell.army;
    // }
};

// 游戏状态类
class GameState {
   public:
    bool super_weapon_unlocked[2] = {false, false};  // 超级武器是否解锁的列表，解锁了是true，分别对应玩家1，玩家2
    char super_weapon_cd[2] = {-1, -1};  // 超级武器的冷却回合数列表，分别对应玩家1，玩家2
    char tech_level[2][4] = {
        {2, 0, 0, 0},
        {2, 0, 0, 0}};  // 科技等级列表，第一层对应玩家一，玩家二，第二层分别对应行动力，免疫沼泽，免疫流沙，超级武器
    char rest_move_step[2] = {2, 2};
    char winner = -1;
    // 游戏棋盘的二维列表，每个元素是一个Cell对象
    short next_generals_id = 0;
    short round = 1;  // 当前游戏回合数
    int coin[2] = {0, 0};  // 每个玩家的金币数量列表，分别对应玩家1，玩家2
    Cell board[Constant::row][Constant::col];
    std::vector<SuperWeapon> active_super_weapon;
    std::vector<short> general_pos;

    std::vector<Generals> get_generals() {
        short x, y;
        std::vector<Generals> generals;
        for (int i = 0; i < general_pos.size(); i++) {
            x = general_pos[i] / 15;
            y = general_pos[i] % 15;
            generals.push_back(board[x][y].generals);
        }
        return generals;
    }

    std::pair<int, int> find_general_position_by_id(int general_id) {
        for (auto &gen : get_generals()) {
            if (gen.id == general_id) {
                return std::pair<int, int>(gen.position[0], gen.position[1]);
            }
        }
        return std::pair<int, int>(-1, -1);
    }  // 寻找将军id对应的格子，找不到返回(-1,-1)
    void update_round();

    // default constructor
    GameState() = default;

    // copy constructor
    // GameState(const GameState &gamestate) {
    //     this->round = gamestate.round;
    //     this->coin[0] = gamestate.coin[0];
    //     this->coin[1] = gamestate.coin[1];
    //     this->active_super_weapon = gamestate.active_super_weapon;
    //     this->super_weapon_unlocked[0] = gamestate.super_weapon_unlocked[0];
    //     this->super_weapon_unlocked[1] = gamestate.super_weapon_unlocked[1];
    //     this->super_weapon_cd[0] = gamestate.super_weapon_cd[0];
    //     this->super_weapon_cd[1] = gamestate.super_weapon_cd[1];
    //     this->tech_level[0][0] = gamestate.tech_level[0][0];
    //     this->tech_level[0][1] = gamestate.tech_level[0][1];
    //     this->tech_level[0][2] = gamestate.tech_level[0][2];
    //     this->tech_level[0][3] = gamestate.tech_level[0][3];
    //     this->tech_level[1][0] = gamestate.tech_level[1][0];
    //     this->tech_level[1][1] = gamestate.tech_level[1][1];
    //     this->tech_level[1][2] = gamestate.tech_level[1][2];
    //     this->tech_level[1][3] = gamestate.tech_level[1][3];
    //     this->rest_move_step[0] = gamestate.rest_move_step[0];
    //     this->rest_move_step[1] = gamestate.rest_move_step[1];
    //     this->next_generals_id = gamestate.next_generals_id;
    //     this->winner = gamestate.winner;
    //     for (int i = 0; i < Constant::row; ++i) {
    //         for (int j = 0; j < Constant::col; ++j) {
    //             this->board[i][j] = gamestate.board[i][j];
    //         }
    //     }
    //     for (auto gen : gamestate.get_generals()) {
    //         if (gen.type == 1) {
    //             auto new_gen = new MainGenerals(*dynamic_cast<MainGenerals *>(gen));
    //             this->generals.push_back(new_gen);
    //             this->board[new_gen.position[0]][new_gen.position[1]].generals = new_gen;
    //         } else if (gen.type == 2) {
    //             auto new_gen = new SubGenerals(*dynamic_cast<SubGenerals *>(gen));
    //             this->generals.push_back(new_gen);
    //             this->board[new_gen.position[0]][new_gen.position[1]].generals = new_gen;
    //         } else {
    //             auto new_gen = new OilWell(*dynamic_cast<OilWell *>(gen));
    //             this->generals.push_back(new_gen);
    //             this->board[new_gen.position[0]][new_gen.position[1]].generals = new_gen;
    //         }
    //     }
    // }

    // copy assignment
    // GameState &operator=(const GameState &gamestate) {
    //     this->round = gamestate.round;
    //     this->coin[0] = gamestate.coin[0];
    //     this->coin[1] = gamestate.coin[1];
    //     this->active_super_weapon = gamestate.active_super_weapon;
    //     this->super_weapon_unlocked[0] = gamestate.super_weapon_unlocked[0];
    //     this->super_weapon_unlocked[1] = gamestate.super_weapon_unlocked[1];
    //     this->super_weapon_cd[0] = gamestate.super_weapon_cd[0];
    //     this->super_weapon_cd[1] = gamestate.super_weapon_cd[1];
    //     this->tech_level[0][0] = gamestate.tech_level[0][0];
    //     this->tech_level[0][1] = gamestate.tech_level[0][1];
    //     this->tech_level[0][2] = gamestate.tech_level[0][2];
    //     this->tech_level[0][3] = gamestate.tech_level[0][3];
    //     this->tech_level[1][0] = gamestate.tech_level[1][0];
    //     this->tech_level[1][1] = gamestate.tech_level[1][1];
    //     this->tech_level[1][2] = gamestate.tech_level[1][2];
    //     this->tech_level[1][3] = gamestate.tech_level[1][3];
    //     this->rest_move_step[0] = gamestate.rest_move_step[0];
    //     this->rest_move_step[1] = gamestate.rest_move_step[1];
    //     this->next_generals_id = gamestate.next_generals_id;
    //     this->winner = gamestate.winner;
    //     for (int i = 0; i < Constant::row; ++i) {
    //         for (int j = 0; j < Constant::col; ++j) {
    //             this->board[i][j] = gamestate.board[i][j];
    //         }
    //     }
    //     this->generals.clear();
    //     for (auto gen : gamestate.generals) {
    //         if (gen.type == 1) {
    //             auto new_gen = new MainGenerals(*dynamic_cast<MainGenerals *>(gen));
    //             this->generals.push_back(new_gen);
    //             this->board[new_gen.position[0]][new_gen.position[1]].generals = new_gen;
    //         } else if (gen.type == 2) {
    //             auto new_gen = new SubGenerals(*dynamic_cast<SubGenerals *>(gen));
    //             this->generals.push_back(new_gen);
    //             this->board[new_gen.position[0]][new_gen.position[1]].generals = new_gen;
    //         } else {
    //             auto new_gen = new OilWell(*dynamic_cast<OilWell *>(gen));
    //             this->generals.push_back(new_gen);
    //             this->board[new_gen.position[0]][new_gen.position[1]].generals = new_gen;
    //         }
    //     }
    //     return *this;
    // }
};

/* 更新游戏回合信息。 */
void GameState::update_round() {
    for (int i = 0; i < Constant::row; ++i) {
        for (int j = 0; j < Constant::col; ++j) {
            // 将军
            if (this->board[i][j].generals.type != 0) {
                this->board[i][j].generals.rest_move = this->board[i][j].generals.mobility_level;
            }
            if (this->board[i][j].generals.type == 1) {
                if (this->board[i][j].generals.player != -1)
                    this->board[i][j].army += this->board[i][j].generals.produce_level;
            } else if (this->board[i][j].generals.type == 2) {
                if (this->board[i][j].generals.player != -1)
                    this->board[i][j].army += this->board[i][j].generals.produce_level;
            } else if (this->board[i][j].generals.type == 3) {
                if (this->board[i][j].generals.player != -1) {
                    this->coin[this->board[i][j].generals.player] += this->board[i][j].generals.produce_level;
                }
            }
            // 每10回合增兵
            if (this->round % 10 == 0) {
                if (this->board[i][j].player != -1) {
                    this->board[i][j].army += 1;
                }
            }
            // 流沙减兵
            if (this->board[i][j].type == CellType(1) && this->board[i][j].player != -1 && this->board[i][j].army > 0) {
                if (this->tech_level[this->board[i][j].player][2] == 0) {
                    this->board[i][j].army -= 1;
                    if (this->board[i][j].army == 0 && this->board[i][j].generals.type == 0) {
                        this->board[i][j].player = -1;
                    }
                }
            }
        }
    }

    // 超级武器判定
    for (auto &weapon : this->active_super_weapon) {
        if (weapon.type == WeaponType(0)) {
            for (int _i = std::max(0, weapon.position[0] - 1);
                 _i <= std::min(Constant::row - 1, weapon.position[0] + 1); ++_i) {
                for (int _j = std::max(0, weapon.position[1] - 1);
                     _j <= std::min(Constant::col - 1, weapon.position[1] + 1); ++_j) {
                    if (this->board[_i][_j].army > 0) {
                        this->board[_i][_j].army = std::max(0, this->board[_i][_j].army - 3);
                        if (this->board[_i][_j].army == 0 && this->board[_i][_j].generals.type == 0) {
                            this->board[_i][_j].player = -1;
                        }
                    }
                }
            }
        }
    }

    for (auto &i : this->super_weapon_cd) {
        if (i > 0) {
            --i;
        }
    }

    for (auto &weapon : this->active_super_weapon) {
        --weapon.rest;
    }

    // cd和duration 减少
    for (auto &pos : this->general_pos) {
        auto &gen = this->board[pos / 15][pos % 15].generals;
        for (auto &i : gen.skills_cd) {
            if (i > 0) {
                --i;
            }
        }
        for (auto &i : gen.skill_duration) {
            if (i > 0) {
                --i;
            }
        }
    }

    // 移动步数恢复
    this->rest_move_step[0] = this->tech_level[0][0];
    this->rest_move_step[1] = this->tech_level[1][0];

    std::vector<SuperWeapon> filtered_super_weapon;
    for (const auto &weapon : this->active_super_weapon) {
        if (weapon.rest > 0) {
            filtered_super_weapon.push_back(weapon);
        }
    }

    this->active_super_weapon = filtered_super_weapon;

    ++this->round;
}

bool MainGenerals__production_up(std::pair<int, int> location, GameState &gamestate, int player) {
    // 获取将军的生产等级
    int level = gamestate.board[location.first][location.second].generals.produce_level;
    // 根据生产等级选择不同的操作
    switch (level) {
        case 1:  // 如果生产等级为1
            // 检查玩家是否有足够的金币
            if (gamestate.coin[player] < Constant::lieutenant_production_T1 / 2) {
                return false;  // 金币不足，返回false
            } else {
                // 金币足够，提升生产等级为2
                gamestate.board[location.first][location.second].generals.produce_level = 2;
                // 扣除相应的金币
                gamestate.coin[player] -= Constant::lieutenant_production_T1 / 2;
                return true;  // 返回true
            }
            break;  // 跳出switch语句
        case 2:     // 如果生产等级为2
            // 检查玩家是否有足够的金币
            if (gamestate.coin[player] < Constant::lieutenant_production_T2 / 2) {
                return false;  // 金币不足，返回false
            } else {
                // 金币足够，提升生产等级为4
                gamestate.board[location.first][location.second].generals.produce_level = 4;
                // 扣除相应的金币
                gamestate.coin[player] -= Constant::lieutenant_production_T2 / 2;
                return true;  // 返回true
            }
            break;         // 跳出switch语句
        default:           // 如果生产等级不是1或2
            return false;  // 返回false
    }
}

bool MainGenerals__defence_up(std::pair<int, int> location, GameState &gamestate, int player) {
    switch ((int)gamestate.board[location.first][location.second].generals.defence_level) {
        case 1:
            if (gamestate.coin[player] < Constant::lieutenant_defense_T1 / 2) {
                return false;
            } else {
                gamestate.board[location.first][location.second].generals.defence_level = 2;
                gamestate.coin[player] -= Constant::lieutenant_defense_T1 / 2;
            }
            break;
        case 2:
            if (gamestate.coin[player] < Constant::lieutenant_defense_T2 / 2) {
                return false;
            } else {
                gamestate.board[location.first][location.second].generals.defence_level = 3;
                gamestate.coin[player] -= Constant::lieutenant_defense_T2 / 2;
            }
            break;
        default:
            return false;
    }
    return true;
}

bool MainGenerals__movement_up(std::pair<int, int> location, GameState &gamestate, int player) {
    switch (gamestate.board[location.first][location.second].generals.mobility_level) {
        case 1:
            if (gamestate.coin[player] < Constant::general_movement_T1 / 2) {
                return false;
            } else {
                gamestate.board[location.first][location.second].generals.mobility_level = 2;
                gamestate.board[location.first][location.second].generals.rest_move++;
                gamestate.coin[player] -= Constant::general_movement_T1 / 2;
            }
            break;
        case 2:
            if (gamestate.coin[player] < Constant::general_movement_T2 / 2) {
                return false;
            } else {
                gamestate.board[location.first][location.second].generals.mobility_level = 4;
                gamestate.board[location.first][location.second].generals.rest_move += 2;
                gamestate.coin[player] -= Constant::general_movement_T2 / 2;
            }
            break;
        default:
            return false;
    }
    return true;
}

bool SubGenerals__production_up(std::pair<int, int> location, GameState &gamestate, int player) {
    switch (gamestate.board[location.first][location.second].generals.produce_level) {
        case 1:
            if (gamestate.coin[player] < Constant::lieutenant_production_T1) {
                return false;
            } else {
                gamestate.board[location.first][location.second].generals.produce_level = 2;
                gamestate.coin[player] -= Constant::lieutenant_production_T1;
            }
            break;
        case 2:
            if (gamestate.coin[player] < Constant::lieutenant_production_T2) {
                return false;
            } else {
                gamestate.board[location.first][location.second].generals.produce_level = 4;
                gamestate.coin[player] -= Constant::lieutenant_production_T2;
            }
            break;
        default:
            return false;
    }
    return true;
}

bool SubGenerals__defence_up(std::pair<int, int> location, GameState &gamestate, int player) {
    switch ((int)gamestate.board[location.first][location.second].generals.defence_level) {
        case 1:
            if (gamestate.coin[player] < Constant::lieutenant_defense_T1) {
                return false;
            } else {
                gamestate.board[location.first][location.second].generals.defence_level = 2;
                gamestate.coin[player] -= Constant::lieutenant_defense_T1;
            }
            break;
        case 2:
            if (gamestate.coin[player] < Constant::lieutenant_defense_T2) {
                return false;
            } else {
                gamestate.board[location.first][location.second].generals.defence_level = 3;
                gamestate.coin[player] -= Constant::lieutenant_defense_T2;
            }
            break;
        default:
            return false;
    }
    return true;
}

bool SubGenerals__movement_up(std::pair<int, int> location, GameState &gamestate, int player) {
    switch (gamestate.board[location.first][location.second].generals.mobility_level) {
        case 1:
            if (gamestate.coin[player] < Constant::general_movement_T1) {
                return false;
            } else {
                gamestate.board[location.first][location.second].generals.mobility_level = 2;
                gamestate.board[location.first][location.second].generals.rest_move++;
                gamestate.coin[player] -= Constant::general_movement_T1;
            }
            break;
        case 2:
            if (gamestate.coin[player] < Constant::general_movement_T2) {
                return false;
            } else {
                gamestate.board[location.first][location.second].generals.mobility_level = 4;
                gamestate.board[location.first][location.second].generals.rest_move += 2;
                gamestate.coin[player] -= Constant::general_movement_T2;
            }
            break;
        default:
            return false;
    }
    return true;
}

bool OilWell__production_up(std::pair<int, int> location, GameState &gamestate, int player) {
    switch (gamestate.board[location.first][location.second].generals.produce_level) {
        case 1:
            if (gamestate.coin[player] < Constant::OilWell_production_T1) {
                return false;
            } else {
                gamestate.board[location.first][location.second].generals.produce_level = 2;
                gamestate.coin[player] -= Constant::OilWell_production_T1;
            }
            break;
        case 2:
            if (gamestate.coin[player] < Constant::OilWell_production_T2) {
                return false;
            } else {
                gamestate.board[location.first][location.second].generals.produce_level = 4;
                gamestate.coin[player] -= Constant::OilWell_production_T2;
            }
            break;
        case 4:
            if (gamestate.coin[player] < Constant::OilWell_production_T3) {
                return false;
            } else {
                gamestate.board[location.first][location.second].generals.produce_level = 6;
                gamestate.coin[player] -= Constant::OilWell_production_T3;
            }
            break;
        default:
            return false;
    }
    return true;
}

bool OilWell__defence_up(std::pair<int, int> location, GameState &gamestate, int player) {
    if (gamestate.board[location.first][location.second].generals.defence_level == 1) {
        if (gamestate.coin[player] < Constant::OilWell_defense_T1) {
            return false;
        } else {
            gamestate.board[location.first][location.second].generals.defence_level = 1.5;
            gamestate.coin[player] -= Constant::OilWell_defense_T1;
        }
    } else if (gamestate.board[location.first][location.second].generals.defence_level == 1.5) {
        if (gamestate.coin[player] < Constant::OilWell_defense_T2) {
            return false;
        } else {
            gamestate.board[location.first][location.second].generals.defence_level = 2;
            gamestate.coin[player] -= Constant::OilWell_defense_T2;
        }
    } else if (gamestate.board[location.first][location.second].generals.defence_level == 2) {
        if (gamestate.coin[player] < Constant::OilWell_defense_T3) {
            return false;
        } else {
            gamestate.board[location.first][location.second].generals.defence_level = 3;
            gamestate.coin[player] -= Constant::OilWell_defense_T3;
        }
    } else {
        return false;
    }
    return true;
}
bool OilWell__movement_up(std::pair<int, int> location, GameState &gamestate, int player) { return false; }

bool Generals::production_up(std::pair<int, int> location, GameState &gamestate, int player) {
    if (type == 0)
        return false;
    else if (type == 1)
        return MainGenerals__production_up(location, gamestate, player);
    else if (type == 2)
        return SubGenerals__production_up(location, gamestate, player);
    else if (type == 3)
        return OilWell__production_up(location, gamestate, player);
    return false;
}

bool Generals::defence_up(std::pair<int, int> location, GameState &gamestate, int player) {
    if (type == 0)
        return false;
    else if (type == 1)
        return MainGenerals__defence_up(location, gamestate, player);
    else if (type == 2)
        return SubGenerals__defence_up(location, gamestate, player);
    else if (type == 3)
        return OilWell__defence_up(location, gamestate, player);
    return false;
}

bool Generals::movement_up(std::pair<int, int> location, GameState &gamestate, int player) {
    if (type == 0)
        return false;
    else if (type == 1)
        return MainGenerals__movement_up(location, gamestate, player);
    else if (type == 2)
        return SubGenerals__movement_up(location, gamestate, player);
    else if (type == 3)
        return OilWell__movement_up(location, gamestate, player);
    return false;
}
