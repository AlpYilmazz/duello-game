

#include "justengine.h"

#include "resources.h"
#include "player.h"

extern Resources RES;
extern RenderResources RENDER_RES;

void transition_into_player_edit_chapter() {
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

        SYSTEM_UPDATE_update_ui_elements(
            &RES.global_ui_store,
            delta_time
        );

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
}