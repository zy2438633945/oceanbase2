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

#define USING_LOG_PREFIX SERVER

#include "observer/table_load/ob_table_load_table_compactor.h"
#include "observer/table_load/ob_table_load_general_table_compactor.h"
#include "observer/table_load/ob_table_load_merger.h"
#include "storage/direct_load/ob_direct_load_i_table.h"
#include "observer/table_load/ob_table_load_store_ctx.h"
#include "observer/table_load/ob_table_load_table_ctx.h"
#include "observer/table_load/ob_table_load_mem_compactor.h"
#include "storage/direct_load/ob_direct_load_external_table.h"
#include "observer/table_load/ob_table_load_multiple_heap_table_compactor.h"

namespace oceanbase
{
namespace observer
{
using namespace common;
using namespace storage;

/**
 * ObTableLoadTableCompactResult
 */

ObTableLoadTableCompactResult::ObTableLoadTableCompactResult()
  : allocator_("TLD_TCResult"), tablet_result_map_(64)
{
  allocator_.set_tenant_id(MTL_ID());
  all_table_array_.set_tenant_id(MTL_ID());
}

ObTableLoadTableCompactResult::~ObTableLoadTableCompactResult()
{
  reset();
}

void ObTableLoadTableCompactResult::reset()
{
  for (int64_t i = 0; i < all_table_array_.count(); ++i) {
    ObIDirectLoadPartitionTable *table = all_table_array_.at(i);
    table->~ObIDirectLoadPartitionTable();
    allocator_.free(table);
  }
  all_table_array_.reset();
  allocator_.reset();
  tablet_result_map_.reset();
}

int ObTableLoadTableCompactResult::init()
{
  int ret = OB_SUCCESS;
  if (OB_FAIL(tablet_result_map_.init("TLD_TCResult", MTL_ID()))) {
    LOG_WARN("fail to init link hash map", KR(ret));
  }
  return ret;
}

int ObTableLoadTableCompactResult::add_table(ObIDirectLoadPartitionTable *table)
{
  int ret = OB_SUCCESS;
  if (OB_UNLIKELY(nullptr == table)) {
    ret = OB_INVALID_ARGUMENT;
    LOG_WARN("invalid args", KR(ret), KP(table));
  } else {
    const ObTabletID &tablet_id = table->get_tablet_id();
    ObTableLoadTableCompactTabletResult *tablet_result = nullptr;
    if (OB_FAIL(tablet_result_map_.get(tablet_id, tablet_result))) {
      if (OB_UNLIKELY(OB_ENTRY_NOT_EXIST != ret)) {
        LOG_WARN("fail to get", KR(ret));
      } else {
        if (OB_FAIL(tablet_result_map_.create(tablet_id, tablet_result))) {
          LOG_WARN("fail to create", KR(ret));
        }
      }
    }
    if (OB_SUCC(ret)) {
      if (OB_FAIL(tablet_result->table_array_.push_back(table))) {
        LOG_WARN("fail to push back", KR(ret));
      } else if (OB_FAIL(all_table_array_.push_back(table))) {
        LOG_WARN("fail to push back", KR(ret));
      }
    }
    if (OB_NOT_NULL(tablet_result)) {
      tablet_result_map_.revert(tablet_result);
    }
  }
  return ret;
}

void ObTableLoadTableCompactResult::release_all_table_data()
{
  for (int64_t i = 0; i < all_table_array_.count(); ++i) {
    ObIDirectLoadPartitionTable *table = all_table_array_.at(i);
    table->release_data();
  }
}

/**
 * ObTableLoadTableCompactCtx
 */

ObTableLoadTableCompactCtx::ObTableLoadTableCompactCtx()
  : store_ctx_(nullptr), merger_(nullptr), compactor_(nullptr)
{
}

ObTableLoadTableCompactCtx::~ObTableLoadTableCompactCtx()
{
  release_compactor();
}

int ObTableLoadTableCompactCtx::init(ObTableLoadStoreCtx *store_ctx, ObTableLoadMerger &merger)
{
  int ret = OB_SUCCESS;
  if (OB_UNLIKELY(nullptr == store_ctx)) {
    ret = OB_INVALID_ARGUMENT;
    LOG_WARN("invalid args", KR(ret), KP(store_ctx));
  } else {
    if (OB_FAIL(result_.init())) {
      LOG_WARN("fail to init result", KR(ret));
    } else {
      store_ctx_ = store_ctx;
      merger_ = &merger;
    }
  }
  return ret;
}

bool ObTableLoadTableCompactCtx::is_valid() const
{
  return nullptr != store_ctx_ && nullptr != merger_;
}

int ObTableLoadTableCompactCtx::new_compactor()
{
  int ret = OB_SUCCESS;
  obsys::ObWLockGuard guard(rwlock_);
  if (OB_NOT_NULL(compactor_)) {
    ret = OB_ERR_UNEXPECTED;
    LOG_WARN("unexpected not null compactor", KR(ret), KP(compactor_));
  } else {
    ObMemAttr attr(MTL_ID(), "TLD_Compactor");
    if (store_ctx_->is_multiple_mode_) {
      if (store_ctx_->table_data_desc_.is_heap_table_) {
        compactor_ = OB_NEW(ObTableLoadMultipleHeapTableCompactor, attr);
      } else {
        compactor_ = OB_NEW(ObTableLoadMemCompactor, attr);
      }
    } else {
      compactor_ = OB_NEW(ObTableLoadGeneralTableCompactor, attr);
    }
    if (OB_ISNULL(compactor_)) {
      ret = OB_ALLOCATE_MEMORY_FAILED;
      LOG_WARN("fail to new ObTableLoadTableCompactor", KR(ret));
    }
  }
  return ret;
}

void ObTableLoadTableCompactCtx::release_compactor()
{
  obsys::ObWLockGuard guard(rwlock_);
  if (nullptr != compactor_) {
    ObMemAttr attr(MTL_ID(), "TLD_Compactor");
    OB_DELETE(ObTableLoadTableCompactor, attr, compactor_);
    compactor_ = nullptr;
  }
}

int ObTableLoadTableCompactCtx::start()
{
  int ret = OB_SUCCESS;
  if (OB_FAIL(new_compactor())) {
    LOG_WARN("fail to new compactor", KR(ret));
  } else {
    obsys::ObRLockGuard guard(rwlock_);
    if (OB_FAIL(compactor_->init(this))) {
      LOG_WARN("fail to init compactor", KR(ret));
    } else if (OB_FAIL(compactor_->start())) {
      LOG_WARN("fail to start compactor", KR(ret));
    }
  }
  return ret;
}

void ObTableLoadTableCompactCtx::stop()
{
  obsys::ObRLockGuard guard(rwlock_);
  if (OB_NOT_NULL(compactor_)) {
    compactor_->stop();
  }
}

int ObTableLoadTableCompactCtx::handle_table_compact_success()
{
  // release compactor
  release_compactor();
  // notify merger
  return merger_->handle_table_compact_success();
}

/**
 * ObTableLoadTableCompactor
 */

ObTableLoadTableCompactor::ObTableLoadTableCompactor()
  : compact_ctx_(nullptr), is_inited_(false)
{
}

ObTableLoadTableCompactor::~ObTableLoadTableCompactor()
{
}

int ObTableLoadTableCompactor::init(ObTableLoadTableCompactCtx *compact_ctx)
{
  int ret = OB_SUCCESS;
  if (IS_INIT) {
    ret = OB_INIT_TWICE;
    LOG_WARN("ObTableLoadTableCompactor init twice", KR(ret), KP(this));
  } else if (OB_UNLIKELY(nullptr == compact_ctx || !compact_ctx->is_valid())) {
    ret = OB_INVALID_ARGUMENT;
    LOG_WARN("invalid args", KR(ret), KPC(compact_ctx));
  } else {
    compact_ctx_ = compact_ctx;
    if (OB_FAIL(inner_init())) {
      LOG_WARN("fail to inner init", KR(ret));
    } else {
      is_inited_ = true;
    }
  }
  return ret;
}

} // namespace observer
} // namespace oceanbase
