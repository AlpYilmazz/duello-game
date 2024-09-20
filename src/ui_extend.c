#include "stdlib.h"
#include "assert.h"

#include "justengine.h"

#include "ui_extend.h"

// BinaryBox

UIElementId put_ui_element_binary_box(UIElementStore* store, BinaryBox bbox) {
    return put_ui_element(store, (void*)&bbox, sizeof(BinaryBox));
}

void ui_draw_binary_box(BinaryBox* bbox) {
    UIElement* elem = &bbox->elem;

    Vector2 top_left = find_rectangle_top_left(elem->anchor, elem->position, elem->size);

    Rectangle rect = {
        .x = top_left.x,
        .y = top_left.y,
        .width = elem->size.width,
        .height = elem->size.height,
    };
    Color color = bbox->selected ? GREEN : RED;

    DrawRectangleRec(rect, color);
}

void ui_handle_binary_box(UIElement* elem, UIEvent event, UIEventContext context) {
    BinaryBox* bbox = (void*)elem;
    switch (event) {
    case UIEvent_BeginHover:
        bbox->elem.state.hover = true;
        break;
    case UIEvent_EndHover:
        bbox->elem.state.hover = false;
        break;
    case UIEvent_Pressed:
        bbox->elem.state.pressed = true;
        break;
    case UIEvent_Released:
        if (bbox->elem.state.hover && bbox->elem.state.pressed) {
            bbox->selected ^= 1;
            bbox->elem.state.just_clicked = true;
            bbox->elem.state.click_point_relative = context.mouse;
        }
        break;
    case UIEvent_Draw:
        ui_draw_binary_box(bbox);
        break;
    }
}

// CustomUIElementType

void ui_extention_register_ui_vtable() {
    _STATIC_ASSERT(COUNT_OF_CustomUIElementType <= CUSTOM_UI_ELEMENT_SLOT_COUNT);
    
    uint32 register_start = __LINE__;
    put_ui_handle_vtable_entry(CustomUIElementType_BinaryBox, ui_handle_binary_box);
    uint32 register_end = __LINE__;

    assert(register_end - register_start - 1 == COUNT_OF_CustomUIElementType);
}