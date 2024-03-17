#include "Item.hpp"

long Item::item_cnt = 0;
Item Item::noItem = {-Item::OVERDUE, 0, Position::npos, -Item::MAX_ITEM_VALUE, true, false};

Items Items::items;
