/*
 * Copyright (c) 2014 - 2019 Jolla Ltd.
 *
 * All rights reserved.
 *
 * This file is part of Sailfish Transfer Engine package.
 *
 * You may use this file under the terms of the GNU Lesser General
 * Public License version 2.1 as published by the Free Software Foundation
 * and appearing in the file license.lgpl included in the packaging
 * of this file.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation
 * and appearing in the file license.lgpl included in the packaging
 * of this file.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 */

#ifndef SYNCHRONIZELISTS_P_H
#define SYNCHRONIZELISTS_P_H

template <typename T>
bool compareIdentity(const T &item, const T &reference)
{
    return item == reference;
}

template <typename Agent, typename ReferenceList>
int insertRange(Agent *agent, int index, int count, const ReferenceList &source, int sourceIndex)
{
    agent->insertRange(index, count, source, sourceIndex);
    return count;
}

template <typename Agent>
int removeRange(Agent *agent, int index, int count)
{
    agent->removeRange(index, count);
    return 0;
}

template <typename Agent, typename ReferenceList>
int updateRange(Agent *agent, int index, int count, const ReferenceList &source, int sourceIndex)
{
    agent->updateRange(index, count, source, sourceIndex);
    return count;
}

template <typename Agent, typename CacheList, typename ReferenceList>
class SynchronizeList
{
public:
    SynchronizeList(
            Agent *agent,
            const CacheList &cache,
            int &c,
            const ReferenceList &reference,
            int &r)
        : agent(agent), cache(cache), c(c), reference(reference), r(r)
    {
        int lastEqualC = c;
        int lastEqualR = r;
        for (; c < cache.count() && r < reference.count(); ++c, ++r) {
            if (compareIdentity(cache.at(c), reference.at(r))) {
                continue;
            }

            if (c > lastEqualC) {
                lastEqualC += updateRange(agent, lastEqualC, c - lastEqualC, reference, lastEqualR);
                c = lastEqualC;
                lastEqualR = r;
            }

            bool match = false;

            // Iterate through both the reference and cache lists in parallel looking for first
            // point of commonality, when that is found resolve the differences and continue
            // looking.
            int count = 1;
            for (; !match && c + count < cache.count() && r + count < reference.count(); ++count) {
                typename CacheList::const_reference cacheItem = cache.at(c + count);
                typename ReferenceList::const_reference referenceItem = reference.at(r + count);

                for (int i = 0; i <= count; ++i) {
                    if (cacheMatch(i, count, referenceItem) || referenceMatch(i, count, cacheItem)) {
                        match = true;
                        break;
                    }
                }
            }

            // Continue scanning the reference list if the cache has been exhausted.
            for (int re = r + count; !match && re < reference.count(); ++re) {
                typename ReferenceList::const_reference referenceItem = reference.at(re);
                for (int i = 0; i < count; ++i) {
                    if (cacheMatch(i, re - r, referenceItem)) {
                        match = true;
                        break;
                    }
                }
            }

            // Continue scanning the cache if the reference list has been exhausted.
            for (int ce = c + count; !match && ce < cache.count(); ++ce) {
                typename CacheList::const_reference cacheItem = cache.at(ce);
                for (int i = 0; i < count; ++i) {
                    if (referenceMatch(i, ce - c, cacheItem)) {
                        match = true;
                        break;
                    }
                }
            }

            if (!match)
                return;

            lastEqualC = c;
            lastEqualR = r;
        }

        if (c > lastEqualC) {
            updateRange(agent, lastEqualC, c - lastEqualC, reference, lastEqualR);
        }
    }

private:
    // Tests if the cached id at i matches a referenceId.
    // If there is a match removes all items traversed in the cache since the previous match
    // and inserts any items in the reference set found to to not be in the cache.
    bool cacheMatch(int i, int count, typename ReferenceList::const_reference referenceItem)
    {
        if (compareIdentity(cache.at(c + i),  referenceItem)) {
            if (i > 0)
                c += removeRange(agent, c, i);
            c += insertRange(agent, c, count, reference, r);
            r += count;
            return true;
        } else {
            return false;
        }
    }

    // Tests if the referenceid at i matches a cacheId.
    // If there is a match inserts all items traversed in the reference set since the
    // previous match and removes any items from the cache that were not found in the
    // reference list.
    bool referenceMatch(int i, int count, typename ReferenceList::const_reference cacheItem)
    {
        if (compareIdentity(reference.at(r + i), cacheItem)) {
            c += removeRange(agent, c, count);
            if (i > 0)
                c += insertRange(agent, c, i, reference, r);
            r += i;
            return true;
        } else {
            return false;
        }
    }

    Agent *const agent = nullptr;
    const CacheList &cache;
    int &c;
    const ReferenceList &reference;
    int &r;
};

template <typename Agent, typename CacheList, typename ReferenceList>
void completeSynchronizeList(
        Agent *agent,
        const CacheList &cache,
        int &cacheIndex,
        const ReferenceList &reference,
        int &referenceIndex)
{
    if (cacheIndex < cache.count()) {
        agent->removeRange(cacheIndex, cache.count() - cacheIndex);
    }
    if (referenceIndex < reference.count()) {
        agent->insertRange(cache.count(), reference.count() - referenceIndex, reference, referenceIndex);
    }

    cacheIndex = 0;
    referenceIndex = 0;
}

template <typename Agent, typename CacheList, typename ReferenceList>
void synchronizeList(
        Agent *agent,
        const CacheList &cache,
        int &cacheIndex,
        const ReferenceList &reference,
        int &referenceIndex)
{
    SynchronizeList<Agent, CacheList, ReferenceList>(
                agent, cache, cacheIndex, reference, referenceIndex);
}

template <typename Agent, typename CacheList, typename ReferenceList>
void synchronizeList(Agent *agent, const CacheList &cache, const ReferenceList &reference)
{
    int cacheIndex = 0;
    int referenceIndex = 0;
    SynchronizeList<Agent, CacheList, ReferenceList>(
                agent, cache, cacheIndex, reference, referenceIndex);
    completeSynchronizeList(agent, cache, cacheIndex, reference, referenceIndex);
}

#endif
