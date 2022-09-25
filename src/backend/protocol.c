#include "protocol.h"
#include "backend/components/player.h"
#include "world.h"
#include "util/stb_ds.h"
#include "sender.h"
#include "world/world.h"
#include "nbt/nbt_writer.h"
#include "backend.h"

#include <nbt/dimension_codec.nbt.h>
#include <nbt/overworld.nbt.h>
#include <nbt/the_nether.nbt.h>
#include <nbt/the_end.nbt.h>

#include <util/defs.h>

static void do_arrfree(void* arr) {
    arrfree(arr);
}

static uint8_t* write_varint(uint8_t* data, int32_t value) {
    uint32_t uvalue = (uint32_t)value;

    while (true) {
        if ((uvalue & ~VARINT_SEGMENT_BITS) == 0) {
            *arraddnptr(data, 1) = uvalue;
            return data;
        }

        *arraddnptr(data, 1) = ((uvalue & VARINT_SEGMENT_BITS) | VARINT_CONTINUE_BIT);
        uvalue >>= 7;
    }
}

static void fake_varint(uint8_t* data, int32_t value, int count) {
    uint32_t uvalue = (uint32_t)value;

    while (count--) {
        if (count == 0) {
            *data = uvalue;
            return;
        }

        *data++ = ((uvalue & VARINT_SEGMENT_BITS) | VARINT_CONTINUE_BIT);
        uvalue >>= 7;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Join Game
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

typedef struct join_game_1 {
    int entity_id;
    bool is_hardcore;
    uint8_t gamemode;
    uint8_t previous_gamemode;
    uint8_t world_count; // varint but we only have small values
} PACKED join_game_1_t;

typedef struct join_game_2 {
    uint64_t hashed_seed;
    uint8_t _ignored;
    uint8_t view_distance;
    bool reduced_debug_info;
    bool enable_respawn_screen;
    bool is_debug;
    bool is_flat;
} PACKED join_game_2_t;

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
    join_game_1->entity_id = htobe32((entity & ECS_ENTITY_MASK));
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
    join_game_2->view_distance = MAX_VIEW_DISTANCE;
    join_game_2->reduced_debug_info = false;
    join_game_2->enable_respawn_screen = true;
    join_game_2->is_debug = false;
    join_game_2->is_flat = g_overworld.flat;

    // send it
    backend_sender_send(fd, data, arrlen(data), do_arrfree);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Join Game
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

typedef struct chunk_data_1 {
    int chunk_x;
    int chunk_z;
    bool full_chunk;
} PACKED chunk_data_1_t;

void send_chunk_data(int fd, chunk_t* chunk) {
    // TODO: calculate in a nicer way
    uint8_t* data = NULL;
    arrsetcap(data,
          1 + // pid
          sizeof(chunk_data_1_t));

    // push the id
    arrpush(data, 0x20);

    // the first chunk of data
    chunk_data_1_t* chunk_data_1 = (chunk_data_1_t*)arraddnptr(data, sizeof(chunk_data_1_t));
    chunk_data_1->chunk_x = SWAP32(chunk->position.x);
    chunk_data_1->chunk_z = SWAP32(chunk->position.z);
    chunk_data_1->full_chunk = true;

    // write the primary bitmask
    uint16_t primary_bit_mask = 0;
    for (int i = 0; i < 16; i++) {
        section_t* section = chunk->sections[i];
        if (section == NULL)
            continue;

        // mark as present
        primary_bit_mask |= 1 << i;
    }
    data = write_varint(data, primary_bit_mask);

        // heightmaps
    // TODO: do we need this?
    NBT_COMPOUND(data, "");
    NBT_END(data);

    // send all the biome ids
    // TODO: support biomes
    data = write_varint(data, 1024);
    uint8_t* biomes = arraddnptr(data, 1024);
    memset(biomes, 127, 1024);

    uint8_t* size_placeholder = arraddnptr(data, 4);
    int32_t sections_start = arrlen(data);

    // send all the sections
    for (int i = 0; i < 16; i++) {
        section_t* section = chunk->sections[i];
        if (section == NULL)
            continue;

        // the block count, pre-calculated
        *(uint16_t*)arraddnptr(data, sizeof(uint16_t)) = htobe16(section->block_count);

        // if we have a palette then use its bits-per-block, otherwise use a default
        // 15 bits per block value
        int32_t bits = section->palette != NULL ? section->palette->bits : 15;
        *arraddnptr(data, sizeof(uint8_t)) = bits;

        // send the pallete, if any
        if (section->palette != NULL) {
            data = write_varint(data, hmlen(section->palette->lookup));

            for (int j = 0; j < hmlen(section->palette->lookup); j++) {
                block_state_id_t id = section->palette->entries[j];
                if (id == INVALID_BLOCK_STATE_ID)
                    break;

                data = write_varint(data, id);
            }
        }

        // actually write the data
        data = section_write_compact(section, data);
    }

    // set the length and the primary bit mask
    fake_varint(size_placeholder, arrlen(data) - sections_start, 4);

    // block entities
    // TODO: this
    data = write_varint(data, 0);

    // send it
    backend_sender_send(fd, data, arrlen(data), do_arrfree);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Player position and look
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

typedef struct player_position_and_look {
    double x;
    double y;
    double z;
    float yaw;
    float pitch;
    uint8_t byte;
} PACKED player_position_and_look_t;

void send_player_position_and_look(int fd, int32_t teleport_id, entity_position_t position, entity_rotation_t rotation) {
    uint8_t* data = NULL;
    arrsetcap(data,
              1 + // pid
              sizeof(player_position_and_look_t)
              + 4);

    // push the id
    arrpush(data, 0x34);

    // set the data
    player_position_and_look_t* pos = (player_position_and_look_t*)arraddnptr(data, sizeof(player_position_and_look_t));
    pos->x = SWAP64(position.x);
    pos->y = SWAP64(position.y);
    pos->z = SWAP64(position.z);
    pos->pitch = SWAP32(rotation.pitch);
    pos->yaw = SWAP32(rotation.yaw);
    pos->byte = 0;

    data = write_varint(data, teleport_id);

    // send it
    backend_sender_send(fd, data, arrlen(data), do_arrfree);
}
