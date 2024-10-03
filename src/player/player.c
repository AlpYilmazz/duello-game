#include "stdlib.h"
#include "stdio.h"
#include "string.h"

#include "justengine.h"

#include "ui_extend.h"

#include "player.h"

// HitColliderSet

bool check_collision_hit_collider_set(HitColliderSet* s1, HitColliderSet* s2) {
    if (s1->count == 0 || s2->count == 0) {
        return false;
    }
    if (just_engine_check_collision_aabb_aabb(s1->bounding_box, s2->bounding_box)) {
        if (s1->count == 1 && s2->count == 1) {
            return true;
        }
        for (uint32 i1 = 0; i1 < s1->count; i1++) {
            for (uint32 i2 = 0; i2 < s2->count; i2++) {
                if (just_engine_check_collision_aabb_aabb(s1->colliders[i1], s2->colliders[i2])) {
                    return true;
                }
            }
        }
    }
    return false;
}

void recalculate_bounding_box(HitColliderSet* hcs) {
    if (hcs->count == 0) {
        return;
    }

    AABBCollider bb = hcs->colliders[0];
    for (uint32 i = 1; i < hcs->count; i++) {
        AABBCollider box = hcs->colliders[i];
        bb.x_left = MIN(box.x_left, bb.x_left);
        bb.x_right = MAX(box.x_right, bb.x_right);
        bb.y_top = MIN(box.y_top, bb.y_top);
        bb.y_bottom = MAX(box.y_bottom, bb.y_bottom);
    }
    hcs->bounding_box = bb;
}

// PlayerColliderType
// typedef HitColliderSet PlayerColliders[PLAYER_COLLIDER_TYPE_COUNT];
// PlayerState
// PlayerStateCollection

void set_player_state(PlayerStateCollection* ps_collection, uint32 state) {
    ps_collection->states[ps_collection->active_state].current_frame = 0;
    ps_collection->active_state = state;
}

void set_player_state_by_name(PlayerStateCollection* ps_collection, const char const* state_name) {
    for (uint32 i = 0; i < ps_collection->state_count; i++) {
        if (strcmp(state_name, ps_collection->state_names[i]) == 0) {
            set_player_state(ps_collection, i);
            return;
        }
    }
    // ERROR
}

PlayerState* get_active_player_state(PlayerStateCollection* ps_collection) {
    return &ps_collection->states[ps_collection->active_state];
}

void tick_current_state(PlayerStateCollection* ps_collection) {
    uint32 frame_count = ps_collection->states[ps_collection->active_state].frame_count;
    uint32* set_current_frame = &ps_collection->states[ps_collection->active_state].current_frame;
    uint32 current_frame = *set_current_frame;

    *set_current_frame = (current_frame + 1) % frame_count;
}

void tick_back_current_state(PlayerStateCollection* ps_collection) {
    uint32 frame_count = ps_collection->states[ps_collection->active_state].frame_count;
    uint32* set_current_frame = &ps_collection->states[ps_collection->active_state].current_frame;
    uint32 current_frame = *set_current_frame;

    *set_current_frame = (current_frame + frame_count - 1) % frame_count;
}

Rectangle get_current_frame_source(PlayerStateCollection* ps_collection) {
    RectSize sprite_size = ps_collection->sprite_size;
    uint32 current_frame = ps_collection->states[ps_collection->active_state].current_frame;

    return (Rectangle) {
        .x = current_frame * sprite_size.width,
        .y = 0,
        .width = sprite_size.width,
        .height = sprite_size.height,
    };
}

