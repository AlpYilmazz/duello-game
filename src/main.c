#include <stdlib.h>
#include <stdio.h>

#include "justengine.h"

typedef struct {
    uint32 count;
    AABBCollider bounding_box;
    AABBCollider colliders[20];
} HitColliderSet;

typedef enum {
    HITBOX          = 0,
    HURTBOX         = 1,
    COLLISIONBOX    = 2,
    PLAYER_COLLIDER_TYPE_COUNT,
} PlayerColliderType;

typedef HitColliderSet PlayerColliders[PLAYER_COLLIDER_TYPE_COUNT];

#define PLAYER_ANIM_1 0
#define PLAYER_ANIM_2 1
#define PLAYER_ANIM_3 2

typedef struct {
    uint32 count;
    uint32 active;
    TextureHandle anim_sheets[20];
    FrameSpriteSheetAnimation anims[20];
    HitColliderSet collider_sets[20][PLAYER_COLLIDER_TYPE_COUNT];
} PlayerState;

typedef struct {
    SpriteComponentId sprite_component;
    PlayerState state;
    //
    bool paused;
} Player;

typedef enum {
    HitboxEditType_Move,
    HitboxEditType_Resize_Xleft,
    HitboxEditType_Resize_Xright,
    HitboxEditType_Resize_Ytop,
    HitboxEditType_Resize_Ybottom,
} HitboxEditType;

typedef struct {
    Player player;
    // editing: [STATE][COLLIDER_TYPE][I_COLLIDER]
    //          [player.state.active][collider_type_index][i_edit]
    //          player.state
    //              .collider_sets[player.state.active][collider_type_index]
    //              .colliders[i_edit]
    bool editing;
    uint32 collider_type_index;
    uint32 i_edit;
    HitboxEditType edit_type;
    Vector2 hold_anchor;
} PlayerEdit;

typedef struct {
    UIElementId new_hitbox_button;
    UIElementId new_hurtbox_button;
    UIElementId delete_bin_area;
} EditSceneUI;

typedef struct {
    URectSize screen_size;
    TemporaryStorage temporary_storage;
    ThreadPool* threadpool;
    FileAssetServer file_asset_server;
    TextureAssets texture_assets;
    Events_TextureAssetEvent texture_asset_events;
    //
    SpriteStore sprite_store;
    UIElementStore global_ui_store;
    //
    Camera2D primary_camera;
    // Player player;
    PlayerEdit player_edit;
    EditSceneUI edit_scene_ui;
} Resources;

typedef struct {
    SortedRenderSprites render_sprites;
} RenderResources;

Resources RES = {
    .screen_size = { 1920, 1080 },
    .file_asset_server = {
        .asset_folder = "assets"
    },
    // -- STRUCT_ZERO_INIT --
    .sprite_store = STRUCT_ZERO_INIT,
    // -- NULL --
    .threadpool = NULL,
    // -- LAZY_INIT --
    .temporary_storage = LAZY_INIT,
    .texture_assets = LAZY_INIT,
    .texture_asset_events = LAZY_INIT,
    .global_ui_store = LAZY_INIT,
    .primary_camera = LAZY_INIT,
    // .player = LAZY_INIT,
    .player_edit = LAZY_INIT,
    .edit_scene_ui = LAZY_INIT,
};

RenderResources RENDER_RES = {
    // -- STRUCT_ZERO_INIT --
    .render_sprites = STRUCT_ZERO_INIT,
};

EditSceneUI INIT_create_edit_scene_ui(
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
            .disabled_color = DARKGRAY,
            .is_bordered = true,
            .border_thick = 2,
            .border_color = {255, 0, 0, 255},
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
            .disabled_color = DARKGRAY,
            .is_bordered = true,
            .border_thick = 2,
            .border_color = {0, 255, 0, 255},
        },
        .title = "HURTBOX",
    };

    Area delete_bin_area = {
        .elem = {
            .id = 0,
            .type = UIElementType_Area,
            .state = {0},
            .anchor = make_anchor(Anchor_Bottom_Right),
            .position = { RES.screen_size.width, RES.screen_size.height },
            .size = {200, 200},
            .disabled = false,
        },
        .style = {
            .idle_color = {230, 41, 55, 100},
            .hovered_color = {230, 41, 55, 255},
            .is_bordered = true,
            .border_thick = 2,
            .border_color = {255, 0, 0, 255},
        },
    };

    UIElementId new_hitbox_button_id = put_ui_element_button(RES_ui_store, new_hitbox_button);
    UIElementId new_hurtbox_button_id = put_ui_element_button(RES_ui_store, new_hurtbox_button);
    UIElementId delete_bin_area_id = put_ui_element_area(RES_ui_store, delete_bin_area);

    EditSceneUI ui = {
        .new_hitbox_button = new_hitbox_button_id,
        .new_hurtbox_button = new_hurtbox_button_id,
        .delete_bin_area = delete_bin_area_id,
    };
}

