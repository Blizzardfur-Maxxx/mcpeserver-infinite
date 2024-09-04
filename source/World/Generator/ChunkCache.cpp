#include "ChunkCache.hpp"
#include <stdexcept>
#include <iostream>  // Added for std::cerr

ChunkCache::ChunkCache(Level* level, ChunkStorage* storage, ChunkSource* source)
    : m_pLevel(level), m_pChunkStorage(storage), m_pChunkSource(source),
    m_pLastChunk(nullptr), m_LastChunkX(-999999999), m_LastChunkZ(-999999999) {
    m_pEmptyChunk = new EmptyLevelChunk(level, nullptr, 0, 0);
    memset(m_chunkMap, 0, sizeof(m_chunkMap));
}

ChunkCache::~ChunkCache() {
    delete m_pChunkSource;
    delete m_pEmptyChunk;
    for (int i = 0; i < 32; ++i) {
        for (int j = 0; j < 32; ++j) {
            delete m_chunkMap[i][j];
        }
    }
}

LevelChunk* ChunkCache::create(int x, int z) {
    return getChunk(x, z);
}

LevelChunk* ChunkCache::getChunk(int x, int z) {
    if (x == m_LastChunkX && z == m_LastChunkZ && m_pLastChunk) {
        return m_pLastChunk;
    }

    if (!fits(x, z)) {
        return m_pEmptyChunk;
    }

    int chunkX = x & (CHUNK_CACHE_WIDTH - 1);
    int chunkZ = z & (CHUNK_CACHE_WIDTH - 1);
    int index = chunkX + chunkZ * CHUNK_CACHE_WIDTH;

    if (!hasChunk(x, z)) {
        if (m_chunkMap[chunkZ][chunkX]) {
            m_chunkMap[chunkZ][chunkX]->unload();
            save(m_chunkMap[chunkZ][chunkX]);
        }

        LevelChunk* chunk = load(x, z);
        if (!chunk) {
            if (m_pChunkSource) {
                chunk = m_pChunkSource->getChunk(x, z);
            }
            else {
                chunk = m_pEmptyChunk;
            }
        }

        m_chunkMap[chunkZ][chunkX] = chunk;
        chunk->lightLava();

        if (chunk) {
            chunk->load();
        }

        if (!chunk->field_234 && hasChunk(x + 1, z + 1) && hasChunk(x, z + 1) && hasChunk(x + 1, z)) {
            postProcess(this, x, z);
        }

        if (hasChunk(x - 1, z) && !getChunk(x - 1, z)->field_234 && hasChunk(x - 1, z + 1) && hasChunk(x, z + 1) && hasChunk(x - 1, z)) {
            postProcess(this, x - 1, z);
        }

        if (hasChunk(x, z - 1) && !getChunk(x, z - 1)->field_234 && hasChunk(x + 1, z - 1) && hasChunk(x + 1, z) && hasChunk(x, z - 1)) {
            postProcess(this, x, z - 1);
        }

        if (hasChunk(x - 1, z - 1) && !getChunk(x - 1, z - 1)->field_234 && hasChunk(x - 1, z - 1) && hasChunk(x, z - 1) && hasChunk(x - 1, z)) {
            postProcess(this, x - 1, z - 1);
        }
    }

    m_LastChunkX = x;
    m_LastChunkZ = z;
    m_pLastChunk = m_chunkMap[chunkZ][chunkX];
    return m_chunkMap[chunkZ][chunkX];
}

std::vector<LevelChunk*> ChunkCache::getLoadedChunks() {
    std::vector<LevelChunk*> loadedChunks;

    for (int i = 0; i < CHUNK_CACHE_WIDTH; ++i) {
        for (int j = 0; j < CHUNK_CACHE_WIDTH; ++j) {
            LevelChunk* chunk = m_chunkMap[i][j];
            if (chunk && chunk != m_pEmptyChunk) {
                loadedChunks.push_back(chunk);
            }
        }
    }

    return loadedChunks;
}