void info_log_player_state_collection(PlayerStateCollection* psc) {
    JUST_LOG_INFO("-- Player State Collection --\n");
    JUST_LOG_INFO("sprite_size: %d %d\n", (uint32)psc->sprite_size.width, (uint32)psc->sprite_size.height);
    JUST_LOG_INFO("state_count: %d\n", psc->state_count);
    JUST_LOG_INFO("active_state: %d\n", psc->active_state);
    for (uint32 state_i = 0; state_i < psc->state_count; state_i++) {
        char* state_name = psc->state_names[state_i];
        PlayerState* ps = &psc->states[state_i];
        JUST_LOG_INFO("\t-- State [%s] --\n", state_name);
        JUST_LOG_INFO("\tframe_count: %d\n", ps->frame_count);
        JUST_LOG_INFO("\tcurrent_frame: %d\n", ps->current_frame);
        for (uint32 frame_i = 0; frame_i < ps->frame_count; frame_i++) {
            JUST_LOG_INFO("\t\t-- Frame %d --\n", frame_i);
            for (uint32 coll_type_i = 0; coll_type_i < PLAYER_COLLIDER_TYPE_COUNT; coll_type_i++) {
                HitColliderSet* hcs = &ps->collider_sets[frame_i][coll_type_i];
                JUST_LOG_INFO("\t\t\t-- Collider %d --\n", coll_type_i);
                JUST_LOG_INFO("\t\t\tcollider_count: %d\n", hcs->count);
                if (hcs->count > 0) {
                    AABBCollider box = hcs->bounding_box;
                    JUST_LOG_INFO("\t\t\tbounding_box: %0.2f %0.2f %0.2f %0.2f\n", box.x_left, box.x_right, box.y_top, box.y_bottom);
                }
                for (uint32 box_i = 0; box_i < hcs->count; box_i++) {
                    AABBCollider box = hcs->colliders[box_i];
                    JUST_LOG_INFO("\t\t\tbox: %0.2f %0.2f %0.2f %0.2f\n", box.x_left, box.x_right, box.y_top, box.y_bottom);
                }
            }
        }
    }
}

// Player

Player spawn_player(
    Events_TextureAssetEvent* RES_texture_asset_events,
    TextureAssets* RES_texture_assets,
    ThreadPool* RES_threadpool,
    FileAssetServer* RES_file_asset_server,
    SpriteStore* RES_sprite_store,
    HERO hero
) {
    PlayerStateCollection player_state_collection = load_hero(
        RES_texture_asset_events,
        RES_texture_assets,
        RES_threadpool,
        RES_file_asset_server,
        hero
    );
    set_player_state_by_name(&player_state_collection, "idle");
    // info_log_player_state_collection(&player_state_collection);

    SpriteTransform transform = {
        .anchor = make_anchor(Anchor_Bottom_Mid),
        .position = { 0, 0 },
        .use_source_size = true,
        // .size = { 93*4, 112*4 },
        .scale = Vector2_From(4),
        .rotation = 0,
        .rway = Rotation_CCW,
    };

    Sprite sprite = {
        .texture = player_state_collection.states[player_state_collection.active_state].anim_sheet,// player_state.anim_sheets[player_state.active],
        .tint = WHITE,
        .use_custom_source = true,
        .source = get_current_frame_source(&player_state_collection),
        .z_index = 10,
        //
        .use_layer_system = true,
        .layers = on_single_layer(1),
        .visible = true,
        .camera_visible = true,
    };

    SpriteComponentId sprite_component_id = spawn_sprite(RES_sprite_store, transform, sprite);

    Player player = {
        .sprite_component = sprite_component_id,
        .state_collection = player_state_collection,
        .hero = hero,
        .update_timer = new_timer(0.5, Timer_Repeating),
        .paused = false,
    };

    return player;
}

// -----------------
// -- PLAYER EDIT --
// -----------------

// HitboxEditType
// PlayerEdit
// EditSceneUI

