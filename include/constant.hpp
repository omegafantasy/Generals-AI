#pragma once

class Constant {
   public:
    static const int col = 15;  // 定义了地图的列数。
    static const int row = 15;  // 定义了地图的行数。

    static const int OilWell_production_T1 = 10;
    static const int OilWell_production_T2 = 25;
    static const int OilWell_production_T3 = 35;
    static const int OilWell_defense_T1 = 10;
    static const int OilWell_defense_T2 = 15;
    static const int OilWell_defense_T3 = 30;

    static const int lieutenant_new_recruit = 50;
    static const int lieutenant_production_T1 = 40;
    static const int lieutenant_production_T2 = 80;
    static const int lieutenant_defense_T1 = 40;
    static const int lieutenant_defense_T2 = 100;

    static const int general_movement_T1 = 20;
    static const int general_movement_T2 = 40;

    static const int tactical_strike = 20;
    static const int breakthrough = 15;
    static const int leadership = 30;
    static const int fortification = 30;
    static const int weakening = 30;

    static const int army_movement_T1 = 80;
    static const int army_movement_T2 = 150;
    static const int swamp_immunity = 100;
    static const int sand_immunity = 75;
    static const int unlock_super_weapon = 250;

    static constexpr double sand_percent = 0.15;
    static constexpr double swamp_percent = 0.05;
};
