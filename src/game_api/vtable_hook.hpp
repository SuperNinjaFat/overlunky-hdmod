#pragma once

#include <cstddef>       // for size_t
#include <functional>    // for equal_to, function, _Func_class
#include <new>           // for operator new
#include <type_traits>   // for forward
#include <unordered_map> // for unordered_map, _Umap_traits<>::allocator_type
#include <utility>       // for min, max
#include <vector>        // for vector, _Vector_iterator, allocator, _Vecto...

#include "state.hpp"
#include "util.hpp" // for function_signature

void* register_hook_function(void*** vtable, size_t index, void* hook_function);
void unregister_hook_function(void*** vtable, size_t index);
void* get_hook_function(void*** vtable, size_t index);

struct VDestructorDetour
{
    using VFunT = void(void*, bool);
    using DtorTaskT = std::function<void(void*)>;

    static void detour(void* self, bool destroy)
    {
    // inline static std::unordered_map<void*, std::vector<DtorTaskT>> s_Tasks{};
        // std::unordered_map<void*, std::vector<DtorTaskT>> tasks;
        // try
        // {
        //     tasks = s_Tasks.at(State::get().ptr());
        // } catch (std::out_of_range e)
        // {
        //     return;
        // }
        auto& tasks = s_Tasks[State::get().ptr()];
        if (tasks.contains(self))
        {
            for (auto& task : tasks[self])
            {
                task(self);
            }
            tasks.erase(self);
        }
        s_OriginalDtors[*(void***)self](self, destroy);
    }

    inline static std::unordered_map<void**, VFunT*> s_OriginalDtors{};
    inline static std::unordered_map<StateMemory*, std::unordered_map<void*, std::vector<DtorTaskT>>> s_Tasks{};
};

template <function_signature VFunT, size_t Index>
struct VTableDetour;

template <class RetT, class ClassT, class... ArgsT, std::size_t Index>
struct VTableDetour<RetT(ClassT*, ArgsT...), Index>
{
    using VFunT = RetT(ClassT*, ArgsT...);
    using DetourFunT = std::function<RetT(ClassT*, ArgsT..., VFunT*)>;

    static RetT detour(ClassT* self, ArgsT... args)
    {
        // std::unordered_map<ClassT*, DetourFunT> functions;
        // try
        // {
        //     functions = s_Functions.at(State::get().ptr());
        // } catch (std::out_of_range e)
        // {
        //     return;
        // }
        auto& functions = s_Functions[State::get().ptr()];
        void** vtable = *(void***)self;
        if constexpr (std::is_void_v<RetT>)
        {
            if (functions.contains(self))
            {
                functions[self](self, args..., s_Originals[vtable]);
                return;
            }
            s_Originals[vtable](self, std::move(args)...);
        }
        else
        {
            if (functions.contains(self))
            {
                return functions[self](self, args..., s_Originals[vtable]);
            }
            return s_Originals[vtable](self, std::move(args)...);
        }
    }

    inline static std::unordered_map<void**, VFunT*> s_Originals{};
    inline static std::unordered_map<StateMemory*, std::unordered_map<ClassT*, DetourFunT>> s_Functions{};
};

template <class HookFunT>
void hook_dtor(void* obj, HookFunT&& hook_fun, std::size_t dtor_index = 0)
{
    using DestructorDetourT = VDestructorDetour;
    using DtorT = DestructorDetourT::VFunT;
    void*** vtable = (void***)obj;
    if (!get_hook_function(vtable, dtor_index))
    {
        DestructorDetourT::s_OriginalDtors[*vtable] = (DtorT*)register_hook_function(vtable, dtor_index, (void*)&DestructorDetourT::detour);
    }
    DestructorDetourT::s_Tasks[State::get().ptr()][obj].push_back(std::forward<HookFunT>(hook_fun));
}

template <class VTableFunT, std::size_t VTableIndex, class T, class HookFunT>
void hook_vtable_no_dtor(T* obj, HookFunT&& hook_fun)
{
    using DetourT = VTableDetour<VTableFunT, VTableIndex>;
    void*** vtable = (void***)obj;
    if (!get_hook_function(vtable, VTableIndex))
    {
        DetourT::s_Originals[*vtable] = (VTableFunT*)register_hook_function(vtable, VTableIndex, (void*)&DetourT::detour);
    }
    DetourT::s_Functions[State::get().ptr()][obj] = std::forward<HookFunT>(hook_fun);
}

template <class VTableFunT, std::size_t VTableIndex, class T, class HookFunT>
void hook_vtable(T* obj, HookFunT&& hook_fun, std::size_t dtor_index = 0)
{
    if constexpr (std::is_same_v<VTableFunT, void(T*)>)
    {
        if (VTableIndex == dtor_index)
        {
            // Just throw this directly into the dtor
            hook_dtor(
                obj,
                [hook_fun_ = std::forward<HookFunT>(hook_fun)](void* self)
                {
                    // Pass a nullop as the original, it's not okay to skip dtors
                    hook_fun_((T*)self, [](T*) {});
                },
                dtor_index);
            return;
        }
    }

    hook_vtable_no_dtor<VTableFunT, VTableIndex>(obj, std::forward<HookFunT>(hook_fun));

    // Unhook in dtor
    hook_dtor(
        obj,
        [](void* self)
        {
            using DetourT = VTableDetour<VTableFunT, VTableIndex>;
            auto& functions = DetourT::s_Functions.at(State::get().ptr());
            functions.erase((T*)self);
        },
        dtor_index);
}
