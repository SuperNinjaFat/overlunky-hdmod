#pragma once

#include <cstdint>
#include <type_traits>

#define ENUM_CLASS_FLAGS(Enum)                                      \
    inline constexpr Enum operator|(Enum Lhs, Enum Rhs)             \
    {                                                               \
        return static_cast<Enum>(                                   \
            static_cast<std::underlying_type_t<Enum>>(Lhs) |        \
            static_cast<std::underlying_type_t<Enum>>(Rhs));        \
    }                                                               \
    inline constexpr Enum operator&(Enum Lhs, Enum Rhs)             \
    {                                                               \
        return static_cast<Enum>(                                   \
            static_cast<std::underlying_type_t<Enum>>(Lhs) &        \
            static_cast<std::underlying_type_t<Enum>>(Rhs));        \
    }                                                               \
    inline constexpr Enum operator^(Enum Lhs, Enum Rhs)             \
    {                                                               \
        return static_cast<Enum>(                                   \
            static_cast<std::underlying_type_t<Enum>>(Lhs) ^        \
            static_cast<std::underlying_type_t<Enum>>(Rhs));        \
    }                                                               \
    inline constexpr Enum operator~(Enum E)                         \
    {                                                               \
        return static_cast<Enum>(                                   \
            ~static_cast<std::underlying_type_t<Enum>>(E));         \
    }                                                               \
    inline Enum& operator|=(Enum& Lhs, Enum Rhs)                    \
    {                                                               \
        return Lhs = static_cast<Enum>(                             \
                   static_cast<std::underlying_type_t<Enum>>(Lhs) | \
                   static_cast<std::underlying_type_t<Enum>>(Rhs)); \
    }                                                               \
    inline Enum& operator&=(Enum& Lhs, Enum Rhs)                    \
    {                                                               \
        return Lhs = static_cast<Enum>(                             \
                   static_cast<std::underlying_type_t<Enum>>(Lhs) & \
                   static_cast<std::underlying_type_t<Enum>>(Rhs)); \
    }                                                               \
    inline Enum& operator^=(Enum& Lhs, Enum Rhs)                    \
    {                                                               \
        return Lhs = static_cast<Enum>(                             \
                   static_cast<std::underlying_type_t<Enum>>(Lhs) ^ \
                   static_cast<std::underlying_type_t<Enum>>(Rhs)); \
    }

using CallbackId = uint32_t;
using Flags = uint32_t;
using uColor = uint32_t;
using SPAWN_TYPE = uint32_t;                  // NoAlias
using VANILLA_SOUND_PARAM = uint32_t;         // NoAlias
using VANILLA_SOUND_CALLBACK_TYPE = uint32_t; // NoAlias
using TEXTURE = int64_t;                      // NoAlias
using INPUTS = uint16_t;                      // NoAlias
using MENU_INPUT = uint16_t;                  // NoAlias
using BUTTON = uint8_t;                       // NoAlias
using ENT_TYPE = uint32_t;                    // NoAlias
using ROOM_TEMPLATE = uint16_t;               // NoAlias
using ROOM_TEMPLATE_TYPE = int32_t;           // NoAlias
using PROCEDURAL_CHANCE = uint32_t;           // NoAlias
using LEVEL_CONFIG = uint32_t;                // NoAlias
using TILE_CODE = uint32_t;                   // NoAlias
using IMAGE = int64_t;                        // NoAlias
using WORLD_SHADER = uint8_t;                 // NoAlias
using SHORT_TILE_CODE = uint8_t;
using STRINGID = uint32_t;
using FEAT = uint8_t;
using KEY = int64_t;    // NoAlias
using RAW_KEY = int8_t; // NoAlias

inline constexpr uint8_t MAX_PLAYERS = 4;

enum class LAYER : int32_t
{
    FRONT = 0,
    BACK = 1,
    PLAYER = -1,
    PLAYER1 = -1,
    PLAYER2 = -2,
    PLAYER3 = -3,
    PLAYER4 = -4,
    BOTH = -128
};

enum class JournalPageType
{
    Progress,
    JournalMenu,
    Places,
    People,
    Bestiary,
    Items,
    Traps,
    Story,
    Feats,
    DeathCause,
    DeathMenu,
    Recap,
    PlayerProfile,
    LastGamePlayed,
};

enum class LogLevel
{
    Info = 0,
    InfoScreen = 3,
    Fatal = 1,
    Error = 2
};

enum class PAUSE_TYPE : int64_t
{
    NONE = 0,
    MENU = 1 << 0,
    FADE = 1 << 1,
    CUTSCENE = 1 << 2,
    FLAG4 = 1 << 3,
    FLAG5 = 1 << 4,
    ANKH = 1 << 5,
    PRE_UPDATE = 1 << 6,
    PRE_GAME_LOOP = 1 << 7,
    PRE_PROCESS_INPUT = 1 << 8,
    FORCE_STATE = 1 << 9,
};
ENUM_CLASS_FLAGS(PAUSE_TYPE);

enum class PAUSE_TRIGGER : int64_t
{
    NONE = 0,
    FADE_START = 1 << 0,
    FADE_END = 1 << 1,
    SCREEN = 1 << 2,
    EXIT = 1 << 3,
    ONCE = 1 << 4,
};
ENUM_CLASS_FLAGS(PAUSE_TRIGGER);

enum class PAUSE_SCREEN : int64_t
{
    NONE = 0,
    LOGO = 1 << 0,
    INTRO = 1 << 1,
    PROLOGUE = 1 << 2,
    TITLE = 1 << 3,
    MENU = 1 << 4,
    OPTIONS = 1 << 5,
    PLAYER_PROFILE = 1 << 6,
    LEADERBOARD = 1 << 7,
    SEED_INPUT = 1 << 8,
    CHARACTER_SELECT = 1 << 9,
    TEAM_SELECT = 1 << 10,
    CAMP = 1 << 11,
    LEVEL = 1 << 12,
    TRANSITION = 1 << 13,
    DEATH = 1 << 14,
    SPACESHIP = 1 << 15,
    WIN = 1 << 16,
    CREDITS = 1 << 17,
    SCORES = 1 << 18,
    CONSTELLATION = 1 << 19,
    RECAP = 1 << 20,
    ARENA_MENU = 1 << 21,
    ARENA_STAGES = 1 << 22,
    ARENA_ITEMS = 1 << 23,
    ARENA_SELECT = 1 << 24,
    ARENA_INTRO = 1 << 25,
    ARENA_LEVEL = 1 << 26,
    ARENA_SCORE = 1 << 27,
    ONLINE_LOADING = 1 << 28,
    ONLINE_LOBBY = 1 << 29,
    LOADING = 1 << 30,
    EXIT = 1 << 31,
};
ENUM_CLASS_FLAGS(PAUSE_SCREEN);
