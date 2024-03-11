#include "Item.hpp"

long Item::item_cnt = 0;
Item Item::noItem = {-Item::OVERDUE};

Items Items::items;
