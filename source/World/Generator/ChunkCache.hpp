#pragma once

#include <vector>
#include <string>
#include <cstring>
#include "ChunkSource.hpp"
#include "Level.hpp"
#include "LevelChunk.hpp"

class ChunkCache : public ChunkSource {
public:
    ChunkCache(Level* level, ChunkStorage* storage, ChunkSource* source);
    ~ChunkCache() override;

    LevelChunk* create(int x, int z) override;
    LevelChunk* getChunk(int x, int z) override;
    bool hasChunk(int x, int z) override;
    std::string gatherStats() override;
    void postProcess(ChunkSource* source, int x, int z) override;
    bool shouldSave() override;
    void saveAll() override;
    int tick() override;
#ifdef ENH_IMPROVED_SAVING
    void saveUnsaved() override;
#endif
    std::vector<LevelChunk*> getLoadedChunks();

private:
    LevelChunk* load(int x, int z);
    void save(LevelChunk* chunk);
    bool fits(int x, int z) const;  // Added declaration

    Level* m_pLevel;
    ChunkStorage* m_pChunkStorage;
    ChunkSource* m_pChunkSource;
    LevelChunk* m_pEmptyChunk;
    LevelChunk* m_chunkMap[32][32];  // Adjust the size as needed
    LevelChunk* m_pLastChunk;
    int m_LastChunkX;
    int m_LastChunkZ;
    static const int CHUNK_CACHE_WIDTH = 32;
};