Player INIT_spawn_player(
    Events_TextureAssetEvent* RES_texture_asset_events,
    TextureAssets* RES_texture_assets,
    ThreadPool* RES_threadpool,
    FileAssetServer* RES_file_asset_server,
    SpriteStore* RES_sprite_store
) {
    SpriteTransform transform = {
        .anchor = make_anchor(Anchor_Bottom_Mid),
        .position = { 0, 0 },
        .size = { 160, 320 },
        .rotation = 0,
        .rway = Rotation_CCW,
    };

    PlayerState player_state = {0};

    RectSize sprite_size = { 320, 320 };

    player_state.count = 3;
    player_state.active = PLAYER_ANIM_1;
    {
        uint32 state_id = PLAYER_ANIM_1;
        uint32 frame_count = 5;
        player_state.anim_sheets[state_id] = just_engine_asyncio_file_load_image(
            RES_threadpool,
            RES_texture_asset_events,
            RES_texture_assets,
            RES_file_asset_server,
            "anim-1.png"
        );
        player_state.anims[state_id] = new_frame_sprite_sheet_animation(
            sprite_size,
            frame_count
        );

        AABBCollider hitbox = {
            .x_left = 0,
            .x_right = 500,
            .y_top = -transform.size.y/2 - 50,
            .y_bottom = -transform.size.y/2 + 50,
        };
        AABBCollider hurtbox = {
            .x_left = -transform.size.x/4.0,
            .x_right = transform.size.x/4.0,
            .y_top = -transform.size.y,
            .y_bottom = 0,
        };

        player_state.collider_sets[state_id][HITBOX].count = 1;
        for (uint32 i = 0; i < 1; i++) {
            player_state.collider_sets[state_id][HITBOX].colliders[i] = hitbox;
        }

        player_state.collider_sets[state_id][HURTBOX].count = 1;
        for (uint32 i = 0; i < 1; i++) {
            player_state.collider_sets[state_id][HURTBOX].colliders[i] = hurtbox;
        }
    }
    {
        uint32 state_id = PLAYER_ANIM_2;
        uint32 frame_count = 5;
        player_state.anim_sheets[state_id] = just_engine_asyncio_file_load_image(
            RES_threadpool,
            RES_texture_asset_events,
            RES_texture_assets,
            RES_file_asset_server,
            "anim-2.png"
        );
        player_state.anims[state_id] = new_frame_sprite_sheet_animation(
            sprite_size,
            frame_count
        );
    }
    {
        uint32 state_id = PLAYER_ANIM_3;
        uint32 frame_count = 5;
        player_state.anim_sheets[state_id] = just_engine_asyncio_file_load_image(
            RES_threadpool,
            RES_texture_asset_events,
            RES_texture_assets,
            RES_file_asset_server,
            "anim-3.png"
        );
        player_state.anims[state_id] = new_frame_sprite_sheet_animation(
            sprite_size,
            frame_count
        );
    }

    Sprite sprite = {
        .texture = player_state.anim_sheets[player_state.active],
        .tint = WHITE,
        .source = sprite_sheet_get_current_frame(&player_state.anims[player_state.active]), // RECTANGLE_NICHE, // (Rectangle) {0, 0, 32, 32},
        .z_index = 10,
        .visible = true,
        .camera_visible = true,
    };

    SpriteComponentId sprite_component_id = spawn_sprite(RES_sprite_store, transform, sprite);

    Player player = {
        .sprite_component = sprite_component_id,
        .state = player_state,
        .paused = false,
    };

    return player;
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
    PlayerState* state = &player->state;

    // Animation Next Frame
    if (!player->paused || IsKeyPressed(KEY_RIGHT)) {
        tick_frame_sprite_sheet_animation(&state->anims[state->active]);
    }

    // Animation Prev Frame
    if (player->paused && IsKeyPressed(KEY_LEFT)) {
        tick_back_frame_sprite_sheet_animation(&state->anims[state->active]);
    }

    // Animation Pause/Unpause
    if (IsKeyPressed(KEY_SPACE)) {
        player->paused ^= 1;
    }

    // Set State
    int32 key = GetKeyPressed() - KEY_ZERO;
    if (1 <= key && key <= 9 && key <= player->state.count) {
        reset_frame_sprite_sheet_animation(&state->anims[state->active]);
        state->active = key - 1;
        sprite->texture = state->anim_sheets[state->active];
    }

    sprite->source = sprite_sheet_get_current_frame(&state->anims[state->active]);
}

