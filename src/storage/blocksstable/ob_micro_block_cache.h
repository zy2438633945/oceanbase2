/**
 * Copyright (c) 2021 OceanBase
 * OceanBase CE is licensed under Mulan PubL v2.
 * You can use this software according to the terms and conditions of the Mulan PubL v2.
 * You may obtain a copy of Mulan PubL v2 at:
 *          http://license.coscl.org.cn/MulanPubL-2.0
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PubL v2 for more details.
 */

#ifndef OCEANBASE_STORAGE_BLOCKSSTABLE_MICRO_BLOCK_CACHE_H_
#define OCEANBASE_STORAGE_BLOCKSSTABLE_MICRO_BLOCK_CACHE_H_
#include "share/io/ob_io_manager.h"
#include "share/cache/ob_kv_storecache.h"
#include "ob_block_sstable_struct.h"
#include "ob_macro_block_reader.h"
#include "index_block/ob_index_block_row_scanner.h"
#include "storage/ob_i_table.h"
#include "storage/blocksstable/ob_micro_block_info.h"
#include "storage/meta_mem/ob_tablet_handle.h"
#include "lib/stat/ob_diagnose_info.h"
#include "storage/blocksstable/ob_block_manager.h"


namespace oceanbase
{
namespace blocksstable
{
class ObIMicroBlockIOCallback;
class ObMicroBlockCacheKey : public common::ObIKVCacheKey
{
public:
  ObMicroBlockCacheKey(
      const uint64_t tenant_id,
      const MacroBlockId &macro_id,
      const int64_t offset,
      const int64_t size);
  ObMicroBlockCacheKey(
      const uint64_t tenant_id,
      const ObMicroBlockId &block_id);
  ObMicroBlockCacheKey();
  ObMicroBlockCacheKey(const ObMicroBlockCacheKey &other);
  virtual ~ObMicroBlockCacheKey();
  virtual bool operator ==(const ObIKVCacheKey &other) const;
  virtual uint64_t get_tenant_id() const;
  virtual uint64_t hash() const;
  virtual int64_t size() const;
  virtual int deep_copy(char *buf, const int64_t buf_len, ObIKVCacheKey *&key) const;
  void set(const uint64_t tenant_id,
           const MacroBlockId &block_id,
           const int64_t offset,
           const int64_t size);
  const ObMicroBlockId &get_micro_block_id() const { return block_id_; }
  TO_STRING_KV(K_(tenant_id), K_(block_id));
private:
  uint64_t tenant_id_;
  ObMicroBlockId block_id_;
};

class ObMicroBlockCacheValue : public common::ObIKVCacheValue
{
public:
  ObMicroBlockCacheValue();
  ObMicroBlockCacheValue(
      const char *buf,
      const int64_t size,
      const char *extra_buf = NULL,
      const int64_t extra_size = 0,
      const ObMicroBlockData::Type block_type = ObMicroBlockData::DATA_BLOCK);
  virtual ~ObMicroBlockCacheValue();
  virtual int64_t size() const;
  virtual int deep_copy(char *buf, const int64_t buf_len, ObIKVCacheValue *&value) const;
  inline const ObMicroBlockData& get_block_data() const { return block_data_; }
  inline ObMicroBlockData& get_block_data() { return block_data_; }
  bool need_free() const {return alloc_by_block_io_; }
  void set_alloc_by_block_io() { alloc_by_block_io_ = true; }
  TO_STRING_KV(K_(block_data));
private:
  ObMicroBlockData block_data_;
  bool alloc_by_block_io_;  // TODO: @lvling to be removed
private:
  DISALLOW_COPY_AND_ASSIGN(ObMicroBlockCacheValue);
};

class ObIMicroBlockCache;

class ObMicroBlockBufferHandle
{
public:
  ObMicroBlockBufferHandle() : micro_block_(NULL) {}
  ~ObMicroBlockBufferHandle() {}
  void reset() { micro_block_ = NULL; handle_.reset(); }
  inline const ObMicroBlockData* get_block_data() const
  { return is_valid() ? &(micro_block_->get_block_data()) : NULL; }
  int64_t get_block_size() const { return is_valid() ? micro_block_->get_block_data().total_size() : 0; }
  inline bool is_valid() const { return NULL != micro_block_ && handle_.is_valid(); }
  TO_STRING_KV(K_(handle), KP_(micro_block));
private:
  friend class ObIMicroBlockCache;
  common::ObKVCacheHandle handle_;
  const ObMicroBlockCacheValue *micro_block_;
};

struct ObMultiBlockIOResult
{
  ObMultiBlockIOResult();
  virtual ~ObMultiBlockIOResult();

