#include <cstdlib>
#include <ctime>
#include <queue>
#include <random>
#define MAX_T0 0.1f
#define MAX_T1 0.15f
#define MAX_T2 0.4f
#define MAX_T3 0.6f
#define MAX_T4 0.85f

#include "include/ai_template.hpp"
bool my;
int now_round;
GameState global_state;

clock_t start_t;
int cerr_cnt = 0;

void printops(std::vector<Operation>& ops) {
    for (auto& op : ops) {
        std::cerr << op.stringize();
    }
}

void show_map(GameState& state) {  // print to stderr
    // write the state information to the file
    std::cerr << "r:" << state.round << "\n";
    std::cerr << "[" << state.coin[0] << ", " << state.coin[1] << "]" << "\n";
    std::cerr << "[[";
    std::cerr << (int)(state.tech_level[0][0]) << ", " << (int)(state.tech_level[0][1]) << ", "
              << (int)(state.tech_level[0][2]) << ", " << (int)(state.tech_level[0][3]);
    std::cerr << "], [";
    std::cerr << (int)(state.tech_level[1][0]) << ", " << (int)(state.tech_level[1][1]) << ", "
              << (int)(state.tech_level[1][2]) << ", " << (int)(state.tech_level[1][3]);
    std::cerr << "]]" << "\n";

    // compute total army
    int total_army[2] = {0, 0};
    for (int i = 0; i < Constant::row; ++i) {
        for (int j = 0; j < Constant::col; ++j) {
            total_army[0] += state.board[i][j].player == 0 ? state.board[i][j].army : 0;
            total_army[1] += state.board[i][j].player == 1 ? state.board[i][j].army : 0;
        }
    }
    std::cerr << "[" << total_army[0] << ", " << total_army[1] << "]\n";

    // for (int j = 14; j >= 0; --j) {
    //     for (int i = 0; i < 15; ++i) {
    //         const auto& cell = state.board[i][j];
    //         std::cerr << "(" << cell.player << "," << cell.army << ") ";
    //     }
    //     std::cerr << "\n";
    // }
    for (const auto& gen : state.get_generals()) {
        std::cerr << gen.id << " ";
        std::cerr << (int)(gen.type) << " ";
        std::cerr << (int)(gen.player) << " ";
        std::cerr << (int)(gen.position[0]) << " ";
        std::cerr << (int)(gen.position[1]) << " ";
        std::cerr << (int)(gen.produce_level) << " ";
        std::cerr << (int)(gen.mobility_level) << " ";
        std::cerr << gen.defence_level << " ";
        std::cerr << state.board[gen.position[0]][gen.position[1]].army << "\n";
    }
}

inline int find_general_by_pos(GameState& state, std::pair<int, int> pos) {
    for (auto& general : state.get_generals()) {
        if (samepos(general.position, pos)) return general.id;
    }
    return -1;
}

inline int main_general_id(GameState& state, bool player) {
    for (const auto& general : state.get_generals()) {
        if (general.type == 1 && general.player == player) return general.id;
    }
    return -1;
}

inline bool gameend(GameState& state, bool player) {
    // game is end to player;
    int enemy_maingen_id = main_general_id(state, !player);
    if (enemy_maingen_id == -1) return true;
    return false;
}

inline Generals find_general_by_id(GameState& state, int id) {
    for (const auto& pos : state.general_pos) {
        if (state.board[pos / 15][pos % 15].generals.id == id) {
            return state.board[pos / 15][pos % 15].generals;
        }
    }
    return NullGenerals();
}

