#pragma once

#include "justengine.h"

typedef enum {
    CustomUIElementType_BinaryBox = 0,
    //
    COUNT_OF_CustomUIElementType,
} CustomUIElementType;

void ui_extention_register_ui_vtable();

typedef struct {
    UIElement elem;
    bool selected;
} BinaryBox;

UIElementId put_ui_element_binary_box(UIElementStore* store, BinaryBox bbox);
