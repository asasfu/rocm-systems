/* Copyright (c) Advanced Micro Devices, Inc., or its affiliates. All rights reserved. */
/**
 ***********************************************************************************************************************
 * @file  palHashMapImpl.h
 * @brief PAL utility collection HashMap class implementation.
 ***********************************************************************************************************************
 */

#pragma once

#include "palHashBaseImpl.h"
#include "palHashMap.h"

namespace Util
{

// =====================================================================================================================
// Gets a pointer to the value that matches the key.  If the key is not present, a pointer to empty space for the value
// is returned.
template<typename Key,
         typename Value,
         typename Allocator,
         template<typename> class HashFunc,
         template<typename> class EqualFunc,
         typename AllocFunc,
         size_t GroupSize>
Result HashMap<Key, Value, Allocator, HashFunc, EqualFunc, AllocFunc, GroupSize>::FindAllocate(
    const Key& key,       // Key to search for.
    bool*      pExisted,  // [out] True if a matching key was found.
    Value**    ppValue)   // [out] Pointer to the value entry of the hash map's entry for the specified key.
{
    PAL_ASSERT(pExisted != nullptr);
    PAL_ASSERT(ppValue != nullptr);

    Entry* pEntry = nullptr;
    Result result = Base::FindAllocateEntry(key, pExisted, &pEntry);
    if (result == Result::Success)
    {
        *ppValue = &pEntry->value;
    }

    return result;
}

// =====================================================================================================================
// Gets a pointer to the value that matches the key.  Returns null if no entry is present matching the specified key.
template<typename Key,
         typename Value,
         typename Allocator,
         template<typename> class HashFunc,
         template<typename> class EqualFunc,
         typename AllocFunc,
         size_t GroupSize>
Value* HashMap<Key, Value, Allocator, HashFunc, EqualFunc, AllocFunc, GroupSize>::FindKey(
    const Key& key
    ) const
{
    Entry* pEntry = Base::FindEntry(key);
    return (pEntry != nullptr) ? &pEntry->value : nullptr;
}

// =====================================================================================================================
// Inserts a key/value pair entry if it doesn't already exist.
template<typename Key,
         typename Value,
         typename Allocator,
         template<typename> class HashFunc,
         template<typename> class EqualFunc,
         typename AllocFunc,
         size_t GroupSize>
Result HashMap<Key, Value, Allocator, HashFunc, EqualFunc, AllocFunc, GroupSize>::Insert(
    const Key&   key,
    const Value& value)
{
    bool   existed = true;
    Entry* pEntry  = nullptr;

    Result result = Base::FindAllocateEntry(key, &existed, &pEntry);

    // Add the new value if it did not exist already. If FindAllocate returns Success, pValue != nullptr.
    if ((result == Result::Success) && (existed == false))
    {
        pEntry->value = value;
    }

    PAL_ASSERT(result == Result::Success);

    return result;
}

} // Util
