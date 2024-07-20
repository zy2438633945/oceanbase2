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

#ifndef OCEANBASE_ENGINE_PX_EXCHANGE_OB_PX_MS_RECEIVE_OP_H_
#define OCEANBASE_ENGINE_PX_EXCHANGE_OB_PX_MS_RECEIVE_OP_H_

#include "sql/engine/px/exchange/ob_receive_op.h"
#include "lib/container/ob_fixed_array.h"
#include "sql/engine/px/exchange/ob_row_heap.h"
#include "sql/engine/px/ob_dfo.h"
#include "sql/engine/px/ob_px_dtl_msg.h"
#include "sql/engine/px/ob_px_data_ch_provider.h"
#include "sql/engine/px/ob_px_dtl_proc.h"
#include "sql/engine/px/exchange/ob_px_receive_op.h"
#include "sql/dtl/ob_dtl_channel_loop.h"
#include "sql/engine/basic/ob_chunk_datum_store.h"
#include "sql/dtl/ob_dtl_linked_buffer.h"
#include "sql/engine/ob_sql_mem_mgr_processor.h"

namespace oceanbase
{
namespace sql
{


class ObPxMSReceiveOpInput : public ObPxReceiveOpInput
{
public:
  OB_UNIS_VERSION_V(1);
public:
  ObPxMSReceiveOpInput(ObExecContext &ctx, const ObOpSpec &spec)
    : ObPxReceiveOpInput(ctx, spec)
  {}
  virtual ~ObPxMSReceiveOpInput()
  {}
};

class ObPxMSReceiveSpec : public ObPxReceiveSpec
{
OB_UNIS_VERSION_V(1);
public:
  ObPxMSReceiveSpec(common::ObIAllocator &alloc, const ObPhyOperatorType type);
  virtual const ObIArray<ObExpr *> *get_all_exprs() const override { return &all_exprs_; }

  // [sort_exprs, output_exprs]前面是排序列，后面是receive output列
  ExprFixedArray all_exprs_;
  ObSortCollations sort_collations_;
  ObSortFuncs sort_cmp_funs_;
  bool local_order_;
};

class ObPxMSReceiveOp : public ObPxReceiveOp
{
public:
  ObPxMSReceiveOp(ObExecContext &exec_ctx, const ObOpSpec &spec, ObOpInput *input);
  virtual ~ObPxMSReceiveOp() {}

  const ObPxMSReceiveSpec &my_spec() const { return static_cast<const ObPxMSReceiveSpec &>(spec_); }
private:
  class MergeSortInput
  {
  public:
    MergeSortInput(ObChunkDatumStore *get_row_store, ObChunkDatumStore *add_row_ptr, bool finish)
      : get_row_store_(get_row_store),
        add_row_store_(add_row_ptr),
        finish_(finish),
        reader_(),
        alloc_(nullptr),
        sql_mem_processor_(nullptr),
        io_event_observer_(nullptr),
        processed_cnt_(0)
    {}
    virtual ~MergeSortInput() = default;

    virtual int get_row(
      ObPxMSReceiveOp *ms_receive_op,
      ObPhysicalPlanCtx *phy_plan_ctx,
      int64_t channel_idx,
      const common::ObIArray<ObExpr*> &exprs,
      ObEvalCtx &eval_ctx,
      const ObChunkDatumStore::StoredRow *&store_row) = 0;
    virtual int add_row(
      ObExecContext &ctx,
      const common::ObIArray<ObExpr*> &exprs,
      ObEvalCtx &eval_ctx) = 0;

    virtual void set_finish(bool finish) { finish_ = finish; }
    virtual int64_t max_pos() = 0;
    virtual void destroy() = 0;
    virtual bool is_finish() const { return finish_; }
    virtual void clean_row_store(ObExecContext &ctx) = 0;
    static int need_dump(ObSqlMemMgrProcessor &sql_mem_processor_,
                         common::ObIAllocator &alloc, bool &need_dump);

