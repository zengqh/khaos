#pragma once

// 内存分配方案
#define KHAOS_MEMORY_ALLOCATOR_STD      1
#define KHAOS_MEMORY_ALLOCATOR_DEFAULT  2
#define KHAOS_MEMORY_ALLOCATOR_USER     3

#ifndef KHAOS_MEMORY_ALLOCATOR
#define KHAOS_MEMORY_ALLOCATOR  KHAOS_MEMORY_ALLOCATOR_STD
#endif

// stl容器是否使用自定义内存分配方案
#ifndef KHAOS_CONTAINERS_USE_CUSTOM_MEMORY_ALLOCATOR
#define KHAOS_CONTAINERS_USE_CUSTOM_MEMORY_ALLOCATOR 1
#endif

// stl string是否使用自定义内存分配方案
#ifndef KHAOS_STRING_USE_CUSTOM_MEMORY_ALLOCATOR
#define KHAOS_STRING_USE_CUSTOM_MEMORY_ALLOCATOR 1
#endif

// 是否开启内存泄露跟踪
#ifndef KHAOS_MEMORY_TRACKER_DEBUG_MODE
#define KHAOS_MEMORY_TRACKER_DEBUG_MODE 1
#endif

#ifndef KHAOS_MEMORY_TRACKER_RELEASE_MODE
#define KHAOS_MEMORY_TRACKER_RELEASE_MODE 0
#endif

// 使用的渲染api
#define KHAOS_RENDER_SYSTEM_D3D9    1
#define KHAOS_RENDER_SYSTEM_OPENGL  2
#define KHAOS_RENDER_SYSTEM_GLES    3

#ifndef KHAOS_RENDER_SYSTEM
#define KHAOS_RENDER_SYSTEM KHAOS_RENDER_SYSTEM_D3D9
#endif

