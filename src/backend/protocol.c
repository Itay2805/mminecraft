#include "protocol.h"
#include "backend/components/player.h"
#include "world.h"
#include "util/stb_ds.h"
#include "sender.h"
#include "world/world.h"
#include "nbt/nbt_writer.h"

#include <nbt/dimension_codec.nbt.h>
#include <nbt/overworld.nbt.h>
#include <nbt/the_nether.nbt.h>
#include <nbt/the_end.nbt.h>

static void do_arrfree(void* arr) {
    arrfree(arr);
}

void send_join_game(int fd, ecs_entity_t entity) {
    game_mode_t mode = *ecs_get(g_ecs, entity, game_mode_t);

    // we include null terminator in the string size calculation because we need the length,
    // which is short enough to fit in a byte
    uint8_t* data = NULL;
    arrsetcap(data,
          1 + // pid
          sizeof(join_game_1_t) +
          sizeof("minecraft:overworld") +
          sizeof("minecraft:the_nether") +
          sizeof("minecraft:the_end") +
          out_build_nbt_dimension_codec_nbt_len +
          out_build_nbt_overworld_nbt_len + // TODO: select dynamically
          sizeof("minecraft:overworld") +
          sizeof(join_game_2_t));

    // push the id
    arrpush(data, 0x24);

    // the first chunk of data
    join_game_1_t* join_game_1 = (join_game_1_t*)arraddnptr(data, sizeof(join_game_1_t));
    join_game_1->entity_id = (int32_t)entity;
    join_game_1->is_hardcore = false;
    join_game_1->gamemode = mode;
    join_game_1->previous_gamemode = -1;
    join_game_1->world_count = 3;

    // the world names
    arrpush(data, sizeof("minecraft:overworld") - 1);
    memcpy(
        arraddnptr(data, sizeof("minecraft:overworld") - 1),
        "minecraft:overworld",
        sizeof("minecraft:overworld") - 1
    );

    arrpush(data, sizeof("minecraft:the_nether") - 1);
    memcpy(
        arraddnptr(data, sizeof("minecraft:the_nether") - 1),
        "minecraft:the_nether",
        sizeof("minecraft:the_nether") - 1
    );

    arrpush(data, sizeof("minecraft:the_end") - 1);
    memcpy(
        arraddnptr(data, sizeof("minecraft:the_end") - 1),
        "minecraft:the_end",
        sizeof("minecraft:the_end") - 1
    );

    // the dimension codec, generated at compile time
    memcpy(
        arraddnptr(data, out_build_nbt_dimension_codec_nbt_len),
        out_build_nbt_dimension_codec_nbt,
        out_build_nbt_dimension_codec_nbt_len
    );

    // the current dimension
    // TODO: select dynamically
    memcpy(
        arraddnptr(data, out_build_nbt_overworld_nbt_len),
        out_build_nbt_overworld_nbt,
        out_build_nbt_overworld_nbt_len
    );

    // zero length for the world name
    arrpush(data, sizeof("minecraft:overworld") - 1);
    memcpy(
        arraddnptr(data, sizeof("minecraft:overworld") - 1),
        "minecraft:overworld",
        sizeof("minecraft:overworld") - 1
    );

    // the second chunk of data
    join_game_2_t* join_game_2 = (join_game_2_t*)arraddnptr(data, sizeof(join_game_2_t));
    join_game_2->hashed_seed = 0;
    join_game_2->view_distance = 32;
    join_game_2->reduced_debug_info = false;
    join_game_2->enable_respawn_screen = true;
    join_game_2->is_debug = false;
    join_game_2->is_flat = g_overworld.flat;

    // send it
    backend_sender_send(fd, data, arrlen(data), do_arrfree);
}
