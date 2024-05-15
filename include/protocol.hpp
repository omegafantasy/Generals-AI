#pragma once
#include <iostream>
#include <string>
#include <tuple>
#include <vector>

#include "gamestate.hpp"
#include "json.hpp"

/**
 * @brief 读取初始地图及先后手信息
 * @return std::tuple<int, Gamestate> - 即先后手编号和游戏状态
 */
std::tuple<int, GameState> read_init_map() {
    GameState gamestate;
    int my_seat, id, player, x, y;
    std::pair<int, int> position;
    std::string s;
    getline(std::cin, s);
    // std::cerr << s << std::endl;
    auto d = nlohmann::json::parse(s);
    my_seat = d["Player"];
    auto map = d["Cells"], generals = d["Generals"], coins = d["Coins"];
    std::string types = d["Cell_type"].dump();
    gamestate.coin[0] = coins[0], gamestate.coin[1] = coins[1];
    for (int i = 0; i < map.size(); ++i) {
        x = int(map[i][0][0]);
        y = int(map[i][0][1]);
        gamestate.board[x][y].type = CellType(int(types[i + 1]) - '0');
        // std::cerr << "cell" << x << "," << y << "has type" << int(types[i + 1]) - '0' << std::endl;
        gamestate.board[x][y].player = int(map[i][1]);
        gamestate.board[x][y].army = int(map[i][2]);
        gamestate.board[x][y].position[0] = x, gamestate.board[x][y].position[1] = y;
    }
    gamestate.next_generals_id = generals.size();
    for (int i = 0; i < generals.size(); ++i) {
        id = int(generals[i]["Id"]);
        player = int(generals[i]["Player"]);
        position = {int(generals[i]["Position"][0]), int(generals[i]["Position"][1])};
        switch (int(generals[i]["Type"])) {
            case 1: {
                gamestate.board[position.first][position.second].generals = MainGenerals(id, player, position);
                gamestate.general_pos.push_back(position.first * 15 + position.second);
                break;
            }
            case 2: {
                gamestate.board[position.first][position.second].generals = SubGenerals(id, player, position);
                gamestate.general_pos.push_back(position.first * 15 + position.second);
                break;
            }
            case 3: {
                gamestate.board[position.first][position.second].generals = OilWell(id, player, position);
                gamestate.general_pos.push_back(position.first * 15 + position.second);
                break;
            }
        }
    }
    return std::tuple<int, GameState>(my_seat, gamestate);
}
/**
 * @brief 读取敌方操作列表
 * @return a vector of operations
 */

std::vector<int> split(const char *str, char c) {
    // until endline
    std::vector<int> result;
    int num = 0;
    while (*str != '\0') {
        if (*str == c) {
            result.push_back(num);
            num = 0;
        } else {
            num = num * 10 + *str - '0';
        }
        str++;
    }
    result.push_back(num);
    return result;
}

std::vector<Operation> read_enemy_operations() {
    std::vector<Operation> operations;
    std::vector<int> params;
    Operation op;
    int param, op_type;
    while (true) {
        std::string s;
        std::getline(std::cin, s);
        char buffer[100];
        strcpy(buffer, s.c_str());
        params = split(buffer, ' ');
        op_type = params[0];
        if (op_type == 8) break;
        params.erase(params.begin());
        op = Operation(static_cast<OperationType>(op_type), params);
        std::cerr << op.stringize();
        operations.push_back(op);
        // std::cin >> op_type;
        // params.clear();
        // if (op_type == 8) {  // 操作结束
        //     break;
        // }
        // int op_num = 0;
        // if (op_type == 6) {
        //     op_num = 5;
        // } else if (op_type == 1 || op_type == 4) {
        //     op_num = 4;
        // } else if (op_type == 2) {
        //     op_num = 3;
        // } else if (op_type == 3 || op_type == 7) {
        //     op_num = 2;
        // } else if (op_type == 5) {
        //     op_num = 1;
        // }
        // for (int i = 0; i < op_num; ++i) {
        //     std::cin >> param;
        //     params.push_back(param);
        // }
        // op = Operation(static_cast<OperationType>(op_type), params);
        // std::cerr << op.stringize();
        // operations.push_back(op);
    }

    return operations;
}
inline void convert_to_big_endian(const void *src, std::size_t size, void *dest) {
    for (std::size_t i = 0; i < size; ++i) static_cast<char *>(dest)[size - i - 1] = static_cast<const char *>(src)[i];
}
inline void print_header(int size) {
    // 按照judger要求发送消息长度
    char buf[4] = {};
    convert_to_big_endian(&size, sizeof(size), buf);
    for (int i = 0; i < 4; ++i) std::cout << buf[i];
}

/**
 * @brief 向评测机发送一个字符串
 * @param msg string
 */
void write_to_judger(const std::string &msg) {
    // 先发送消息长度，再发送消息本身
    print_header(msg.length());
    std::cout << msg;
}
/**
 * @brief 向评测机发送我方操作列表
 * @param ops A vector of Operation
 */
void write_our_operation(const std::vector<Operation> &ops) {
    std::string msg = "";

    for (const auto &op : ops) {
        msg += (op.stringize() + '\n');
    }

    write_to_judger(msg);
}