    TO_STRING_KV(K_(finish));
  public:
    ObChunkDatumStore *get_row_store_;
    ObChunkDatumStore *add_row_store_;
    bool finish_;
    ObChunkDatumStore::Iterator reader_;
    ObIAllocator *alloc_;
    ObSqlMemMgrProcessor *sql_mem_processor_;
    ObIOEventObserver *io_event_observer_;
    int64_t processed_cnt_;
  };

  // 全局有序，表示merge sort receive的每一个channel传入的数据是有序的。只要对所有路进行归并排序即可
  // 每一个channel对应一个GlobalOrderInput，每一路会缓存数据来解决由于限流导致的卡死现象。
  // 同时为了减少缓存的数据，通过两个row store来回交换地add和get数据，以达到减少buffer数据量
  // 即一个get_row_store_来吐出数据，一个add_row_store_来获取channel数据，只要get_row_store_全部吐完，
  // 则达到一定阙值后清空get_row_store_数据，同时切换add_row_store_为get_row_store_，get_row_store_为add_row_store_
  // 这样来回切换数据的add和get
  class GlobalOrderInput : public MergeSortInput
  {
  public:
    GlobalOrderInput(uint64_t tenant_id)
    : MergeSortInput(nullptr, nullptr, false),
      get_reader_(),
      add_row_reader_(nullptr),
      get_row_reader_(nullptr)
    {
      tenant_id_ = tenant_id;
    }
    virtual ~GlobalOrderInput() { destroy(); }

    virtual int get_row(
      ObPxMSReceiveOp *ms_receive_op,
      ObPhysicalPlanCtx *phy_plan_ctx,
      int64_t channel_idx,
      const common::ObIArray<ObExpr*> &exprs,
      ObEvalCtx &eval_ctx,
      const ObChunkDatumStore::StoredRow *&store_row);
    virtual int add_row(
      ObExecContext &ctx,
      const common::ObIArray<ObExpr*> &exprs,
      ObEvalCtx &eval_ctx);

    virtual int64_t max_pos();
    virtual void destroy();
    virtual void clean_row_store(ObExecContext &ctx);
    virtual bool is_empty();
  private:
    int create_chunk_datum_store(
      ObExecContext &ctx, uint64_t tenant_id, ObChunkDatumStore *&row_store);
    virtual int reset_add_row_store(bool &reset);
    virtual int switch_get_row_store();
    int get_one_row_from_channels(
      ObPxMSReceiveOp *ms_receive_op,
      ObPhysicalPlanCtx *phy_plan_ctx,
      int64_t channel_idx,
      const common::ObIArray<ObExpr*> &exprs,
      ObEvalCtx &eval_ctx);
    int process_dump(ObPxMSReceiveOp &ms_receive_op);
  private:
    static const int64_t MAX_ROWS_PER_STORE = 50L;
    uint64_t  tenant_id_;
    // 由于需要两个datum store来回切，为了避免每次切的时候都将数据清空重新开始插入，
    // 所以需要两个iterator保存当前读的位置
    // eg:
    // reader1        reader2     step
    //  1               1          reader1(1) ->reader2(1) //即先读reader1的row 1，然后读reader2的row 2
    //  2               2          reader1(2) ->reader2(2)
    //  4               3          reader2(3) ->reader1(4)
    // 这里默认父类的reader是add_reader
    ObChunkDatumStore::Iterator get_reader_;
    ObChunkDatumStore::Iterator *add_row_reader_;
    ObChunkDatumStore::Iterator *get_row_reader_;
  };

  // 局部有序，表示merge sort receive的每一个channel的输入数据是局部有序，即分段有序
  // 可以通过切分的方式，把局部有序切分成更多路的有序，然后可以进行归并排序。
  // 主要优化是从之前的全部数据进行排序优化成更多路的归并排序。
  // 这样每一个channel可能对应多个LocalOrderInput，即分成了多个有序的数据段，每一个有序段通过LocalOrderInput来进行get和add
  // 同时每一个channel会有一个row_store缓存所有数据，LocalOrderInput则指定自己的有序段的范围[start_pos, end_pos).
  // 然后根据范围来不断pop数据
  class LocalOrderInput : public MergeSortInput
  {
  public:
    explicit LocalOrderInput()
      : MergeSortInput(nullptr, nullptr, false),
        datum_store_("PxMSRecvLocal")
      {
        get_row_store_ = &datum_store_;
        add_row_store_ = &datum_store_;
      }