EditSceneUI INIT_create_edit_scene_ui(
    URectSize RES_screen_size,
    UIElementStore* RES_ui_store
) {
    Button new_hitbox_button = {
        .elem = {
            .id = 0,
            .type = UIElementType_Button,
            .state = {0},
            .anchor = make_anchor(Anchor_Center),
            .position = {150, 150},
            .size = {100, 50},
            .disabled = false,
        },
        .style = {
            .idle_color = RED,
            .hovered_color = MAROON,
            .pressed_color = YELLOW,
            .disabled_color = BLACK,
            //
            .is_bordered = true,
            .border_thick = 2,
            .border_color = {255, 0, 0, 255},
            //
            .title_font = GetFontDefault(),
            .title_font_size = 15,
            .title_spacing = 2,
            .title_color = WHITE,
        },
        .title = "HITBOX",
    };

    Button new_hurtbox_button = {
        .elem = {
            .id = 0,
            .type = UIElementType_Button,
            .state = {0},
            .anchor = make_anchor(Anchor_Center),
            .position = {150, 150 + 100},
            .size = {100, 50},
            .disabled = false,
        },
        .style = {
            .idle_color = GREEN,
            .hovered_color = LIME,
            .pressed_color = YELLOW,
            .disabled_color = BLACK,
            //
            .is_bordered = true,
            .border_thick = 2,
            .border_color = {0, 255, 0, 255},
            //
            .title_font = GetFontDefault(),
            .title_font_size = 15,
            .title_spacing = 2,
            .title_color = WHITE,
        },
        .title = "HURTBOX",
    };

    Button save_button = {
        .elem = {
            .id = 0,
            .type = UIElementType_Button,
            .state = {0},
            .anchor = make_anchor(Anchor_Center),
            .position = {150, (float32)RES_screen_size.height - 150.0 - 50/2.0},
            .size = {100, 50},
            .disabled = false,
        },
        .style = {
            .idle_color = BLUE,
            .hovered_color = DARKBLUE,
            .pressed_color = YELLOW,
            .disabled_color = BLACK,
            //
            .is_bordered = true,
            .border_thick = 2,
            .border_color = {0, 0, 255, 255},
            //
            .title_font = GetFontDefault(),
            .title_font_size = 15,
            .title_spacing = 2,
            .title_color = WHITE,
        },
        .title = "SAVE",
    };

    Area delete_bin_area = {
        .elem = {
            .id = 0,
            .type = UIElementType_Area,
            .state = {0},
            .anchor = make_anchor(Anchor_Bottom_Right),
            .position = { RES_screen_size.width, RES_screen_size.height },
            .size = {200, 200},
            .disabled = false,
        },
        .style = {
            .idle_color = {230, 41, 55, 50},
            .hovered_color = {230, 41, 55, 150},
            .is_bordered = true,
            .border_thick = 2,
            .border_color = {255, 0, 0, 150},
        },
    };

    SelectionBox test_selection_box = {
        .elem = {
            .id = 0,
            .type = UIElementType_SelectionBox,
            .state = {0},
            .anchor = make_anchor(Anchor_Bottom_Mid),
            .position = {RES_screen_size.width/2.0, (float32)RES_screen_size.height - 150.0 - 50/2.0},
            .size = {100, 100},
            .disabled = false,
        },
        .style = {
            .selected_color = GREEN,
            .unselected_color = RED,
            .disabled_color = DARKGRAY,
            //
            .is_bordered = false,
            // .border_thick = 2,
            // .border_color = {0, 0, 255, 255},
            //
            .title_font = GetFontDefault(),
            .title_font_size = 15,
            .title_spacing = 2,
            .title_color = WHITE,
        },
        .title = "PLAY",
        .selected = false,
    };

    Slider test_slider = {
        .elem = {
            .id = 0,
            .type = UIElementType_Slider,
            .state = {0},
            .anchor = make_anchor(Anchor_Bottom_Mid),
            .position = {RES_screen_size.width/2.0, (float32)RES_screen_size.height - 30 - 100/2.0},
            .size = {500, 20},
            .disabled = false,
        },
        .style = {
            .line_color = { 100, 100, 100, 50 },
            .cursor_color = YELLOW,
            //
            .title_font = GetFontDefault(),
            .title_font_size = 15,
            .title_spacing = 2,
            .title_color = WHITE,
        },
        .title = "UPDATE RATE",
        .low_value = 0.0,
        .high_value = 1.0,
        .cursor = 0.5,
    };

    Panel test_panel = {
        .elem = {
            .id = 0,
            .type = UIElementType_Panel,
            .state = {0},
            .anchor = make_anchor(Anchor_Top_Left),
            .position = {0, 0},
            .size = {RES_screen_size.width, RES_screen_size.height},
            .disabled = false,
        },
        .store = ui_element_store_new_active(),
        .open = true,
    };

    ChoiceList test_choice_list = {
        .elem = {
            .id = 0,
            .type = UIElementType_ChoiceList,
            .state = {0},
            .anchor = make_anchor(Anchor_Top_Left),
            .position = {150, 150 + 100 + 200},
            .size = {250*2, 250},
            .disabled = false,
        },
        .style = {
            // .rows = 2,
            // .cols = 2,
            // .option_size = {100, 100},
            // .option_margin = 10,
            //
            .selected_color = GREEN,
            .unselected_color = RED,
            .disabled_color = DARKGRAY,
            //
            .is_bordered = false,
            // .border_thick = 2,
            // .border_color = {0, 0, 255, 255},
            //
            .title_font = GetFontDefault(),
            .title_font_size = 15,
            .title_spacing = 2,
            .title_color = WHITE,
        },
        .layout = grid_layout((GridLayoutBegin) {
            .box = {
                .x = 150,
                .y = 150 + 100 + 200,
                .width = 250*2,
                .height = 250,
            },
            .box_padding = 20,
            .cell_padding = 10,
            .major = Grid_RowMajor,
            .rows = 1,
            .cols = 3,
        }),
        .option_count = 3,
        .options = {
            {
                .id = HERO_SAMURAI,
                .title = "SAMURAI",
            },
            {
                .id = HERO_KNIGHT,
                .title = "KNIGHT",
            },
            {
                .id = HERO_XXX,
                .title = "XXX",
            },
        }
    };

    Button test_overlap_button = new_hitbox_button;
    test_overlap_button.elem = (UIElement) {
        .id = 0,
        .type = UIElementType_Button,
        .state = {0},
        .layer = 10,
        .anchor = make_anchor(Anchor_Top_Left),
        .position = {150 + 30, 150},
        .size = {100, 100},
        .disabled = false,
    };
    test_overlap_button.style.idle_color = MAGENTA;
    test_overlap_button.style.border_color = BLACK;
    test_overlap_button.title[0] = 'T';
    test_overlap_button.title[1] = 'E';
    test_overlap_button.title[2] = 'S';
    test_overlap_button.title[3] = 'T';
    test_overlap_button.title[4] = '\0';

    new_hurtbox_button.elem.layer = 20;

    // UIElementStore* ui_store = RES_ui_store;
    UIElementStore* ui_store = &test_panel.store;

    // UIElementId test_panel_id = put_ui_element_panel(RES_ui_store, test_panel);
    // JUST_LOG_DEBUG("GLOBAL STORE count: %d\n", RES_ui_store->count);
    // Panel* panel = get_ui_element_unchecked(RES_ui_store, test_panel_id);
    // UIElementStore* ui_store = &panel->store;

    UIElementId new_hitbox_button_id = put_ui_element_button(ui_store, new_hitbox_button);
    UIElementId new_hurtbox_button_id = put_ui_element_button(ui_store, new_hurtbox_button);
    UIElementId delete_bin_area_id = put_ui_element_area(ui_store, delete_bin_area);
    UIElementId save_button_id = put_ui_element_button(ui_store, save_button);

    UIElementId test_selection_box_id = put_ui_element_selection_box(ui_store, test_selection_box);
    UIElementId test_slider_id = put_ui_element_slider(ui_store, test_slider);

    UIElementId test_choice_list_id = put_ui_element_choice_list(ui_store, test_choice_list);

    UIElementId test_overlap_button_id = put_ui_element_button(ui_store, test_overlap_button);

    UIElementId test_panel_id = put_ui_element_panel(RES_ui_store, test_panel);

    EditSceneUI ui = {
        .new_hitbox_button = new_hitbox_button_id,
        .new_hurtbox_button = new_hurtbox_button_id,
        .delete_bin_area = delete_bin_area_id,
        .save_button = save_button_id,
        .test_selection_box = test_selection_box_id,
        .test_slider = test_slider_id,
        .test_choice_list = test_choice_list_id,
        .test_overlap_button = test_overlap_button_id,
        .test_panel = test_panel_id,
    };

    return ui;
}

