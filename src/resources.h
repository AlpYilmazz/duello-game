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