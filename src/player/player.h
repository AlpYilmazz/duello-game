#pragma once

#include "justengine.h"

typedef struct {
    uint32 count; // collider count
    AABBCollider bounding_box;
    AABBCollider colliders[20];
} HitColliderSet;

bool check_collision_hit_collider_set(HitColliderSet* s1, HitColliderSet* s2);

typedef enum {
    HITBOX          = 0,
    HURTBOX         = 1,
    COLLISIONBOX    = 2,
    PLAYER_COLLIDER_TYPE_COUNT,
} PlayerColliderType;

typedef struct {
    uint32 frame_count;
    uint32 current_frame;
    TextureHandle anim_sheet;
    HitColliderSet collider_sets[20][PLAYER_COLLIDER_TYPE_COUNT]; // [current_frame][PlayerColliderType]
} PlayerState;

typedef struct {
    RectSize sprite_size;
    uint32 state_count; // animation count
    uint32 active_state; // the active animation
    char* state_names[20]; // [active_state]
    PlayerState states[20]; // [active_state]
} PlayerStateCollection;

void set_player_state(PlayerStateCollection* ps_collection, uint32 state);

typedef enum {
    HERO_SAMURAI,
    HERO_KNIGHT,
    HERO_XXX,
} HERO;

const char const* hero_name_static(HERO hero);

typedef struct {
    SpriteComponentId sprite_component;
    PlayerStateCollection state_collection;
    //
    HERO hero;
    Timer update_timer;
    bool paused;
} Player;

Player spawn_player(
    Events_TextureAssetEvent* RES_texture_asset_events,
    TextureAssets* RES_texture_assets,
    ThreadPool* RES_threadpool,
    FileAssetServer* RES_file_asset_server,
    SpriteStore* RES_sprite_store,
    HERO hero
);

void SYSTEM_UPDATE_update_player(
    SpriteStore* RES_sprite_store,
    Player* player,
    float32 delta_time
);

void SYSTEM_RENDER_hitbox(
    SpriteStore* RES_sprite_store,
    Player* player
);

// -----------------
// -- PLAYER EDIT --
// -----------------

typedef enum {
    ColliderEditType_Move,
    ColliderEditType_Resize_Xleft,
    ColliderEditType_Resize_Xright,
    ColliderEditType_Resize_Ytop,
    ColliderEditType_Resize_Ybottom,
} ColliderEditType;

typedef struct {
    Player player;
    //
    bool editing;
    uint32 edit_collider_index;
    PlayerColliderType edit_collider_type;
    ColliderEditType edit_type;
    Vector2 hold_anchor;
} PlayerEdit;

typedef struct {
    UIElementId new_hitbox_button;
    UIElementId new_hurtbox_button;
    UIElementId delete_bin_area;
    UIElementId save_button;
    UIElementId test_selection_box;
    UIElementId test_slider;
    UIElementId test_panel;
} EditSceneUI;

EditSceneUI INIT_create_edit_scene_ui(
    URectSize RES_screen_size,
    UIElementStore* RES_ui_store
);

void SYSTEM_UPDATE_handle_edit_scene_ui(
    FileAssetServer* RES_file_asset_server,
    UIElementStore* RES_ui_store,
    EditSceneUI* edit_scene_ui,
    PlayerEdit* player_edit
);

void SYSTEM_UPDATE_move_hitbox(
    SpriteStore* RES_sprite_store,
    PlayerEdit* player_edit,
    SpriteCamera* primary_camera
);

// -------------------
// -- SERIALIZATION --
// -------------------

PlayerStateCollection load_hero(
    Events_TextureAssetEvent* RES_texture_asset_events,
    TextureAssets* RES_texture_assets,
    ThreadPool* RES_threadpool,
    FileAssetServer* RES_file_asset_server,
    HERO hero
);

void save_hero(
    FileAssetServer* RES_file_asset_server,
    PlayerStateCollection* psc,
    HERO hero
);