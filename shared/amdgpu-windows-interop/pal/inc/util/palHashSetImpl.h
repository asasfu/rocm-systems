/* Copyright (c) Advanced Micro Devices, Inc., or its affiliates. All rights reserved. */
/**
 ***********************************************************************************************************************
 * @file  palHashSetImpl.h
 * @brief PAL utility collection HashSet class implementation.
 ***********************************************************************************************************************
 */

#pragma once

#include "palHashBaseImpl.h"
#include "palHashSet.h"

namespace Util
{

// =====================================================================================================================
// Inserts a key if it doesn't already exist.
template<typename Key,
         typename Allocator,
         template<typename> class HashFunc,
         template<typename> class EqualFunc,
         typename AllocFunc,
         size_t GroupSize>
Result HashSet<Key, Allocator, HashFunc, EqualFunc, AllocFunc, GroupSize>::Insert(
    const Key& key)
{
    Entry* pEntry = nullptr;
    bool   existed = false;
    return Base::FindAllocateEntry(key, &existed, &pEntry);
}

// =====================================================================================================================
// Finds a given entry; if no entry was found, allocate it.
template<typename Key,
         typename Allocator,
         template<typename> class HashFunc,
         template<typename> class EqualFunc,
         typename AllocFunc,
         size_t GroupSize>
Result HashSet<Key, Allocator, HashFunc, EqualFunc, AllocFunc, GroupSize>::FindAllocate(
    Key** ppKey,
    bool* pExisted)
{
    PAL_ASSERT(ppKey != nullptr);
    PAL_ASSERT(pExisted != nullptr);

    static_assert(offsetof(Entry, key) == 0);
    return Base::FindAllocateEntry(**ppKey, pExisted, reinterpret_cast<Entry**>(ppKey));
}


} // Util