bool ChunkCache::hasChunk(int x, int z) {
    if (!fits(x, z)) {
        return false;
    }

    if (x == m_LastChunkX && z == m_LastChunkZ) {
        return true;
    }

    int chunkX = x & (CHUNK_CACHE_WIDTH - 1);
    int chunkZ = z & (CHUNK_CACHE_WIDTH - 1);
    LevelChunk* chunk = m_chunkMap[chunkZ][chunkX];

    if (chunk == nullptr) {
        return false;
    }

    if (chunk == m_pEmptyChunk) {
        return true;
    }

    return chunk->isAt(x, z);
}

LevelChunk* ChunkCache::load(int x, int z) {
    if (!m_pChunkStorage) {
        return m_pEmptyChunk;
    }

    try {
        LevelChunk* chunk = m_pChunkStorage->load(m_pLevel, x, z);
        if (chunk) {
            chunk->field_23C = m_pLevel->getTime();
        }
        return chunk;
    }
    catch (const std::exception& e) {
        e.what();  // Log the error
        return m_pEmptyChunk;
    }
}

void ChunkCache::save(LevelChunk* chunk) {
    if (m_pChunkStorage) {
        try {
            m_pChunkStorage->save(m_pLevel, chunk); // Save the chunk to storage
        }
        catch (const std::exception& e) {
            // Handle exceptions here
            std::cerr << "Error saving chunk: " << e.what() << std::endl;
        }
    }
}


void ChunkCache::postProcess(ChunkSource* pChkSrc, int x, int z)
{
    /*if (x < 0 || z < 0 || x >= C_MAX_CHUNKS_X || z >= C_MAX_CHUNKS_Z)
        return;*/

    LevelChunk* pChunk = getChunk(x, z);
    if (!pChunk->field_234)
    {
        pChunk->field_234 = 1;
        if (m_pChunkSource)
        {
            m_pChunkSource->postProcess(m_pChunkSource, x, z);
            pChunk->clearUpdateMap();
        }
    }
}

bool ChunkCache::shouldSave() {
    return true;
}

void ChunkCache::saveAll() {
    if (m_pChunkStorage) {
        int chunksToSave = 0;
        int savedChunks = 0;

        // Count chunks that need saving
        for (int i = 0; i < 32; ++i) {
            for (int j = 0; j < 32; ++j) {
                if (m_chunkMap[i][j] && m_chunkMap[i][j]->shouldSave(true)) {
                    ++chunksToSave;
                }
            }
        }

        for (int i = 0; i < 32; ++i) {
            for (int j = 0; j < 32; ++j) {
                if (m_chunkMap[i][j]) {
                    if (m_chunkMap[i][j]->shouldSave(true)) {
                        save(m_chunkMap[i][j]);
                        ++savedChunks;
                        if (savedChunks >= 2) {
                            return; // Limit to 2 chunks saved if not forced
                        }
                    }
                }
            }
        }

        // Optionally flush storage if saving is complete
        if (m_pChunkStorage) {
            m_pChunkStorage->flush();
        }
    }
}

#ifdef ENH_IMPROVED_SAVING

void ChunkCache::saveUnsaved()
{
    if (!m_pChunkStorage) return;

    std::vector<LevelChunk*> chunksToSave;

    for (int i = 0; i < CHUNK_CACHE_WIDTH; i++)
    {
        for (int j = 0; j < CHUNK_CACHE_WIDTH; j++)
        {
            LevelChunk* pChunk = m_chunkMap[i][j];
            if (pChunk && pChunk != m_pEmptyChunk && pChunk->m_bUnsaved)
            {
                chunksToSave.push_back(pChunk);
            }
        }
    }
    m_pChunkStorage->saveAll(m_pLevel, chunksToSave);
}

#endif



int ChunkCache::tick() {
    if (m_pChunkStorage) {
        m_pChunkStorage->tick();
    }
    return m_pChunkSource->tick();
}

std::string ChunkCache::gatherStats() {
    // Implement this based on your needs
    return "ChunkCache Stats";
}

bool ChunkCache::fits(int x, int z) const {
    return true;
}