  int get_block_data(const int64_t index, ObMicroBlockData &block_data) const;
  void reset();
  const ObMicroBlockCacheValue **micro_blocks_;
  common::ObKVCacheHandle *handles_;
  int64_t block_count_;
  int ret_code_;
};

struct ObMultiBlockIOParam
{
  ObMultiBlockIOParam() { reset(); }
  virtual ~ObMultiBlockIOParam() {}
  void reset();
  bool is_valid() const;
  inline void get_io_range(int64_t &offset, int64_t &size) const;
  inline int get_block_des_info(ObIMicroBlockIOCallback &des_meta) const;
  TO_STRING_KV(KPC(micro_index_infos_), K_(start_index), K_(block_count));
  common::ObIArray<ObMicroIndexInfo> *micro_index_infos_;
  int64_t start_index_;
  int64_t block_count_;
};

struct ObMultiBlockIOCtx
{
  ObMultiBlockIOCtx()
    : micro_index_infos_(nullptr), hit_cache_bitmap_(nullptr), block_count_(0) {}
  virtual ~ObMultiBlockIOCtx() {}
  void reset();
  bool is_valid() const;
  ObMicroIndexInfo *micro_index_infos_;
  bool *hit_cache_bitmap_;
  int64_t block_count_;
  TO_STRING_KV(KP_(micro_index_infos), KP_(hit_cache_bitmap), K_(block_count));
};

class ObIPutSizeStat
{
public:
  ObIPutSizeStat() {}
  virtual ~ObIPutSizeStat() {}
  virtual int add_put_size(const int64_t put_size) = 0;
};

// New Block IO Callbacks for version 4.0
class ObIMicroBlockIOCallback : public common::ObIOCallback
{
public:
  ObIMicroBlockIOCallback();
  virtual ~ObIMicroBlockIOCallback();
  virtual int alloc_data_buf(const char *io_data_buffer, const int64_t data_size);
  virtual ObIAllocator *get_allocator() { return allocator_; }
  void set_micro_des_meta(const ObIndexBlockRowHeader *idx_row_header);
protected:
  friend class ObIMicroBlockCache;
  friend class ObDataMicroBlockCache;
  int process_block(
      ObMacroBlockReader *reader,
      const char *buffer,
      const int64_t offset,
      const int64_t size,
      const ObMicroBlockCacheValue *&micro_block,
      common::ObKVCacheHandle &cache_handle);
private:
  int read_block_and_copy(
      const ObMicroBlockHeader &header,
      ObMacroBlockReader &reader,
      const char *buffer,
      const int64_t size,
      ObMicroBlockData &block_data,
      const ObMicroBlockCacheValue *&micro_block,
      common::ObKVCacheHandle &handle);
  static const int64_t ALLOC_BUF_RETRY_INTERVAL = 100 * 1000;
  static const int64_t ALLOC_BUF_RETRY_TIMES = 3;
protected:
  ObIMicroBlockCache *cache_;
  ObIPutSizeStat *put_size_stat_;
  common::ObIAllocator *allocator_;
  char *data_buffer_;
  uint64_t tenant_id_;
  MacroBlockId block_id_;
  int64_t offset_;
  ObMicroBlockDesMeta block_des_meta_;
  bool use_block_cache_;
  char encrypt_key_[share::OB_MAX_TABLESPACE_ENCRYPT_KEY_LENGTH];
  DISALLOW_COPY_AND_ASSIGN(ObIMicroBlockIOCallback);
};

class ObAsyncSingleMicroBlockIOCallback : public ObIMicroBlockIOCallback
{
public:
  ObAsyncSingleMicroBlockIOCallback();
  virtual ~ObAsyncSingleMicroBlockIOCallback();
  virtual int64_t size() const;
  virtual int inner_process(const char *data_buffer, const int64_t size) override;
  virtual const char *get_data() override;
  TO_STRING_KV("callback_type:", "ObAsyncSingleMicroBlockIOCallback", KP_(micro_block), K_(cache_handle), K_(offset), K_(block_des_meta));
private:
  DISALLOW_COPY_AND_ASSIGN(ObAsyncSingleMicroBlockIOCallback);
  friend class ObIMicroBlockCache;
  // Notice: lifetime shoule be longer than AIO or deep copy here
  const ObMicroBlockCacheValue *micro_block_;
  common::ObKVCacheHandle cache_handle_;
};

class ObMultiDataBlockIOCallback : public ObIMicroBlockIOCallback
{
public:
  ObMultiDataBlockIOCallback();
  virtual ~ObMultiDataBlockIOCallback();
  virtual int64_t size() const;
  virtual int inner_process(const char *data_buffer, const int64_t size) override;
  virtual const char *get_data() override;
  TO_STRING_KV("callback_type:", "ObMultiDataBlockIOCallback", K_(io_ctx), K_(offset));
private:
  friend class ObDataMicroBlockCache;
  int set_io_ctx(const ObMultiBlockIOParam &io_param);
  void reset_io_ctx() { io_ctx_.reset(); }
  int deep_copy_ctx(const ObMultiBlockIOCtx &io_ctx);
  int alloc_result();
  void free_result();
  DISALLOW_COPY_AND_ASSIGN(ObMultiDataBlockIOCallback);
  ObMultiBlockIOCtx io_ctx_;
  ObMultiBlockIOResult io_result_;
};

class ObSyncSingleMicroBLockIOCallback : public ObIMicroBlockIOCallback
{
public:
  ObSyncSingleMicroBLockIOCallback();
  virtual ~ObSyncSingleMicroBLockIOCallback();
  virtual int64_t size() const;
  virtual int inner_process(const char *data_buffer, const int64_t size) override;
  virtual const char *get_data() override;
  TO_STRING_KV("callback_type:", "ObSyncSingleMicroBLockIOCallback", KP_(macro_reader), KP_(block_data), K_(is_data_block));
  DISALLOW_COPY_AND_ASSIGN(ObSyncSingleMicroBLockIOCallback);
protected:
  friend class ObDataMicroBlockCache;
  friend class ObIndexMicroBlockCache;
  ObMacroBlockReader *macro_reader_;
  ObMicroBlockData *block_data_;
  bool is_data_block_;
};

class ObMicroBlockBufTransformer final
{
 public:
   ObMicroBlockBufTransformer(const ObMicroBlockDesMeta &block_des_meta,
                              ObMacroBlockReader *reader,
                              ObMicroBlockHeader &header,
                              const char *buf,
                              const int64_t buf_size);
   int init();
   int get_buf_size(int64_t &buf_size) const;
   int transfrom(char *block_buf, const int64_t buf_size);