void SYSTEM_UPDATE_handle_edit_scene_ui(
    FileAssetServer* RES_file_asset_server,
    UIElementStore* RES_ui_store,
    EditSceneUI* edit_scene_ui,
    PlayerEdit* player_edit
) {
    // UIElementStore* ui_store = RES_ui_store;
    Panel* test_panel = get_ui_element_unchecked(RES_ui_store, edit_scene_ui->test_panel);
    UIElementStore* ui_store = &test_panel->store;
    // JUST_LOG_DEBUG("Panel store count: %d\n", ui_store->count);
    // JUST_LOG_DEBUG("mem: %d\n", ui_store->memory.cursor - ui_store->memory.base);

    Button* new_hitbox_button = get_ui_element_unchecked(ui_store, edit_scene_ui->new_hitbox_button);
    Button* new_hurtbox_button = get_ui_element_unchecked(ui_store, edit_scene_ui->new_hurtbox_button);
    Area* delete_bin_area = get_ui_element_unchecked(ui_store, edit_scene_ui->delete_bin_area);
    Button* save_button = get_ui_element_unchecked(ui_store, edit_scene_ui->save_button);

    SelectionBox* test_selection_box = get_ui_element_unchecked(ui_store, edit_scene_ui->test_selection_box);
    Slider* test_slider = get_ui_element_unchecked(ui_store, edit_scene_ui->test_slider);
    Button* test_overlap_button = get_ui_element_unchecked(ui_store, edit_scene_ui->test_overlap_button);

    PlayerStateCollection* psc = &player_edit->player.state_collection;
    PlayerState* player_state = get_active_player_state(psc);

    player_edit->player.paused = !test_selection_box->selected;

    new_hitbox_button->elem.disabled = !player_edit->player.paused;
    new_hurtbox_button->elem.disabled = !player_edit->player.paused;

    if (!test_slider->elem.disabled && test_slider->elem.state.on_press) {
        float32 slider_value = get_slider_value(test_slider);
        JUST_LOG_TRACE("Slider value: %0.2f\n", slider_value);
        player_edit->player.update_timer.time_setup = slider_value;
    }

    if (!test_overlap_button->elem.disabled) {
        if (test_overlap_button->elem.state.on_press) {
            test_overlap_button->draw_offset.y = 5;
        }
        else {
            test_overlap_button->draw_offset.y = 0;
        }

        if (test_overlap_button->elem.state.just_clicked) {
            printf("Test Button Was Pressed\n");
        }
    }

    if (!save_button->elem.disabled) {
        if (save_button->elem.state.on_press) {
            save_button->draw_offset.y = 5;
        }
        else {
            save_button->draw_offset.y = 0;
        }

        if (save_button->elem.state.just_clicked) {
            for (uint32 state_i = 0; state_i < psc->state_count; state_i++) {
                PlayerState* ps = &psc->states[state_i];
                for (uint32 frame_i = 0; frame_i < ps->frame_count; frame_i++) {
                    for (uint32 coll_type_i = 0; coll_type_i < PLAYER_COLLIDER_TYPE_COUNT; coll_type_i++) {
                        HitColliderSet* hcs = &ps->collider_sets[frame_i][coll_type_i];
                        recalculate_bounding_box(hcs);
                    }
                }
            }
            save_hero(RES_file_asset_server, psc, player_edit->player.hero);

            JUST_LOG_INFO("===========\n");
            JUST_LOG_INFO("== SAVED ==\n");
            info_log_player_state_collection(psc);
            JUST_LOG_INFO("== SAVED ==\n");
            JUST_LOG_INFO("===========\n");
        }
    }

    if (!new_hitbox_button->elem.disabled) {
        if (new_hitbox_button->elem.state.on_press) {
            new_hitbox_button->draw_offset.y = 5;
        }
        else {
            new_hitbox_button->draw_offset.y = 0;
        }
        
        if (new_hitbox_button->elem.state.just_clicked) {
            AABBCollider box = {
                .x_left = -50,
                .x_right = 50,
                .y_top = 50,
                .y_bottom = 150,
            };
            HitColliderSet* hcs = &player_state->collider_sets[player_state->current_frame][HITBOX];
            hcs->colliders[hcs->count] = box;
            hcs->count++;
        }
    }
    if (!new_hurtbox_button->elem.disabled) {
        if (new_hurtbox_button->elem.state.on_press) {
            new_hurtbox_button->draw_offset.y = 5;
        }
        else {
            new_hurtbox_button->draw_offset.y = 0;
        }

        if (new_hurtbox_button->elem.state.just_clicked) {
            AABBCollider box = {
                .x_left = -50,
                .x_right = 50,
                .y_top = 50,
                .y_bottom = 150,
            };
            HitColliderSet* hcs = &player_state->collider_sets[player_state->current_frame][HURTBOX];
            hcs->colliders[hcs->count] = box;
            hcs->count++;
        }
    }

    if (!delete_bin_area->elem.disabled
        && player_edit->editing
        && player_edit->edit_type == ColliderEditType_Move
        && IsMouseButtonReleased(MOUSE_BUTTON_LEFT)
        && delete_bin_area->elem.state.on_hover
    ) {
        HitColliderSet* hcs = &player_state->collider_sets[player_state->current_frame][player_edit->edit_collider_type];
        hcs->colliders[player_edit->edit_collider_index] = hcs->colliders[hcs->count - 1];
        hcs->count--;
    }
}

