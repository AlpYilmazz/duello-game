#include <stdlib.h>
#include <stdio.h>

#include "justengine.h"

#include "resources.h"
#include "ui_extend.h"
#include "player/player.h"
#include "player/player_edit_chapter.h"

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

void RES_cleanup() {
    free_bump_allocator(&RES.temporary_storage);
    thread_pool_shutdown(RES.threadpool, THREADPOOL_IMMEDIATE_SHUTDOWN);
    ui_element_store_drop(&RES.global_ui_store);
}

void RENDER_RES_cleanup() {

}

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

    transition_into_player_edit_chapter();

    // CLEANUP

    RES_cleanup();
    RENDER_RES_cleanup();

    return 0;
}