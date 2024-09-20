#include <stdlib.h>
#include <stdio.h>

#include "justengine.h"

#include "ui_extend.h"
#include "player.h"

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

int main() {
    ui_extention_register_ui_vtable();

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

    RES.player_edit.player = spawn_player(
        &RES.texture_asset_events,
        &RES.texture_assets,
        RES.threadpool,
        &RES.file_asset_server,
        &RES.sprite_store,
        HERO_SAMURAI // HERO_SAMURAI // HERO_XXX
    );

    RES.edit_scene_ui = INIT_create_edit_scene_ui(RES.screen_size, &RES.global_ui_store);

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

        SYSTEM_UPDATE_handle_edit_scene_ui(
            &RES.file_asset_server,
            &RES.global_ui_store,
            &RES.edit_scene_ui,
            &RES.player_edit,
            &RES.primary_camera
        );
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