void SYSTEM_UPDATE_move_hitbox(
    SpriteStore* RES_sprite_store,
    PlayerEdit* player_edit,
    SpriteCamera* primary_camera
) {
    if (!player_edit->player.paused) {
        return;
    }

    Player* player = &player_edit->player;
    PlayerState* player_state = get_active_player_state(&player->state_collection);

    SpriteTransform* player_transform = &RES_sprite_store->transforms[player->sprite_component.id];
    Vector2 player_position = player_transform->position;

    Vector2 mouse = GetScreenToWorld2D(GetMousePosition(), primary_camera->camera);
    Vector2 relative_mouse = Vector2Subtract(mouse, player_position);

    const float32 MARGIN = 4;
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        for (uint32 collider_type_i = 0; collider_type_i < PLAYER_COLLIDER_TYPE_COUNT; collider_type_i++) {

            HitColliderSet* collider_set = &player_state->collider_sets[player_state->current_frame][collider_type_i];

            for (uint32 collider_i = 0; collider_i < collider_set->count; collider_i++) {
                AABBCollider box = collider_set->colliders[collider_i];
                AABBCollider margin_box = {
                    .x_left = box.x_left - MARGIN,
                    .x_right = box.x_right + MARGIN,
                    .y_top = box.y_top - MARGIN,
                    .y_bottom = box.y_bottom + MARGIN,
                };
                if (!just_engine_check_point_inside_aabb(margin_box, relative_mouse)) {
                    continue;
                }
                if (box.x_left - MARGIN <= relative_mouse.x && relative_mouse.x <= box.x_left + MARGIN) {
                    player_edit->edit_type = ColliderEditType_Resize_Xleft;
                    player_edit->hold_anchor.x = box.x_left - relative_mouse.x;
                    goto EDIT_FOUND;
                }
                if (box.x_right - MARGIN <= relative_mouse.x && relative_mouse.x <= box.x_right + MARGIN) {
                    player_edit->edit_type = ColliderEditType_Resize_Xright;
                    player_edit->hold_anchor.x = box.x_right - relative_mouse.x;
                    goto EDIT_FOUND;
                }
                if (box.y_top - MARGIN <= relative_mouse.y && relative_mouse.y <= box.y_top + MARGIN) {
                    player_edit->edit_type = ColliderEditType_Resize_Ytop;
                    player_edit->hold_anchor.y = box.y_top - relative_mouse.y;
                    goto EDIT_FOUND;
                }
                if (box.y_bottom - MARGIN <= relative_mouse.y && relative_mouse.y <= box.y_bottom + MARGIN) {
                    player_edit->edit_type = ColliderEditType_Resize_Ybottom;
                    player_edit->hold_anchor.y = box.y_bottom - relative_mouse.y;
                    goto EDIT_FOUND;
                }

                // Already inside margin box
                // but not close to sides
                //  => inside the box
                player_edit->edit_type = ColliderEditType_Move;
                player_edit->hold_anchor = (Vector2) {
                    .x = box.x_left - relative_mouse.x,
                    .y = box.y_top - relative_mouse.y,
                };

                EDIT_FOUND:
                player_edit->editing = true;
                player_edit->edit_collider_index = collider_i;
                player_edit->edit_collider_type = collider_type_i;
                goto CHECK_END;
            }
        }
        CHECK_END:
    }
    if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
        player_edit->editing = false;
    }

    if (player_edit->editing) {
        AABBCollider* editing_box =
            &player_state
                ->collider_sets[player_state->current_frame][player_edit->edit_collider_type]
                .colliders[player_edit->edit_collider_index];

        switch (player_edit->edit_type) {
        case ColliderEditType_Move:
            float32 w = editing_box->x_right - editing_box->x_left;
            float32 h = editing_box->y_bottom - editing_box->y_top;
            editing_box->x_left = relative_mouse.x + player_edit->hold_anchor.x;
            editing_box->x_right = editing_box->x_left + w;
            editing_box->y_top = relative_mouse.y + player_edit->hold_anchor.y;
            editing_box->y_bottom = editing_box->y_top + h;
            break;
        case ColliderEditType_Resize_Xleft:
            editing_box->x_left = MIN(relative_mouse.x + player_edit->hold_anchor.x, editing_box->x_right - 2*MARGIN);
            break;
        case ColliderEditType_Resize_Xright:
            editing_box->x_right = MAX(relative_mouse.x + player_edit->hold_anchor.x, editing_box->x_left + 2*MARGIN);
            break;
        case ColliderEditType_Resize_Ytop:
            editing_box->y_top = MIN(relative_mouse.y + player_edit->hold_anchor.y, editing_box->y_bottom - 2*MARGIN);
            break;
        case ColliderEditType_Resize_Ybottom:
            editing_box->y_bottom = MAX(relative_mouse.y + player_edit->hold_anchor.y, editing_box->y_top + 2*MARGIN);
            break;
        }
    }
}