inline bool execute(GameState& game_state, const Operation& op, bool player) {
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
            // const auto& gen = find_general_by_id(game_state, id);
            auto pos = game_state.find_general_position_by_id(id);
            if (pos.first == -1) break;
            return general_move(pos, game_state, player, {params[1], params[2]});
        }
        case OperationType::UPDATE_GENERALS: {
            // 更新将军
            int id = params[0];
            // const auto& gen = find_general_by_id(game_state, id);
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
            // const auto& gen = find_general_by_id(game_state, id);
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

bool block_list[11];  // if block_list[val] == true, then val=board[i][j] is blocked
int board[15][15];  // -1: default, 0: ours, 1: enemy, 2: our_gen, 3: enemy_gen, 4: our_oil, 5: enemy_oil, 6: neut_gen,
                    // 7: neut_oil, 8: swamp, 9: sand, 10: nuclear_boom
float threat_value[15][15];    // higher means more dangerous to me
int threat_origin[15][15][5];  // threat_origin[i][j][k] means the idx of the position that threatens board[i][j] is
                               // board[threat_origin[i][j][k]/15][threat_origin[i][j][k]%15]
int threat_origin_cnt[15][15];
float impact_value[15][15];  // higher means safer to me
float enemy_impact[15][15];  // if enemy_impact[i][j]==0, then [i][j] is absolute safe to me
float attack_value[15][15];  // TODO
float my_eval, enemy_eval;
const int dx[4] = {-1, 1, 0, 0};
const int dy[4] = {0, 0, -1, 1};  // up, down, left, right

// std::pair<int, int> global_best_oil;
bool block_flag = false;
bool lock_upgrade, lock_move;

inline float cell_attack(int cell_x, int cell_y, bool player, GameState& state) {
    if (player == -1) {
        return 1.0;
    }
    float attack = 1.0;
    // 遍历cell周围至少5*5的区域，寻找里面是否有将军，他们是否使用了增益或减益技能
    for (int i = -2; i <= 2; ++i) {
        for (int j = -2; j <= 2; ++j) {
            int x = cell_x + i;
            int y = cell_y + j;
            if (0 <= x && x < Constant::row && 0 <= y && y < Constant::col) {
                const auto& gen = state.board[x][y].generals;
                if (gen.type != 0) {
                    if (gen.player == player && gen.skill_duration[0] > 0) {
                        attack = attack * 1.5;
                    }
                    if (gen.player != player && gen.skill_duration[2] > 0) {
                        attack = attack * 0.75;
                    }
                }
            }
        }
    }
    // 考虑gamestate中的超级武器是否被激活，（可以获取到激活的位置）该位置的军队是否会被影响
    for (SuperWeapon& active_weapon : state.active_super_weapon) {
        if (active_weapon.type == WeaponType::ATTACK_ENHANCE && cell_x - 1 <= active_weapon.position[0] &&
            active_weapon.position[0] <= cell_x + 1 && cell_y - 1 <= active_weapon.position[1] &&
            active_weapon.position[1] <= cell_y + 1 && active_weapon.player == player) {
            attack = attack * 3;
            break;
        }
    }

    return attack;
}

inline float cell_defence(int cell_x, int cell_y, bool player, GameState& state) {
    if (player == -1) {
        return 1.0;
    }
    float defence = 1.0;
    // 遍历cell周围至少5*5的区域，寻找里面是否有将军，他们是否使用了增益或减益技能
    for (int i = -2; i <= 2; ++i) {
        for (int j = -2; j <= 2; ++j) {
            int x = cell_x + i;
            int y = cell_y + j;
            if (0 <= x && x < Constant::row && 0 <= y && y < Constant::col) {
                const auto& gen = state.board[x][y].generals;
                if (gen.type != 0) {
                    if (gen.player == player && gen.skill_duration[1] > 0) {
                        defence = defence * 1.5;
                    }
                    if (gen.player != player && gen.skill_duration[2] > 0) {
                        defence = defence * 0.75;
                    }
                }
            }
        }
    }
    const auto& cell_gen = state.board[cell_x][cell_y].generals;
    // 考虑cell上是否有general，它的防御力是否被升级
    if (cell_gen.type != 0 && cell_gen.player == player) {
        defence *= cell_gen.defence_level;
    }
    // 考虑gamestate中的超级武器是否被激活，（可以获取到激活的位置）该位置的军队是否会被影响
    for (SuperWeapon& active_weapon : state.active_super_weapon) {
        if (active_weapon.type == WeaponType::ATTACK_ENHANCE && cell_x - 1 <= active_weapon.position[0] &&
            active_weapon.position[0] <= cell_x + 1 && cell_y - 1 <= active_weapon.position[1] &&
            active_weapon.position[1] <= cell_y + 1 && active_weapon.player == player) {
            defence = defence * 3;
            break;
        }
    }

    return defence;
}

int army_board[15][15];                 // for computing threat value
int player_board[15][15];               // for computing threat value
std::vector<int> threat_origin_idx[5];  // for computing threat value
inline void threat_val_prepare(GameState& state) {
    for (int i = 0; i < 15; i++) {
        for (int j = 0; j < 15; j++) {
            army_board[i][j] = state.board[i][j].army;
            player_board[i][j] = state.board[i][j].player;
        }
    }
}
inline float compute_threat_val(int cell_x, int cell_y, bool player,
                                GameState& state) {  // higher means more dangerous to player
    int enemy_movement = state.tech_level[!player][0];
    int enemy_armys_cnt[5];
    float enemy_armys[5][100];
    int enemy_army_origin[5][100];
    float dp[5];
    float cell_def = cell_defence(cell_x, cell_y, player, state);
    float max_atk = 0;
    int enemy_coin_grow = 0;
    for (const auto& gen : state.get_generals()) {
        if (gen.player == !player && gen.type == 3) {
            enemy_coin_grow += gen.produce_level;
        }
    }
    int round_buffer = 2;
    if (player == 1) round_buffer += 1;
    int additional_coin = enemy_coin_grow * round_buffer;
    for (int k = 0; k < 4; k++) {
        int x = cell_x + dx[k], y = cell_y + dy[k];
        if (x < 0 || x >= 15 || y < 0 || y >= 15) continue;
        max_atk = std::max(max_atk, cell_attack(x, y, !player, state));
    }
    memset(enemy_armys_cnt, 0, sizeof(enemy_armys_cnt));
    memset(enemy_armys, 0, sizeof(enemy_armys));
    memset(enemy_army_origin, -1, sizeof(enemy_army_origin));
    for (int k = 0; k < 15; k++) {
        for (int l = 0; l < 15; l++) {
            if (cell_x == k && cell_y == l) continue;
            if ((player_board[k][l] == -1) || (player_board[k][l] == player)) continue;
            int dist = abs(cell_x - k) + abs(cell_y - l);
            if (state.board[k][l].generals.type == 0 || state.board[k][l].generals.type == 3) {
                if (dist <= enemy_movement) {
                    enemy_armys[dist - 1][enemy_armys_cnt[dist - 1]++] = army_board[k][l];
                    enemy_army_origin[dist - 1][enemy_armys_cnt[dist - 1] - 1] = k * 15 + l;
                }
            } else {
                const auto& gen = state.board[k][l].generals;
                int coin_penalty = 0;
                if (gen.skills_cd[0] <= round_buffer &&
                    state.coin[!player] + additional_coin >= Constant::tactical_strike) {
                    dist = std::max(std::max(0, abs(cell_x - k) - 2) + std::max(0, abs(cell_y - l) - 2), 1);
                    if (abs(cell_x - k) + abs(cell_y - l) > 2) {
                        coin_penalty = 20;
                    }
                }
                if (dist <= enemy_movement) {
                    float gen_atk = std::max(cell_attack(k, l, !player, state), max_atk);
                    float buff_atk = 1;
                    int used_coin = 0;
                    if (gen.skill_duration[0] > 0 && gen.skill_duration[2] > 0) {
                        buff_atk = 2;
                    } else if (gen.skill_duration[0] > 0 && gen.skill_duration[2] == 0) {
                        if (state.coin[!player] + additional_coin - coin_penalty >= Constant::weakening) {
                            buff_atk = 2;
                            used_coin = Constant::weakening;
                        } else
                            buff_atk = 1.5;
                    } else if (gen.skill_duration[0] == 0 && gen.skill_duration[2] > 0) {
                        if (state.coin[!player] + additional_coin - coin_penalty >= Constant::leadership) {
                            buff_atk = 2;
                            used_coin = Constant::leadership;
                        } else
                            buff_atk = 1 / 0.75;
                    } else {
                        if (state.coin[!player] + additional_coin - coin_penalty >=
                            Constant::leadership + Constant::weakening) {
                            buff_atk = 2;
                            used_coin = Constant::leadership + Constant::weakening;
                        } else if (state.coin[!player] + additional_coin - coin_penalty >= Constant::leadership) {
                            buff_atk = 1.5;
                            used_coin = Constant::leadership;
                        }
                    }
                    if (state.coin[!player] + additional_coin - coin_penalty - used_coin >=
                        Constant::lieutenant_new_recruit + Constant::leadership + Constant::weakening) {
                        buff_atk *= 2;
                        used_coin += Constant::lieutenant_new_recruit + Constant::leadership + Constant::weakening;
                    } else if (state.coin[!player] + additional_coin - coin_penalty - used_coin >=
                               Constant::lieutenant_new_recruit + Constant::leadership) {
                        buff_atk *= 1.5;
                        used_coin += Constant::lieutenant_new_recruit + Constant::leadership;
                    }
                    gen_atk = std::max(gen_atk, buff_atk);
                    float total_val = (army_board[k][l] - dist) * gen_atk;
                    if (gen.skills_cd[1] <= round_buffer &&
                        state.coin[!player] + additional_coin - coin_penalty >= Constant::breakthrough) {
                        int max_army_adjacent = army_board[cell_x][cell_y];
                        float max_cell_def_adj = cell_def;
                        for (int i = 0; i < 4; i++) {
                            int nx = cell_x + dx[i], ny = cell_y + dy[i];
                            if (nx < 0 || nx >= 15 || ny < 0 || ny >= 15) continue;
                            if (player_board[nx][ny] == player) {
                                max_army_adjacent = std::max(max_army_adjacent, army_board[nx][ny]);
                                max_cell_def_adj = std::max(max_cell_def_adj, cell_defence(nx, ny, player, state));
                            }
                        }
                        total_val += max_cell_def_adj * std::min(20, max_army_adjacent);
                        if (state.coin[!player] + additional_coin - coin_penalty - used_coin >=
                            Constant::breakthrough * 2) {
                            total_val += max_cell_def_adj * std::min(20, max_army_adjacent);
                        }
                    }
                    enemy_armys[dist - 1][enemy_armys_cnt[dist - 1]++] = total_val;
                    enemy_army_origin[dist - 1][enemy_armys_cnt[dist - 1] - 1] = k * 15 + l;
                }
            }
        }
    }

    // bag packing problem
    // total volume = enemy_movement
    // enemy_armys[i][j] has volume = i+1, value = enemy_armys[i][j]
    // enemy_armys_cnt[i] = number of items with volume = i+1
    // record the idxs to reach the max value
    memset(dp, 0, sizeof(dp));
    for (int i = 0; i < 5; i++) threat_origin_idx[i].clear();
    for (int k = 0; k < enemy_movement; k++) {
        for (int l = 0; l < enemy_armys_cnt[k]; l++) {
            for (int m = enemy_movement - 1; m >= k; m--) {
                if (m == k) {
                    if (dp[m] < enemy_armys[k][l]) {
                        dp[m] = enemy_armys[k][l];
                        threat_origin_idx[m].clear();
                        threat_origin_idx[m].push_back(enemy_army_origin[k][l]);
                    }
                } else if (dp[m] < dp[m - k - 1] + enemy_armys[k][l]) {
                    dp[m] = dp[m - k - 1] + enemy_armys[k][l];
                    threat_origin_idx[m] = threat_origin_idx[m - k - 1];
                    threat_origin_idx[m].push_back(enemy_army_origin[k][l]);
                }
            }
        }
    }
    return dp[enemy_movement - 1];
}

bool connected_component_res[15][15];
int find_connected_component(GameState& state, int cell_x, int cell_y) {
    memset(connected_component_res, 0, sizeof(connected_component_res));
    std::queue<std::pair<int, int>> q;
    q.push({cell_x, cell_y});
    connected_component_res[cell_x][cell_y] = true;
    int cnt = 0;
    while (!q.empty()) {
        auto& cur = q.front();
        q.pop();
        cnt++;
        int x = cur.first, y = cur.second;
        for (int i = 0; i < 4; i++) {
            int nx = x + dx[i], ny = y + dy[i];
            if (nx < 0 || nx >= 15 || ny < 0 || ny >= 15) continue;
            if (connected_component_res[nx][ny]) continue;
            if (board[nx][ny] >= 0 && block_list[board[nx][ny]]) continue;
            connected_component_res[nx][ny] = true;
            q.push({nx, ny});
        }
    }
    return cnt;
}

std::pair<std::vector<int>, int> shortest_path_dirs(GameState& state, int sx, int sy, int ex, int ey) {
    // if block_list[val] == true, then val=board[i][j] is blocked
    // -1: default, 0: ours, 1: enemy, 2: our_gen, 3: enemy_gen, 4: our_oil, 5: enemy_oil, 6: neut_gen, 7: neut_oil,
    // 8: swamp, 9: sand
    // use bfs to find shortest path
    std::set<int> res = {};
    int min_dist = 1000000;
    for (char r = 0; r < 4; r++) {
        std::queue<std::pair<char, char>> q;
        q.push({sx, sy});
        bool vis[15][15];
        memset(vis, 0, sizeof(vis));
        vis[sx][sy] = true;
        char pre[15][15];
        memset(pre, -1, sizeof(pre));
        while (!q.empty()) {
            auto& cur = q.front();
            q.pop();
            char x = cur.first, y = cur.second;
            if (x == ex && y == ey) break;
            for (char _i = r; _i < r + 4; _i++) {
                char i = _i % 4;
                char nx = x + dx[i], ny = y + dy[i];
                if (nx == ex && ny == ey) {
                    vis[nx][ny] = true;
                    pre[nx][ny] = i;
                    q.push({nx, ny});
                    break;
                }
                if (nx < 0 || nx >= 15 || ny < 0 || ny >= 15) continue;
                if (vis[nx][ny]) continue;
                if (board[nx][ny] >= 0 && block_list[board[nx][ny]]) continue;
                vis[nx][ny] = true;
                pre[nx][ny] = i;
                q.push({nx, ny});
            }
        }
        if (pre[ex][ey] == -1) {
            return std::make_pair(std::vector<int>(), -1);
        }
        char x = ex, y = ey;
        char first_dir = -1;
        char dist = 0;
        while (true) {
            char dir = pre[x][y] ^ 1;
            char nx = x + dx[dir], ny = y + dy[dir];
            dist++;
            if (nx == sx && ny == sy) {
                first_dir = pre[x][y];
                break;
            }
            x = nx, y = ny;
        }
        if (dist < min_dist) {
            min_dist = dist;
            res.clear();
            res.insert(first_dir);
        } else if (dist == min_dist) {
            res.insert(first_dir);
        }
    }

    return std::make_pair(std::vector<int>(res.begin(), res.end()), min_dist);
}

int shortest_path_dist(GameState& state, int sx, int sy, int ex, int ey, bool player) {
    std::queue<std::pair<char, char>> q;
    q.push({sx, sy});
    char dis[15][15];
    for (int i = 0; i < 15; i++)
        for (int j = 0; j < 15; j++) dis[i][j] = 100;
    dis[sx][sy] = 0;
    while (!q.empty()) {
        auto& cur = q.front();
        q.pop();
        char x = cur.first, y = cur.second;
        for (int i = 0; i < 4; i++) {
            char nx = x + dx[i], ny = y + dy[i];
            if (nx == ex && ny == ey) {
                return dis[x][y] + 1;
            }
            if (nx < 0 || nx >= 15 || ny < 0 || ny >= 15) continue;
            if (state.tech_level[player][int(TechType::IMMUNE_SWAMP)] == 0 &&
                state.board[nx][ny].type == CellType::SWAMP)
                continue;
            if (state.board[nx][ny].generals.type != 0) continue;
            if (dis[nx][ny] <= dis[x][y] + 1) continue;
            dis[nx][ny] = dis[x][y] + 1;
            q.push({nx, ny});
        }
    }
    return dis[ex][ey];
}

std::vector<int> best_escape_path(GameState& state, int sx, int sy, int fx, int fy, bool player, int movement) {
    std::queue<std::pair<char, char>> q;
    q.push({sx, sy});
    char dis[15][15];
    for (int i = 0; i < 15; i++)
        for (int j = 0; j < 15; j++) dis[i][j] = 100;
    dis[sx][sy] = 0;
    char jump_dis[15][15];
    for (int i = 0; i < 15; i++)
        for (int j = 0; j < 15; j++)
            jump_dis[i][j] = std::max(std::max(abs(fx - i) - 2, 0) + std::max(abs(fy - j) - 2, 0), 1);
    int val[15][15];
    for (int i = 0; i < 15; i++)
        for (int j = 0; j < 15; j++) val[i][j] = -10000000;
    val[sx][sy] = 0;
    char pre[15][15];
    memset(pre, -1, sizeof(pre));
    bool sand_immunity = state.tech_level[player][int(TechType::IMMUNE_SAND)] > 0;
    bool swamp_bias = state.tech_level[player][int(TechType::IMMUNE_SWAMP)] > 0 &&
                      state.tech_level[!player][int(TechType::IMMUNE_SWAMP)] == 0;
    while (!q.empty()) {
        auto& cur = q.front();
        q.pop();
        char x = cur.first, y = cur.second;
        for (int i = 0; i < 4; i++) {
            char nx = x + dx[i], ny = y + dy[i];
            if (nx < 0 || nx >= 15 || ny < 0 || ny >= 15) continue;
            if (state.tech_level[player][int(TechType::IMMUNE_SWAMP)] == 0 &&
                state.board[nx][ny].type == CellType::SWAMP)
                continue;
            if (state.board[nx][ny].generals.type != 0) continue;
            if (dis[nx][ny] < dis[x][y] + 1) continue;

            int army_val = val[x][y];
            if (state.board[nx][ny].type == CellType::SAND && !sand_immunity) army_val -= 8;
            if (state.board[nx][ny].player == player)
                army_val += state.board[nx][ny].army;
            else
                army_val -= state.board[nx][ny].army;
            int total_val = army_val + val[x][y] + jump_dis[nx][ny] + abs(fx - nx) + abs(fy - ny);
            if (swamp_bias && state.board[nx][ny].type == CellType::SWAMP) total_val += 10;
            if (dis[nx][ny] == dis[x][y] + 1 && total_val <= val[nx][ny]) continue;
            dis[nx][ny] = dis[x][y] + 1;
            val[nx][ny] = total_val;
            pre[nx][ny] = i;
            q.push({nx, ny});
        }
    }
    int target_dis = std::max(4, movement * 2);
    std::vector<int> res;
    char best_target_x = -1, best_target_y = -1;
    float best_target_val = -10000000;
    for (int i = 0; i < 15; i++) {
        for (int j = 0; j < 15; j++) {
            if (dis[i][j] <= target_dis) {
                float target_val = (dis[i][j] - target_dis) * 1e9 + val[i][j];
                if (target_val > best_target_val) {
                    best_target_val = target_val;
                    best_target_x = i;
                    best_target_y = j;
                }
            }
        }
    }
    if (best_target_x == -1) {
        return res;
    }
    int x = best_target_x, y = best_target_y;
    while (true) {
        int dir = pre[x][y] ^ 1;
        int nx = x + dx[dir], ny = y + dy[dir];
        res.push_back(pre[x][y]);
        if (nx == sx && ny == sy) {
            break;
        }
        x = nx, y = ny;
    }
    res = std::vector<int>(res.rbegin(), res.rend());  // reverse
    return res;
}

std::vector<int> best_attack_path(GameState& state, int sx, int sy, int ex, int ey, bool player) {
    // player attack !player
    std::queue<std::pair<char, char>> q;
    q.push({sx, sy});
    char dis[15][15];
    for (int i = 0; i < 15; i++)
        for (int j = 0; j < 15; j++) dis[i][j] = 100;
    dis[sx][sy] = 0;
    char jump_dis[15][15];
    for (int i = 0; i < 15; i++)
        for (int j = 0; j < 15; j++)
            jump_dis[i][j] = std::max(std::max(abs(ex - i) - 2, 0) + std::max(abs(ey - j) - 2, 0), 1);
    int val[15][15];
    for (int i = 0; i < 15; i++)
        for (int j = 0; j < 15; j++) val[i][j] = -10000000;
    char pre[15][15];
    memset(pre, -1, sizeof(pre));
    bool sand_immunity = state.tech_level[player][int(TechType::IMMUNE_SAND)] > 0;
    bool swamp_bias = state.tech_level[player][int(TechType::IMMUNE_SWAMP)] > 0 &&
                      state.tech_level[!player][int(TechType::IMMUNE_SWAMP)] == 0;
    int target_dist = 100;
    while (!q.empty()) {
        auto& cur = q.front();
        q.pop();
        char x = cur.first, y = cur.second;
        for (int i = 0; i < 4; i++) {
            char nx = x + dx[i], ny = y + dy[i];
            if (nx < 0 || nx >= 15 || ny < 0 || ny >= 15) continue;
            if (nx == ex && ny == ey) {
            } else {
                if (state.tech_level[player][int(TechType::IMMUNE_SWAMP)] == 0 &&
                    state.board[nx][ny].type == CellType::SWAMP)
                    continue;
                if (state.board[nx][ny].generals.type != 0) continue;
            }

            if (dis[nx][ny] < dis[x][y] + 1) continue;
            if (dis[x][y] > target_dist) continue;
            int army_val = val[x][y];
            if (state.board[nx][ny].type == CellType::SAND && !sand_immunity) army_val -= 8;
            if (state.board[nx][ny].player == player)
                army_val += state.board[nx][ny].army;
            else
                army_val -= state.board[nx][ny].army;
            int total_val = army_val + val[x][y] + (-jump_dis[nx][ny] - dis[x][y]);
            if (swamp_bias && state.board[nx][ny].type == CellType::SWAMP) total_val += 10;
            if (dis[nx][ny] == dis[x][y] + 1 && total_val <= val[nx][ny]) continue;
            dis[nx][ny] = dis[x][y] + 1;
            val[nx][ny] = total_val;
            pre[nx][ny] = i;
            q.push({nx, ny});
            if (nx == ex && ny == ey) {
                target_dist = dis[nx][ny];
                break;
            }
        }
    }
    if (pre[ex][ey] == -1) {
        return std::vector<int>();
    }
    std::vector<int> res;
    int x = ex, y = ey;
    while (true) {
        int dir = pre[x][y] ^ 1;
        int nx = x + dx[dir], ny = y + dy[dir];
        res.push_back(pre[x][y]);
        if (nx == sx && ny == sy) {
            break;
        }
        x = nx, y = ny;
    }
    res = std::vector<int>(res.rbegin(), res.rend());  // reverse
    return res;
}

std::vector<int> best_mobile_path(GameState& state, int sx, int sy, int ex, int ey, bool player, bool allow_sand,
                                  bool allow_general) {
    std::queue<std::pair<char, char>> q;
    q.push({sx, sy});
    char dis[15][15];
    for (int i = 0; i < 15; i++)
        for (int j = 0; j < 15; j++) dis[i][j] = 100;
    dis[sx][sy] = 0;
    int val[15][15];
    for (int i = 0; i < 15; i++)
        for (int j = 0; j < 15; j++) val[i][j] = -10000000;
    char pre[15][15];
    memset(pre, -1, sizeof(pre));
    bool sand_immunity = state.tech_level[player][int(TechType::IMMUNE_SAND)] > 0;
    int target_dist = 100;
    while (!q.empty()) {
        auto& cur = q.front();
        q.pop();
        char x = cur.first, y = cur.second;
        for (int i = 0; i < 4; i++) {
            char nx = x + dx[i], ny = y + dy[i];
            if (nx < 0 || nx >= 15 || ny < 0 || ny >= 15) continue;
            if (nx == ex && ny == ey) {
            } else {
                if (state.tech_level[player][int(TechType::IMMUNE_SWAMP)] == 0 &&
                    state.board[nx][ny].type == CellType::SWAMP)
                    continue;
                if (state.board[nx][ny].generals.type != 0 &&
                    !(allow_general && state.board[nx][ny].generals.player == player))
                    continue;
            }

            if (dis[nx][ny] < dis[x][y] + 1) continue;
            if (dis[x][y] >= target_dist) continue;
            int army_val = val[x][y];
            if (state.board[nx][ny].type == CellType::SAND && !sand_immunity) {
                if (!allow_sand) continue;
                army_val -= 8;
            }
            if (state.board[nx][ny].player == player)
                army_val += state.board[nx][ny].army;
            else
                army_val -= state.board[nx][ny].army;
            int total_val = army_val + val[x][y];
            if (dis[nx][ny] == dis[x][y] + 1 && total_val <= val[nx][ny]) continue;
            dis[nx][ny] = dis[x][y] + 1;
            val[nx][ny] = total_val;
            pre[nx][ny] = i;
            q.push({nx, ny});
            if (nx == ex && ny == ey) {
                target_dist = dis[nx][ny];
                break;
            }
        }
    }
    if (pre[ex][ey] == -1) {
        return std::vector<int>();
    }
    std::vector<int> res;
    int x = ex, y = ey;
    while (true) {
        int dir = pre[x][y] ^ 1;
        int nx = x + dx[dir], ny = y + dy[dir];
        res.push_back(pre[x][y]);
        if (nx == sx && ny == sy) {
            break;
        }
        x = nx, y = ny;
    }
    res = std::vector<int>(res.rbegin(), res.rend());  // reverse
    return res;
}

std::vector<std::pair<int, int>> reachable_positions(GameState& state, int sx, int sy, int movement) {
    std::vector<std::pair<int, int>> res;
    std::queue<std::pair<int, int>> q;
    q.push({sx, sy});
    int dis[15][15];
    for (int i = 0; i < 15; i++)
        for (int j = 0; j < 15; j++) dis[i][j] = 1000000;
    dis[sx][sy] = 0;
    while (!q.empty()) {
        auto& cur = q.front();
        q.pop();
        int x = cur.first, y = cur.second;
        if (dis[x][y] > movement) continue;
        if (dis[x][y] == movement) {
            res.push_back({x, y});
            continue;
        }
        for (int i = 0; i < 4; i++) {
            int nx = x + dx[i], ny = y + dy[i];
            if (nx < 0 || nx >= 15 || ny < 0 || ny >= 15) continue;
            if (dis[nx][ny] < dis[x][y] + 1) continue;
            if (board[nx][ny] >= 0 && block_list[board[nx][ny]]) continue;
            dis[nx][ny] = dis[x][y] + 1;
            q.push({nx, ny});
        }
    }
    return res;
}

std::vector<std::pair<int, int>> all_reachable_positions(GameState& state, int sx, int sy, int movement, bool player) {
    std::vector<std::pair<int, int>> res;
    std::queue<std::pair<int, int>> q;
    q.push({sx, sy});
    int dis[15][15];
    for (int i = 0; i < 15; i++)
        for (int j = 0; j < 15; j++) dis[i][j] = 1000000;
    dis[sx][sy] = 0;
    while (!q.empty()) {
        auto& cur = q.front();
        q.pop();
        int x = cur.first, y = cur.second;
        if (dis[x][y] > movement) continue;
        if (dis[x][y] <= movement) {
            res.push_back({x, y});
            if (dis[x][y] == movement) continue;
        }
        for (int i = 0; i < 4; i++) {
            int nx = x + dx[i], ny = y + dy[i];
            if (nx < 0 || nx >= 15 || ny < 0 || ny >= 15) continue;
            if (dis[nx][ny] <= dis[x][y] + 1) continue;
            if (state.tech_level[player][int(TechType::IMMUNE_SWAMP)] == 0 &&
                state.board[nx][ny].type == CellType::SWAMP)
                continue;
            if (state.board[nx][ny].generals.type != 0) continue;
            dis[nx][ny] = dis[x][y] + 1;
            q.push({nx, ny});
        }
    }
    return res;
}

std::pair<int, int> eval(GameState& state) {
    int m_eval = 0, e_eval = 0;
    int my_cell_cnt = 0, enemy_cell_cnt = 0;
    int my_gen_produce = 0, enemy_gen_produce = 0;
    int my_oil_produce = 0, enemy_oil_produce = 0;
    int my_oil = state.coin[my], enemy_oil = state.coin[!my];
    int my_army_cnt = 0, enemy_army_cnt = 0;
    int my_maingen_army = 0, enemy_maingen_army = 0;
    for (int i = 0; i < 15; i++) {
        for (int j = 0; j < 15; j++) {
            auto& cell = state.board[i][j];
            const auto& gen = cell.generals;
            if (cell.player == my) {
                my_cell_cnt++;
                my_army_cnt += cell.army;
                if (gen.type != 0) {
                    if (gen.type == 3)
                        my_oil_produce += gen.produce_level;
                    else if (gen.type == 1) {
                        my_gen_produce += gen.produce_level;
                        my_maingen_army = cell.army;
                    } else
                        my_gen_produce += gen.produce_level;
                }
            } else if (cell.player == !my) {
                enemy_cell_cnt++;
                enemy_army_cnt += cell.army;
                if (gen.type != 0) {
                    if (gen.type == 3)
                        enemy_oil_produce += gen.produce_level;
                    else if (gen.type == 1) {
                        enemy_gen_produce += gen.produce_level;
                        enemy_maingen_army = cell.army;
                    } else
                        enemy_gen_produce += gen.produce_level;
                }
            }
        }
    }
    m_eval += my_cell_cnt * std::min(20, (500 - now_round));
    m_eval += my_army_cnt * 5;
    m_eval += my_gen_produce * std::min(20, (500 - now_round)) * 10;
    m_eval += my_oil_produce * std::min(20, (500 - now_round)) * 20;
    m_eval += std::min(500, my_oil) * 10;
    m_eval += my_maingen_army * 50;
    e_eval += enemy_cell_cnt * std::min(20, (500 - now_round));
    e_eval += enemy_army_cnt * 5;
    e_eval += enemy_gen_produce * std::min(20, (500 - now_round)) * 10;
    e_eval += enemy_oil_produce * std::min(20, (500 - now_round)) * 20;
    e_eval += std::min(500, enemy_oil) * 10;
    e_eval += enemy_maingen_army * 50;
    return std::make_pair(m_eval, e_eval);
}

void get_board(GameState& state, bool simple = false) {
    memset(board, -1, sizeof(board));
    for (int i = 0; i < 15; i++) {
        for (int j = 0; j < 15; j++) {
            auto& cell = state.board[i][j];
            const auto& gen = cell.generals;
            if (cell.player == my) {
                if (gen.type == 0) {
                    board[i][j] = 0;
                } else {
                    if (gen.type == 3)
                        board[i][j] = 4;
                    else
                        board[i][j] = 2;
                }
            } else if (cell.player == !my) {
                if (gen.type == 0) {
                    board[i][j] = 1;
                } else {
                    if (gen.type == 3)
                        board[i][j] = 5;
                    else
                        board[i][j] = 3;
                }
            } else {
                if (gen.type != 0) {
                    if (gen.type == 3)
                        board[i][j] = 7;
                    else
                        board[i][j] = 6;
                }
            }
            if (cell.type == CellType::SAND && state.tech_level[my][int(TechType::IMMUNE_SAND)] == 0) board[i][j] = 9;
            for (const auto& weapon : state.active_super_weapon) {
                if (weapon.type == WeaponType::NUCLEAR_BOOM && abs(weapon.position[0] - i) <= 1 &&
                    abs(weapon.position[1] - j) <= 1) {
                    board[i][j] = 10;
                }
            }
            if (cell.type == CellType::SWAMP && state.tech_level[my][int(TechType::IMMUNE_SWAMP)] == 0) board[i][j] = 8;
        }
    }

    if (!simple) {
        memset(threat_origin, -1, sizeof(threat_origin));
        memset(threat_origin_cnt, 0, sizeof(threat_origin_cnt));
        memset(impact_value, 0, sizeof(impact_value));
        memset(enemy_impact, 0, sizeof(enemy_impact));
        // compute enemy impact
        for (int i = 0; i < 15; i++) {
            for (int j = 0; j < 15; j++) {
                for (int di = -6; di <= 6; di++) {
                    for (int dj = -6; dj <= 6; dj++) {
                        int x = i + di, y = j + dj;
                        if (di == 0 && dj == 0) continue;
                        if (x < 0 || x >= 15 || y < 0 || y >= 15) continue;
                        float dist = abs(di) + abs(dj);
                        float short_dist = std::max(0, abs(di) - 2) + std::max(0, abs(dj) - 2);
                        if (dist > 8) continue;
                        if (state.board[x][y].player != !my) continue;
                        if (state.board[x][y].generals.type == 0 || state.board[x][y].generals.type == 3) {
                            enemy_impact[i][j] += state.board[x][y].army / std::max(1.0f, dist * dist);
                        } else {
                            enemy_impact[i][j] +=
                                0.5 * state.board[x][y].army / std::max(1.0f, dist * dist) +
                                0.5 * state.board[x][y].army / std::max(1.0f, short_dist * short_dist);
                        }
                    }
                }
            }
        }

        // compute impact val
        for (int i = 0; i < 15; i++) {
            for (int j = 0; j < 15; j++) {
                for (int di = -4; di <= 4; di++) {
                    for (int dj = -4; dj <= 4; dj++) {
                        if (abs(di) + abs(dj) > 4) continue;
                        int x = i + di, y = j + dj;
                        if (x < 0 || x >= 15 || y < 0 || y >= 15) continue;
                        if (board[x][y] == 1 || board[x][y] == 3 || board[x][y] == 5) {
                            impact_value[i][j] -= state.board[x][y].army;
                        } else if (board[x][y] == 0 || board[x][y] == 2 || board[x][y] == 4) {
                            impact_value[i][j] += state.board[x][y].army;
                        }
                    }
                }
            }
        }

        // compute threat val
        threat_val_prepare(state);
        int enemy_movement = state.tech_level[!my][0];
        for (int i = 0; i < 15; i++) {
            for (int j = 0; j < 15; j++) {
                threat_value[i][j] = compute_threat_val(i, j, my, state);
                threat_origin_cnt[i][j] = threat_origin_idx[enemy_movement - 1].size();
                for (int k = 0; k < threat_origin_cnt[i][j]; k++) {
                    threat_origin[i][j][k] = threat_origin_idx[enemy_movement - 1][k];
                }
            }
        }

        // for (int j = 14; j >= 0; j--) {
        //     for (int i = 0; i < 15; i++) {
        //         std::cerr << threat_value[i][j] << " ";
        //     }
        //     std::cerr << "\n";
        // }

        // eval
        std::tie(my_eval, enemy_eval) = eval(state);
        std::cerr << "my eval: " << my_eval << "\n";
        std::cerr << "enemy eval: " << enemy_eval << "\n";
    }
}

std::vector<Operation> use_weapon(GameState& state) {
    std::vector<Operation> ops;
    if (state.tech_level[my][int(TechType::UNLOCK)] == 0 || state.super_weapon_cd[my] > 0)
        return std::vector<Operation>();

    // use bomb
    float value[15][15];
    memset(value, 0, sizeof(value));
    int max_x = -1, max_y = -1;
    float max_value = -1000000;
    for (int i = 0; i < 15; i++) {
        for (int j = 0; j < 15; ++j) {
            float total_val = 0;
            for (int di = -1; di <= 1; di++) {
                for (int dj = -1; dj <= 1; dj++) {
                    int x = i + di, y = j + dj;
                    if (x < 0 || x >= 15 || y < 0 || y >= 15) continue;
                    if (state.board[x][y].player == my) {
                        total_val -= state.board[x][y].army * 20;
                        if (state.board[x][y].generals.type != 0) {
                            const auto& gen = state.board[x][y].generals;
                            if (gen.type == 1) {  // main general damage half
                                total_val -= state.board[x][y].army * 100;
                            } else if (gen.type == 2) {
                                total_val -= gen.produce_level * std::min(20, (500 - now_round)) * 40;
                            } else {
                                total_val -= gen.produce_level * std::min(20, (500 - now_round)) * 80;
                            }
                        }
                    } else if (state.board[x][y].player == !my) {
                        total_val += state.board[x][y].army * 10;
                        if (state.board[x][y].generals.type != 0) {
                            const auto& gen = state.board[x][y].generals;
                            if (gen.type == 1) {  // main general damage half
                                total_val += state.board[x][y].army * 50;
                            } else if (gen.type == 2) {
                                total_val += gen.produce_level * std::min(20, (500 - now_round)) * 20;
                            } else {
                                total_val += gen.produce_level * std::min(20, (500 - now_round)) * 40;
                            }
                        }
                    }
                }
            }
            if (total_val > max_value) {
                max_value = total_val;
                max_x = i;
                max_y = j;
            }
        }
    }

    if (max_value > 5000) {
        auto op = use_superweapon_op(WeaponType::NUCLEAR_BOOM, {max_x, max_y});
        ops.push_back(op);
        execute(state, op, my);
    }

    return ops;
}

inline std::vector<Operation> execute_simp_op(GameState& state, std::tuple<char, char, char, char, char> simp_op,
                                              bool player) {
    // tuple: (op, x, y, tx, ty)
    // op: 0-3 from (x,y), army move in each direction
    // op: 4-7 from (x,y), army+general move in each direction
    // op: 8 from (x,y), strike to (tx,ty)
    std::vector<Operation> ops;
    char op_type, x, y, tx, ty;
    std::tie(op_type, x, y, tx, ty) = simp_op;
    if (op_type < 4) {
        // move
        if (state.board[x][y].player != player) return ops;
        int num = state.board[x][y].army;
        if (num <= 1) return ops;
        auto op = move_army_op(std::make_pair(x, y), static_cast<Direction>(op_type), num - 1);
        execute(state, op, player);
        ops.push_back(op);
    } else if (op_type < 8) {
        // move army and general
        if (state.board[x][y].player != player) return ops;
        int num = state.board[x][y].army;
        if (num <= 1) return ops;
        auto op = move_army_op(std::make_pair(x, y), static_cast<Direction>(op_type - 4), num - 1);
        execute(state, op, player);
        ops.push_back(op);
        if (state.board[x][y].generals.type == 0 || state.board[x][y].generals.type == 3) return ops;
        int nx = x + dx[op_type - 4], ny = y + dy[op_type - 4];
        op = move_generals_op(state.board[x][y].generals.id, std::make_pair(nx, ny));
        execute(state, op, player);
        ops.push_back(op);
    } else {
        // strike
        if (state.board[x][y].player != player) return ops;
        if (state.board[x][y].generals.type == 0 || state.board[x][y].generals.type == 3) return ops;
        if (state.board[x][y].generals.skills_cd[0] > 0) return ops;
        if (state.coin[player] < Constant::tactical_strike) return ops;
        if (state.board[tx][ty].generals.type != 0) return ops;
        auto op = generals_skill_op(state.board[x][y].generals.id, SkillType::SURPRISE_ATTACK, std::make_pair(tx, ty));
        execute(state, op, player);
        ops.push_back(op);
    }
    return ops;
}

std::pair<std::vector<std::tuple<char, char, char, char, char>>, int> best_dfs(
    GameState& state, int sx, int sy, int ex, int ey, int steps, bool player, bool allow_jump,
    std::tuple<char, char, char, char, char> last_op = std::make_tuple(-1, -1, -1, -1, -1)) {
    std::vector<int> best_candidates;
    for (int k = 0; k < 4; k++) {
        int nx = ex + dx[k], ny = ey + dy[k];
        if (nx < 0 || nx >= 15 || ny < 0 || ny >= 15) continue;
        if (state.board[nx][ny].player == player && state.board[nx][ny].army > 1) {
            best_candidates.push_back(state.board[nx][ny].army - 1);
        }
    }
    int rest_movements = state.rest_move_step[player];
    std::sort(best_candidates.begin(), best_candidates.end(), std::greater<int>());
    int best_value = 0;
    for (int i = 0; i < std::min((int)(best_candidates.size()), rest_movements); i++) {
        best_value += best_candidates[i];
    }
    int already_coins = 0;
    const auto& maingen = find_general_by_id(state, main_general_id(state, player));
    if (maingen.skill_duration[2] > 0) already_coins += Constant::leadership;
    if (maingen.skill_duration[4] > 0) already_coins += Constant::weakening;

    int reserve_positions = std::min((state.coin[player] + already_coins) / 110, 2);
    int my_cell_cnt = 0;
    for (int di = -2; di <= 2; di++) {
        for (int dj = -2; dj <= 2; dj++) {
            int nx = ex + di, ny = ey + dj;
            if (nx < 0 || nx >= 15 || ny < 0 || ny >= 15) continue;
            if (state.board[nx][ny].player == player && state.board[nx][ny].generals.type == 0) {
                my_cell_cnt++;
            }
        }
    }
    if (my_cell_cnt >= reserve_positions) {
        best_value += 1000;
    } else if (reserve_positions > 0) {
        best_value += my_cell_cnt * 500;
    }

    std::vector<std::tuple<char, char, char, char, char>> res;
    int banned_dir = -1;  // not going back
    if (last_op != std::make_tuple(-1, -1, -1, -1, -1)) {
        res.push_back(last_op);
        int op_type, x, y, tx, ty;
        std::tie(op_type, x, y, tx, ty) = last_op;
        if (op_type < 4) {
            banned_dir = op_type ^ 1;
        } else if (op_type < 8) {
            banned_dir = (op_type - 4) ^ 1;
        }
    }
    const int dist = abs(sx - ex) + abs(sy - ey);
    bool adjacent_flag = dist == 1;
    if (state.board[sx][sy].player != player) return std::make_pair(res, best_value);

    std::vector<std::tuple<char, char, char, char, char>> best_res;
    auto& gen = state.board[sx][sy].generals;
    if (gen.type == 0 || gen.type == 3) {
        if (steps == 1) return std::make_pair(res, best_value);
        if (steps < dist) return std::make_pair(res, best_value);
        if (steps > dist) {  // consider move adjacent army to support
            for (int i = 0; i < 4; i++) {
                if (i == banned_dir) continue;
                int suppx = sx + dx[i], suppy = sy + dy[i];
                if (suppx < 0 || suppx >= 15 || suppy < 0 || suppy >= 15) continue;
                if (state.board[suppx][suppy].player != player) continue;
                if (state.board[suppx][suppy].army <= 1) continue;
                int dir = i ^ 1;
                GameState ns = state;
                auto simp_op = std::make_tuple(dir, suppx, suppy, -1, -1);
                execute_simp_op(ns, simp_op, player);
                auto now_res = best_dfs(ns, sx, sy, ex, ey, steps - 1, player, allow_jump, simp_op);
                if (now_res.second > best_value) {
                    best_value = now_res.second;
                    best_res = now_res.first;
                }
            }
        }
        if (!adjacent_flag) {
            int mindist = 100;
            for (int i = 0; i < 4; i++) {  // move to nx, ny
                if (i == banned_dir) continue;
                int nx = sx + dx[i], ny = sy + dy[i];
                if (nx < 0 || nx >= 15 || ny < 0 || ny >= 15) continue;
                if (state.board[nx][ny].type == CellType::SWAMP &&
                    state.tech_level[player][int(TechType::IMMUNE_SWAMP)] == 0)
                    continue;
                if (state.board[sx][sy].army <= 1) continue;
                int new_dist = abs(nx - ex) + abs(ny - ey);
                if (new_dist > steps - 1) continue;  // too far
                if (new_dist > mindist) continue;
                GameState ns = state;
                auto simp_op = std::make_tuple(i, sx, sy, -1, -1);
                execute_simp_op(ns, simp_op, player);
                auto now_res = best_dfs(ns, nx, ny, ex, ey, steps - 1, player, allow_jump, simp_op);
                if (now_res.second > best_value) {
                    best_value = now_res.second;
                    best_res = now_res.first;
                }
                if (new_dist < mindist) mindist = new_dist;
            }
        }
    } else {
        if (gen.skills_cd[0] > 0 || state.coin[player] < Constant::tactical_strike) allow_jump = false;
        if (!allow_jump && steps == 1) return std::make_pair(res, best_value);
        if (!allow_jump && steps < dist) return std::make_pair(res, best_value);
        int jump_dist = std::max(std::max(0, abs(sx - ex) - 2) + std::max(0, abs(sy - ey) - 2), 1);
        if (steps > dist || (allow_jump && steps > jump_dist)) {  // consider move adjacent army to support
            for (int i = 0; i < 4; i++) {
                if (i == banned_dir) continue;
                int suppx = sx + dx[i], suppy = sy + dy[i];
                if (suppx < 0 || suppx >= 15 || suppy < 0 || suppy >= 15) continue;
                if (state.board[suppx][suppy].player != player) continue;
                if (state.board[suppx][suppy].army <= 1) continue;
                int dir = i ^ 1;
                GameState ns = state;
                auto simp_op = std::make_tuple(dir, suppx, suppy, -1, -1);
                execute_simp_op(ns, simp_op, player);
                auto now_res = best_dfs(ns, sx, sy, ex, ey, steps - 1, player, allow_jump, simp_op);
                if (now_res.second > best_value) {
                    best_value = now_res.second;
                    best_res = now_res.first;
                }
            }
        }
        if (!adjacent_flag) {
            int mindist = 100;
            for (int i = 0; i < 4; i++) {  // move to nx, ny
                if (i == banned_dir) continue;
                int nx = sx + dx[i], ny = sy + dy[i];
                if (nx < 0 || nx >= 15 || ny < 0 || ny >= 15) continue;
                if (state.board[nx][ny].type == CellType::SWAMP &&
                    state.tech_level[player][int(TechType::IMMUNE_SWAMP)] == 0)
                    continue;
                if (state.board[sx][sy].army <= 1) continue;
                int new_dist = abs(nx - ex) + abs(ny - ey);
                if (allow_jump) new_dist = std::max(std::max(0, abs(nx - ex) - 2) + std::max(0, abs(ny - ey) - 2), 1);
                if (new_dist > steps - 1) continue;  // too far
                if (new_dist > mindist) continue;

                GameState ns = state;
                std::tuple<char, char, char, char, char> simp_op = std::make_tuple(i, sx, sy, -1, -1);
                if (ns.board[sx][sy].generals.rest_move > 0) {
                    if (ns.board[nx][ny].generals.type == 0) {
                        simp_op = std::make_tuple(i + 4, sx, sy, -1, -1);
                    }
                }
                execute_simp_op(ns, simp_op, player);
                auto now_res = best_dfs(ns, nx, ny, ex, ey, steps - 1, player, allow_jump, simp_op);
                if (now_res.second > best_value) {
                    best_value = now_res.second;
                    best_res = now_res.first;
                }
                if (new_dist < mindist) mindist = new_dist;
            }

            // jump
            if (allow_jump) {
                for (int di = -2; di <= 2; di++) {
                    for (int dj = -2; dj <= 2; dj++) {
                        int nx = sx + di, ny = sy + dj;
                        if (di == 0 && dj == 0) continue;
                        if (nx < 0 || nx >= 15 || ny < 0 || ny >= 15) continue;
                        if (state.board[nx][ny].type == CellType::SWAMP &&
                            state.tech_level[player][int(TechType::IMMUNE_SWAMP)] == 0)
                            continue;
                        if (state.board[nx][ny].generals.type != 0) continue;
                        if (state.board[sx][sy].army <= 1) continue;
                        int new_dist = abs(nx - ex) + abs(ny - ey);
                        if (new_dist > dist) continue;   // not jumping further
                        if (new_dist > steps) continue;  // too far
                        GameState ns = state;
                        std::tuple<char, char, char, char, char> simp_op = std::make_tuple(8, sx, sy, nx, ny);
                        execute_simp_op(ns, simp_op, player);
                        auto now_res = best_dfs(ns, nx, ny, ex, ey, steps, player, allow_jump, simp_op);
                        if (now_res.second > best_value) {
                            best_value = now_res.second;
                            best_res = now_res.first;
                        }
                    }
                }
            }
        }
    }

    if (last_op != std::make_tuple(-1, -1, -1, -1, -1)) best_res.insert(best_res.begin(), last_op);
    return std::make_pair(best_res, best_value);
}

std::pair<std::vector<Operation>, int> best_skill_dfs(GameState& state, int ex, int ey, bool player,
                                                      std::vector<int> allow_gen_ids, Operation last_op = Operation()) {
    std::vector<Operation> res;
    if (last_op.opcode != OperationType::DEFAULT_OP) res.push_back(last_op);
    std::vector<Operation> best_res;
    int best_value = state.board[ex][ey].player == player ? 1 : 0;
    if (best_value == 1) return std::make_pair(res, best_value);
    const auto& gen_list = state.get_generals();

    int max_army_x = -1, max_army_y = -1, max_army = -1;
    for (int k = 0; k < 4; k++) {
        int nx = ex + dx[k], ny = ey + dy[k];
        if (nx < 0 || nx >= 15 || ny < 0 || ny >= 15) continue;
        if (state.board[nx][ny].player == player && state.board[nx][ny].army > 1) {
            if (state.board[nx][ny].army > max_army) {
                max_army = state.board[nx][ny].army;
                max_army_x = nx;
                max_army_y = ny;
            }
        }
    }

    int limit_level = 0;
    if (last_op.opcode != OperationType::DEFAULT_OP) {
        if (last_op.opcode == OperationType::CALL_GENERAL) {
            limit_level = 1;
        } else if (last_op.opcode == OperationType::USE_GENERAL_SKILLS) {
            if (last_op.operand[1] == (int)(SkillType::ROUT) + 1) limit_level = 2;
            if (last_op.operand[1] == (int)(SkillType::COMMAND) + 1) limit_level = 3;
            if (last_op.operand[1] == (int)(SkillType::WEAKEN) + 1) limit_level = 4;
            if (last_op.operand[1] == (int)(SkillType::SURPRISE_ATTACK) + 1) limit_level = 6;
        } else if (last_op.opcode == OperationType::MOVE_GENERALS) {
            limit_level = 5;
        }
    }

    for (auto& gen : gen_list) {
        if (gen.player != player) continue;
        if (gen.type == 3) continue;
        if (best_value == 1) break;
        auto vec_loc = std::find(allow_gen_ids.begin(), allow_gen_ids.end(), gen.id);
        if (vec_loc == allow_gen_ids.end()) continue;
        std::vector<int> new_allow_ids;
        for (auto i = vec_loc; i < allow_gen_ids.end(); i++) {
            new_allow_ids.push_back(*i);
        }
        int gen_x = gen.position[0], gen_y = gen.position[1];
        if ((abs(gen_x - ex) <= 2 && abs(gen_y - ey) <= 2) ||
            (abs(gen_x - max_army_x) <= 2 && abs(gen_y - max_army_y) <= 2)) {  // support
            if (gen.skills_cd[1] <= 0 && state.coin[player] >= Constant::breakthrough && limit_level <= 2 &&
                (abs(gen_x - ex) <= 2 && abs(gen_y - ey) <= 2)) {
                GameState ns = state;
                auto op = generals_skill_op(gen.id, SkillType::ROUT, std::make_pair(ex, ey));
                execute(ns, op, player);
                auto now_res = best_skill_dfs(ns, ex, ey, player, new_allow_ids, op);
                if (now_res.second > best_value) {
                    best_value = now_res.second;
                    best_res = now_res.first;
                    break;
                }
            }
            if (gen.skills_cd[2] <= 0 && state.coin[player] >= Constant::leadership && limit_level <= 3 &&
                (abs(gen_x - max_army_x) <= 2 && abs(gen_y - max_army_y) <= 2)) {
                GameState ns = state;
                auto op = generals_skill_op(gen.id, SkillType::COMMAND, std::make_pair(gen_x, gen_y));
                execute(ns, op, player);
                auto now_res = best_skill_dfs(ns, ex, ey, player, new_allow_ids, op);
                if (now_res.second > best_value) {
                    best_value = now_res.second;
                    best_res = now_res.first;
                    break;
                }
            }
            if (gen.skills_cd[4] <= 0 && state.coin[player] >= Constant::weakening && limit_level <= 4 &&
                (abs(gen_x - ex) <= 2 && abs(gen_y - ey) <= 2)) {
                GameState ns = state;
                auto op = generals_skill_op(gen.id, SkillType::WEAKEN, std::make_pair(gen_x, gen_y));
                execute(ns, op, player);
                auto now_res = best_skill_dfs(ns, ex, ey, player, new_allow_ids, op);
                if (now_res.second > best_value) {
                    best_value = now_res.second;
                    best_res = now_res.first;
                    break;
                }
            }
        } else {  // jump/move to the area and support
            if (limit_level <= 5) {
                for (int i = 0; i < 4; i++) {
                    int nx = gen_x + dx[i], ny = gen_y + dy[i];
                    if (nx < 0 || nx >= 15 || ny < 0 || ny >= 15) continue;
                    if (state.board[nx][ny].player != player) continue;
                    if (state.board[nx][ny].generals.type != 0) continue;
                    if (state.board[gen_x][gen_y].generals.rest_move <= 0) continue;
                    if ((abs(nx - ex) > 2 || abs(ny - ey) > 2) &&
                        (abs(nx - max_army_x) > 2 || abs(ny - max_army_y) > 2))
                        continue;

                    GameState ns = state;
                    auto op = move_generals_op(gen.id, std::make_pair(nx, ny));
                    execute(ns, op, player);
                    auto now_res = best_skill_dfs(ns, ex, ey, player, new_allow_ids, op);
                    if (now_res.second > best_value) {
                        best_value = now_res.second;
                        best_res = now_res.first;
                    }
                    break;
                }
                if (best_value == 1) break;
            }

            if (gen.skills_cd[0] <= 0 && state.coin[player] >= Constant::tactical_strike && limit_level <= 6) {
                bool jump_suc = false;
                for (int di = -2; di <= 2; di++) {
                    for (int dj = -2; dj <= 2; dj++) {
                        int nx = gen_x + di, ny = gen_y + dj;
                        if (di == 0 && dj == 0) continue;
                        if (nx < 0 || nx >= 15 || ny < 0 || ny >= 15) continue;
                        if (state.board[gen_x][gen_y].army <= 1) continue;
                        if (state.board[nx][ny].type == CellType::SWAMP &&
                            state.tech_level[player][int(TechType::IMMUNE_SWAMP)] == 0)
                            continue;
                        if (state.board[nx][ny].player != player && state.board[nx][ny].army >= 1) continue;
                        if (state.board[nx][ny].generals.type != 0) continue;
                        if ((abs(nx - ex) > 2 || abs(ny - ey) > 2) &&
                            (abs(nx - max_army_x) > 2 || abs(ny - max_army_y) > 2))
                            continue;

                        GameState ns = state;
                        auto op = generals_skill_op(gen.id, SkillType::SURPRISE_ATTACK, std::make_pair(nx, ny));
                        execute(ns, op, player);
                        auto now_res = best_skill_dfs(ns, ex, ey, player, new_allow_ids, op);
                        if (now_res.second > best_value) {
                            best_value = now_res.second;
                            best_res = now_res.first;
                        }
                        jump_suc = true;
                        break;
                    }
                    if (jump_suc) break;
                }
            }
            if (best_value == 1) break;
        }
    }
    if (best_value == 1) {
        if (last_op.opcode != OperationType::DEFAULT_OP) best_res.insert(best_res.begin(), last_op);
        return std::make_pair(best_res, best_value);
    }

    // call general to support
    int gen_num = 0;
    for (int di = -3; di <= 3; di++) {
        for (int dj = -3; dj <= 3; dj++) {
            int nx = ex + di, ny = ey + dj;
            if (di == 0 && dj == 0) continue;
            if (nx < 0 || nx >= 15 || ny < 0 || ny >= 15) continue;
            if ((abs(nx - max_army_x) > 2 || abs(ny - max_army_y) > 2) && (abs(nx - ex) > 2 || abs(ny - ey) > 2))
                continue;
            if (state.board[nx][ny].player != player) continue;
            if (state.board[nx][ny].generals.type == 0 || state.board[nx][ny].generals.type == 3) continue;
            gen_num++;
        }
    }
    bool call_succ = false;
    if (state.coin[player] >= Constant::lieutenant_new_recruit && gen_num < 3 && limit_level != 1) {
        for (int di = -2; di <= 2; di++) {
            for (int dj = -2; dj <= 2; dj++) {
                int nx = ex + di, ny = ey + dj;
                if (di == 0 && dj == 0) continue;
                if (nx < 0 || nx >= 15 || ny < 0 || ny >= 15) continue;
                if (abs(nx - max_army_x) > 2 || abs(ny - max_army_y) > 2) continue;
                if (state.board[nx][ny].player != player) continue;
                if (state.board[nx][ny].generals.type != 0) continue;
                GameState ns = state;
                auto op = call_generals_op(std::make_pair(nx, ny));
                execute(ns, op, player);
                auto now_res = best_skill_dfs(ns, ex, ey, player, {ns.next_generals_id - 1}, op);
                if (now_res.second > best_value) {
                    best_value = now_res.second;
                    best_res = now_res.first;
                }
                call_succ = true;
                break;
            }
            if (call_succ) break;
        }
        if (!call_succ) {
            for (int di = -2; di <= 2; di++) {
                for (int dj = -2; dj <= 2; dj++) {
                    int nx = max_army_x + di, ny = max_army_y + dj;
                    if (di == 0 && dj == 0) continue;
                    if (nx < 0 || nx >= 15 || ny < 0 || ny >= 15) continue;
                    if (abs(nx - ex) > 2 || abs(ny - ey) > 2) continue;
                    if (state.board[nx][ny].player != player) continue;
                    if (state.board[nx][ny].generals.type != 0) continue;
                    GameState ns = state;
                    auto op = call_generals_op(std::make_pair(nx, ny));
                    execute(ns, op, player);
                    auto now_res = best_skill_dfs(ns, ex, ey, player, {ns.next_generals_id - 1}, op);
                    if (now_res.second > best_value) {
                        best_value = now_res.second;
                        best_res = now_res.first;
                    }
                    call_succ = true;
                    break;
                }
                if (call_succ) break;
            }
        }
        if (!call_succ) {
            for (int di = -2; di <= 2; di++) {
                for (int dj = -2; dj <= 2; dj++) {
                    int nx = ex + di, ny = ey + dj;
                    if (di == 0 && dj == 0) continue;
                    if (nx < 0 || nx >= 15 || ny < 0 || ny >= 15) continue;
                    if (abs(nx - max_army_x) > 2 || abs(ny - max_army_y) > 2) continue;
                    if (state.board[nx][ny].player != player) continue;
                    if (state.board[nx][ny].generals.type != 0) continue;
                    GameState ns = state;
                    auto op = call_generals_op(std::make_pair(nx, ny));
                    execute(ns, op, player);
                    auto now_res = best_skill_dfs(ns, ex, ey, player, {ns.next_generals_id - 1}, op);
                    if (now_res.second > best_value) {
                        best_value = now_res.second;
                        best_res = now_res.first;
                    }
                    call_succ = true;
                    break;
                }
                if (call_succ) break;
            }
        }
    }
    if (best_value == 1) {
        if (last_op.opcode != OperationType::DEFAULT_OP) best_res.insert(best_res.begin(), last_op);
        return std::make_pair(best_res, best_value);
    }

    // direct attack
    std::vector<std::tuple<int, int, int>> attack_candidates;
    for (int i = 0; i < 4; i++) {
        int nx = ex + dx[i], ny = ey + dy[i];
        if (nx < 0 || nx >= 15 || ny < 0 || ny >= 15) continue;
        if (state.board[nx][ny].player == player && state.board[nx][ny].army > 1) {
            attack_candidates.push_back(std::make_tuple(nx, ny, state.board[nx][ny].army - 1));
        }
    }
    std::sort(attack_candidates.begin(), attack_candidates.end(),
              [](const std::tuple<int, int, int>& a, const std::tuple<int, int, int>& b) {
                  return std::get<2>(a) > std::get<2>(b);
              });
    int rest_move = state.rest_move_step[player];
    for (int i = 0; i < std::min(rest_move, (int)(attack_candidates.size())); i++) {
        int nx, ny, army;
        std::tie(nx, ny, army) = attack_candidates[i];
        int dx = ex - nx, dy = ey - ny;
        int dir = -1;
        if (dx == -1 && dy == 0) dir = 0;
        if (dx == 1 && dy == 0) dir = 1;
        if (dx == 0 && dy == -1) dir = 2;
        if (dx == 0 && dy == 1) dir = 3;
        auto op = move_army_op(std::make_pair(nx, ny), static_cast<Direction>(dir), army);
        best_res.push_back(op);
        execute(state, op, player);
    }
    if (last_op.opcode != OperationType::DEFAULT_OP) best_res.insert(best_res.begin(), last_op);
    best_value = state.board[ex][ey].player == player ? 1 : 0;
    return std::make_pair(best_res, best_value);
}

std::vector<Operation> new_kill(GameState& state, bool player, bool allow_jump) {
    int maingen_id = main_general_id(state, !player);
    if (maingen_id == -1) return std::vector<Operation>();
    const auto& maingen = find_general_by_id(state, maingen_id);
    int maingen_x = maingen.position[0], maingen_y = maingen.position[1];
    const int movement = state.tech_level[player][0];
    std::vector<Operation> final_ops;

    GameState ns = state;
    std::vector<std::tuple<int, int, int, int, int>> possible_pos_dist_type_val;
    std::vector<std::vector<std::tuple<char, char, char, char, char>>> best_ops[6];
    // best_ops[i]: given i steps, the best ops to reach the maingen

    for (int i = 0; i < 15; i++) {
        for (int j = 0; j < 15; j++) {
            if (ns.board[i][j].player != player) continue;
            if (ns.board[i][j].army <= 1) continue;
            int dist = abs(i - maingen_x) + abs(j - maingen_y);
            int val = ns.board[i][j].army;
            if (ns.board[i][j].generals.type == 0 || ns.board[i][j].generals.type == 3) {
                if (dist <= movement) {
                    possible_pos_dist_type_val.push_back(std::make_tuple(i, j, dist, 0, val));
                }
            } else {
                auto& gen = ns.board[i][j].generals;
                if (gen.skills_cd[0] > 0 || ns.coin[player] < Constant::tactical_strike) {
                    if (dist <= movement) {
                        possible_pos_dist_type_val.push_back(std::make_tuple(i, j, dist, 0, val));
                    }
                } else {
                    int jumpdist =
                        std::max(std::max(0, abs(i - maingen_x) - 2) + std::max(0, abs(j - maingen_y) - 2), 1);
                    if (jumpdist <= movement) {
                        possible_pos_dist_type_val.push_back(std::make_tuple(i, j, jumpdist, 1, val));
                    }
                }
            }
        }
    }
    while (true) {
        bool end_flag = true;
        int x1, y1, dist1, type1, val1;
        int x2, y2, dist2, type2, val2;
        for (int i = 0; i < possible_pos_dist_type_val.size(); i++) {
            for (int j = 0; j < possible_pos_dist_type_val.size(); j++) {
                if (i == j) continue;
                std::tie(x1, y1, dist1, type1, val1) = possible_pos_dist_type_val[i];
                std::tie(x2, y2, dist2, type2, val2) = possible_pos_dist_type_val[j];
                if (type1 >= type2 && val1 > val2 && dist2 > movement - dist1) {
                    possible_pos_dist_type_val.erase(possible_pos_dist_type_val.begin() + j);
                    end_flag = false;
                    break;
                }
            }
            if (!end_flag) break;
        }
        if (end_flag) break;
    }
    int siz = possible_pos_dist_type_val.size();

    clock_t st = clock();

    // decide the best ops
    for (int i = 1; i <= 5; i++) best_ops[i].resize(siz);
    for (int i = 0; i < siz; i++) {
        for (int j = 1; j <= movement; j++) {
            int x, y, dist, type, val;
            std::tie(x, y, dist, type, val) = possible_pos_dist_type_val[i];
            if (dist > j) continue;
            if (j == 1 && type == 0) continue;
            auto res = best_dfs(ns, x, y, maingen_x, maingen_y, j, player, allow_jump);
            best_ops[j][i] = res.first;
            // if (res.first.size() > 0) {
            //     std::cerr << "ops[" << j << "][" << i << "]: ";
            //     for (auto& op : res.first) {
            //         std::cerr << "(" << (int)(std::get<0>(op)) << "," << (int)(std::get<1>(op)) << ","
            //                   << (int)(std::get<2>(op)) << "," << (int)(std::get<3>(op)) << ","
            //                   << (int)(std::get<4>(op)) << ") ";
            //     }
            // }
        }
    }

    // std::cerr << "t1: " << clock() - st << " ";
    // st = clock();
    // distribute movement to at max two positions, execute the best ops
    std::vector<Operation> real_ops;
    int best_value = -1000000;

    int already_coins = 0;
    if (maingen.skill_duration[2] > 0) already_coins += Constant::leadership;
    if (maingen.skill_duration[4] > 0) already_coins += Constant::weakening;
    for (int i = 0; i < siz; i++) {
        auto& ops = best_ops[movement][i];
        GameState tmp_s = ns;
        std::vector<Operation> r_ops;
        for (auto& op : ops) {
            auto op_vec = execute_simp_op(tmp_s, op, player);
            r_ops.insert(r_ops.end(), op_vec.begin(), op_vec.end());
        }
        std::vector<int> val_candidates;
        for (int n = 0; n < 4; n++) {
            int nx = maingen_x + dx[n], ny = maingen_y + dy[n];
            if (nx < 0 || nx >= 15 || ny < 0 || ny >= 15) continue;
            if (tmp_s.board[nx][ny].player == player && tmp_s.board[nx][ny].army > 1) {
                val_candidates.push_back(tmp_s.board[nx][ny].army - 1);
            }
        }
        std::sort(val_candidates.begin(), val_candidates.end(), std::greater<int>());
        int val = 0;
        for (int j = 0; j < std::min((int)(val_candidates.size()), (int)(tmp_s.rest_move_step[player])); j++) {
            val += val_candidates[j];
        }
        int reserve_positions = std::min((tmp_s.coin[player] + already_coins) / 110, 2);
        int my_cell_cnt = 0;
        for (int di = -2; di <= 2; di++) {
            for (int dj = -2; dj <= 2; dj++) {
                int nx = maingen_x + di, ny = maingen_y + dj;
                if (nx < 0 || nx >= 15 || ny < 0 || ny >= 15) continue;
                if (tmp_s.board[nx][ny].player == player && tmp_s.board[nx][ny].generals.type == 0) {
                    my_cell_cnt++;
                }
            }
        }
        if (my_cell_cnt >= reserve_positions) {
            val += 1000;
        } else if (reserve_positions > 0) {
            val += 500 * my_cell_cnt;
        }
        if (val > best_value) {
            best_value = val;
            real_ops = r_ops;
        }

        for (int j = i + 1; j < siz; j++) {
            int x1, y1, dist1, type1, value1;
            std::tie(x1, y1, dist1, type1, value1) = possible_pos_dist_type_val[i];
            int x2, y2, dist2, type2, value2;
            std::tie(x2, y2, dist2, type2, value2) = possible_pos_dist_type_val[j];
            if (dist1 + dist2 > movement) continue;
            for (int k = dist1; k <= movement - dist2; k++) {
                int remain = movement - k;
                auto& ops1 = best_ops[k][i];
                auto& ops2 = best_ops[remain][j];
                GameState tmp_s1 = ns, tmp_s2 = ns;
                std::vector<Operation> real_ops1, real_ops2;
                for (auto& op : ops1) {
                    auto op_vec = execute_simp_op(tmp_s1, op, player);
                    real_ops1.insert(real_ops1.end(), op_vec.begin(), op_vec.end());
                }
                for (auto& op : ops2) {
                    auto op_vec = execute_simp_op(tmp_s1, op, player);
                    real_ops1.insert(real_ops1.end(), op_vec.begin(), op_vec.end());
                }
                for (auto& op : ops2) {
                    auto op_vec = execute_simp_op(tmp_s2, op, player);
                    real_ops2.insert(real_ops2.end(), op_vec.begin(), op_vec.end());
                }
                for (auto& op : ops1) {
                    auto op_vec = execute_simp_op(tmp_s2, op, player);
                    real_ops2.insert(real_ops2.end(), op_vec.begin(), op_vec.end());
                }
                std::vector<int> val_candidates1, val_candidates2;
                for (int n = 0; n < 4; n++) {
                    int nx = maingen_x + dx[n], ny = maingen_y + dy[n];
                    if (nx < 0 || nx >= 15 || ny < 0 || ny >= 15) continue;
                    if (tmp_s1.board[nx][ny].player == player && tmp_s1.board[nx][ny].army > 1) {
                        val_candidates1.push_back(tmp_s1.board[nx][ny].army - 1);
                    }
                    if (tmp_s2.board[nx][ny].player == player && tmp_s2.board[nx][ny].army > 1) {
                        val_candidates2.push_back(tmp_s2.board[nx][ny].army - 1);
                    }
                }
                std::sort(val_candidates1.begin(), val_candidates1.end(), std::greater<int>());
                std::sort(val_candidates2.begin(), val_candidates2.end(), std::greater<int>());
                int val1 = 0, val2 = 0;
                for (int l = 0; l < std::min((int)(val_candidates1.size()), (int)(tmp_s1.rest_move_step[player]));
                     l++) {
                    val1 += val_candidates1[l];
                }
                for (int l = 0; l < std::min((int)(val_candidates2.size()), (int)(tmp_s2.rest_move_step[player]));
                     l++) {
                    val2 += val_candidates2[l];
                }
                int reserve_positions1 = std::min((tmp_s1.coin[player] + already_coins) / 110, 2),
                    reserve_positions2 = std::min((tmp_s2.coin[player] + already_coins) / 110, 2);
                int my_cell_cnt1 = 0, my_cell_cnt2 = 0;
                for (int di = -2; di <= 2; di++) {
                    for (int dj = -2; dj <= 2; dj++) {
                        int nx = maingen_x + di, ny = maingen_y + dj;
                        if (nx < 0 || nx >= 15 || ny < 0 || ny >= 15) continue;
                        if (tmp_s1.board[nx][ny].player == player && tmp_s1.board[nx][ny].generals.type == 0) {
                            my_cell_cnt1++;
                        }
                        if (tmp_s2.board[nx][ny].player == player && tmp_s2.board[nx][ny].generals.type == 0) {
                            my_cell_cnt2++;
                        }
                    }
                }
                if (my_cell_cnt1 >= reserve_positions1) {
                    val1 += 1000;
                } else if (reserve_positions1 > 0) {
                    val1 += 500 * my_cell_cnt1;
                }
                if (my_cell_cnt2 >= reserve_positions2) {
                    val2 += 1000;
                } else if (reserve_positions2 > 0) {
                    val2 += 500 * my_cell_cnt2;
                }
                if (val1 > best_value) {
                    best_value = val1;
                    real_ops = real_ops1;
                }
                if (val2 > best_value) {
                    best_value = val2;
                    real_ops = real_ops2;
                }
            }
        }
    }
    // std::cerr << "mo" << std::endl;
    for (auto& op : real_ops) {
        final_ops.push_back(op);
        bool suc = execute(ns, op, player);
        // std::cerr << "[" << (int)(suc) << "] " << op.stringize();
    }
    // std::cerr << "mend" << std::endl;

    // std::cerr << "t2: " << clock() - st << " ";
    // st = clock();
    // use skills
    std::vector<int> gen_ids;
    for (auto& gen : ns.get_generals()) {
        if (gen.player == player && gen.type != 3) {
            gen_ids.push_back(gen.id);
        }
    }
    std::sort(gen_ids.begin(), gen_ids.end());
    GameState new_s = ns;
    auto skill_res = best_skill_dfs(new_s, maingen_x, maingen_y, player, gen_ids);
    // std::cerr << "sk" << std::endl;
    for (auto& op : skill_res.first) {
        final_ops.push_back(op);
        bool suc = execute(ns, op, player);
        // std::cerr << "[" << (int)(suc) << "] " << op.stringize();
    }
    // std::cerr << "send" << std::endl;
    // std::cerr << "t3: " << clock() - st << "\n";

    if (gameend(ns, player)) {
        // std::cerr << "gameend" << std::endl;
        return final_ops;
    }
    return std::vector<Operation>();
}

std::vector<Operation> new_kill(GameState& state, bool player) {
    auto ops1 = new_kill(state, player, true);
    if (ops1.size() > 0) return ops1;
    auto ops2 = new_kill(state, player, false);
    if (ops2.size() > 0) return ops2;
    return std::vector<Operation>();
}

std::vector<Operation> try_get_oil(GameState& state, int x, int y, bool player) {
    std::vector<Operation> ops;
    if (state.rest_move_step[player] == 0) return ops;
    if (state.board[x][y].player != player) return ops;
    int target_x = -1, target_y = -1;
    const auto& genlist = state.get_generals();
    for (const auto& gen : genlist) {
        if (gen.type == 3 && gen.player != player) {
            int gen_x = gen.position[0], gen_y = gen.position[1];
            if (state.board[gen_x][gen_y].type == CellType::SWAMP &&
                state.tech_level[player][int(TechType::IMMUNE_SWAMP)] == 0)
                continue;
            if (abs(gen_x - x) + abs(gen_y - y) == 1) {
                target_x = gen_x;
                target_y = gen_y;
                break;
            }
        }
    }
    if (target_x == -1) return ops;
    float my_att = cell_attack(x, y, player, state);
    float target_def = cell_defence(target_x, target_y, !player, state);
    int minimum_army_required = state.board[target_x][target_y].army * target_def / my_att + 1;
    if (state.board[x][y].army < minimum_army_required + 1) return ops;
    GameState ns = state;
    int dir = -1;
    if (target_x - x == -1)
        dir = 0;
    else if (target_x - x == 1)
        dir = 1;
    else if (target_y - y == -1)
        dir = 2;
    else if (target_y - y == 1)
        dir = 3;
    auto op = move_army_op({x, y}, static_cast<Direction>(dir), minimum_army_required);
    execute(ns, op, player);
    if (player == 1) ns.update_round();
    auto enemy_kill_ops = new_kill(ns, !player);
    if (enemy_kill_ops.size() == 0) {  // safe
        ops.push_back(op);
        execute(state, op, player);
    }
    return ops;
}

// // std::vector<Operation> try_get_oil_simple(GameState& state, int x, int y, bool player) {
// //     std::vector<Operation> ops;
// //     if (state.rest_move_step[player] == 0) return ops;
// //     if (state.board[x][y].player != player) return ops;
// //     int target_x = -1, target_y = -1;
// //     const auto& genlist = state.get_generals();
// //     for (const auto& gen : genlist) {
// //         if (gen.type == 3 && gen.player != player) {
// //             int gen_x = gen.position[0], gen_y = gen.position[1];
// //             if (state.board[gen_x][gen_y].type == CellType::SWAMP &&
// //                 state.tech_level[player][int(TechType::IMMUNE_SWAMP)] == 0)
// //                 continue;
// //             if (abs(gen_x - x) + abs(gen_y - y) == 1) {
// //                 target_x = gen_x;
// //                 target_y = gen_y;
// //                 break;
// //             }
// //         }
// //     }
// //     if (target_x == -1) return ops;
// //     float my_att = cell_attack(x, y, player, state);
// //     float target_def = cell_defence(target_x, target_y, !player, state);
// //     int minimum_army_required = state.board[target_x][target_y].army * target_def / my_att + 1;
// //     if (state.board[x][y].army <= minimum_army_required + 1) return ops;
// //     int dir = -1;
// //     if (target_x - x == -1)
// //         dir = 0;
// //     else if (target_x - x == 1)
// //         dir = 1;
// //     else if (target_y - y == -1)
// //         dir = 2;
// //     else if (target_y - y == 1)
// //         dir = 3;
// //     auto op = move_army_op({x, y}, static_cast<Direction>(dir), minimum_army_required);
// //     ops.push_back(op);
// //     execute(state, op, player);
// //     return ops;
// // }

// std::vector<Operation> sim_get_oil(GameState& state, bool player, std::pair<int, int> target = std::make_pair(-1,
// -1)) {
//     std::vector<Operation> ops;
//     const auto& my_maingen = find_general_by_id(state, main_general_id(state, player));
//     const int my_maingen_x = my_maingen.position[0], my_maingen_y = my_maingen.position[1];
//     const auto& enemy_maingen = find_general_by_id(state, main_general_id(state, !player));
//     const int enemy_maingen_x = enemy_maingen.position[0], enemy_maingen_y = enemy_maingen.position[1];
//     const int jump_dist = std::max(
//         std::max(abs(my_maingen_x - enemy_maingen_x) - 2, 0) + std::max(abs(my_maingen_y - enemy_maingen_y) - 2, 0),
//         1);
//     const int movement = std::min(state.tech_level[player][0], my_maingen.rest_move);

//     auto kill_ops = new_kill(state, player);
//     if (kill_ops.size() > 0) {
//         for (auto& op : kill_ops) {
//             execute(state, op, player);
//             ops.push_back(op);
//         }
//         return ops;
//     }

//     GameState ns = state;
//     if (player == 1) ns.update_round();
//     auto enemy_kill_ops = new_kill(ns, !player);
//     if (enemy_kill_ops.size() > 0) {  // escape
//         auto path =
//             best_escape_path(state, my_maingen_x, my_maingen_y, enemy_maingen_x, enemy_maingen_y, player, movement);
//         int now_x = my_maingen_x, now_y = my_maingen_y;
//         for (int i = 0; i < std::min((int)(path.size()), movement); i++) {
//             int dir = path[i];
//             int army_num = state.board[now_x][now_y].army;
//             auto op = move_army_op(std::make_pair(now_x, now_y), static_cast<Direction>(dir), army_num - 1);
//             bool exe_suc = execute(state, op, player);
//             if (!exe_suc) break;
//             ops.push_back(op);
//             now_x += dx[dir];
//             now_y += dy[dir];
//             op = move_generals_op(my_maingen.id, {now_x, now_y});
//             exe_suc = execute(state, op, player);
//             if (!exe_suc) break;
//             ops.push_back(op);
//         }
//         return ops;
//     }

//     const auto& new_my_maingen = find_general_by_id(state, main_general_id(state, player));
//     const int new_my_maingen_x = new_my_maingen.position[0], new_my_maingen_y = new_my_maingen.position[1];
//     auto oil_ops = try_get_oil(state, new_my_maingen_x, new_my_maingen_y, player);
//     ops.insert(ops.end(), oil_ops.begin(), oil_ops.end());
//     int current_movement = std::min(state.rest_move_step[player], new_my_maingen.rest_move);

//     int min_x = -1, min_y = -1;
//     if (target.first != -1) {
//         min_x = target.first;
//         min_y = target.second;
//     } else {
//         const auto& gen_list = state.get_generals();
//         int min_dist = 100;
//         for (auto& gen : gen_list) {
//             if (gen.type == 3 && gen.player != player) {
//                 int gen_x = gen.position[0], gen_y = gen.position[1];
//                 if (state.board[gen_x][gen_y].type == CellType::SWAMP &&
//                     state.tech_level[player][int(TechType::IMMUNE_SWAMP)] == 0)
//                     continue;
//                 int target_x = gen.position[0], target_y = gen.position[1];
//                 int dist = shortest_path_dist(state, new_my_maingen_x, new_my_maingen_y, target_x, target_y, player);
//                 if (dist < min_dist) {
//                     min_dist = dist;
//                     min_x = target_x;
//                     min_y = target_y;
//                 }
//             }
//         }
//     }

//     if (min_x == -1) return ops;

//     ns = state;
//     auto path = best_mobile_path(ns, new_my_maingen_x, new_my_maingen_y, min_x, min_y, player, true);
//     int now_x = new_my_maingen_x, now_y = new_my_maingen_y;
//     std::vector<Operation> new_ops;
//     for (int i = 0; i < std::min(movement, (int)(path.size())); i++) {
//         if (abs(now_x - min_x) + abs(now_y - min_y) == 1) {
//             oil_ops = try_get_oil(ns, now_x, now_y, player);
//             new_ops.insert(new_ops.end(), oil_ops.begin(), oil_ops.end());
//             break;
//         }
//         int dir = path[i];
//         int army_num = ns.board[now_x][now_y].army;
//         if (army_num <= 1) break;
//         auto op = move_army_op(std::make_pair(now_x, now_y), static_cast<Direction>(dir), army_num - 1);
//         bool exe_suc = execute(ns, op, player);
//         if (!exe_suc) break;
//         new_ops.push_back(op);
//         now_x += dx[dir];
//         now_y += dy[dir];
//         op = move_generals_op(new_my_maingen.id, {now_x, now_y});
//         exe_suc = execute(ns, op, player);
//         if (!exe_suc) break;
//         new_ops.push_back(op);
//     }
//     if (player == 1) ns.update_round();
//     enemy_kill_ops = new_kill(ns, !player);
//     if (enemy_kill_ops.size() == 0) {  // safe, can proceed
//         for (auto& op : new_ops) {
//             execute(state, op, player);
//             ops.push_back(op);
//         }
//         return ops;
//     }

//     return ops;
// }

// std::pair<int, int> best_oil_target(GameState& state, bool player) {
//     const auto& my_maingen = find_general_by_id(state, main_general_id(state, player));
//     int my_maingen_x = my_maingen.position[0], my_maingen_y = my_maingen.position[1];
//     const auto& enemy_maingen = find_general_by_id(state, main_general_id(state, !player));
//     int enemy_maingen_x = enemy_maingen.position[0], enemy_maingen_y = enemy_maingen.position[1];
//     auto gen_list = state.get_generals();
//     int best_x = -1, best_y = -1, max_val = -1000000;
//     GameState max_state;
//     for (auto& gen : gen_list) {
//         if (gen.type == 3) {
//             int target_x = gen.position[0], target_y = gen.position[1];
//             if (state.board[target_x][target_y].type == CellType::SWAMP &&
//                 state.tech_level[player][int(TechType::IMMUNE_SWAMP)] == 0)
//                 continue;
//             int dist = shortest_path_dist(state, my_maingen_x, my_maingen_y, target_x, target_y, player);
//             if (dist == 100) continue;
//             GameState ns = state;
//             for (int i = 0; i < 20; i++) {
//                 if (ns.board[target_x][target_y].player != player) {
//                     sim_get_oil(ns, player, std::make_pair(target_x, target_y));
//                 } else {
//                     sim_get_oil(ns, player);
//                 }
//                 if (gameend(ns, player)) break;
//                 if (player == 1) ns.update_round();
//                 sim_get_oil(ns, !player);
//                 if (gameend(ns, !player)) break;
//                 if (player == 0) ns.update_round();
//                 if (clock() - start_t > MAX_T0 * CLOCKS_PER_SEC) break;
//             }

//             int val = 0;
//             for (auto& new_gen : ns.get_generals()) {
//                 if (new_gen.type == 3) {
//                     if (new_gen.player == player) {
//                         val += (new_gen.produce_level * 2e5 + 1e5) * 1.5;
//                     } else if (new_gen.player == !player) {
//                         val -= new_gen.produce_level * 2e5 + 1e5;
//                     }
//                 }
//             }
//             auto ns_eval = eval(ns);
//             val += ns_eval.first - ns_eval.second;
//             if (gen.player != player) {
//                 val += (10 - dist) * 5e4;
//             }
//             if (val > max_val) {
//                 max_val = val;
//                 best_x = target_x;
//                 best_y = target_y;
//                 max_state = ns;
//             }
//         }
//     }
//     std::cerr << "best oil: " << global_best_oil.first << " " << global_best_oil.second << " " << max_val <<
//     std::endl; show_map(max_state);

//     return std::make_pair(best_x, best_y);
// }

std::vector<Operation> post_escape(GameState& state) {
    // if maingen is in danger, add defense or jump (strike)
    if (gameend(state, my)) return std::vector<Operation>();
    std::vector<Operation> ops;
    GameState newstate = state;
    if (my == 1) {
        newstate.update_round();
    }
    auto kill_ops = new_kill(newstate, !my);
    if (kill_ops.size() == 0) return std::vector<Operation>();
    int maingen_id = main_general_id(state, my), enemy_maingen_id = main_general_id(state, !my);
    const auto& maingen = find_general_by_id(state, maingen_id);
    const auto& enemy_maingen = find_general_by_id(state, enemy_maingen_id);
    int maingen_x = maingen.position[0], maingen_y = maingen.position[1];
    int enemy_maingen_x = enemy_maingen.position[0], enemy_maingen_y = enemy_maingen.position[1];
    int maingen_army = state.board[maingen_x][maingen_y].army;

    if (maingen.skills_cd[(int)SkillType::WEAKEN] == 0 && state.coin[my] >= Constant::weakening && maingen_army >= 40) {
        GameState newstate = state;
        auto op = generals_skill_op(maingen_id, SkillType::WEAKEN, {maingen_x, maingen_y});
        // ops.push_back(op);
        execute(newstate, op, my);
        if (my == 1) newstate.update_round();
        auto kill_ops = new_kill(newstate, !my);
        if (kill_ops.size() == 0) {
            ops.push_back(op);
            execute(state, op, my);
        }
    }
    if (maingen.skills_cd[(int)SkillType::SURPRISE_ATTACK] == 0 && state.coin[my] >= Constant::tactical_strike &&
        maingen_army > 1 && ops.size() == 0) {
        int best_x = -1, best_y = -1, max_val = -1000000;
        for (int di = -2; di <= 2; di++) {
            for (int dj = -2; dj <= 2; dj++) {
                int x = maingen_x + di, y = maingen_y + dj;
                if (x < 0 || x >= 15 || y < 0 || y >= 15) continue;
                if (state.board[x][y].player == !my) continue;
                if (state.board[x][y].generals.type != 0) continue;
                if (state.board[x][y].type == CellType::SWAMP && state.tech_level[my][int(TechType::IMMUNE_SWAMP)] == 0)
                    continue;
                int safe_dist = std::max(0, abs(x - enemy_maingen_x) - 2) + std::max(0, abs(y - enemy_maingen_y) - 2);
                if (safe_dist <= 2) {
                    continue;
                }
                int dist = abs(x - enemy_maingen_x) + abs(y - enemy_maingen_y);
                int val = safe_dist * 10 + dist + state.board[x][y].army;
                if (val > max_val) {
                    max_val = val;
                    best_x = x;
                    best_y = y;
                }
            }
        }
        if (best_x != -1 && best_y != -1) {
            auto op = generals_skill_op(maingen_id, SkillType::SURPRISE_ATTACK, {best_x, best_y});
            ops.push_back(op);
            execute(state, op, my);
        }
    }
    return ops;
}

std::vector<Operation> tp(GameState& state) {
    if (gameend(state, my)) return std::vector<Operation>();
    std::vector<Operation> ops;
    memset(block_list, 0, sizeof(block_list));
    block_list[4] = block_list[5] = block_list[6] = block_list[7] = block_list[8] = true;
    int my_maingen_id = main_general_id(state, my);
    const auto& my_maingen = find_general_by_id(state, my_maingen_id);
    int my_maingen_x = my_maingen.position[0], my_maingen_y = my_maingen.position[1];
    int connected_comp_cnt = find_connected_component(state, my_maingen_x, my_maingen_y);
    int total_available_cnt = 0;
    for (int i = 0; i < 15; i++) {
        for (int j = 0; j < 15; j++) {
            if (block_list[board[i][j]] == false) {
                total_available_cnt++;
            }
        }
    }
    if (float(connected_comp_cnt) / total_available_cnt > 0.25) {
        block_flag = false;
    } else {
        block_flag = true;
    }
    memset(block_list, 0, sizeof(block_list));
    block_list[4] = block_list[5] = block_list[7] = block_list[8] = true;
    connected_comp_cnt = find_connected_component(state, my_maingen_x, my_maingen_y);
    if (float(connected_comp_cnt) / total_available_cnt > 0.25) return std::vector<Operation>();

    bool have_oil_target = false;
    for (const auto& gen : state.get_generals()) {
        if (gen.type == 3 && gen.player != my) {
            if (board[gen.position[0]][gen.position[1]] == 8 || board[gen.position[0]][gen.position[1]] == 9 ||
                board[gen.position[0]][gen.position[1]] == 10)
                continue;
            memset(block_list, 0, sizeof(block_list));
            block_list[3] = block_list[4] = block_list[5] = block_list[6] = block_list[7] = block_list[8] =
                block_list[10] = true;
            auto dir_dist = shortest_path_dirs(state, my_maingen_x, my_maingen_y, gen.position[0], gen.position[1]);
            if (dir_dist.first.size() > 0) {
                have_oil_target = true;
                break;
            }
        }
    }
    if (have_oil_target) return std::vector<Operation>();
    if (my_maingen.skills_cd[(int)SkillType::SURPRISE_ATTACK] > 0 || state.coin[my] < Constant::tactical_strike ||
        state.board[my_maingen_x][my_maingen_y].army <= 1)
        return std::vector<Operation>();

    std::cerr << "tp\n";
    memset(block_list, 0, sizeof(block_list));
    block_list[4] = block_list[5] = block_list[6] = block_list[7] = block_list[8] = true;
    int connected_component_cnt[15][15];
    int nearest_oil_dist[15][15];
    memset(block_list, 1, sizeof(block_list));
    for (int i = 0; i < 15; i++) {
        for (int j = 0; j < 15; j++) {
            nearest_oil_dist[i][j] = 100;
            for (const auto& gen : state.get_generals()) {
                if (gen.type == 3 && gen.player != my) {
                    int dist = abs(gen.position[0] - i) + abs(gen.position[1] - j);
                    nearest_oil_dist[i][j] = std::min(nearest_oil_dist[i][j], dist);
                }
            }
        }
    }
    memset(connected_component_cnt, 0, sizeof(connected_component_cnt));
    int best_x = -1, best_y = -1;
    float best_val = -1e15;
    for (int di = -2; di <= 2; di++) {
        for (int dj = -2; dj <= 2; dj++) {
            int x = my_maingen_x + di, y = my_maingen_y + dj;
            if (x < 0 || x >= 15 || y < 0 || y >= 15) continue;
            if (board[x][y] == -1 || board[x][y] == 0) {
                if (connected_component_cnt[x][y] == 0) {
                    int cnt = find_connected_component(state, x, y);
                    for (int k = 0; k < 15; k++) {
                        for (int l = 0; l < 15; l++) {
                            if (connected_component_res[k][l]) {
                                connected_component_cnt[k][l] = cnt;
                            }
                        }
                    }
                }
                int cnt = connected_component_cnt[x][y];
                if (float(cnt) / total_available_cnt <= 0.25) continue;

                float val = -nearest_oil_dist[x][y] * 100;
                float my_battleval = (state.board[x][y].army + state.board[my_maingen_x][my_maingen_y].army - 1) *
                                         cell_defence(x, y, my, state) * my_maingen.defence_level -
                                     threat_value[x][y];
                if (my_battleval < 1) {
                    val += (my_battleval - 1) * 1e5;
                } else {
                    val += my_battleval * 10;
                }
                if (val > best_val) {
                    best_val = val;
                    best_x = x;
                    best_y = y;
                }
            }
        }
    }
    if (best_x != -1 && best_y != -1 && best_val > -1e5) {
        auto op = generals_skill_op(my_maingen.id, SkillType::SURPRISE_ATTACK, {best_x, best_y});
        ops.push_back(op);
        execute(state, op, my);
    }
    return ops;
}

std::vector<Operation> use_tech_main(GameState& state) {
    if (gameend(state, my)) return std::vector<Operation>();
    std::vector<Operation> ops;
    int my_maingen_id = main_general_id(state, my);
    const auto& my_maingen = find_general_by_id(state, my_maingen_id);
    // if (my_maingen.defence_level < 2) return std::vector<Operation>();
    int my_maingen_x = my_maingen.position[0], my_maingen_y = my_maingen.position[1];
    float my_maingen_battle_val =
        state.board[my_maingen_x][my_maingen_y].army * cell_defence(my_maingen_x, my_maingen_y, my, state) -
        threat_value[my_maingen_x][my_maingen_y];
    bool maingen_safe = my_maingen_battle_val >= 0;
    if (!maingen_safe && my_maingen.skills_cd[(int)SkillType::ROUT] <= 0 && state.coin[my] >= Constant::breakthrough) {
        int best_x = -1, best_y = -1, best_val = -1000000;
        for (int dx = -2; dx <= 2; dx++) {
            for (int dy = -2; dy <= 2; dy++) {
                if (dx == 0 && dy == 0) continue;
                int x = my_maingen_x + dx, y = my_maingen_y + dy;
                int val = 0;
                if (x < 0 || x >= 15 || y < 0 || y >= 15) continue;
                if ((board[x][y] == 1 || board[x][y] == 3 || board[x][y] == 5) && state.board[x][y].army > 20) {
                    if (state.board[x][y].generals.type != 0) {
                        if (state.board[x][y].generals.type == 1) {
                            val += 10000;
                        } else if (state.board[x][y].generals.type == 2) {
                            val += 5000;
                        }
                    }
                    val += state.board[x][y].army;
                    if (val > best_val) {
                        best_val = val;
                        best_x = x;
                        best_y = y;
                    }
                }
            }
        }
        if (best_x != -1 && best_y != -1) {
            auto op = generals_skill_op(my_maingen_id, SkillType::ROUT, {best_x, best_y});
            ops.push_back(op);
            execute(state, op, my);
        }
    }

    return ops;
}

std::vector<Operation> use_tech_sub(GameState& state) {
    if (gameend(state, my)) return std::vector<Operation>();
    std::vector<Operation> ops;
    return ops;
}

std::vector<Operation> upgrade(GameState& state) {
    if (gameend(state, my)) return std::vector<Operation>();
    std::vector<Operation> ops;
    // upgrade MainGeneral
    int my_maingen_id = main_general_id(state, my);
    const auto& my_maingen = find_general_by_id(state, my_maingen_id);
    int my_maingen_x = my_maingen.position[0], my_maingen_y = my_maingen.position[1];
    int enemy_maingen_id = main_general_id(state, !my);
    const auto& enemy_maingen = find_general_by_id(state, enemy_maingen_id);
    int maingen_safedist = std::max(0, abs(my_maingen_x - enemy_maingen.position[0]) - 2) +
                           std::max(0, abs(my_maingen_y - enemy_maingen.position[1]) - 2);
    bool maingen_produce_flag = false;
    bool maingen_move_flag = false;
    bool maingen_defence_flag = false;
    int safety_coin = 0;
    int prepare_coin = 35;
    int prepare_coin2 = 50;
    float maingen_battleval =
        state.board[my_maingen_x][my_maingen_y].army * cell_defence(my_maingen_x, my_maingen_y, my, state) -
        threat_value[my_maingen_x][my_maingen_y];
    bool maingen_safe = maingen_battleval >= 1;
    if (maingen_safedist <= 4) {
        safety_coin = 35;
    }
    if (my_maingen.produce_level == 1) {
        if (state.coin[my] >= Constant::lieutenant_production_T1 / 2) {
            auto op = update_generals_op(my_maingen_id, QualityType::PRODUCTION);
            ops.push_back(op);
            execute(state, op, my);
            if (!block_flag) {
                op = update_generals_op(my_maingen_id, QualityType::MOBILITY);
                ops.push_back(op);
                execute(state, op, my);
            }
        }
    } else if (my_maingen.produce_level == 2) {
        if (state.coin[my] >= Constant::lieutenant_production_T2 / 2 + safety_coin) {
            auto op = update_generals_op(my_maingen_id, QualityType::PRODUCTION);
            ops.push_back(op);
            execute(state, op, my);
            maingen_produce_flag = true;
        }
    } else {
        maingen_produce_flag = true;
    }

    if (maingen_produce_flag) {
        if (my_maingen.mobility_level == 1) {
            if ((!maingen_safe && state.coin[my] >= Constant::general_movement_T1 / 2) ||
                state.coin[my] >= Constant::general_movement_T1 / 2 + safety_coin) {
                auto op = update_generals_op(my_maingen_id, QualityType::MOBILITY);
                ops.push_back(op);
                execute(state, op, my);
                maingen_move_flag = true;
            }
        } else {
            maingen_move_flag = true;
        }
    }

    if (maingen_produce_flag && maingen_move_flag) {
        if (my_maingen.defence_level == 1) {
            if ((!maingen_safe && state.coin[my] >= Constant::lieutenant_defense_T1 / 2) ||
                state.coin[my] >= Constant::lieutenant_defense_T1 / 2 + safety_coin) {
                auto op = update_generals_op(my_maingen_id, QualityType::DEFENCE);
                ops.push_back(op);
                execute(state, op, my);
                maingen_defence_flag = true;
            }
        } else {
            maingen_defence_flag = true;
        }
    }
    bool maingen_ready = maingen_produce_flag && maingen_move_flag && maingen_defence_flag;

    for (const auto& general : state.get_generals()) {
        int pos_x = general.position[0], pos_y = general.position[1];
        float def = state.board[pos_x][pos_y].army * cell_defence(pos_x, pos_y, my, state) - threat_value[pos_x][pos_y];
        float imp = impact_value[pos_x][pos_y];
        float safety = def + imp * 0.1;
        bool abs_safe = enemy_impact[pos_x][pos_y] <= 0;
        if (general.player == my) {
            if (general.type == 3) {
                if (general.produce_level == 1 && abs_safe) {
                    if ((maingen_safedist > 4 && state.coin[my] >= Constant::OilWell_production_T1) ||
                        state.coin[my] >= Constant::OilWell_production_T1 + prepare_coin) {
                        auto op = update_generals_op(general.id, QualityType::PRODUCTION);
                        ops.push_back(op);
                        execute(state, op, my);
                    }
                }
            }
        }
    }

    // bool subgen_flag = false;
    // for (const auto& general : state.get_generals()) {
    //     if (general.type == 2 && general.player == my) {
    //         subgen_flag = true;
    //         break;
    //     }
    // }

    if (maingen_ready && (!maingen_safe || state.coin[my] > 150)) {
        if (my_maingen.defence_level == 2) {
            if (state.coin[my] >= Constant::lieutenant_defense_T2 / 2) {
                auto op = update_generals_op(my_maingen_id, QualityType::DEFENCE);
                ops.push_back(op);
                execute(state, op, my);
            }
        }
        if (my_maingen.mobility_level == 2 && state.tech_level[my][int(TechType::MOBILITY)] >= 3) {
            if (state.coin[my] >= Constant::general_movement_T2 / 2) {
                auto op = update_generals_op(my_maingen_id, QualityType::MOBILITY);
                ops.push_back(op);
                execute(state, op, my);
            }
        }
    }

    // if (maingen_ready && (!subgen_flag)) {
    //     if (state.coin[my] >= 200) {
    //         int max_x = -1, max_y = -1;
    //         float max_safety = -1000000;
    //         for (int i = 0; i < 15; i++) {
    //             for (int j = 0; j < 15; j++) {
    //                 if (board[i][j] == 0) {
    //                     float def = state.board[i][j].army * cell_defence(i, j, my, state) - threat_value[i][j];
    //                     float imp = impact_value[i][j];
    //                     float safety = def + imp * 0.1;
    //                     if (safety > max_safety) {
    //                         max_safety = safety;
    //                         max_x = i;
    //                         max_y = j;
    //                     }
    //                 }
    //             }
    //         }
    //         if (max_x != -1 && max_y != -1 && max_safety > 0) {
    //             auto op = call_generals_op({max_x, max_y});
    //             ops.push_back(op);
    //             execute(state, op, my);
    //         }
    //     }
    // }

    bool sub_gen_ready = true;
    if (maingen_ready) {
        if (state.tech_level[my][int(TechType::IMMUNE_SWAMP)] == 0) {
            if (state.coin[my] >= Constant::swamp_immunity + prepare_coin) {
                auto op = update_tech_op(TechType::IMMUNE_SWAMP);
                ops.push_back(op);
                execute(state, op, my);
            }
        }
        for (const auto& general : state.get_generals()) {
            int pos_x = general.position[0], pos_y = general.position[1];
            float def =
                state.board[pos_x][pos_y].army * cell_defence(pos_x, pos_y, my, state) - threat_value[pos_x][pos_y];
            float imp = impact_value[pos_x][pos_y];
            float safety = def + imp * 0.1;
            bool abs_safe = enemy_impact[pos_x][pos_y] <= 0;
            if (general.player == my) {
                if (general.type == 3) {
                    if (general.produce_level == 1 && (safety > 8 || abs_safe)) {
                        if (state.coin[my] >= Constant::OilWell_production_T1 + prepare_coin2) {
                            auto op = update_generals_op(general.id, QualityType::PRODUCTION);
                            ops.push_back(op);
                            execute(state, op, my);
                        }
                    }
                    if (general.produce_level == 2 && (safety > 20)) {
                        if (state.coin[my] >= Constant::OilWell_production_T2 + prepare_coin2) {
                            auto op = update_generals_op(general.id, QualityType::PRODUCTION);
                            ops.push_back(op);
                            execute(state, op, my);
                        }
                    }
                    if (general.produce_level == 4 && safety > 100) {
                        if (state.coin[my] >= Constant::OilWell_production_T3 + prepare_coin2) {
                            auto op = update_generals_op(general.id, QualityType::PRODUCTION);
                            ops.push_back(op);
                            execute(state, op, my);
                        }
                    }
                } else if (general.type == 2) {
                    if (general.produce_level == 1 && (safety > 15 || abs_safe)) {
                        if (state.coin[my] >= Constant::lieutenant_production_T1 + prepare_coin2) {
                            auto op = update_generals_op(general.id, QualityType::PRODUCTION);
                            ops.push_back(op);
                            execute(state, op, my);
                        } else {
                            sub_gen_ready = false;
                        }
                    }
                }
            }
        }

        if (sub_gen_ready || state.coin[my] >= 100) {
            if (state.tech_level[my][int(TechType::IMMUNE_SWAMP)] == 0) {
                if (state.coin[my] >= Constant::swamp_immunity + prepare_coin) {
                    auto op = update_tech_op(TechType::IMMUNE_SWAMP);
                    ops.push_back(op);
                    execute(state, op, my);
                }
            } else if (state.tech_level[my][int(TechType::MOBILITY)] == 2) {
                if (state.coin[my] >= Constant::army_movement_T1 + prepare_coin) {
                    auto op = update_tech_op(TechType::MOBILITY);
                    ops.push_back(op);
                    execute(state, op, my);
                }
            } else if (state.tech_level[my][int(TechType::IMMUNE_SAND)] == 0) {
                if (state.coin[my] >= Constant::sand_immunity + prepare_coin2) {
                    auto op = update_tech_op(TechType::IMMUNE_SAND);
                    ops.push_back(op);
                    execute(state, op, my);
                }
            } else if (state.tech_level[my][int(TechType::MOBILITY)] == 3) {
                if (state.coin[my] >= Constant::army_movement_T2 + prepare_coin2) {
                    auto op = update_tech_op(TechType::MOBILITY);
                    ops.push_back(op);
                    execute(state, op, my);
                }
            } else if (state.tech_level[my][int(TechType::UNLOCK)] == 0) {
                if (state.coin[my] >= Constant::unlock_super_weapon + prepare_coin2) {
                    auto op = update_tech_op(TechType::UNLOCK);
                    ops.push_back(op);
                    execute(state, op, my);
                }
            }
        }
    }

    bool all_ready = true;
    if (my_maingen.produce_level < 4 || my_maingen.mobility_level < 4 || my_maingen.defence_level < 3)
        all_ready = false;
    // if (!subgen_flag) all_ready = false;
    if (state.tech_level[my][int(TechType::IMMUNE_SWAMP)] < 1 || state.tech_level[my][int(TechType::MOBILITY)] < 5 ||
        state.tech_level[my][int(TechType::IMMUNE_SAND)] < 1 || state.tech_level[my][int(TechType::UNLOCK)] < 1)
        all_ready = false;

    if (all_ready) {
        for (const auto& general : state.get_generals()) {
            int pos_x = general.position[0], pos_y = general.position[1];
            float def =
                state.board[pos_x][pos_y].army * cell_defence(pos_x, pos_y, my, state) - threat_value[pos_x][pos_y];
            float imp = impact_value[pos_x][pos_y];
            float safety = def + imp * 0.1;
            bool abs_safe = enemy_impact[pos_x][pos_y] <= 0;
            if (general.player == my) {
                if (general.type == 3) {
                    if (general.produce_level >= 2 && safety > 20 && general.defence_level == 1) {
                        if (state.coin[my] >= Constant::OilWell_defense_T1 + prepare_coin2) {
                            auto op = update_generals_op(general.id, QualityType::DEFENCE);
                            ops.push_back(op);
                            execute(state, op, my);
                        }
                    } else if (general.produce_level >= 4 && safety > 50 && general.defence_level == 1.5) {
                        if (state.coin[my] >= Constant::OilWell_defense_T2 + prepare_coin2) {
                            auto op = update_generals_op(general.id, QualityType::DEFENCE);
                            ops.push_back(op);
                            execute(state, op, my);
                        }
                    } else if (general.produce_level >= 4 && safety > 100 && general.defence_level == 2) {
                        if (state.coin[my] >= Constant::OilWell_defense_T3 + prepare_coin2) {
                            auto op = update_generals_op(general.id, QualityType::DEFENCE);
                            ops.push_back(op);
                            execute(state, op, my);
                        }
                    }
                } else if (general.type == 2) {
                    if (general.produce_level == 2 && (safety > 25 || abs_safe)) {
                        if (state.coin[my] >= Constant::lieutenant_production_T2 + prepare_coin2) {
                            auto op = update_generals_op(general.id, QualityType::PRODUCTION);
                            ops.push_back(op);
                            execute(state, op, my);
                        }
                    } else if (general.produce_level >= 2 && safety > 25 && general.defence_level == 1) {
                        if (state.coin[my] >= Constant::lieutenant_defense_T1 + prepare_coin2) {
                            auto op = update_generals_op(general.id, QualityType::DEFENCE);
                            ops.push_back(op);
                            execute(state, op, my);
                        }
                    } else if (general.produce_level >= 2 && safety > 25 && general.mobility_level == 1) {
                        if (state.coin[my] >= Constant::general_movement_T1 + prepare_coin2) {
                            auto op = update_generals_op(general.id, QualityType::MOBILITY);
                            ops.push_back(op);
                            execute(state, op, my);
                        }
                    } else if (general.produce_level >= 4 && safety > 100 && general.mobility_level == 2) {
                        if (state.coin[my] >= Constant::general_movement_T2 + prepare_coin2) {
                            auto op = update_generals_op(general.id, QualityType::MOBILITY);
                            ops.push_back(op);
                            execute(state, op, my);
                        }
                    } else if (general.produce_level >= 4 && safety > 100 && general.defence_level == 2) {
                        if (state.coin[my] >= Constant::lieutenant_defense_T2 + prepare_coin2) {
                            auto op = update_generals_op(general.id, QualityType::DEFENCE);
                            ops.push_back(op);
                            execute(state, op, my);
                        }
                    }
                }
            }
        }
        if (state.coin[my] >= Constant::lieutenant_new_recruit + 200) {
            int max_x = -1, max_y = -1;
            float max_safety = -1000000;
            for (int i = 0; i < 15; i++) {
                for (int j = 0; j < 15; j++) {
                    if (board[i][j] == 0) {
                        float def = state.board[i][j].army * cell_defence(i, j, my, state) - threat_value[i][j];
                        float imp = impact_value[i][j];
                        float safety = def + imp * 0.1;
                        if (safety > max_safety) {
                            max_safety = safety;
                            max_x = i;
                            max_y = j;
                        }
                    }
                }
            }
            if (max_x != -1 && max_y != -1 && max_safety > 5) {
                auto op = call_generals_op({max_x, max_y});
                ops.push_back(op);
                execute(state, op, my);
            }
        }
    }

    return ops;
}

std::vector<Operation> support_maingen(GameState& state) {
    if (gameend(state, my)) return std::vector<Operation>();
    std::vector<Operation> ops;
    int maingen_id = main_general_id(state, my);
    const auto& maingen = find_general_by_id(state, maingen_id);
    int maingen_x = maingen.position[0], maingen_y = maingen.position[1];
    for (int k = 0; k < 4; k++) {
        int nx = maingen_x + dx[k], ny = maingen_y + dy[k];
        if (nx < 0 || nx >= 15 || ny < 0 || ny >= 15) continue;
        if (state.board[nx][ny].player == my && state.board[nx][ny].generals.type != 0) {
            const auto& gen = state.board[nx][ny].generals;
            if (gen.type != 2) continue;
            int gen_x = gen.position[0], gen_y = gen.position[1];
            if (state.board[gen_x][gen_y].army <= 10) continue;
            int move_dir = k ^ 1;
            auto op =
                move_army_op({gen_x, gen_y}, static_cast<Direction>(move_dir), state.board[gen_x][gen_y].army - 1);
            ops.push_back(op);
            execute(state, op, my);
            break;
        }
    }
    return ops;
}

std::vector<Operation> move_gen_and_army(GameState& state, int gen_id) {
    // std::cerr << "move_gen: " << gen_id << "\n";
    if (gameend(state, my)) return std::vector<Operation>();
    std::vector<Operation> ops;
    const auto& gen = find_general_by_id(state, gen_id);
    const int gen_x = gen.position[0], gen_y = gen.position[1];
    int enemy_maingen_id = main_general_id(state, !my);
    const auto& enemy_maingen = find_general_by_id(state, enemy_maingen_id);
    int enemy_maingen_x = enemy_maingen.position[0], enemy_maingen_y = enemy_maingen.position[1];
    const int enemy_maingen_army = state.board[enemy_maingen_x][enemy_maingen_y].army;
    const int now_army = state.board[gen_x][gen_y].army;
    if (now_army <= 1) return ops;

    int move_dir = -1;
    float value[5];
    bool launch_attack[4];
    memset(value, 0, sizeof(value));
    memset(launch_attack, 0, sizeof(launch_attack));
    float min_dist = 1000000;
    std::vector<int> oil_dir, neut_gen_dir, enemy_gen_dir, support_gen_dir;
    // if (gen.type == 2) {
    for (const auto& new_gen : state.get_generals()) {
        if (new_gen.type == 3) {
            if (board[new_gen.position[0]][new_gen.position[1]] == 8 ||
                board[new_gen.position[0]][new_gen.position[1]] == 9 ||
                board[new_gen.position[0]][new_gen.position[1]] == 10)
                continue;
            // memset(block_list, 0, sizeof(block_list));
            // block_list[3] = block_list[4] = block_list[5] = block_list[6] = block_list[7] = block_list[8] =
            //     block_list[9] = block_list[10] = true;
            // auto dir_dist_my = shortest_path_dirs(state, gen_x, gen_y, gen.position[0], gen.position[1]);
            // memset(block_list, 0, sizeof(block_list));
            // block_list[2] = block_list[4] = block_list[5] = block_list[6] = block_list[7] = block_list[8] =
            //     block_list[10] = true;
            // auto dir_dist_enemy =
            //     shortest_path_dirs(state, enemy_maingen_x, enemy_maingen_y, gen.position[0], gen.position[1]);
            auto my_path =
                best_mobile_path(state, gen_x, gen_y, new_gen.position[0], new_gen.position[1], my, true, false);
            auto my_path2 =
                best_mobile_path(state, gen_x, gen_y, new_gen.position[0], new_gen.position[1], my, false, false);
            auto enemy_path = best_mobile_path(state, enemy_maingen_x, enemy_maingen_y, new_gen.position[0],
                                               new_gen.position[1], !my, true, false);
            float loss = 0;
            int my_dist = std::min(my_path.size() > 0 ? (int)(my_path.size()) : 20, 20);
            int my_dist2 = std::min(my_path2.size() > 0 ? (int)(my_path2.size()) : 20, 20);

            int enemy_dist = std::min(enemy_path.size() > 0 ? (int)(enemy_path.size()) : 20, 20);
            if (new_gen.player == my) {  // defence
                loss += my_dist * 0.5 + enemy_dist * 2 - 6;
            } else {
                loss += my_dist - enemy_dist * 0.25;
                if (new_gen.player == !my) {
                    loss -= 2;
                }
            }
            loss -= new_gen.produce_level * 1.5;
            if (my_dist < my_dist2 - 2 && my_path.size() > 0) {
                loss += 2;
                if (loss < min_dist) {
                    min_dist = loss;
                    oil_dir = {my_path[0]};
                }
            } else if (my_path2.size() > 0) {
                loss += my_dist2 - my_dist;
                if (loss < min_dist) {
                    min_dist = loss;
                    oil_dir = {my_path2[0]};
                }
            }
        }
    }
    // }
    // else {
    //     if (global_best_oil.first != -1) {
    //         auto path = best_mobile_path(state, gen_x, gen_y, global_best_oil.first, global_best_oil.second, my,
    //         true); if (path.size() > 0) {
    //             oil_dir.push_back(path[0]);
    //         }
    //     }
    // }

    if (oil_dir.size() > 0) {
        for (int dir : oil_dir) {
            value[dir] += 50000;
        }
    }

    // min_dist = 1000000;
    // for (const auto& gen : state.get_generals()) {
    //     if (gen.type == 2 && gen.player == -1) {
    //         if (board[gen.position[0]][gen.position[1]] == 8 || board[gen.position[0]][gen.position[1]] == 9 ||
    //             board[gen.position[0]][gen.position[1]] == 10)
    //             continue;
    //         memset(block_list, 0, sizeof(block_list));
    //         block_list[3] = block_list[4] = block_list[5] = block_list[6] = block_list[7] = block_list[8] =
    //             block_list[9] = block_list[10] = true;
    //         auto dir_dist = shortest_path_dirs(state, gen_x, gen_y, gen.position[0], gen.position[1]);
    //         if (dir_dist.first.size() > 0 && dir_dist.second < min_dist) {
    //             min_dist = dir_dist.second;
    //             neut_gen_dir = dir_dist.first;
    //         }
    //     }
    // }

    if (neut_gen_dir.size() == 0) {
        min_dist = 1000000;
        for (const auto& gen : state.get_generals()) {
            if (gen.type == 2 && gen.player == -1) {
                if (board[gen.position[0]][gen.position[1]] == 8 || board[gen.position[0]][gen.position[1]] == 9)
                    continue;
                memset(block_list, 0, sizeof(block_list));
                block_list[3] = block_list[4] = block_list[5] = block_list[6] = block_list[7] = block_list[8] =
                    block_list[10] = true;
                auto dir_dist = shortest_path_dirs(state, gen_x, gen_y, gen.position[0], gen.position[1]);
                if (dir_dist.first.size() > 0 && dir_dist.second < min_dist) {
                    min_dist = dir_dist.second;
                    neut_gen_dir = dir_dist.first;
                }
            }
        }
    }

    if (neut_gen_dir.size() > 0) {
        for (int dir : neut_gen_dir) {
            value[dir] += 5000;
        }
    }

    // min_dist = 1000000;
    // for (const auto& gen : state.get_generals()) {
    //     if (gen.type == 2 && gen.player == !my && gen.defence_level == 1 &&
    //         state.board[gen.position[0]][gen.position[1]].army < now_army / 2) {
    //         if (board[gen.position[0]][gen.position[1]] == 8 || board[gen.position[0]][gen.position[1]] == 9 ||
    //             board[gen.position[0]][gen.position[1]] == 10)
    //             continue;
    //         memset(block_list, 0, sizeof(block_list));
    //         block_list[3] = block_list[4] = block_list[5] = block_list[7] = block_list[8] = block_list[10] = true;
    //         auto dir_dist = shortest_path_dirs(state, gen_x, gen_y, gen.position[0], gen.position[1]);
    //         if (dir_dist.first.size() > 0 && dir_dist.second < min_dist) {
    //             min_dist = dir_dist.second;
    //             enemy_gen_dir = dir_dist.first;
    //         }
    //     }
    // }

    // if (enemy_gen_dir.size() > 0) {
    //     for (int dir : enemy_gen_dir) {
    //         value[dir] += 10000;
    //     }
    // }

    // consider enemy main general
    // float buff_atk = 1;
    // int used_coin = 0;
    // int additional_dmg = 0;
    // if (enemy_maingen.skill_duration[0] > 0 && enemy_maingen.skill_duration[2] > 0) {
    //     buff_atk = 2;
    // } else if (enemy_maingen.skill_duration[0] > 0 && enemy_maingen.skill_duration[2] == 0) {
    //     if (state.coin[!my] >= Constant::weakening) {
    //         buff_atk = 2;
    //         used_coin = Constant::weakening;
    //     } else
    //         buff_atk = 1.5;
    // } else if (enemy_maingen.skill_duration[0] == 0 && enemy_maingen.skill_duration[2] > 0) {
    //     if (state.coin[!my] >= Constant::leadership) {
    //         buff_atk = 2;
    //         used_coin = Constant::leadership;
    //     } else
    //         buff_atk = 1 / 0.75;
    // } else {
    //     if (state.coin[!my] >= Constant::leadership + Constant::weakening) {
    //         buff_atk = 2;
    //         used_coin = Constant::leadership + Constant::weakening;
    //     } else if (state.coin[!my] >= Constant::leadership) {
    //         buff_atk = 1.5;
    //         used_coin = Constant::leadership;
    //     }
    // }
    // if (state.coin[!my] - used_coin >= Constant::lieutenant_new_recruit + Constant::leadership + Constant::weakening)
    // {
    //     buff_atk *= 2;
    //     used_coin += Constant::lieutenant_new_recruit + Constant::leadership + Constant::weakening;
    // } else if (state.coin[!my] - used_coin >= Constant::lieutenant_new_recruit + Constant::leadership) {
    //     buff_atk *= 1.5;
    //     used_coin += Constant::lieutenant_new_recruit + Constant::leadership;
    // }
    // if (enemy_maingen.skills_cd[(int)SkillType::ROUT] <= 0 && state.coin[!my] - used_coin >= Constant::breakthrough)
    // {
    //     additional_dmg += 20;
    //     used_coin += Constant::breakthrough;
    // }
    // if (used_coin >= 100 && state.coin[!my] - used_coin >= Constant::breakthrough) {
    //     additional_dmg += 20;
    //     used_coin += Constant::breakthrough;
    // }
    // if (my_eval - enemy_eval > 2000 &&
    //     (now_army - additional_dmg) * gen.defence_level - enemy_maingen_army * buff_atk >= 1) {
    //     memset(block_list, 0, sizeof(block_list));
    //     block_list[3] = block_list[4] = block_list[5] = block_list[7] = block_list[8] = block_list[9] = true;
    //     auto dir_dist = shortest_path_dirs(state, gen_x, gen_y, enemy_maingen_x, enemy_maingen_y);
    //     if (dir_dist.first.size() > 0) {
    //         for (int dir : dir_dist.first) {
    //             value[dir] += 1e5;
    //         }
    //     }
    // }

    // consider supporting
    int supp_x = -1, supp_y = -1;
    float max_supp_val = -1e15;
    if (gen.type == 1) {
        for (const auto& supp_gen : state.get_generals()) {
            if (supp_gen.type == 2 && supp_gen.player == my) {
                int supp_gen_x = supp_gen.position[0], supp_gen_y = supp_gen.position[1];
                if ((state.board[supp_gen_x][supp_gen_y].army < 50 &&
                     state.board[supp_gen_x][supp_gen_y].army < now_army / 2) ||
                    state.board[supp_gen_x][supp_gen_y].army < 20)
                    continue;
                int dist = abs(supp_gen_x - gen_x) + abs(supp_gen_y - gen_y);
                float val =
                    state.board[supp_gen_x][supp_gen_y].army * state.board[supp_gen_x][supp_gen_y].army / (float)(dist);
                if (val > max_supp_val) {
                    max_supp_val = val;
                    supp_x = supp_gen_x;
                    supp_y = supp_gen_y;
                }
            }
        }

        // if (supp_x == -1 && supp_y == -1) {
        //     if (enemy_maingen_army >= 40 &&
        //         ((enemy_maingen_army * buff_atk >= now_army * gen.defence_level * 1.5 && now_army <= 40) ||
        //          (enemy_maingen_army * buff_atk >= now_army * gen.defence_level * 1.5 && now_army > 40 &&
        //           now_army <= 100) ||
        //          (enemy_maingen_army * buff_atk >= now_army * gen.defence_level * 1.5))) {
        //         for (int i = 0; i < 15; i++) {
        //             for (int j = 0; j < 15; j++) {
        //                 if (board[i][j] == 0) {
        //                     int dist = abs(i - gen_x) + abs(j - gen_y);
        //                     float val = state.board[i][j].army * state.board[i][j].army / (float)(dist);
        //                     if (val > max_supp_val) {
        //                         max_supp_val = val;
        //                         supp_x = i;
        //                         supp_y = j;
        //                     }
        //                 }
        //             }
        //         }
        //     }
        // }
    } else {
        int maingen_id = main_general_id(state, my);
        const auto& maingen = find_general_by_id(state, maingen_id);
        int maingen_x = maingen.position[0], maingen_y = maingen.position[1];
        int maingen_army = state.board[maingen_x][maingen_y].army;
        if (now_army >= 10 && abs(maingen_x - gen_x) + abs(maingen_y - gen_y) > 2) {
            supp_x = maingen_x;
            supp_y = maingen_y;
        }
    }
    if (supp_x != -1 && supp_y != -1) {
        memset(block_list, 0, sizeof(block_list));
        block_list[2] = block_list[3] = block_list[4] = block_list[5] = block_list[7] = block_list[8] = block_list[9] =
            block_list[10] = true;
        auto dir_dist = shortest_path_dirs(state, gen_x, gen_y, supp_x, supp_y);
        if (dir_dist.first.size() > 0) {
            for (int dir : dir_dist.first) {
                value[dir] += 1e5;
            }
        }
    }

    std::vector<int> primary_dirs;
    int primary_val = 0;
    for (int i = 0; i < 4; i++) {
        if (value[i] > primary_val) {
            primary_val = value[i];
            primary_dirs.clear();
            primary_dirs.push_back(i);
        } else if (value[i] == primary_val) {
            primary_dirs.push_back(i);
        }
    }

    bool final_check = gen.type == 1 && gen.rest_move == 1;
    float threat_vals[4][2];
    for (int i = 0; i < 4; i++) {
        int nx = gen_x + dx[i], ny = gen_y + dy[i];
        if (nx < 0 || nx >= 15 || ny < 0 || ny >= 15) {
            value[i] = -1e15;
            continue;
        }
        if (board[nx][ny] == 8 || board[nx][ny] == 2 || board[nx][ny] == 4) {
            value[i] = -1e15;
            continue;
        }
        if (board[nx][ny] == 9) {
            value[i] -= 15000;
        }
        if (board[nx][ny] == 10) {
            value[i] -= 100000;
        }
        if (board[nx][ny] == 1 || board[nx][ny] == 3 || board[nx][ny] == 5 || board[nx][ny] == 6 ||
            board[nx][ny] == 7) {
            launch_attack[i] = true;
        }
        if (board[nx][ny] == 9 || board[nx][ny] == 10) {
            if (state.board[nx][ny].player != my && state.board[nx][ny].army > 0) {
                launch_attack[i] = true;
            }
            if (state.board[nx][ny].generals.type != 0) {
                if (state.board[nx][ny].generals.player == my) {
                    value[i] = -1e15;
                    continue;
                }
                if (state.board[nx][ny].generals.type == 1) {
                    value[i] += 1e12;
                } else if (state.board[nx][ny].army < 20) {
                    value[i] += 3e6;
                    continue;
                }
            }
        }
        if (board[nx][ny] == 3) {
            const auto& new_gen = state.board[nx][ny].generals;
            if (new_gen.type == 1)
                value[i] += 1e12;
            else if (state.board[nx][ny].army < 20) {
                value[i] += 3e6;
                continue;
            }
        } else if (board[nx][ny] == 1) {
            value[i] += 50;
        } else if (board[nx][ny] == 5) {
            value[i] += 3e6;
        } else if (board[nx][ny] == 6) {
            value[i] += 1e6;
        } else if (board[nx][ny] == 7) {
            value[i] += 2e6;
        } else if (board[nx][ny] == -1) {
            value[i] += 100;
        }

        if (launch_attack[i]) {  // attack the target, not move
            float my_att = cell_attack(gen_x, gen_y, my, state);
            float target_def = cell_defence(nx, ny, !my, state);
            int target_army = state.board[nx][ny].army;
            if (std::find(primary_dirs.begin(), primary_dirs.end(), i) != primary_dirs.end() &&
                state.coin[my] >= Constant::breakthrough && gen.skills_cd[(int)SkillType::ROUT] <= 0) {
                target_army = std::max(0, target_army - 20);
            }
            int minimum_army_required = target_army * target_def / my_att + 1;
            int left_army = now_army - minimum_army_required;
            if (left_army < 1) {
                value[i] = -1e15;
                continue;
            }
            threat_val_prepare(state);
            army_board[gen_x][gen_y] = left_army;
            army_board[nx][ny] = 1;
            player_board[nx][ny] = my;
            threat_vals[i][0] = compute_threat_val(gen_x, gen_y, my, state);
            threat_vals[i][1] = compute_threat_val(nx, ny, my, state);
            float my_battleval = cell_defence(gen_x, gen_y, my, state) * left_army - threat_vals[i][0];
            if (final_check) {
                GameState ns = state;
                auto op = move_army_op({gen_x, gen_y}, static_cast<Direction>(i), minimum_army_required);
                execute(ns, op, my);
                if (my == 1) ns.update_round();
                auto kill_ops = new_kill(ns, !my);
                if (kill_ops.size() > 0) {
                    value[i] -= 1e12;
                }
            }
            // for (int k = 0; k < 4; k++) {
            //     int tx = nx + dx[k], ty = ny + dy[k];
            //     if (tx < 0 || tx >= 15 || ty < 0 || ty >= 15) continue;
            //     if (board[tx][ty] == 8 || board[tx][ty] == 9 || board[tx][ty] == 10) continue;
            //     float future_battleval =
            //         cell_defence(gen_x, gen_y, my, state) * left_army - compute_threat_val(tx, ty, my, state);
            //     if (future_battleval < 0) {
            //         value[i] += future_battleval * 10000;
            //     }
            // }
            if (my_battleval < 1) {
                value[i] += (my_battleval - 1) * 1e6;
            } else {
                value[i] += my_battleval * 10;
            }
        } else {  // move to the target
            float my_battleval = cell_defence(gen_x, gen_y, my, state) * (now_army - 1 + state.board[nx][ny].army) -
                                 threat_value[nx][ny];
            float my_impact = impact_value[nx][ny];
            if (final_check) {
                GameState ns = state;
                auto op = move_army_op({gen_x, gen_y}, static_cast<Direction>(i), now_army - 1);
                execute(ns, op, my);
                op = move_generals_op(gen.id, {nx, ny});
                execute(ns, op, my);
                if (my == 1) ns.update_round();
                auto kill_ops = new_kill(ns, !my);
                if (kill_ops.size() > 0) {
                    value[i] -= 1e12;
                }
            }
            // for (int k = 0; k < 4; k++) {
            //     int tx = nx + dx[k], ty = ny + dy[k];
            //     if (tx < 0 || tx >= 15 || ty < 0 || ty >= 15) continue;
            //     if (board[tx][ty] == 8 || board[tx][ty] == 9 || board[tx][ty] == 10) continue;
            //     float future_battleval =
            //         cell_defence(gen_x, gen_y, my, state) * (now_army - 1 + state.board[nx][ny].army) -
            //         compute_threat_val(tx, ty, my, state);
            //     if (future_battleval < 0) {
            //         value[i] += future_battleval * 10000;
            //     }
            // }
            if (my_battleval < 1) {
                value[i] += (my_battleval - 1) * 1e6 + my_impact * 100;
            } else
                value[i] += my_battleval * 10 + my_impact * 2;
        }
    }

    float now_battleval = cell_defence(gen_x, gen_y, my, state) * now_army - threat_value[gen_x][gen_y];
    float now_impact = impact_value[gen_x][gen_y];
    if (now_battleval < 1)
        value[4] = (now_battleval - 1) * 1e6 + now_impact * 100;
    else
        value[4] = now_battleval * 10 + now_impact * 2;
    if (final_check) {
        GameState ns = state;
        if (my == 1) ns.update_round();
        auto kill_ops = new_kill(ns, !my);
        if (kill_ops.size() > 0) {
            value[4] -= 1e12;
        }
    }
    if (board[gen_x][gen_y] == 9)
        value[4] -= 15000;
    else if (board[gen_x][gen_y] == 10)
        value[4] -= 100000;
    for (int i = 0; i < 5; i++) {
        value[i] += rand() % 4;
    }

    float max_value = -1e15;
    for (int i = 0; i < 5; i++) {
        if (value[i] > max_value) {
            max_value = value[i];
            move_dir = i;
        }
    }
    if (max_value < -1e6) {  // in danger
        for (int i = 0; i < 4; i++) {
            value[i] -= enemy_impact[gen_x + dx[i]][gen_y + dy[i]] * 1e8;
            if (board[gen_x + dx[i]][gen_y + dy[i]] == 9)
                value[i] -= 1e9;
            else if (board[gen_x + dx[i]][gen_y + dy[i]] == 10)
                value[i] -= 1e10;
        }
        value[4] -= enemy_impact[gen_x][gen_y] * 1e8;
        if (board[gen_x][gen_y] == 9)
            value[4] -= 1e9;
        else if (board[gen_x][gen_y] == 10)
            value[4] -= 1e10;
        memset(block_list, 0, sizeof(block_list));
        block_list[3] = block_list[4] = block_list[5] = block_list[6] = block_list[7] = block_list[8] = block_list[10] =
            true;
        auto reachable_pos =
            reachable_positions(state, gen_x, gen_y, std::min(state.rest_move_step[my], gen.rest_move));
        int target_x = -1, target_y = -1;
        float max_val = -1e15;
        for (const auto& pos : reachable_pos) {
            int nx = pos.first, ny = pos.second;
            float val = -threat_value[nx][ny] * 1e4;
            if (board[nx][ny] == 0)
                val += state.board[nx][ny].army * 1e4;
            else if (board[nx][ny] == 1)
                val -= state.board[nx][ny].army * 1e4;
            else if (board[nx][ny] == 9 || board[nx][ny] == 10)
                val -= 1e5;
            val -= enemy_impact[nx][ny] * 1e2;
            if (val > max_val) {
                max_val = val;
                target_x = nx;
                target_y = ny;
            }
        }
        if (target_x != -1 && target_y != -1) {
            auto dir_dist = shortest_path_dirs(state, gen_x, gen_y, target_x, target_y);
            if (dir_dist.first.size() > 0) {
                for (int dir : dir_dist.first) {
                    value[dir] += 1e9;
                }
            }
        }

        max_value = -1e15;
        for (int i = 0; i < 5; i++) {
            if (value[i] > max_value) {
                max_value = value[i];
                move_dir = i;
            }
        }
    }
    if (cerr_cnt < 1500) {
        cerr_cnt++;
        std::cerr << value[0] << " " << value[1] << " " << value[2] << " " << value[3] << " " << value[4] << std::endl;
    }

    if (max_value > 0 && max_value < 1e6 && value[4] > 0 &&
        find(primary_dirs.begin(), primary_dirs.end(), move_dir) == primary_dirs.end())
        return ops;
    if (move_dir == 4 || move_dir == -1) return ops;

    if (!launch_attack[move_dir]) {
        auto op = move_army_op(std::make_pair(gen_x, gen_y), static_cast<Direction>(move_dir), now_army - 1);
        ops.push_back(op);
        execute(state, op, my);
        int target_x = gen_x + dx[move_dir], target_y = gen_y + dy[move_dir];
        op = move_generals_op(gen_id, {target_x, target_y});
        ops.push_back(op);
        execute(state, op, my);
    } else {
        int target_x = gen_x + dx[move_dir], target_y = gen_y + dy[move_dir];
        auto my_att = cell_attack(gen_x, gen_y, my, state);
        auto target_def = cell_defence(target_x, target_y, !my, state);
        int minimum_army_required = state.board[target_x][target_y].army * target_def / my_att + 1;
        if (state.board[target_x][target_y].generals.type != 0 &&
            state.board[target_x][target_y].generals.player == -1 &&
            state.board[target_x][target_y].generals.type == 2 && gen.skills_cd[(int)SkillType::ROUT] <= 0 &&
            state.coin[my] >= Constant::breakthrough) {
            auto op = generals_skill_op(gen_id, SkillType::ROUT, {target_x, target_y});
            ops.push_back(op);
            execute(state, op, my);
            minimum_army_required = 1;
        }
        if (state.board[target_x][target_y].generals.type != 0) {  // decide whether to move enough army to defend
            int move_army = minimum_army_required;
            auto op = move_army_op(std::make_pair(gen_x, gen_y), static_cast<Direction>(move_dir), move_army);
            ops.push_back(op);
            execute(state, op, my);
        } else {  // decide whether to move the general along with the army
            int move_army = minimum_army_required;
            bool move_along = false;
            float target_def = cell_defence(target_x, target_y, my, state) * gen.defence_level;
            int target_safe_army = threat_value[target_x][target_y] / target_def + 1;
            if (target_safe_army + minimum_army_required - 1 <= now_army) {
                move_army = now_army - 1;
                move_along = true;
            }
            auto op = move_army_op(std::make_pair(gen_x, gen_y), static_cast<Direction>(move_dir), move_army);
            ops.push_back(op);
            execute(state, op, my);
            if (move_along) {
                op = move_generals_op(gen_id, {target_x, target_y});
                ops.push_back(op);
                execute(state, op, my);
            }
        }
    }

    return ops;
}

std::vector<Operation> move_gen(GameState& state, int gen_id) {
    if (gameend(state, my)) return std::vector<Operation>();
    std::vector<Operation> ops;
    const auto& gen = find_general_by_id(state, gen_id);
    const int gen_x = gen.position[0], gen_y = gen.position[1];
    const int now_army = state.board[gen_x][gen_y].army;
    float value[5];
    memset(value, 0, sizeof(value));
    for (int i = 0; i < 4; i++) {
        int nx = gen_x + dx[i], ny = gen_y + dy[i];
        if (nx < 0 || nx >= 15 || ny < 0 || ny >= 15) {
            value[i] = -1e15;
            continue;
        }
        if (!(board[nx][ny] == 0 || board[nx][ny] == 9)) {
            value[i] = -1e15;
            continue;
        }
        if (board[nx][ny] == 9) {
            if (state.board[nx][ny].player != my || state.board[nx][ny].generals.type != 0) {
                value[i] = -1e15;
                continue;
            }
            value[i] -= 15000;
        }
        float my_battleval =
            cell_defence(gen_x, gen_y, my, state) * (now_army - 1 + state.board[nx][ny].army) - threat_value[nx][ny];
        float my_impact = impact_value[nx][ny];
        if (my_battleval < 1)
            value[i] += (my_battleval - 1) * 1e6 + my_impact * 100;
        else
            value[i] += my_battleval * 10 + my_impact * 2;
        for (int di = -1; di <= 1; di++) {
            for (int dj = -1; dj <= 1; dj++) {
                if (di == 0 && dj == 0) continue;
                int tx = nx + di, ty = ny + dj;
                if (tx < 0 || tx >= 15 || ty < 0 || ty >= 15) continue;
                if (state.board[tx][ty].player == my && state.board[tx][ty].generals.type != 0) {
                    value[i] -= 200;
                }
            }
        }
    }

    float now_battleval = cell_defence(gen_x, gen_y, my, state) * now_army - threat_value[gen_x][gen_y];
    float now_impact = impact_value[gen_x][gen_y];
    if (now_battleval < 1)
        value[4] = (now_battleval - 1) * 1e6 + now_impact * 100;
    else
        value[4] = now_battleval * 10 + now_impact * 2;
    if (board[gen_x][gen_y] == 9) value[4] -= 15000;
    if (board[gen_x][gen_y] == 10) value[4] -= 100000;
    for (int di = -1; di <= 1; di++) {
        for (int dj = -1; dj <= 1; dj++) {
            if (di == 0 && dj == 0) continue;
            int tx = gen_x + di, ty = gen_y + dj;
            if (tx < 0 || tx >= 15 || ty < 0 || ty >= 15) continue;
            if (state.board[tx][ty].player == my && state.board[tx][ty].generals.type != 0) {
                value[4] -= 200;
            }
        }
    }
    for (int i = 0; i < 5; i++) {
        value[i] += rand() % 4;
    }

    float max_value = -1e15;
    int move_dir = -1;
    for (int i = 0; i < 5; i++) {
        if (value[i] > max_value) {
            max_value = value[i];
            move_dir = i;
        }
    }
    if (max_value < -1e6) {
        for (int i = 0; i < 4; i++) {
            value[i] -= enemy_impact[gen_x + dx[i]][gen_y + dy[i]] * 1e8;
            if (board[gen_x + dx[i]][gen_y + dy[i]] == 9) value[i] -= 2e8;
            if (board[gen_x + dx[i]][gen_y + dy[i]] == 10) value[i] -= 1e10;
        }
        value[4] -= enemy_impact[gen_x][gen_y] * 1e8;
        if (board[gen_x][gen_y] == 9) value[4] -= 2e8;
        if (board[gen_x][gen_y] == 10) value[4] -= 1e10;

        max_value = -1e15;
        for (int i = 0; i < 5; i++) {
            if (value[i] > max_value) {
                max_value = value[i];
                move_dir = i;
            }
        }
    }

    if (move_dir == 4 || move_dir == -1) return ops;

    auto op = move_generals_op(gen_id, {gen_x + dx[move_dir], gen_y + dy[move_dir]});
    ops.push_back(op);
    execute(state, op, my);

    return ops;
}

std::vector<Operation> mobile_army(GameState& state) {
    if (gameend(state, my)) return std::vector<Operation>();
    std::vector<Operation> ops;

    const auto& my_maingen = find_general_by_id(state, main_general_id(state, my));
    int my_maingen_x = my_maingen.position[0], my_maingen_y = my_maingen.position[1];
    int target_x = -1, target_y = -1;
    float max_val = -100000000;
    for (const auto& gen : state.get_generals()) {
        int gen_x = gen.position[0], gen_y = gen.position[1];
        float val = -abs(gen_x - my_maingen_x) - abs(gen_y - my_maingen_y);
        int nearest_my_dist = 100;
        if (gen.player != my) {
            int dis[15][15];
            for (int i = 0; i < 15; i++) {
                for (int j = 0; j < 15; j++) {
                    dis[i][j] = 100;
                }
            }
            std::queue<std::pair<int, int>> q;
            q.push({gen_x, gen_y});
            dis[gen_x][gen_y] = 0;
            while (!q.empty()) {
                auto cur = q.front();
                q.pop();
                int x = cur.first, y = cur.second;
                if (state.board[x][y].player == my) {
                    nearest_my_dist = dis[x][y];
                    break;
                }
                for (int i = 0; i < 4; i++) {
                    int nx = x + dx[i], ny = y + dy[i];
                    if (nx < 0 || nx >= 15 || ny < 0 || ny >= 15) continue;
                    if (state.board[nx][ny].type == CellType::SWAMP &&
                        state.tech_level[my][int(TechType::IMMUNE_SWAMP)] == 0)
                        continue;
                    if (state.board[nx][ny].type == CellType::SAND &&
                        state.tech_level[my][int(TechType::IMMUNE_SAND)] == 0)
                        continue;
                    if (state.board[nx][ny].generals.type != 0 && state.board[nx][ny].generals.player != my) continue;
                    if (dis[nx][ny] > dis[x][y] + 1) {
                        dis[nx][ny] = dis[x][y] + 1;
                        q.push({nx, ny});
                    }
                }
            }
        }
        if (gen.type == 3) {
            val -= state.board[gen_x][gen_y].army;
            val += gen.produce_level * 50 + state.round * 2;
            if (gen.player != my) {
                val += std::max(0, 100 - nearest_my_dist * 20);
            }
            // val -= nearest_my_army_dist[gen_x][gen_y] * 20;
        } else if (gen.type == 2) {
            val -= state.board[gen_x][gen_y].army;
            val += gen.produce_level * 100 + state.round * 2;
            if (gen.player != my) {
                val += std::max(0, 100 - nearest_my_dist * 20);
            }
            // val -= nearest_my_army_dist[gen_x][gen_y] * 20;
        } else if (gen.type == 1) {
            val += 350;
            if (gen.player != my) {
                continue;
            }
            // val -= nearest_my_army_dist[gen_x][gen_y] * 20;
        }
        if (val > max_val) {
            max_val = val;
            target_x = gen_x;
            target_y = gen_y;
        }
    }

    if (target_x == -1 || target_y == -1) return ops;

    int target_type = state.board[target_x][target_y].generals.type;
    int dist_to_target[15][15];
    for (int i = 0; i < 15; i++) {
        for (int j = 0; j < 15; j++) {
            dist_to_target[i][j] = 100;
        }
    }
    dist_to_target[target_x][target_y] = 0;
    std::queue<std::pair<int, int>> q;
    q.push({target_x, target_y});
    int min_dist_to_target = 100;
    while (!q.empty()) {
        auto& cur = q.front();
        q.pop();
        int x = cur.first, y = cur.second;
        if (state.board[x][y].player == my) {
            min_dist_to_target = std::min(min_dist_to_target, dist_to_target[x][y]);
        }
        for (int i = 0; i < 4; i++) {
            int nx = x + dx[i], ny = y + dy[i];
            if (nx < 0 || nx >= 15 || ny < 0 || ny >= 15) continue;
            if (state.tech_level[my][int(TechType::IMMUNE_SWAMP)] == 0 && state.board[nx][ny].type == CellType::SWAMP)
                continue;
            if (state.tech_level[my][int(TechType::IMMUNE_SAND)] == 0 && state.board[nx][ny].type == CellType::SAND)
                continue;
            if (state.board[nx][ny].generals.type != 0 && state.board[nx][ny].generals.player != my) continue;
            if (dist_to_target[nx][ny] > dist_to_target[x][y] + 1) {
                dist_to_target[nx][ny] = dist_to_target[x][y] + 1;
                q.push({nx, ny});
            }
        }
    }

    int best_x = -1, best_y = -1;
    max_val = -100000000;
    for (int i = 0; i < 15; i++) {
        for (int j = 0; j < 15; j++) {
            if (i == target_x && j == target_y) continue;
            if (state.board[i][j].player == my && state.board[i][j].army > 1) {
                if (state.board[i][j].generals.type == 1) continue;
                if (state.board[i][j].generals.type == 2 && state.board[i][j].army < 10) continue;
                if (state.board[i][j].generals.type == 3) {
                    int dang_val = 0;
                    for (int k = 0; k < 4; k++) {
                        int nx = i + dx[k], ny = j + dy[k];
                        if (nx < 0 || nx >= 15 || ny < 0 || ny >= 15) continue;
                        if (state.board[nx][ny].player == !my) {
                            dang_val += std::max(0, state.board[nx][ny].army - 1);
                        }
                    }
                    if (dang_val >= state.board[i][j].army) continue;
                }
                // bool valid = false;
                // for (int k = 0; k < 4; k++) {
                //     int nx = i + dx[k], ny = j + dy[k];
                //     if (nx < 0 || nx >= 15 || ny < 0 || ny >= 15) continue;
                //     if (state.board[nx][ny].player == my) {
                //         valid = true;
                //     }
                // }
                // if (!valid) continue;
                int dist = dist_to_target[i][j];
                if (dist == 100) continue;
                int army = state.board[i][j].army;
                float val = dist * 2 + army;
                if (target_type != 1 && dist > min_dist_to_target + 5) continue;

                if (val > max_val) {
                    max_val = val;
                    best_x = i;
                    best_y = j;
                }
            }
        }
    }

    if (best_x == -1 || best_y == -1) return ops;

    auto mobile_path = best_mobile_path(state, best_x, best_y, target_x, target_y, my, false, true);
    if (mobile_path.size() == 0) return ops;
    int move_dir = mobile_path[0];
    int move_army = state.board[best_x][best_y].army - 1;
    if (state.board[best_x][best_y].generals.type == 3 || state.board[best_x][best_y].generals.type == 2) {
        int dang_val = 0;
        for (int k = 0; k < 4; k++) {
            int nx = best_x + dx[k], ny = best_y + dy[k];
            if (nx < 0 || nx >= 15 || ny < 0 || ny >= 15) continue;
            if (state.board[nx][ny].player == !my) {
                dang_val += std::max(0, state.board[nx][ny].army - 1);
            }
        }
        move_army = std::max(0, state.board[best_x][best_y].army - dang_val);
    }
    if (move_army <= 0) return ops;
    auto op = move_army_op({best_x, best_y}, static_cast<Direction>(move_dir), move_army);
    ops.push_back(op);
    execute(state, op, my);
    // std::cerr << "mobile " << best_x << " " << best_y << " " << move_dir << " " << move_army << std::endl;
    return ops;
}

std::vector<Operation> explore(GameState& state, bool urgent) {
    if (gameend(state, my)) return std::vector<Operation>();
    std::vector<Operation> ops;
    int pos_x = -1, pos_y = -1, move_dir = -1, move_army = -1;
    float max_val = -100000000;

    int connected_component_cnt[15][15];
    int nearest_oil_dist[15][15];
    memset(block_list, 1, sizeof(block_list));
    memset(connected_component_cnt, 0, sizeof(connected_component_cnt));
    for (int i = 0; i < 15; i++) {
        for (int j = 0; j < 15; j++) {
            nearest_oil_dist[i][j] = 1000000;
        }
    }
    for (int i = 0; i < 15; i++) {
        for (int j = 0; j < 15; j++) {
            if (board[i][j] == -1 && connected_component_cnt[i][j] == 0) {
                int cnt = find_connected_component(state, i, j);
                for (int k = 0; k < 15; k++) {
                    for (int l = 0; l < 15; l++) {
                        if (connected_component_res[k][l]) {
                            connected_component_cnt[k][l] = cnt;
                        }
                    }
                }
            }
            for (const auto& gen : state.get_generals()) {
                if (gen.type == 3) {
                    int dist = abs(gen.position[0] - i) + abs(gen.position[1] - j);
                    nearest_oil_dist[i][j] = std::min(nearest_oil_dist[i][j], dist);
                }
            }
        }
    }

    int my_maingen_id = main_general_id(state, my);
    const auto& my_maingen = find_general_by_id(state, my_maingen_id);
    int enemy_maingen_id = main_general_id(state, !my);
    const auto& enemy_maingen = find_general_by_id(state, enemy_maingen_id);
    const int my_maingen_x = my_maingen.position[0], my_maingen_y = my_maingen.position[1];
    const int enemy_maingen_x = enemy_maingen.position[0], enemy_maingen_y = enemy_maingen.position[1];
    int my_army = state.board[my_maingen.position[0]][my_maingen.position[1]].army;
    int enemy_army = state.board[enemy_maingen.position[0]][enemy_maingen.position[1]].army;
    for (int i = 0; i < 15; i++) {
        for (int j = 0; j < 15; j++) {
            if (state.board[i][j].army <= 1) continue;
            const int now_army = state.board[i][j].army;
            if (board[i][j] == 0) {
                int best_dir = -1, best_army = -1;
                float best_val = -50000;
                for (int k = 0; k < 4; k++) {
                    int now_move_army;
                    float val = 0;
                    int nx = i + dx[k], ny = j + dy[k];
                    if (nx < 0 || nx >= 15 || ny < 0 || ny >= 15) continue;
                    if (board[nx][ny] == 0 || board[nx][ny] == 2 || board[nx][ny] == 4 || board[nx][ny] == 8 ||
                        board[nx][ny] == 9 || board[nx][ny] == 10)
                        continue;
                    if (board[nx][ny] == 1 || board[nx][ny] == 3 || board[nx][ny] == 5 || board[nx][ny] == 6 ||
                        board[nx][ny] == 7) {
                        float my_att = cell_attack(i, j, my, state);
                        float target_def = cell_defence(nx, ny, !my, state);
                        int minimum_army_required = state.board[nx][ny].army * target_def / my_att + 1;
                        int left_army = state.board[i][j].army - minimum_army_required;
                        if (left_army < 1) continue;
                        if (board[nx][ny] == 3) {
                            const auto& gen = state.board[nx][ny].generals;
                            if (gen.type == 1) {
                                val += 10000000;
                            } else {
                                val -= 1000;
                                continue;
                            }
                        } else if (board[nx][ny] == 1) {
                            val += 50;
                        } else if (board[nx][ny] == 5 || board[nx][ny] == 7) {
                            val += 10000;
                        } else if (board[nx][ny] == 6) {
                            val -= 1000;
                        }
                        val += left_army * 10;
                        now_move_army = now_army - 1;
                    } else if (board[nx][ny] == -1) {
                        val += 100;
                        now_move_army = now_army - 1;
                        val += connected_component_cnt[nx][ny] * 2;
                    }
                    val -= nearest_oil_dist[nx][ny] * 100;
                    val += rand() % 5;
                    if (val > best_val) {
                        best_val = val;
                        best_dir = k;
                        best_army = now_move_army;
                    }
                }
                if (best_dir != -1) {
                    if (best_val > max_val) {
                        max_val = best_val;
                        pos_x = i;
                        pos_y = j;
                        move_dir = best_dir;
                        move_army = best_army;
                    }
                }
            } else if (board[i][j] == 2 || board[i][j] == 4) {
                if (state.board[i][j].generals.type == 1) continue;
                int best_dir = -1, best_army = -1;
                float best_val = -50000;
                for (int k = 0; k < 4; k++) {
                    int now_move_army;
                    float val = 0;
                    int nx = i + dx[k], ny = j + dy[k];
                    if (nx < 0 || nx >= 15 || ny < 0 || ny >= 15) continue;
                    if (board[nx][ny] == 0 || board[nx][ny] == 2 || board[nx][ny] == 4 || board[nx][ny] == 8 ||
                        board[nx][ny] == 9 || board[nx][ny] == 10)
                        continue;
                    if (board[nx][ny] == 1 || board[nx][ny] == 3 || board[nx][ny] == 5 || board[nx][ny] == 6 ||
                        board[nx][ny] == 7) {
                        float my_att = cell_attack(i, j, my, state);
                        float target_def = cell_defence(nx, ny, !my, state);
                        int minimum_army_required = state.board[nx][ny].army * target_def / my_att + 1;
                        int left_army = state.board[i][j].army - minimum_army_required;
                        if (left_army < 1) continue;
                        if (board[nx][ny] == 3) {
                            const auto& gen = state.board[nx][ny].generals;
                            if (gen.type == 1) {
                                val += 10000000;
                            } else {
                                val -= 1000;
                                continue;
                            }
                        } else if (board[nx][ny] == 1) {
                            val += 50;
                        } else if (board[nx][ny] == 5 || board[nx][ny] == 7) {
                            val += 10000;
                        } else if (board[nx][ny] == 6) {
                            val -= 1000;
                        }
                        threat_val_prepare(state);
                        army_board[i][j] = left_army;
                        army_board[nx][ny] = 1;
                        player_board[nx][ny] = my;
                        float my_battleval =
                            cell_defence(i, j, my, state) * left_army - compute_threat_val(i, j, my, state);
                        if (my_battleval < 1)
                            val += (my_battleval - 1) * 1e6;
                        else
                            val += my_battleval * 10;
                        now_move_army = minimum_army_required;
                    } else if (board[nx][ny] == -1) {
                        val += 100;
                        now_move_army = std::min(now_army - 1, 2);
                        int left_army = now_army - now_move_army;
                        float my_battleval = cell_defence(i, j, my, state) * left_army - threat_value[i][j];
                        if (my_battleval < 1)
                            val += (my_battleval - 1) * 1e6;
                        else
                            val += my_battleval * 10;
                        val += connected_component_cnt[nx][ny] * 2;
                    }
                    val += rand() % 5;
                    if (val > best_val) {
                        best_val = val;
                        best_dir = k;
                        best_army = now_move_army;
                    }
                }
                int jumpdist = std::max(std::max(abs(my_maingen_x - enemy_maingen_x) - 2, 0) +
                                            std::max(abs(my_maingen_y - enemy_maingen_y) - 2, 0),
                                        1);
                if ((my_army - 1) * my_maingen.defence_level - enemy_army * 1.5 <= 0 && jumpdist <= 3) continue;
                if (best_val < 5000) continue;
                if (best_dir != -1) {
                    if (best_val > max_val) {
                        max_val = best_val;
                        pos_x = i;
                        pos_y = j;
                        move_dir = best_dir;
                        move_army = best_army;
                    }
                }
            }
        }
    }
    if (urgent && max_val < 5000) return ops;

    if (pos_x != -1 && pos_y != -1) {
        auto op = move_army_op({pos_x, pos_y}, static_cast<Direction>(move_dir), move_army);
        ops.push_back(op);
        execute(state, op, my);
    }

    return ops;
}

std::vector<Operation> action(GameState& state) {
    std::vector<Operation> ops;
    bool should_update = true;
    for (int attempt = 0; attempt < 10; attempt++) {
        if (gameend(state, my)) return ops;
        if (should_update) {
            get_board(state);
            should_update = false;
        }
        bool should_break = true;
        if (state.rest_move_step[my] == 0) {
            for (const auto& general : state.get_generals()) {
                if (general.player == my) {
                    if (general.type == 1 || general.type == 2) {
                        if (general.skill_duration[0] > 0) {
                            should_break = false;
                            break;
                        }
                    }
                }
            }
        } else {
            should_break = false;
        }
        if (should_break) break;

        int my_maingen_id = main_general_id(state, my);
        const auto& my_maingen = find_general_by_id(state, my_maingen_id);
        int my_maingen_x = my_maingen.position[0], my_maingen_y = my_maingen.position[1];
        float maingen_battleval =
            state.board[my_maingen_x][my_maingen_y].army * cell_defence(my_maingen_x, my_maingen_y, my, state) -
            threat_value[my_maingen_x][my_maingen_y];
        bool maingen_safe = maingen_battleval >= 1;

        std::vector<std::pair<int, float>> subgen_id_and_val;
        for (const auto& general : state.get_generals()) {
            if (general.type == 2 && general.player == my && general.rest_move > 0) {
                subgen_id_and_val.push_back({general.id, 0});
            }
        }
        for (auto& subgen_id_and_val : subgen_id_and_val) {
            int subgen_id = subgen_id_and_val.first;
            const auto& subgen = find_general_by_id(state, subgen_id);
            float subgen_battleval = state.board[subgen.position[0]][subgen.position[1]].army *
                                         cell_defence(subgen.position[0], subgen.position[1], my, state) -
                                     threat_value[subgen.position[0]][subgen.position[1]];
            bool subgen_safe = subgen_battleval >= 1;
            float val = state.board[subgen.position[0]][subgen.position[1]].army;
            val += subgen_battleval * 5;
            if (!subgen_safe) {
                val += 10000;
            }
            subgen_id_and_val.second = val;
        }

        std::sort(subgen_id_and_val.begin(), subgen_id_and_val.end(),
                  [](const std::pair<int, float>& a, const std::pair<int, float>& b) { return a.second > b.second; });

        if (state.rest_move_step[my] > 0 && my_maingen.rest_move > 0 && !lock_move) {
            auto new_ops = move_gen_and_army(state, my_maingen_id);
            if (new_ops.size() > 0) {
                ops.insert(ops.end(), new_ops.begin(), new_ops.end());
                should_update = true;
                continue;
            }
        }
        for (const auto& subgen_id_and_val : subgen_id_and_val) {
            // if (subgen_id_and_val.second < 5000) break;
            int subgen_id = subgen_id_and_val.first;
            const auto& subgen = find_general_by_id(state, subgen_id);
            if (state.rest_move_step[my] > 0 && subgen.rest_move > 0) {
                auto new_ops = move_gen_and_army(state, subgen_id);
                if (new_ops.size() > 0) {
                    ops.insert(ops.end(), new_ops.begin(), new_ops.end());
                    should_update = true;
                    break;
                }
            }
        }
        if (should_update) continue;
        if (state.rest_move_step[my] > 0) {
            auto new_ops = explore(state, true);
            if (new_ops.size() > 0) {
                ops.insert(ops.end(), new_ops.begin(), new_ops.end());
                should_update = true;
                continue;
            }
        }
        if (state.rest_move_step[my] > 0) {
            auto new_ops = mobile_army(state);
            if (new_ops.size() > 0) {
                ops.insert(ops.end(), new_ops.begin(), new_ops.end());
                should_update = true;
                continue;
            }
        }
        if (state.rest_move_step[my] > 0) {
            auto new_ops = explore(state, false);
            if (new_ops.size() > 0) {
                ops.insert(ops.end(), new_ops.begin(), new_ops.end());
                should_update = true;
                continue;
            }
        }
        // for (const auto& subgen_id_and_val : subgen_id_and_val) {
        //     int subgen_id = subgen_id_and_val.first;
        //     const auto& subgen = find_general_by_id(state, subgen_id);
        //     if (state.rest_move_step[my] > 0 && subgen.rest_move > 0) {
        //         auto new_ops = move_gen_and_army(state, subgen_id);
        //         if (new_ops.size() > 0) {
        //             ops.insert(ops.end(), new_ops.begin(), new_ops.end());
        //             should_update = true;
        //             break;
        //         }
        //     } else if (subgen.rest_move > 0) {
        //         auto new_ops = move_gen(state, subgen_id);
        //         if (new_ops.size() > 0) {
        //             ops.insert(ops.end(), new_ops.begin(), new_ops.end());
        //             should_update = true;
        //             break;
        //         }
        //     }
        // }
        if (should_update) continue;
        if (state.rest_move_step[my] > 0) {
            auto new_ops = support_maingen(state);
            if (new_ops.size() > 0) {
                ops.insert(ops.end(), new_ops.begin(), new_ops.end());
                should_update = true;
                continue;
            }
        }
    }
    return ops;
}

std::pair<int, int> find_best_oil(GameState& state, bool player) {
    const auto& my_maingen = find_general_by_id(state, main_general_id(state, player));
    const int my_maingen_x = my_maingen.position[0], my_maingen_y = my_maingen.position[1];
    const auto& enemy_maingen = find_general_by_id(state, main_general_id(state, !player));
    const int enemy_maingen_x = enemy_maingen.position[0], enemy_maingen_y = enemy_maingen.position[1];
    const auto& gen_list = state.get_generals();

    int max_val = 0, best_x = -1, best_y = -1;
    for (auto& gen : gen_list) {
        if (gen.type == 3 && gen.player == player) {
            int val = 0, coin_required = 0;
            if (gen.produce_level == 1) {
                val += 1000;
                coin_required = Constant::OilWell_production_T1;
            } else if (gen.produce_level == 2) {
                val += 200;
                coin_required = Constant::OilWell_production_T2;
            } else if (gen.produce_level >= 4) {
                // val -= 3000;
                // coin_required = Constant::OilWell_production_T3;
                continue;
            }
            if (state.coin[player] < coin_required) continue;
            int dist = shortest_path_dist(state, my_maingen_x, my_maingen_y, gen.position[0], gen.position[1], player);
            int enemy_dist =
                shortest_path_dist(state, enemy_maingen_x, enemy_maingen_y, gen.position[0], gen.position[1], !player);
            val += ((enemy_dist * 1.5) * (enemy_dist * 1.5) - dist * dist) * 50;
            if (val > max_val) {
                max_val = val;
                best_x = gen.position[0];
                best_y = gen.position[1];
            }
            val += state.board[gen.position[0]][gen.position[1]].army * 10;
        }
    }
    return {best_x, best_y};
}

std::pair<std::vector<Operation>, bool> sim_operation(GameState& state, bool get_oil, bool long_oil, bool inst_tech,
                                                      bool player, bool only_escape = false, bool judge = true) {
    std::vector<Operation> ops;
    const auto& my_maingen = find_general_by_id(state, main_general_id(state, player));
    const int my_maingen_x = my_maingen.position[0], my_maingen_y = my_maingen.position[1];
    const auto& enemy_maingen = find_general_by_id(state, main_general_id(state, !player));
    const int enemy_maingen_x = enemy_maingen.position[0], enemy_maingen_y = enemy_maingen.position[1];
    const int jump_dist = std::max(
        std::max(abs(my_maingen_x - enemy_maingen_x) - 2, 0) + std::max(abs(my_maingen_y - enemy_maingen_y) - 2, 0), 1);
    const int movement = std::min(state.tech_level[player][0], my_maingen.rest_move);

    if (!only_escape) {
        auto kill_ops = new_kill(state, player);
        if (kill_ops.size() > 0) {
            for (auto& op : kill_ops) {
                execute(state, op, player);
                ops.push_back(op);
            }
            return std::make_pair(ops, true);
        }
    }
    if (state.coin[player] >= Constant::breakthrough &&
        state.board[my_maingen_x][my_maingen_y].generals.skills_cd[1] <= 0) {
        int enemy_army = state.board[enemy_maingen_x][enemy_maingen_y].army;
        if (abs(my_maingen_x - enemy_maingen_x) <= 2 && abs(my_maingen_y - enemy_maingen_y) <= 2) {
            if (enemy_army >= 20 || inst_tech) {
                auto op = generals_skill_op(my_maingen.id, SkillType::ROUT, {enemy_maingen_x, enemy_maingen_y});
                execute(state, op, player);
                ops.push_back(op);
            }
        }
    }

    if (only_escape) {  // escape
        auto path =
            best_escape_path(state, my_maingen_x, my_maingen_y, enemy_maingen_x, enemy_maingen_y, player, movement);
        int now_x = my_maingen_x, now_y = my_maingen_y;
        for (int i = 0; i < std::min((int)(path.size()), movement); i++) {
            int dir = path[i];
            int army_num = state.board[now_x][now_y].army;
            if (army_num > 1) {
                auto op = move_army_op(std::make_pair(now_x, now_y), static_cast<Direction>(dir), army_num - 1);
                bool exe_suc = execute(state, op, player);
                if (!exe_suc) break;
                ops.push_back(op);
            }
            now_x += dx[dir];
            now_y += dy[dir];
            auto op = move_generals_op(my_maingen.id, {now_x, now_y});
            bool exe_suc = execute(state, op, player);
            if (!exe_suc) break;
            ops.push_back(op);
        }
        return std::make_pair(ops, false);
    }

    if (get_oil) {
        auto oil_ops = try_get_oil(state, my_maingen_x, my_maingen_y, player);
        ops.insert(ops.end(), oil_ops.begin(), oil_ops.end());
        bool adjacent_oil = false;
        for (int i = 0; i < 4; i++) {
            int nx = my_maingen_x + dx[i], ny = my_maingen_y + dy[i];
            if (nx < 0 || nx >= 15 || ny < 0 || ny >= 15) continue;
            if (state.board[nx][ny].type == CellType::SWAMP &&
                state.tech_level[player][int(TechType::IMMUNE_SWAMP)] == 0)
                continue;
            if (state.board[nx][ny].generals.type == 3 && state.board[nx][ny].generals.player != player) {
                adjacent_oil = true;
                break;
            }
        }
        if (!adjacent_oil) {
            const auto& genlist = state.get_generals();
            int target_x = -1, target_y = -1, best_val = -1e9;
            int max_oil_dist = long_oil ? 8 : 3;
            for (const auto& gen : genlist) {
                if (gen.type == 3 && gen.player != player) {
                    int gen_x = gen.position[0], gen_y = gen.position[1];
                    if (state.board[gen_x][gen_y].type == CellType::SWAMP &&
                        state.tech_level[player][int(TechType::IMMUNE_SWAMP)] == 0)
                        continue;
                    int dist = shortest_path_dist(state, my_maingen_x, my_maingen_y, gen_x, gen_y, player);
                    if (dist >= 2 && dist <= max_oil_dist) {
                        int target_val = -dist * 100 - (abs(gen_x - enemy_maingen_x) + abs(gen_x - enemy_maingen_y));
                        if (target_val > best_val) {
                            best_val = target_val;
                            target_x = gen.position[0];
                            target_y = gen.position[1];
                        }
                    }
                }
            }
            if (target_x != -1) {
                auto path =
                    best_mobile_path(state, my_maingen_x, my_maingen_y, target_x, target_y, player, true, false);
                if (path.size() > 0) {
                    GameState ns = state;
                    std::vector<Operation> new_ops;
                    int now_x = my_maingen_x, now_y = my_maingen_y;
                    for (int i = 0; i < std::min((int)(path.size()), movement); i++) {
                        if (abs(now_x - target_x) + abs(now_y - target_y) == 1) break;
                        int move_dir = path[i];
                        int army_num = ns.board[now_x][now_y].army;
                        auto op =
                            move_army_op(std::make_pair(now_x, now_y), static_cast<Direction>(move_dir), army_num - 1);
                        bool exe_suc = execute(ns, op, player);
                        if (!exe_suc) break;
                        new_ops.push_back(op);
                        now_x += dx[move_dir];
                        now_y += dy[move_dir];
                        op = move_generals_op(my_maingen.id, {now_x, now_y});
                        exe_suc = execute(ns, op, player);
                        if (!exe_suc) break;
                        new_ops.push_back(op);
                        if (abs(now_x - target_x) + abs(now_y - target_y) == 1) break;
                    }
                    if (player == 1) ns.update_round();
                    auto enemy_kill_ops = new_kill(ns, !player);
                    if (enemy_kill_ops.size() == 0 || !judge) {  // safe, can proceed
                        for (auto& op : new_ops) {
                            execute(state, op, player);
                            ops.push_back(op);
                        }
                    }
                }
            }
        }
    }

    const auto& new_my_maingen = find_general_by_id(state, main_general_id(state, player));
    const int new_my_maingen_x = new_my_maingen.position[0], new_my_maingen_y = new_my_maingen.position[1];
    auto oil_ops = try_get_oil(state, new_my_maingen_x, new_my_maingen_y, player);
    ops.insert(ops.end(), oil_ops.begin(), oil_ops.end());
    bool adjacent_oil = false;
    for (int i = 0; i < 4; i++) {
        int nx = new_my_maingen_x + dx[i], ny = new_my_maingen_y + dy[i];
        if (nx < 0 || nx >= 15 || ny < 0 || ny >= 15) continue;
        if (state.board[nx][ny].type == CellType::SWAMP && state.tech_level[player][int(TechType::IMMUNE_SWAMP)] == 0)
            continue;
        if (state.board[nx][ny].generals.type == 3 && state.board[nx][ny].generals.player != player) {
            adjacent_oil = true;
            break;
        }
    }
    int current_movement = std::min(state.rest_move_step[player], new_my_maingen.rest_move);
    if (state.coin[player] >= Constant::breakthrough &&
        state.board[new_my_maingen_x][new_my_maingen_y].generals.skills_cd[1] <= 0) {
        int enemy_army = state.board[enemy_maingen_x][enemy_maingen_y].army;
        if (abs(new_my_maingen_x - enemy_maingen_x) <= 2 && abs(new_my_maingen_y - enemy_maingen_y) <= 2) {
            if (enemy_army >= 20 || inst_tech) {
                auto op = generals_skill_op(new_my_maingen.id, SkillType::ROUT, {enemy_maingen_x, enemy_maingen_y});
                execute(state, op, player);
                ops.push_back(op);
            }
        }
    }
    if (abs(new_my_maingen_x - enemy_maingen_x) + abs(new_my_maingen_y - enemy_maingen_y) > 1 && current_movement > 0 &&
        !(get_oil && adjacent_oil)) {  // try approaching
        GameState ns = state;
        auto path = best_attack_path(ns, new_my_maingen_x, new_my_maingen_y, enemy_maingen_x, enemy_maingen_y, player);
        int now_x = new_my_maingen_x, now_y = new_my_maingen_y;
        std::vector<Operation> new_ops;
        for (int i = 0; i < std::min((int)(path.size()), current_movement); i++) {
            if (abs(now_x - enemy_maingen_x) + abs(now_y - enemy_maingen_y) == 1) break;
            int dir = path[i];
            int army_num = ns.board[now_x][now_y].army;
            auto op = move_army_op(std::make_pair(now_x, now_y), static_cast<Direction>(dir), army_num - 1);
            bool exe_suc = execute(ns, op, player);
            if (!exe_suc) break;
            new_ops.push_back(op);
            now_x += dx[dir];
            now_y += dy[dir];
            op = move_generals_op(my_maingen.id, {now_x, now_y});
            exe_suc = execute(ns, op, player);
            if (!exe_suc) break;
            new_ops.push_back(op);
            if (ns.coin[player] >= Constant::breakthrough && ns.board[now_x][now_y].generals.skills_cd[1] <= 0) {
                int enemy_army = ns.board[enemy_maingen_x][enemy_maingen_y].army;
                if (abs(now_x - enemy_maingen_x) <= 2 && abs(now_y - enemy_maingen_y) <= 2 &&
                    (enemy_army >= 20 || inst_tech)) {
                    auto op = generals_skill_op(new_my_maingen.id, SkillType::ROUT, {enemy_maingen_x, enemy_maingen_y});
                    execute(ns, op, player);
                    new_ops.push_back(op);
                }
            }
        }

        if (player == 1) ns.update_round();
        auto enemy_kill_ops = new_kill(ns, !player);
        if (enemy_kill_ops.size() == 0 || !judge) {  // safe, can proceed
            for (auto& op : new_ops) {
                execute(state, op, player);
                ops.push_back(op);
            }
            return std::make_pair(ops, true);
        }
    }

    int current_status = 0;
    if (ops.size() == 0) {
        if (judge) {
            GameState ns = state;
            for (int i = 0; i < 3; i++) {
                if (player == 1) ns.update_round();
                auto enemy_kill_ops = new_kill(ns, !player);
                if (enemy_kill_ops.size() > 0) {
                    current_status = i == 0 ? -2 : -1;
                    break;
                }
                if (ns.board[enemy_maingen_x][enemy_maingen_y].generals.skills_cd[1] <= 0 &&
                    ns.coin[!player] >= Constant::breakthrough && abs(my_maingen_x - enemy_maingen_x) <= 2 &&
                    abs(my_maingen_y - enemy_maingen_y) <= 2 && ns.board[my_maingen_x][my_maingen_y].army >= 20) {
                    auto op = generals_skill_op(enemy_maingen.id, SkillType::ROUT, {my_maingen_x, my_maingen_y});
                    execute(ns, op, !player);
                }
                if (player == 0) ns.update_round();
                auto my_kill_ops = new_kill(ns, player);
                if (my_kill_ops.size() > 0) {
                    current_status = i == 0 ? 2 : 1;
                    break;
                }
                if (ns.board[my_maingen_x][my_maingen_y].generals.skills_cd[1] <= 0 &&
                    ns.coin[player] >= Constant::breakthrough && abs(my_maingen_x - enemy_maingen_x) <= 2 &&
                    abs(my_maingen_y - enemy_maingen_y) <= 2 && ns.board[enemy_maingen_x][enemy_maingen_y].army >= 20) {
                    auto op = generals_skill_op(my_maingen.id, SkillType::ROUT, {enemy_maingen_x, enemy_maingen_y});
                    execute(ns, op, player);
                }
            }
        }
        if (current_status < 0 && judge) {  // escape
            auto path =
                best_escape_path(state, my_maingen_x, my_maingen_y, enemy_maingen_x, enemy_maingen_y, player, movement);
            int now_x = my_maingen_x, now_y = my_maingen_y;
            for (int i = 0; i < std::min((int)(path.size()), movement); i++) {
                int dir = path[i];
                int army_num = state.board[now_x][now_y].army;
                if (army_num > 1) {
                    auto op = move_army_op(std::make_pair(now_x, now_y), static_cast<Direction>(dir), army_num - 1);
                    bool exe_suc = execute(state, op, player);
                    if (!exe_suc) break;
                    ops.push_back(op);
                }
                now_x += dx[dir];
                now_y += dy[dir];
                auto op = move_generals_op(my_maingen.id, {now_x, now_y});
                bool exe_suc = execute(state, op, player);
                if (!exe_suc) break;
                ops.push_back(op);
            }
            return std::make_pair(ops, false);
        }
    }

    if (state.rest_move_step[player] > 0) {  // mobile army
        const auto& new_my_maingen = find_general_by_id(state, main_general_id(state, player));
        int target_x = new_my_maingen.position[0], target_y = new_my_maingen.position[1];
        int dist_to_target[15][15];
        for (int i = 0; i < 15; i++) {
            for (int j = 0; j < 15; j++) {
                dist_to_target[i][j] = 100;
            }
        }
        dist_to_target[target_x][target_y] = 0;
        std::queue<std::pair<int, int>> q;
        q.push({target_x, target_y});
        while (!q.empty()) {
            auto& cur = q.front();
            q.pop();
            int x = cur.first, y = cur.second;
            for (int i = 0; i < 4; i++) {
                int nx = x + dx[i], ny = y + dy[i];
                if (nx < 0 || nx >= 15 || ny < 0 || ny >= 15) continue;
                if (state.tech_level[player][int(TechType::IMMUNE_SWAMP)] == 0 &&
                    state.board[nx][ny].type == CellType::SWAMP)
                    continue;
                if (state.tech_level[player][int(TechType::IMMUNE_SAND)] == 0 &&
                    state.board[nx][ny].type == CellType::SAND)
                    continue;
                if (state.board[nx][ny].generals.type != 0 && state.board[nx][ny].generals.player != player) continue;
                if (dist_to_target[nx][ny] > dist_to_target[x][y] + 1) {
                    dist_to_target[nx][ny] = dist_to_target[x][y] + 1;
                    q.push({nx, ny});
                }
            }
        }

        int best_x = -1, best_y = -1;
        int max_val = -100000000;
        for (int i = 0; i < 15; i++) {
            for (int j = 0; j < 15; j++) {
                if (state.board[i][j].player == player && state.board[i][j].army > 1) {
                    if (i == target_x && j == target_y) continue;
                    bool valid = false;
                    for (int k = 0; k < 4; k++) {
                        int nx = i + dx[k], ny = j + dy[k];
                        if (nx < 0 || nx >= 15 || ny < 0 || ny >= 15) continue;
                        if (state.board[nx][ny].player == player) {
                            valid = true;
                        }
                    }
                    if (!valid) continue;
                    int dist = dist_to_target[i][j];
                    if (dist == 100) continue;
                    int army = state.board[i][j].army;
                    float val = dist * 2 + army;
                    if (val > max_val) {
                        max_val = val;
                        best_x = i;
                        best_y = j;
                    }
                }
            }
        }

        if (best_x == -1 || best_y == -1) return std::make_pair(ops, false);

        auto mobile_path = best_mobile_path(state, best_x, best_y, target_x, target_y, player, false, true);
        if (mobile_path.size() == 0) return std::make_pair(ops, false);
        int steps = std::min((int)(mobile_path.size()), (int)(state.rest_move_step[player]));
        for (int i = 0; i < steps; i++) {
            int move_dir = mobile_path[i];
            int army_num = state.board[best_x][best_y].army;
            auto op = move_army_op(std::make_pair(best_x, best_y), static_cast<Direction>(move_dir), army_num - 1);
            bool suc = execute(state, op, player);
            if (!suc) {
                break;
            }
            ops.push_back(op);
            best_x += dx[move_dir];
            best_y += dy[move_dir];
        }
    }

    return std::make_pair(ops, false);
}

std::vector<Operation> search(GameState& state) {
    std::vector<Operation> ops;
    if (gameend(state, my)) return ops;
    const auto& my_maingen = find_general_by_id(state, main_general_id(state, my));
    const int my_maingen_x = my_maingen.position[0], my_maingen_y = my_maingen.position[1];
    const auto& enemy_maingen = find_general_by_id(state, main_general_id(state, !my));
    const int enemy_maingen_x = enemy_maingen.position[0], enemy_maingen_y = enemy_maingen.position[1];
    const int jump_dist = std::max(
        std::max(abs(my_maingen_x - enemy_maingen_x) - 2, 0) + std::max(abs(my_maingen_y - enemy_maingen_y) - 2, 0), 1);
    int enemy_move_step = state.tech_level[!my][0];
    int my_move_step = std::min(state.tech_level[my][0], my_maingen.mobility_level);
    if (jump_dist > my_move_step + enemy_move_step * 2) return ops;
    bool skill_unlock = false;
    if (my_maingen.produce_level == 4 && my_maingen.defence_level >= 2) {
        skill_unlock = true;
    }

    // consider updating
    int best_choice = -1, best_val = -1e9;
    std::vector<Operation> best_ops;
    bool best_lock_skill = false, best_lock_move = false, best_escape = false;
    int up_bonuses[7] = {1000, 0, 0, 450, 500, -3500, 500};
    for (int up = 0; up < 14; up++) {
        bool my_long_oil = up / 2 == 4;
        bool my_inst_tech = up / 2 == 5;
        int sim_round = (up / 2 == 3) ? 12 : 6;
        GameState ns = state;
        std::vector<Operation> new_ops;
        if (up / 2 == 0) {
            if (my_maingen.produce_level == 2 && ns.coin[my] >= Constant::lieutenant_production_T2 / 2) {
                auto op = update_generals_op(my_maingen.id, QualityType::PRODUCTION);
                execute(ns, op, my);
                new_ops.push_back(op);
            } else if (ns.tech_level[my][(int)(TechType::IMMUNE_SWAMP)] == 0 &&
                       ns.coin[my] >= Constant::swamp_immunity) {
                auto op = update_tech_op(TechType::IMMUNE_SWAMP);
                execute(ns, op, my);
                new_ops.push_back(op);
            } else {
                continue;
            }
        } else if (up / 2 == 1) {
            if (my_maingen.produce_level < 4) {
                continue;
            }
            if (my_maingen.defence_level == 1 && ns.coin[my] >= Constant::lieutenant_defense_T1 / 2) {
                auto op = update_generals_op(my_maingen.id, QualityType::DEFENCE);
                execute(ns, op, my);
                new_ops.push_back(op);
            } else if (my_maingen.defence_level == 2 && ns.coin[my] >= Constant::lieutenant_defense_T2 / 2) {
                auto op = update_generals_op(my_maingen.id, QualityType::DEFENCE);
                execute(ns, op, my);
                new_ops.push_back(op);
            } else {
                continue;
            }
        } else if (up / 2 == 2) {
            if (my_maingen.mobility_level == 1 && ns.coin[my] >= Constant::general_movement_T1 / 2) {
                auto op = update_generals_op(my_maingen.id, QualityType::MOBILITY);
                execute(ns, op, my);
                new_ops.push_back(op);
            } else if (my_maingen.mobility_level == 2 && ns.coin[my] >= Constant::general_movement_T2 / 2 &&
                       ns.tech_level[my][0] >= 3) {
                auto op = update_generals_op(my_maingen.id, QualityType::MOBILITY);
                execute(ns, op, my);
                new_ops.push_back(op);
            } else {
                continue;
            }
        } else if (up / 2 == 3) {
            auto oil_pos = find_best_oil(ns, my);
            if (oil_pos.first != -1) {
                int oil_id = state.board[oil_pos.first][oil_pos.second].generals.id;
                auto op = update_generals_op(oil_id, QualityType::PRODUCTION);
                execute(ns, op, my);
                new_ops.push_back(op);
            } else {
                continue;
            }
        }
        int new_movement = std::min(ns.tech_level[my][0], ns.board[my_maingen_x][my_maingen_y].generals.mobility_level);
        int current_status = 0;
        GameState now_s = ns;
        for (int i = 0; i < 6; i++) {
            if (clock() - start_t > MAX_T4 * CLOCKS_PER_SEC) break;
            if (my == 1) now_s.update_round();
            auto kill_ops = new_kill(now_s, !my);
            if (kill_ops.size() > 0) {
                current_status = i == 0 ? -2 : -1;
                break;
            }
            if (my == 0) now_s.update_round();
            kill_ops = new_kill(now_s, my);
            if (kill_ops.size() > 0) {
                current_status = i == 0 ? 2 : 1;
                break;
            }
            if (clock() - start_t > MAX_T1 * CLOCKS_PER_SEC) break;
        }

        std::cerr << "cs: " << up << " " << current_status << std::endl;
        // if (current_status >= -1) {  // sim
        int best_down_val = 1e9, best_down_choice = -1;
        std::vector<Operation> best_down_ops;
        bool best_down_lock_skill = false, best_down_lock_move = false, best_down_escape = false;
        for (int down = 0; down < 8; down++) {
            std::vector<Operation> down_new_ops = new_ops;
            bool enemy_use_tech = true;
            bool enemy_get_oil = (down + 1) % 2;
            int future_status = 0;
            int attack_val = 0;
            GameState now_s = ns;
            if (down / 2 == 0) {
                if (enemy_maingen.produce_level == 2 && now_s.coin[!my] >= Constant::lieutenant_production_T2 / 2) {
                    auto op = update_generals_op(enemy_maingen.id, QualityType::PRODUCTION);
                    execute(now_s, op, !my);
                } else {
                    continue;
                }
            } else if (down / 2 == 1) {
                if (enemy_maingen.defence_level == 1 && now_s.coin[!my] >= Constant::lieutenant_defense_T1 / 2) {
                    auto op = update_generals_op(enemy_maingen.id, QualityType::DEFENCE);
                    execute(now_s, op, !my);
                } else if (enemy_maingen.defence_level == 2 && now_s.coin[!my] >= Constant::lieutenant_defense_T2 / 2) {
                    auto op = update_generals_op(enemy_maingen.id, QualityType::DEFENCE);
                    execute(now_s, op, !my);
                } else {
                    continue;
                }
            } else if (down / 2 == 2) {
                if (enemy_maingen.mobility_level == 1 && now_s.coin[!my] >= Constant::general_movement_T1 / 2) {
                    auto op = update_generals_op(enemy_maingen.id, QualityType::MOBILITY);
                    execute(now_s, op, !my);
                } else if (enemy_maingen.mobility_level == 2 && now_s.coin[!my] >= Constant::general_movement_T2 / 2 &&
                           now_s.tech_level[!my][0] >= 3) {
                    auto op = update_generals_op(enemy_maingen.id, QualityType::MOBILITY);
                    execute(now_s, op, !my);
                } else {
                    continue;
                }
            }

            for (int i = 0; i < sim_round; i++) {
                if (clock() - start_t > MAX_T4 * CLOCKS_PER_SEC) break;
                if (up % 2 == 0) {
                    auto ops_attack = sim_operation(now_s, true, my_long_oil, my_inst_tech, my);
                    if (up == 10) {
                        std::cerr << i << ".a:" << std::endl;
                        for (auto op : ops_attack.first) {
                            std::cerr << op.stringize();
                        }
                    }
                    if (gameend(now_s, my)) {
                        future_status = i == 0 ? 2 : 1;
                        break;
                    }
                    if (ops_attack.second) attack_val++;
                }
                if (my == 1) now_s.update_round();
                auto ops_attack = sim_operation(now_s, enemy_get_oil, false, false, !my);
                if (up == 10) {
                    std::cerr << i << ".b:" << std::endl;
                    for (auto op : ops_attack.first) {
                        std::cerr << op.stringize();
                    }
                }
                if (gameend(now_s, !my)) {
                    future_status = i == 0 ? -2 : -1;
                    break;
                }
                if (ops_attack.second) attack_val--;
                if (my == 0) now_s.update_round();
                if (up % 2 == 1) {
                    auto ops_attack = sim_operation(now_s, true, my_long_oil, my_inst_tech, my);
                    if (gameend(now_s, my)) {
                        future_status = i == 0 ? 2 : 1;
                        break;
                    }
                    if (ops_attack.second) attack_val++;
                }
                if (up / 2 < 4 && clock() - start_t > MAX_T2 * CLOCKS_PER_SEC) break;
                if (clock() - start_t > MAX_T3 * CLOCKS_PER_SEC) break;
            }
            int val = current_status * 10000 + future_status * 3000 + attack_val * 100;
            auto start_eval = eval(ns), end_eval = eval(now_s);
            int start_eval_diff = start_eval.first - start_eval.second;
            int end_eval_diff = end_eval.first - end_eval.second;
            val += end_eval_diff - start_eval_diff;
            val += up_bonuses[up / 2];
            std::cerr << "fs: " << future_status << " " << val << std::endl;
            if ((up / 2 == 2 || up / 2 == 1) && my_maingen.produce_level < 4) val -= 2000;
            if (future_status > 0) {
                if (up % 2 == 0) {
                    GameState tmp_s = ns;
                    auto ops_attack = sim_operation(tmp_s, true, my_long_oil, my_inst_tech, my);
                    for (int i = 0; i < ops_attack.first.size(); i++) {
                        down_new_ops.push_back(ops_attack.first[i]);
                    }
                }
                if (val < best_down_val) {
                    best_down_val = val;
                    best_down_choice = down;
                    best_down_ops = down_new_ops;
                    best_down_lock_skill = true;
                    best_down_lock_move = true;
                    best_down_escape = false;
                }
            } else if (future_status <= 0 && current_status > 0) {
                if (val < best_down_val) {
                    best_down_val = val;
                    best_down_choice = down;
                    best_down_ops = down_new_ops;
                    best_down_lock_skill = true;
                    best_down_lock_move = true;
                    best_down_escape = false;
                }
            } else if (future_status == 0) {
                if (ns.tech_level[my][1] > 0 && ns.tech_level[!my][1] == 0) {
                    if (val < best_down_val) {
                        best_down_val = val;
                        best_down_choice = down;
                        best_down_ops = down_new_ops;
                        best_down_lock_skill = false;
                        best_down_lock_move = false;
                        best_down_escape = false;
                    }
                } else {
                    if (up % 2 == 0) {
                        GameState tmp_s = ns;
                        auto ops_attack = sim_operation(tmp_s, true, my_long_oil, my_inst_tech, my);
                        for (int i = 0; i < ops_attack.first.size(); i++) {
                            down_new_ops.push_back(ops_attack.first[i]);
                        }
                    }
                    if (val < best_down_val) {
                        best_down_val = val;
                        best_down_choice = down;
                        best_down_ops = down_new_ops;
                        best_down_lock_skill = !skill_unlock;
                        best_down_lock_move = true;
                        best_down_escape = false;
                    }
                }
            } else if (future_status < 0) {
                val = -8000 + current_status * 10000;
                val += up_bonuses[up / 2];
                if (up / 2 == 0) val -= 1000;
                if ((up / 2 == 2 || up / 2 == 1) && my_maingen.produce_level < 4) val -= 2000;
                if (val < best_down_val) {
                    best_down_val = val;
                    best_down_choice = down;
                    best_down_ops = new_ops;
                    best_down_lock_skill = false;
                    best_down_lock_move = false;
                    best_down_escape = true;
                }
            }
        }
        std::cerr << "bd: " << best_down_choice << std::endl;
        if (best_down_val > best_val) {
            best_val = best_down_val;
            best_choice = up;
            best_ops = best_down_ops;
            best_lock_skill = best_down_lock_skill;
            best_lock_move = best_down_lock_move;
            best_escape = best_down_escape;
        }
        // }
        // else {  // only escape
        //     int val = -20000;
        //     val += up_bonuses[up / 2];
        //     if (up / 2 == 0) val -= 1000;
        //     if ((up / 2 == 2 || up / 2 == 1) && my_maingen.produce_level < 4) val -= 2000;
        //     if (val > best_val) {
        //         best_val = val;
        //         best_choice = up;
        //         best_ops = new_ops;
        //         best_lock_skill = false;
        //         best_lock_move = false;
        //         best_escape = true;
        //     }
        // }
    }

    std::cerr << "choice: " << best_choice << " val:" << best_val << std::endl;
    if (!best_escape) {
        ops = best_ops;
        for (auto& op : ops) {
            execute(state, op, my);
        }
        lock_upgrade = best_lock_skill;
        lock_move = best_lock_move;
        return ops;
    }

    // escape
    ops = best_ops;
    for (auto& op : ops) {
        execute(state, op, my);
    }
    lock_upgrade = lock_move = true;
    auto reachable_pos = all_reachable_positions(state, my_maingen_x, my_maingen_y, my_move_step, my);
    std::sort(reachable_pos.begin(), reachable_pos.end(),
              [&](const std::pair<int, int>& a, const std::pair<int, int>& b) {
                  return abs(enemy_maingen_x - a.first) + abs(enemy_maingen_y - a.second) >
                         abs(enemy_maingen_x - b.first) + abs(enemy_maingen_y - b.second);
              });
    int best_x = -1, best_y = -1;
    best_val = -1e9;
    for (int i = 0; i < reachable_pos.size(); i++) {
        if (clock() - start_t > MAX_T4 * CLOCKS_PER_SEC) break;
        GameState ns = state;
        int target_x = reachable_pos[i].first, target_y = reachable_pos[i].second;
        if (!(target_x == my_maingen_x && target_y == my_maingen_y)) {
            auto path = best_mobile_path(ns, my_maingen_x, my_maingen_y, target_x, target_y, my, true, false);
            int now_x = my_maingen_x, now_y = my_maingen_y;
            for (int i = 0; i < std::min((int)(path.size()), my_move_step); i++) {
                int dir = path[i];
                int army_num = ns.board[now_x][now_y].army;
                if (army_num > 1) {
                    auto op = move_army_op(std::make_pair(now_x, now_y), static_cast<Direction>(dir), army_num - 1);
                    bool exe_suc = execute(ns, op, my);
                    if (!exe_suc) break;
                }
                now_x += dx[dir];
                now_y += dy[dir];
                auto op = move_generals_op(my_maingen.id, {now_x, now_y});
                bool exe_suc = execute(ns, op, my);
                if (!exe_suc) break;
            }
        }
        int target_army = ns.board[target_x][target_y].army;
        int val = 0;
        for (int j = 0; j < 8; j++) {
            if (my == 1) ns.update_round();
            auto attack_ops = sim_operation(ns, false, false, false, !my, false, false);
            if (gameend(ns, !my)) {
                if (j == 0)
                    val = -1e8;
                else
                    val = -1e6 + 1e5 * j;
                break;
            }
            if (my == 0) ns.update_round();
            attack_ops = sim_operation(ns, false, false, false, my, true);
            if (gameend(ns, my)) {
                if (j == 0)
                    val = 1e8;
                else
                    val = 1e6 - 1e5 * j;
                break;
            }
        }
        if (val >= 0) {
            val += 1e5 + eval(ns).first - eval(ns).second;
            val -= (abs(target_x - my_maingen_x) + abs(target_y - my_maingen_y)) * 1e4;
            if (ns.board[target_x][target_y].type == CellType::SAND &&
                ns.tech_level[my][int(TechType::IMMUNE_SAND)] == 0)
                val -= 2e4;
        } else {
            val += (abs(target_x - enemy_maingen_x) + abs(target_y - enemy_maingen_y)) * 1e4;
            val += target_army * 10;
            if (ns.board[target_x][target_y].type == CellType::SAND &&
                ns.tech_level[my][int(TechType::IMMUNE_SAND)] == 0)
                val -= 2e3;
        }

        if (val > best_val) {
            best_val = val;
            best_x = target_x;
            best_y = target_y;
        }
    }
    if (best_val >= 0) {
        std::cerr << "suc esc:" << best_x << " " << best_y << " " << best_val << std::endl;
        if (!(best_x == my_maingen_x && best_y == my_maingen_y)) {
            auto path = best_mobile_path(state, my_maingen_x, my_maingen_y, best_x, best_y, my, true, false);
            int now_x = my_maingen_x, now_y = my_maingen_y;
            for (int i = 0; i < std::min((int)(path.size()), my_move_step); i++) {
                int dir = path[i];
                int army_num = state.board[now_x][now_y].army;
                if (army_num > 1) {
                    auto op = move_army_op(std::make_pair(now_x, now_y), static_cast<Direction>(dir), army_num - 1);
                    bool exe_suc = execute(state, op, my);
                    if (!exe_suc) break;
                    ops.push_back(op);
                }
                now_x += dx[dir];
                now_y += dy[dir];
                auto op = move_generals_op(my_maingen.id, {now_x, now_y});
                bool exe_suc = execute(state, op, my);
                if (!exe_suc) break;
                ops.push_back(op);
            }
        }
        return ops;
    }
    if (my_maingen.mobility_level == 1 && state.coin[my] >= Constant::general_movement_T1 / 2) {
        GameState ns = state;
        auto op = update_generals_op(my_maingen.id, QualityType::MOBILITY);
        execute(ns, op, my);
        my_move_step = std::min(ns.tech_level[my][0], ns.board[my_maingen_x][my_maingen_y].generals.mobility_level);
        auto new_reachable_pos = all_reachable_positions(ns, my_maingen_x, my_maingen_y, my_move_step, my);
        std::sort(new_reachable_pos.begin(), new_reachable_pos.end(),
                  [&](const std::pair<int, int>& a, const std::pair<int, int>& b) {
                      return abs(enemy_maingen_x - a.first) + abs(enemy_maingen_y - a.second) >
                             abs(enemy_maingen_x - b.first) + abs(enemy_maingen_y - b.second);
                  });
        int new_best_x = -1, new_best_y = -1, new_best_val = -1e9;
        for (int i = 0; i < new_reachable_pos.size(); i++) {
            if (clock() - start_t > MAX_T4 * CLOCKS_PER_SEC) break;
            if (std::find(reachable_pos.begin(), reachable_pos.end(), new_reachable_pos[i]) != reachable_pos.end())
                continue;
            GameState new_s = ns;
            int target_x = new_reachable_pos[i].first, target_y = new_reachable_pos[i].second;
            if (!(target_x == my_maingen_x && target_y == my_maingen_y)) {
                auto path = best_mobile_path(new_s, my_maingen_x, my_maingen_y, target_x, target_y, my, true, false);
                int now_x = my_maingen_x, now_y = my_maingen_y;
                for (int i = 0; i < std::min((int)(path.size()), my_move_step); i++) {
                    int dir = path[i];
                    int army_num = new_s.board[now_x][now_y].army;
                    if (army_num > 1) {
                        op = move_army_op(std::make_pair(now_x, now_y), static_cast<Direction>(dir), army_num - 1);
                        bool exe_suc = execute(new_s, op, my);
                        if (!exe_suc) break;
                    }
                    now_x += dx[dir];
                    now_y += dy[dir];
                    op = move_generals_op(my_maingen.id, {now_x, now_y});
                    bool exe_suc = execute(new_s, op, my);
                    if (!exe_suc) break;
                }
            }

            int target_army = new_s.board[target_x][target_y].army;
            int val = 0;
            for (int j = 0; j < 8; j++) {
                if (my == 1) new_s.update_round();
                auto attack_ops = sim_operation(new_s, false, false, false, !my, false, false);
                if (gameend(new_s, !my)) {
                    if (j == 0)
                        val = -1e8;
                    else
                        val = -1e6 + 1e5 * j;
                    break;
                }
                if (my == 0) new_s.update_round();
                attack_ops = sim_operation(new_s, false, false, false, my, true);
                if (gameend(new_s, my)) {
                    if (j == 0)
                        val = 1e8;
                    else
                        val = 1e6 - 1e5 * j;
                    break;
                }
            }
            if (val >= 0) {
                val += 5e4 + eval(new_s).first - eval(new_s).second;
                val -= (abs(target_x - my_maingen_x) + abs(target_y - my_maingen_y)) * 1e4;
                if (new_s.board[target_x][target_y].type == CellType::SAND &&
                    new_s.tech_level[my][int(TechType::IMMUNE_SAND)] == 0)
                    val -= 2e4;
            } else {
                val += (abs(target_x - enemy_maingen_x) + abs(target_y - enemy_maingen_y)) * 1e4;
                val += target_army * 10;
                if (new_s.board[target_x][target_y].type == CellType::SAND &&
                    new_s.tech_level[my][int(TechType::IMMUNE_SAND)] == 0)
                    val -= 2e3;
            }

            if (val > new_best_val) {
                new_best_val = val;
                new_best_x = target_x;
                new_best_y = target_y;
            }
        }
        if (new_best_val >= 0 || new_best_val > best_val) {
            std::cerr << "suc esc2:" << new_best_x << " " << new_best_y << " " << new_best_val << std::endl;
            auto op = update_generals_op(my_maingen.id, QualityType::MOBILITY);
            execute(state, op, my);
            ops.push_back(op);
            if (!(new_best_x == my_maingen_x && new_best_y == my_maingen_y)) {
                auto path =
                    best_mobile_path(state, my_maingen_x, my_maingen_y, new_best_x, new_best_y, my, true, false);
                int now_x = my_maingen_x, now_y = my_maingen_y;
                for (int i = 0; i < std::min((int)(path.size()), my_move_step); i++) {
                    int dir = path[i];
                    int army_num = state.board[now_x][now_y].army;
                    if (army_num > 1) {
                        op = move_army_op(std::make_pair(now_x, now_y), static_cast<Direction>(dir), army_num - 1);
                        bool exe_suc = execute(state, op, my);
                        if (!exe_suc) break;
                        ops.push_back(op);
                    }
                    now_x += dx[dir];
                    now_y += dy[dir];
                    op = move_generals_op(my_maingen.id, {now_x, now_y});
                    bool exe_suc = execute(state, op, my);
                    if (!exe_suc) break;
                    ops.push_back(op);
                }
            }
            return ops;
        }
    }

    std::cerr << "fail esc:" << best_x << " " << best_y << std::endl;
    my_move_step = std::min(state.tech_level[my][0], state.board[my_maingen_x][my_maingen_y].generals.mobility_level);
    if (!(best_x == my_maingen_x && best_y == my_maingen_y)) {
        auto path = best_mobile_path(state, my_maingen_x, my_maingen_y, best_x, best_y, my, true, false);
        int now_x = my_maingen_x, now_y = my_maingen_y;
        for (int i = 0; i < std::min((int)(path.size()), my_move_step); i++) {
            int dir = path[i];
            int army_num = state.board[now_x][now_y].army;
            if (army_num > 1) {
                auto op = move_army_op(std::make_pair(now_x, now_y), static_cast<Direction>(dir), army_num - 1);
                bool exe_suc = execute(state, op, my);
                if (!exe_suc) break;
                ops.push_back(op);
            }
            now_x += dx[dir];
            now_y += dy[dir];
            auto op = move_generals_op(my_maingen.id, {now_x, now_y});
            bool exe_suc = execute(state, op, my);
            if (!exe_suc) break;
            ops.push_back(op);
        }
    }
    return ops;
}

std::vector<Operation> simple_ai(int my_seat, const GameState& gamestate) {
    start_t = clock();
    lock_upgrade = lock_move = false;
    if (gamestate.round <= 1) {
        srand(time(NULL));
        my = my_seat;
    }
    global_state = gamestate;
    now_round = global_state.round;
    show_map(global_state);
    get_board(global_state);
    // global_best_oil = best_oil_target(global_state, my);

    std::vector<Operation> ops;
    auto weapon_ops = use_weapon(global_state);
    if (weapon_ops.size() > 0) {
        ops.insert(ops.end(), weapon_ops.begin(), weapon_ops.end());
        get_board(global_state);
    }
    auto kill_ops = new_kill(global_state, my);
    if (kill_ops.size() > 0) {
        ops.insert(ops.end(), kill_ops.begin(), kill_ops.end());
        printops(ops);
        return ops;
    }
    auto search_ops = search(global_state);
    if (search_ops.size() > 0) {
        ops.insert(ops.end(), search_ops.begin(), search_ops.end());
        get_board(global_state);
    }

    auto tp_ops = tp(global_state);
    if (tp_ops.size() > 0) {
        ops.insert(ops.end(), tp_ops.begin(), tp_ops.end());
        get_board(global_state);
    }

    if (!lock_upgrade) {
        auto main_tech_ops = use_tech_main(global_state);
        if (main_tech_ops.size() > 0) {
            ops.insert(ops.end(), main_tech_ops.begin(), main_tech_ops.end());
            get_board(global_state);
        }
        auto upgrade_ops = upgrade(global_state);
        if (upgrade_ops.size() > 0) {
            ops.insert(ops.end(), upgrade_ops.begin(), upgrade_ops.end());
            get_board(global_state);
        }
    }
    auto action_ops = action(global_state);
    if (action_ops.size() > 0) {
        ops.insert(ops.end(), action_ops.begin(), action_ops.end());
        get_board(global_state);
    }
    auto post_esc_ops = post_escape(global_state);
    if (post_esc_ops.size() > 0) {
        ops.insert(ops.end(), post_esc_ops.begin(), post_esc_ops.end());
        get_board(global_state);
    }
    if (!lock_upgrade) {
        auto upgrade_ops2 = upgrade(global_state);
        if (upgrade_ops2.size() > 0) {
            ops.insert(ops.end(), upgrade_ops2.begin(), upgrade_ops2.end());
        }
    }
    printops(ops);
    // show_map(gamestate);
    std::cerr << "--------------------------\n";

    return ops;
}

int main() { run_ai(simple_ai); }