void SYSTEM_UPDATE_move_hitbox(
    SpriteStore* RES_sprite_store,
    PlayerEdit* player_edit,
    Camera2D* primary_camera
) {
    Player* player = &player_edit->player;
    SpriteTransform* player_transform = &RES_sprite_store->transforms[player->sprite_component.id];
    Vector2 player_position = player_transform->position;

    Vector2 mouse = GetScreenToWorld2D(GetMousePosition(), *primary_camera);
    Vector2 relative_mouse = Vector2Subtract(mouse, player_position);

    const float32 MARGIN = 10;
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        for (uint32 i_set = 0; i_set < PLAYER_COLLIDER_TYPE_COUNT; i_set++) {
            HitColliderSet* collider_set = &player->state
                                        .collider_sets[player->state.active][i_set];
            for (uint32 i = 0; i < collider_set->count; i++) {
                AABBCollider box = collider_set->colliders[i];
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
                    player_edit->edit_type = HitboxEditType_Resize_Xleft;
                    goto EDIT_FOUND;
                }
                if (box.x_right - MARGIN <= relative_mouse.x && relative_mouse.x <= box.x_right + MARGIN) {
                    player_edit->edit_type = HitboxEditType_Resize_Xright;
                    goto EDIT_FOUND;
                }
                if (box.y_top - MARGIN <= relative_mouse.y && relative_mouse.y <= box.y_top + MARGIN) {
                    player_edit->edit_type = HitboxEditType_Resize_Ytop;
                    goto EDIT_FOUND;
                }
                if (box.y_bottom - MARGIN <= relative_mouse.y && relative_mouse.y <= box.y_bottom + MARGIN) {
                    player_edit->edit_type = HitboxEditType_Resize_Ybottom;
                    goto EDIT_FOUND;
                }

                // Already inside margin box
                // but not close to sides
                //  => inside the box
                player_edit->edit_type = HitboxEditType_Move;
                player_edit->hold_anchor = (Vector2) {
                    .x = box.x_left - relative_mouse.x,
                    .y = box.y_top - relative_mouse.y,
                };

                EDIT_FOUND:
                player_edit->i_edit = i;
                player_edit->collider_type_index = i_set;
                player_edit->editing = true;
                goto CHECK_END;
            }
        }
        CHECK_END:
    }
    if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
        player_edit->editing = false;
    }

    AABBCollider* editing_box =
        &player->state
            .collider_sets[player->state.active][player_edit->collider_type_index]
            .colliders[player_edit->i_edit];

    if (player_edit->editing) {
        switch (player_edit->edit_type) {
        case HitboxEditType_Move:
            float32 w = editing_box->x_right - editing_box->x_left;
            float32 h = editing_box->y_bottom - editing_box->y_top;
            editing_box->x_left = relative_mouse.x + player_edit->hold_anchor.x;
            editing_box->x_right = editing_box->x_left + w;
            editing_box->y_top = relative_mouse.y + player_edit->hold_anchor.y;
            editing_box->y_bottom = editing_box->y_top + h;
            break;
        case HitboxEditType_Resize_Xleft:
            editing_box->x_left = MIN(relative_mouse.x, editing_box->x_right - 2*MARGIN);
            break;
        case HitboxEditType_Resize_Xright:
            editing_box->x_right = MAX(relative_mouse.x, editing_box->x_left + 2*MARGIN);
            break;
        case HitboxEditType_Resize_Ytop:
            editing_box->y_top = MIN(relative_mouse.y, editing_box->y_bottom - 2*MARGIN);
            break;
        case HitboxEditType_Resize_Ybottom:
            editing_box->y_bottom = MAX(relative_mouse.y, editing_box->y_top + 2*MARGIN);
            break;
        }
    }
}