void SYSTEM_UPDATE_update_player(
    SpriteStore* RES_sprite_store,
    Player* player,
    float32 delta_time
) {
    // Move
    {
        SpriteTransform* transform = &RES_sprite_store->transforms[player->sprite_component.id];

        Vector2 move = {0};
        if (IsKeyDown(KEY_A) && IsKeyDown(KEY_D)) {
            move.x = 0;
        }
        else if (IsKeyDown(KEY_A)) {
            move.x = -1;
        }
        else if (IsKeyDown(KEY_D)) {
            move.x = 1;
        }

        float32 speed = 200;
        transform->position = Vector2Add(
            transform->position,
            Vector2Scale(move, speed * delta_time)
        );
    }

    Sprite* sprite = &RES_sprite_store->sprites[player->sprite_component.id];
    PlayerStateCollection* psc = &player->state_collection;

    tick_timer(&player->update_timer, delta_time);
    bool player_frame_update_ready = timer_is_finished(&player->update_timer);

    if (!player->paused) {
        if (player_frame_update_ready) {
            tick_current_state(psc);
        }
    }
    else { // if (player->paused)
        // Animation Next Frame
        if (IsKeyPressed(KEY_RIGHT)) {
            tick_current_state(psc);
        }

        // Animation Prev Frame
        if (IsKeyPressed(KEY_LEFT)) {
            tick_back_current_state(psc);
        }
    }

    // Animation Pause/Unpause
    if (IsKeyPressed(KEY_SPACE)) {
        // player->paused ^= 1;
    }

    // Set State
    int32 key = GetKeyPressed() - KEY_ZERO;
    if (1 <= key && key <= 9 && key <= psc->state_count) {
        set_player_state(psc, key-1);
        sprite->texture = psc->states[psc->active_state].anim_sheet;
    }

    sprite->source = get_current_frame_source(psc);
}