 private:
   bool is_inited_;
   bool is_cs_full_transfrom_;
   const ObMicroBlockDesMeta &block_des_meta_;
   ObMacroBlockReader *reader_;
   ObMicroBlockHeader &header_;
   const char *payload_buf_;
   int64_t payload_size_;
   ObCSMicroBlockTransformer transformer_;
};


class ObIMicroBlockCache : public ObIPutSizeStat
{
public:
  typedef common::ObIKVCache<ObMicroBlockCacheKey, ObMicroBlockCacheValue> BaseBlockCache;
  ObIMicroBlockCache() {}
  virtual ~ObIMicroBlockCache() {}
  int get_cache_block(
      const uint64_t tenant_id,
      const MacroBlockId block_id,
      const int64_t offset,
      const int64_t size,
      ObMicroBlockBufferHandle &handle);
  int prefetch(
      const uint64_t tenant_id,
      const MacroBlockId &macro_id,
      const ObMicroIndexInfo& idx_row,
      const bool use_cache,
      ObMacroBlockHandle &macro_handle,
      ObIAllocator *allocator);
  virtual int load_block(
      const ObMicroBlockId &micro_block_id,
      const ObMicroBlockDesMeta &des_meta,
      ObMacroBlockReader *macro_reader,
      ObMicroBlockData &block_data,
      ObIAllocator *allocator) = 0;
  virtual void destroy() = 0;
  virtual int get_cache(BaseBlockCache *&cache) = 0;
  virtual int get_allocator(common::ObIAllocator *&allocator) = 0;
  virtual int put_cache_block(
      const ObMicroBlockDesMeta &des_meta,
      const char *raw_block_buf,
      const ObMicroBlockCacheKey &key,
      ObMacroBlockReader &reader,
      ObIAllocator &allocator,
      const ObMicroBlockCacheValue *&micro_block,
      common::ObKVCacheHandle &cache_handle) = 0;
  virtual int reserve_kvpair(
      const ObMicroBlockDesc &micro_block_desc,
      ObKVCacheInstHandle &inst_handle,
      ObKVCacheHandle &cache_handle,
      ObKVCachePair *&kvpair,
      int64_t &kvpair_size) = 0;
  virtual ObMicroBlockData::Type get_type() = 0;
  virtual int add_put_size(const int64_t put_size) override;

protected:
  int prefetch(
      const uint64_t tenant_id,
      const MacroBlockId &macro_id,
      const ObMicroIndexInfo& idx_row,
      ObMacroBlockHandle &macro_handle,
      ObIMicroBlockIOCallback &callback);
  int prefetch(
      const uint64_t tenant_id,
      const MacroBlockId &macro_id,
      const ObMultiBlockIOParam &io_param,
      const bool use_cache,
      ObMacroBlockHandle &macro_handle,
      ObIMicroBlockIOCallback &callback);
};

class ObDataMicroBlockCache
  : public common::ObKVCache<ObMicroBlockCacheKey, ObMicroBlockCacheValue>,
    public ObIMicroBlockCache
{
public:
  ObDataMicroBlockCache() {}
  virtual ~ObDataMicroBlockCache() {}
  int init(const char *cache_name, const int64_t priority = 1);
  virtual void destroy() override;
  using ObIMicroBlockCache::prefetch;
  int prefetch(
      const uint64_t tenant_id,
      const MacroBlockId &macro_id,
      const ObMultiBlockIOParam &io_param,
      const bool use_cache,
      ObMacroBlockHandle &macro_handle);
  int load_block(
      const ObMicroBlockId &micro_block_id,
      const ObMicroBlockDesMeta &des_meta,
      ObMacroBlockReader *macro_reader,
      ObMicroBlockData &block_data,
      ObIAllocator *allocator) override;
  virtual int get_cache(BaseBlockCache *&cache) override;
  virtual int get_allocator(common::ObIAllocator *&allocator) override;
  virtual int put_cache_block(
      const ObMicroBlockDesMeta &des_meta,
      const char *raw_block_buf,
      const ObMicroBlockCacheKey &key,
      ObMacroBlockReader &reader,
      ObIAllocator &allocator,
      const ObMicroBlockCacheValue *&micro_block,
      common::ObKVCacheHandle &cache_handle) override;
  virtual int reserve_kvpair(
      const ObMicroBlockDesc &micro_block_desc,
      ObKVCacheInstHandle &inst_handle,
      ObKVCacheHandle &cache_handle,
      ObKVCachePair *&kvpair,
      int64_t &kvpair_size) override;
  virtual ObMicroBlockData::Type get_type() override;
private:
  int64_t calc_value_size(const int64_t data_length, const ObRowStoreType &type, bool &need_decoder);
  int write_extra_buf(
      const ObRowStoreType row_store_type,
      const char *block_buf,
      const int64_t block_size,
      char *extra_buf,
      ObMicroBlockData &micro_data);
private:
  common::ObConcurrentFIFOAllocator allocator_;
  DISALLOW_COPY_AND_ASSIGN(ObDataMicroBlockCache);
};

class ObIndexMicroBlockCache : public ObDataMicroBlockCache
{
public:
  ObIndexMicroBlockCache();
  virtual ~ObIndexMicroBlockCache();
  int init(const char *cache_name, const int64_t priority = 10);
  int load_block(
      const ObMicroBlockId &micro_block_id,
      const ObMicroBlockDesMeta &des_meta,
      ObMacroBlockReader *macro_reader,
      ObMicroBlockData &block_data,
      ObIAllocator *allocator) override;
  virtual int put_cache_block(
      const ObMicroBlockDesMeta &des_meta,
      const char *raw_block_buf,
      const ObMicroBlockCacheKey &key,
      ObMacroBlockReader &reader,
      ObIAllocator &allocator,
      const ObMicroBlockCacheValue *&micro_block,
      common::ObKVCacheHandle &cache_handle) override;
  virtual int reserve_kvpair(
      const ObMicroBlockDesc &micro_block_desc,
      ObKVCacheInstHandle &inst_handle,
      ObKVCacheHandle &cache_handle,
      ObKVCachePair *&kvpair,
      int64_t &kvpair_size) override
  {
    // not support pre-reserve kvpair interface for index block
    return OB_NOT_SUPPORTED;
  }
  virtual ObMicroBlockData::Type get_type() override;
};


}//end namespace blocksstable
}//end namespace oceanbase
#endif
