#include "illumination.hpp"

#include "color.hpp"
#include "entity.hpp"
#include "math.hpp"
#include "search.hpp"
#include "state.hpp"
#include "thread_utils.hpp"
#include <type_traits>

Illumination* create_illumination(Vec2 pos, Color col, LIGHT_TYPE type, float size, uint8_t light_flags, int32_t uid, LAYER layer)
{
    static size_t offset = get_address("generate_illumination");

    if (offset != 0)
    {
        auto state = get_state_ptr();

        typedef Illumination* create_illumination_func(custom_vector<Illumination*>*, Vec2*, Color, LIGHT_TYPE, float, uint8_t light_flags, int32_t uid, uint8_t layer);
        static create_illumination_func* cif = (create_illumination_func*)(offset);
        auto emitted_light = cif(state->lightsources, &pos, std::move(col), type, size, light_flags, uid, enum_to_layer(layer));
        return emitted_light;
    }
    return nullptr;
}

Illumination* create_illumination(Color color, float size, float x, float y)
{
    return create_illumination(Vec2{x, y}, std::move(color), LIGHT_TYPE::NONE, size, 0x20, -1, LAYER::FRONT);
}

Illumination* create_illumination(Color color, float size, int32_t uid)
{
    auto entity = get_entity_ptr(uid);
    if (entity != nullptr)
    {
        return create_illumination(Vec2{entity->abs_x, entity->abs_y}, std::move(color), LIGHT_TYPE::FOLLOW_ENTITY, size, 0x20, uid, (LAYER)entity->layer);
    }
    return nullptr;
}

void refresh_illumination(Illumination* illumination)
{
    static uint32_t* offset = 0;
    if (offset == 0)
    {
        size_t** heap_offset = (size_t**)get_address("refresh_illumination_heap_offset");
        auto illumination_counter = OnHeapPointer<uint32_t>(**heap_offset);
        offset = illumination_counter.decode();
    }
    illumination->timer = *offset;
}
