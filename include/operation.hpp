#pragma once
#include <vector>

#include "gamestate.hpp"
enum class OperationType {
    DEFAULT_OP,
    MOVE_ARMY,
    MOVE_GENERALS,
    UPDATE_GENERALS,
    USE_GENERAL_SKILLS,
    UPDATE_TECH,
    USE_SUPERWEAPON,
    CALL_GENERAL
};

class Operation {
   public:
    OperationType opcode;
    std::vector<int> operand;

    Operation(OperationType opcode = OperationType::DEFAULT_OP, const std::vector<int>& operand = {})
        : opcode(opcode), operand(operand) {}

    std::string stringize() const {
        std::string result = std::to_string(int(opcode)) + " ";
        for (const int& param : operand) {
            result += std::to_string(param) + " ";
        }
        result += "\n";
        return result;
    }
};
// 封装的operation类，用来封装操作码为更易读的形式
/* 创建移动军队的操作对象 */
Operation move_army_op(std::pair<int, int> position, Direction direction, int num) {
    return Operation(OperationType::MOVE_ARMY, {position.first, position.second, static_cast<int>(direction) + 1, num});
}

/* 创建移动将军的操作对象 */
Operation move_generals_op(int generals_id, std::pair<int, int> position) {
    return Operation(OperationType::MOVE_GENERALS, {generals_id, position.first, position.second});
}

/* 创建升级将军属性的操作对象 */
Operation update_generals_op(int generals_id, QualityType type) {
    return Operation(OperationType::UPDATE_GENERALS, {generals_id, static_cast<int>(type) + 1});
}

/* 创建使用将军技能的操作对象 */
Operation generals_skill_op(int generals_id, SkillType type, std::pair<int, int> position) {
    return Operation(OperationType::USE_GENERAL_SKILLS,
                     {generals_id, static_cast<int>(type) + 1, position.first, position.second});
}

/* 创建升级科技的操作对象 */
Operation update_tech_op(TechType type) { return Operation(OperationType::UPDATE_TECH, {static_cast<int>(type) + 1}); }

/* 创建使用超级武器的操作对象 */
Operation use_superweapon_op(WeaponType type, std::pair<int, int> destination, std::pair<int, int> origin = {-1, -1}) {
    return Operation(OperationType::USE_SUPERWEAPON,
                     {static_cast<int>(type) + 1, destination.first, destination.second, origin.first, origin.second});
}

/* 创建召唤副将的操作对象 */
Operation call_generals_op(std::pair<int, int> position) {
    return Operation(OperationType::CALL_GENERAL, {position.first, position.second});
}