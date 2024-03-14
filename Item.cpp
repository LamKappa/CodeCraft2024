#include "Item.hpp"

long Item::item_cnt = 0;
Item Item::noItem = {-Item::OVERDUE, -1, Position::npos, -Item::MAX_ITEM_VALUE};

Items Items::items;
