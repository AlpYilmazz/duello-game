#pragma once

#include "justengine.h"

#include "player/player.h"

typedef struct {
    URectSize screen_size;
    TemporaryStorage temporary_storage;
    ThreadPool* threadpool;
    FileAssetServer file_asset_server;
    TextureAssets texture_assets;
    Events_TextureAssetEvent texture_asset_events;
    //
    SpriteCameraStore camera_store;
    SpriteStore sprite_store;
    UIElementStore global_ui_store;
    //
    // Player player;
    PlayerEdit player_edit;
    EditSceneUI edit_scene_ui;
} Resources;

typedef struct {
    PreparedRenderSprites prepared_render_sprites;
} RenderResources;