    virtual ~LocalOrderInput() { destroy(); }
    virtual int get_row(
      ObPxMSReceiveOp *ms_receive_op,
      ObPhysicalPlanCtx *phy_plan_ctx,
      int64_t channel_idx,
      const common::ObIArray<ObExpr*> &exprs,
      ObEvalCtx &eval_ctx,
      const ObChunkDatumStore::StoredRow *&store_row);
    virtual int add_row(
      ObExecContext &ctx,
      const common::ObIArray<ObExpr*> &exprs,
      ObEvalCtx &eval_ctx);
    virtual int64_t max_pos();
    virtual void destroy();
    virtual void clean_row_store(ObExecContext &ctx);
    int open();
  public:
    ObChunkDatumStore datum_store_;
  };

  class Compare
  {
  public:
    Compare();
    int init(const ObIArray<ObSortFieldCollation> *sort_collations,
        const ObIArray<ObSortCmpFunc> *sort_cmp_funs);

    bool operator()(
        const ObChunkDatumStore::StoredRow *l,
        const common::ObIArray<ObExpr*> *r,
        ObEvalCtx &eval_ctx);

    bool is_inited() const { return NULL != sort_collations_; }
    // interface required by ObBinaryHeap
    int get_error_code() { return ret_; }

    void reset() { this->~Compare(); new (this)Compare(); }

  public:
    int ret_;
    const ObIArray<ObSortFieldCollation> *sort_collations_;
    const ObIArray<ObSortCmpFunc> *sort_cmp_funs_;
    const common::ObIArray<const ObChunkDatumStore::StoredRow*> *rows_;
  private:
    DISALLOW_COPY_AND_ASSIGN(Compare);
  };

  virtual int inner_open() override;
  virtual void destroy() override;
  virtual int inner_close() override;
  virtual int inner_get_next_row() override;
  virtual int inner_get_next_batch(const int64_t max_row_cnt) override;
  virtual int inner_rescan() override;
  int process_dump(const common::ObIArray<ObChunkDatumStore *> &full_dump_array,
                   const common::ObIArray<ObChunkDatumStore *> &part_dump_array);

  OB_INLINE virtual int64_t get_channel_count() { return task_channels_.count(); }
private:
  int new_local_order_input(MergeSortInput *&out_msi);
  int get_all_rows_from_channels(
      ObPhysicalPlanCtx *phy_plan_ctx);
  int try_link_channel() override;
  int init_merge_sort_input(int64_t n_channel);
  int release_merge_inputs();
  int get_one_row_from_channels(
    ObPhysicalPlanCtx *phy_plan_ctx,
    int64_t channel_idx,
    const ObIArray<ObExpr*> &exprs,
    ObEvalCtx &eval_ctx,
    const ObChunkDatumStore::StoredRow *&store_row);
private:
  static const int64_t MAX_INPUT_NUMBER = 10000L;
  dtl::ObDtlChannelLoop *ptr_row_msg_loop_;
  ObPxInterruptP interrupt_proc_;
  ObRowHeap<ObDatumRowCompare, ObChunkDatumStore::StoredRow> row_heap_;

  // every merge sort inputs, the number of merge sort inputs may be different from channels
  common::ObArray<MergeSortInput *> merge_inputs_;
  bool finish_;
  lib::MemoryContext mem_context_;
  ObSqlWorkAreaProfile profile_;
  ObSqlMemMgrProcessor sql_mem_processor_;
  int64_t processed_cnt_;
};

} // end namespace sql
} // end namespace oceanbase

#endif // OCEANBASE_ENGINE_PX_EXCHANGE_OB_PX_RECEIVE_OP_H_