void SYSTEM_RENDER_hitbox(
    SpriteStore* RES_sprite_store,
    Player* player
) {
    PlayerState* player_state = get_active_player_state(&player->state_collection);

    SpriteTransform* player_transform = &RES_sprite_store->transforms[player->sprite_component.id];

    const float32 BORDER_THICK = 2;

    Color box_colors[PLAYER_COLLIDER_TYPE_COUNT] = {0};
    box_colors[HITBOX] = RED;
    box_colors[HURTBOX] = GREEN;
    box_colors[COLLISIONBOX] = YELLOW;
    for (uint32 i_set = 0; i_set < 2; i_set++) {
        HitColliderSet* collider_set = &player_state->collider_sets[player_state->current_frame][i_set];
        Color box_color = box_colors[i_set];
        Color box_border_color = box_color;

        box_color.a = 20;
        box_border_color.a = 100;

        for (uint32 i = 0; i < collider_set->count; i++) {
            AABBCollider aabb = collider_set->colliders[i];
            Rectangle rect = {
                .x = player_transform->position.x + aabb.x_left,
                .y = player_transform->position.y + aabb.y_top,
                .width = aabb.x_right - aabb.x_left,
                .height = aabb.y_bottom - aabb.y_top,
            };
            DrawRectangleRec(
                rect,
                box_color
            );
            DrawRectangleLinesEx(
                rect,
                BORDER_THICK,
                box_border_color
            );
        }
    }
}


// SERIALIZATION

/*
===
state_count
sprite.h sprite.w
===

state_name
state_anim_sheet_filepath
frame_count

box_type_id
box_count
x_left x_right y_top y_bottom

*/

const char const* hero_name_static(HERO hero) {
    switch (hero) {
    case HERO_SAMURAI:
        return "SAMURAI";
    case HERO_KNIGHT:
        return "KNIGHT";
    case HERO_XXX:
    default:
        return "XXX";
    }
}

const char const* heroes_asset_directory_static() {
    return "heroes";
}

char* hero_file_path(
    FileAssetServer* RES_file_asset_server,
    HERO hero
) {
    const char const* hero_name = hero_name_static(hero);
    const char const* heroes_asset_dir = heroes_asset_directory_static();
    char* filepath = malloc(50 * sizeof(char));
    sprintf(
        filepath,
        "%s/%s/%s/hero_%s.txt",
        RES_file_asset_server->asset_folder,
        heroes_asset_dir,
        hero_name,
        hero_name
    );
    return filepath;
}