void SYSTEM_RENDER_hitbox(
    SpriteStore* RES_sprite_store,
    Player* player
) {
    SpriteTransform* player_transform = &RES_sprite_store->transforms[player->sprite_component.id];

    const float32 BORDER_THICK = 5;

    Color box_colors[PLAYER_COLLIDER_TYPE_COUNT] = {0};
    box_colors[HITBOX] = RED;
    box_colors[HURTBOX] = GREEN;
    for (uint32 i_set = 0; i_set < 2; i_set++) {
        HitColliderSet* collider_set = &player->state
                                    .collider_sets[player->state.active][i_set];
        Color box_color = box_colors[i_set];
        Color box_border_color = box_color;

        box_color.a = 50;
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

int main() {

    SET_LOG_LEVEL(LOG_LEVEL_INFO);

    InitWindow(RES.screen_size.width, RES.screen_size.height, "Duello Game");
    SetTargetFPS(60);

    // Resources Lazy Init
    {
        RES.temporary_storage = make_bump_allocator();

        RES.threadpool = thread_pool_create(5, 100);

        RES.texture_assets = just_engine_new_texture_assets();
        RES.texture_asset_events = just_engine_events_texture_asset_event_create();

        RES.global_ui_store = ui_element_store_new_active();

        RES.primary_camera = (Camera2D) {0};
        RES.primary_camera.zoom = 1;
        RES.primary_camera.offset = (Vector2) {
            .x = RES.screen_size.width / 2.0,
            .y = RES.screen_size.height / 2.0,
        };
    }

    /**
     * INIT
     */
    
    JUST_LOG_TRACE("INIT start\n");

    RES.player_edit.player = INIT_spawn_player(
        &RES.texture_asset_events,
        &RES.texture_assets,
        RES.threadpool,
        &RES.file_asset_server,
        &RES.sprite_store
    );

    RES.edit_scene_ui = INIT_create_edit_scene_ui(&RES.global_ui_store);

    JUST_LOG_TRACE("INIT end\n");

    while (!WindowShouldClose()) {

        float32 delta_time = GetFrameTime();

        /**
         * PRE_UPDATE
         */
        
        JUST_LOG_TRACE("PRE_UPDATE start\n");

        SYSTEM_PRE_UPDATE_handle_input_for_ui_store(
            &RES.global_ui_store
        );
        
        JUST_LOG_TRACE("PRE_UPDATE end\n");

        /**
         * UPDATE
         */
        
        JUST_LOG_TRACE("UPDATE start\n");

        SYSTEM_UPDATE_update_player(
            &RES.sprite_store,
            &RES.player_edit.player,
            delta_time
        );
        SYSTEM_UPDATE_move_hitbox(
            &RES.sprite_store,
            &RES.player_edit,
            &RES.primary_camera
        );
        
        JUST_LOG_TRACE("UPDATE end\n");

        /**
         * POST_UPDATE
         */

        JUST_LOG_TRACE("POST_UPDATE start\n");

        SYSTEM_POST_UPDATE_check_mutated_images(
            &RES.texture_assets,
            &RES.texture_asset_events
        );
        
        JUST_LOG_TRACE("POST_UPDATE end\n");

        /**
         * EXTRACT_RENDER
         */
        
        JUST_LOG_TRACE("EXTRACT_RENDER start\n");

        SYSTEM_EXTRACT_RENDER_cull_and_sort_sprites(
            &RES.sprite_store,
            &RENDER_RES.render_sprites
        );
        JUST_LOG_TRACE("SYSTEM_cull_and_sort_sprites end\n");

        SYSTEM_EXTRACT_RENDER_load_textures_for_loaded_or_changed_images(
            &RES.texture_assets,
            &RES.texture_asset_events
        );
        JUST_LOG_TRACE("SYSTEM_load_textures_for_loaded_or_changed_images end\n");
        
        JUST_LOG_TRACE("EXTRACT_RENDER end\n");

        /**
         * RENDER
         */
        
        JUST_LOG_TRACE("RENDER start\n");

        BeginDrawing();
            ClearBackground(DARKGRAY);

            BeginMode2D(RES.primary_camera);

                SYSTEM_RENDER_sorted_sprites(
                    &RES.texture_assets,
                    &RENDER_RES.render_sprites
                );
                SYSTEM_RENDER_hitbox(
                    &RES.sprite_store,
                    &RES.player_edit.player
                );

            EndMode2D();

            SYSTEM_RENDER_draw_ui_elements(
                &RES.global_ui_store
            );

        EndDrawing();
        
        JUST_LOG_TRACE("RENDER end\n");

        /**
         * FRAME_BOUNDARY
         */
        
        JUST_LOG_TRACE("FRAME_BOUNDARY start\n");

        SYSTEM_FRAME_BOUNDARY_swap_event_buffers(
            &RES.texture_asset_events
        );

        SYSTEM_FRAME_BOUNDARY_reset_temporary_storage(
            &RES.temporary_storage
        );
        
        JUST_LOG_TRACE("FRAME_BOUNDARY end\n");

    }

    return 0;
}