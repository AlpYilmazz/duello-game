@echo off
SETLOCAL

set LIB_COMMON_DIR=C:\dev\c\lib-common

set CC=gcc

set COMPILER_FLAGS=-std=c11

set INCLUDE=-I%LIB_COMMON_DIR%/justengine/include -Isrc
set LIB=-L%LIB_COMMON_DIR%/justengine/lib

set LINK=^
    -ljustengine^
    -lraylib -lgdi32 -lwinmm

set SRC_DIR=src
set TARGET_DIR=target

set OUTPUT=find-the-lilu.exe
set COMPILE=^
    %SRC_DIR%/ui_extend.c^
    %SRC_DIR%/player/player.c^
    %SRC_DIR%/player/player_edit_chapter.c^
    %SRC_DIR%/main.c

mkdir %TARGET_DIR%

@echo on

%CC% %COMPILER_FLAGS% %COMPILE% %INCLUDE% %LIB% %LINK% -o ./%TARGET_DIR%/%OUTPUT%

@echo off

set OUTPUT_LOCAL=game-local.exe
copy %TARGET_DIR%\%OUTPUT% %OUTPUT_LOCAL%

:: gcc main.c -Ivendor/raylib/include -Lvendor/raylib/lib -lraylib -lgdi32 -lwinmm -o target/game.exe