PlayerStateCollection load_hero(
    Events_TextureAssetEvent* RES_texture_asset_events,
    TextureAssets* RES_texture_assets,
    ThreadPool* RES_threadpool,
    FileAssetServer* RES_file_asset_server,
    HERO hero
) {
    char* hero_filepath = hero_file_path(RES_file_asset_server, hero);
    FILE* player_serial_file = fopen(hero_filepath, "r");
    free(hero_filepath);

    const char const* heroes_asset_dir = heroes_asset_directory_static();
    const char const* hero_name = hero_name_static(hero);

    const char const* asset_file_prefix = "hero";
    uint32 prefix_len = 
        strlen(heroes_asset_dir)
        + 1                             // '/'
        + strlen(hero_name)
        + 1                             // '/'
        + strlen(asset_file_prefix)
        + 1                             // '_'
        + strlen(hero_name)
        + 1;                            // '_'
    uint32 path_static_part_len =
        prefix_len
        + 4                             // ".png"
        + 1;                            // '\0'

    char* state_asset_path = malloc(2*path_static_part_len * sizeof(char));
    char* state_asset_path_cursor = state_asset_path + prefix_len;
    sprintf(state_asset_path, "%s/%s/%s_%s_", heroes_asset_dir, hero_name, asset_file_prefix, hero_name);
    
    PlayerStateCollection psc = {0};

    fscanf(player_serial_file, "%d\n", &psc.state_count);
    fscanf(player_serial_file, "%f %f\n", &psc.sprite_size.width, &psc.sprite_size.height);

    for (uint32 state_i = 0; state_i < psc.state_count; state_i++) {
        PlayerState* ps = &psc.states[state_i];

        char* state_name = malloc(50 * sizeof(char));
        fscanf(player_serial_file, "%s\n", state_name);
        fscanf(player_serial_file, "%d\n", &ps->frame_count);

        psc.state_names[state_i] = state_name;
        uint32 path_len = path_static_part_len + strlen(state_name);
        sprintf(state_asset_path_cursor, "%s.png", state_name);

        ps->anim_sheet = just_engine_asyncio_file_load_image(
            RES_threadpool,
            RES_texture_asset_events,
            RES_texture_assets,
            RES_file_asset_server,
            state_asset_path
        );

        for (uint32 frame_i = 0; frame_i < ps->frame_count; frame_i++) {
            for (uint32 box_type_i = 0; box_type_i < PLAYER_COLLIDER_TYPE_COUNT; box_type_i++) {
                uint32 box_type_id;
                uint32 box_count;
                fscanf(player_serial_file, "%d\n", &box_type_id);
                fscanf(player_serial_file, "%d\n", &box_count);

                HitColliderSet* hcs = &ps->collider_sets[frame_i][box_type_id];

                hcs->count = box_count;
                if (box_count > 0) {
                    AABBCollider box;
                    fscanf(player_serial_file, "%f %f %f %f\n", &box.x_left, &box.x_right, &box.y_top, &box.y_bottom);
                    hcs->bounding_box = box;
                }

                for (uint32 box_i = 0; box_i < box_count; box_i++) {
                    AABBCollider box;
                    fscanf(player_serial_file, "%f %f %f %f\n", &box.x_left, &box.x_right, &box.y_top, &box.y_bottom);
                    hcs->colliders[box_i] = box;
                }
            }
        }
    }
    
    free(state_asset_path);

    return psc;
}

void save_hero(
    FileAssetServer* RES_file_asset_server,
    PlayerStateCollection* psc,
    HERO hero
) {
    char* hero_filepath = hero_file_path(RES_file_asset_server, hero);
    FILE* player_serial_file = fopen(hero_filepath, "w");
    free(hero_filepath);

    fprintf(player_serial_file, "%d\n", psc->state_count);
    fprintf(player_serial_file, "%d %d\n", (uint32)psc->sprite_size.width, (uint32)psc->sprite_size.height);

    for (uint32 state_i = 0; state_i < psc->state_count; state_i++) {
        PlayerState* ps = &psc->states[state_i];

        char* state_name = psc->state_names[state_i];

        fprintf(player_serial_file, "%s\n", state_name);
        fprintf(player_serial_file, "%d\n", ps->frame_count);

        for (uint32 frame_i = 0; frame_i < ps->frame_count; frame_i++) {
            for (uint32 box_type_i = 0; box_type_i < PLAYER_COLLIDER_TYPE_COUNT; box_type_i++) {
                HitColliderSet* hcs = &ps->collider_sets[frame_i][box_type_i];

                fprintf(player_serial_file, "%d\n", box_type_i);
                fprintf(player_serial_file, "%d\n", hcs->count);

                if (hcs->count > 0) {
                    AABBCollider box = hcs->bounding_box;
                    fprintf(player_serial_file, "%f %f %f %f\n", box.x_left, box.x_right, box.y_top, box.y_bottom);
                }

                for (uint32 box_i = 0; box_i < hcs->count; box_i++) {
                    AABBCollider box = hcs->colliders[box_i];
                    fprintf(player_serial_file, "%f %f %f %f\n", box.x_left, box.x_right, box.y_top, box.y_bottom);
                }
            }
        }
    }
}