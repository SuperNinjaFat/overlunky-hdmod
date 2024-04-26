#pragma once

#include <cstddef>       // for size_t
#include <functional>    // for equal_to, function, _Func_class
#include <mutex>         // for lock_guard
#include <new>           // for operator new
#include <shared_mutex>  // for unique_lock
#include <type_traits>   // for forward
#include <unordered_map> // for unordered_map, _Umap_traits<>::allocator_type
#include <utility>       // for min, max
#include <vector>        // for vector, _Vector_iterator, allocator, _Vecto...

#include "script/lua_vm.hpp"
#include "state.hpp"
#include "thread_utils.hpp"
#include "util.hpp" // for function_signature

void* register_hook_function(void*** vtable, size_t index, void* hook_function);
void unregister_hook_function(void*** vtable, size_t index);
void* get_hook_function(void*** vtable, size_t index);

// TODO: See if can optimize the locks to prevent having to log the global lua lock sometimes, test if everything works

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
        {
            std::lock_guard lock_lua{global_lua_lock};
            std::unique_lock lock{my_mutex};

            auto& tasks = s_Tasks[State::get().ptr()];
            auto self_heap_ptr = OnHeapPointer<void>::from_raw_ptr(self);
            if (tasks.contains(self_heap_ptr))
            {
                for (auto& task : tasks[self_heap_ptr])
                {
                    task(self);
                }
                tasks.erase(self_heap_ptr);
            }
        }
        s_OriginalDtors[*(void***)self](self, destroy);
    }

    inline static std::unordered_map<void**, VFunT*> s_OriginalDtors{};
    inline static std::unordered_map<StateMemory*, std::unordered_map<OnHeapPointer<void>, std::vector<DtorTaskT>>> s_Tasks{};
    inline static std::shared_mutex my_mutex{};
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
        void** vtable = *(void***)self;
        {
            auto self_heap_ptr = OnHeapPointer<ClassT>::from_raw_ptr(self);
            std::lock_guard lock_lua{global_lua_lock};
            my_mutex.lock_shared(); // lock, and unlock before calling cb, because the cb may set add a hook, and it would require this lock
            // std::shared_lock lock{my_mutex};

            std::unordered_map<OnHeapPointer<ClassT>, DetourFunT>& functions = s_Functions[State::get().ptr()];
            if constexpr (std::is_void_v<RetT>)
            {
                if (auto search = functions.find(self_heap_ptr); search != functions.end())
                {
                    DetourFunT fun = search->second;
                    my_mutex.unlock_shared();
                    fun(self, args..., s_Originals[vtable]);
                    return;
                }
            }
            else
            {
                if (auto search = functions.find(self_heap_ptr); search != functions.end())
                {
                    DetourFunT fun = search->second;
                    my_mutex.unlock_shared();
                    return fun(self, args..., s_Originals[vtable]);
                }
            }
            my_mutex.unlock_shared();
        }
        return s_Originals[vtable](self, std::move(args)...);
    }

    inline static std::unordered_map<void**, VFunT*> s_Originals{};
    inline static std::unordered_map<StateMemory*, std::unordered_map<OnHeapPointer<ClassT>, DetourFunT>> s_Functions{};
    inline static std::shared_mutex my_mutex{};
};

template <class HookFunT>
void hook_dtor(void* obj, HookFunT&& hook_fun, std::size_t dtor_index = 0)
{
    using DestructorDetourT = VDestructorDetour;
    using DtorT = DestructorDetourT::VFunT;
    void*** vtable = (void***)obj;
    std::unique_lock lock{DestructorDetourT::my_mutex};
    if (!get_hook_function(vtable, dtor_index))
    {
        DestructorDetourT::s_OriginalDtors[*vtable] = (DtorT*)register_hook_function(vtable, dtor_index, (void*)&DestructorDetourT::detour);
    }
    DestructorDetourT::s_Tasks[State::get().ptr()][OnHeapPointer<void>::from_raw_ptr(obj)].push_back(std::forward<HookFunT>(hook_fun));
}

template <class VTableFunT, std::size_t VTableIndex, class T, class HookFunT>
void hook_vtable_no_dtor(T* obj, HookFunT&& hook_fun)
{
    using DetourT = VTableDetour<VTableFunT, VTableIndex>;
    void*** vtable = (void***)obj;
    std::unique_lock lock{DetourT::my_mutex};
    if (!get_hook_function(vtable, VTableIndex))
    {
        DetourT::s_Originals[*vtable] = (VTableFunT*)register_hook_function(vtable, VTableIndex, (void*)&DetourT::detour);
    }
    DetourT::s_Functions[State::get().ptr()][OnHeapPointer<T>::from_raw_ptr(obj)] = std::forward<HookFunT>(hook_fun);
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
            std::unique_lock lock{DetourT::my_mutex};
            auto& functions = DetourT::s_Functions.at(State::get().ptr());
            functions.erase(OnHeapPointer<T>::from_raw_ptr((T*)self));
        },
        dtor_index);
}
