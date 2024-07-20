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

#ifndef OB_COMPACTION_PARTITION_MERGE_PROGRESS_H_
#define OB_COMPACTION_PARTITION_MERGE_PROGRESS_H_

#include "storage/ob_i_store.h"

namespace oceanbase
{
namespace storage
{
class ObTabletHandle;
struct ObSSTableMergeInfo;
}

namespace compaction
{
class ObTabletMergeDag;
struct ObCompactionProgress;
struct ObDiagnoseTabletCompProgress;
struct ObBasicTabletMergeCtx;
class ObCompactionTimeGuard;
class ObPartitionMergeProgress
{
public:
  ObPartitionMergeProgress(common::ObIAllocator &allocator);
  virtual ~ObPartitionMergeProgress();
  void reset();
  OB_INLINE bool is_inited() const { return is_inited_; }
  int init(ObBasicTabletMergeCtx *ctx, ObTabletMergeDag *merge_dag,
    const int64_t start_cg_idx = 0, const int64_t end_cg_idx = 0);
  virtual int update_merge_progress(const int64_t idx, const int64_t scanned_row_count);
  virtual int finish_merge_progress() { return OB_SUCCESS; }
  virtual int update_tenant_merge_progress(
      const int64_t total_data_size_delta,
      const int64_t scan_data_size_delta)
  {
    UNUSED(total_data_size_delta);
    UNUSED(scan_data_size_delta);
    return OB_SUCCESS;
  }
  int update_merge_info(storage::ObSSTableMergeInfo &merge_info);
  int get_progress_info(ObCompactionProgress &input_progress);
  int diagnose_progress(ObDiagnoseTabletCompProgress &input_progress);
  int64_t get_estimated_finish_time() const { return estimated_finish_time_; }
  int64_t get_concurrent_count() const { return concurrent_cnt_; }
  int64_t *get_scanned_row_cnt_arr() const { return scanned_row_cnt_arr_; } // make sure get array after compaction finish!!!
  DECLARE_TO_STRING;
public:
  static const int32_t UPDATE_INTERVAL = 2 * 1000 * 1000; // 2 second
  static const int32_t NORMAL_UPDATE_PARAM = 300;
  static const int32_t DEFAULT_ROW_CNT_PER_MACRO_BLOCK = 1000;
  static const int32_t DEFAULT_INCREMENT_ROW_FACTOR = 10;
  static const int64_t MAX_ESTIMATE_SPEND_TIME = 24 * 60 * 60 * 1000 * 1000l; // 24 hours
  static const int64_t PRINT_ESTIMATE_WARN_INTERVAL = 5 * 60 * 1000 * 1000; // 1 min
protected:
  int estimate();
  void update_estimated_finish_time_();
private:
  int estimate_mini_merge(const ObIArray<storage::ObITable*> &tables, const storage::ObTabletHandle &tablet_handle);

protected:
  common::ObIAllocator &allocator_;
  ObBasicTabletMergeCtx *ctx_;
  compaction::ObTabletMergeDag *merge_dag_;
  int64_t *scanned_row_cnt_arr_;
  int64_t concurrent_cnt_;
  int64_t estimate_row_cnt_;
  int64_t estimate_occupy_size_;
  int64_t estimate_occupy_size_delta_;
  float avg_row_length_;
  int64_t latest_update_ts_;
  int64_t estimated_finish_time_;
  int64_t pre_scanned_row_cnt_; // for smooth the progress curve
  int64_t start_cg_idx_;
  int64_t end_cg_idx_;
  bool is_updating_; // atomic lock
  bool is_inited_;
};

class ObPartitionMajorMergeProgress : public ObPartitionMergeProgress
{
public:
  ObPartitionMajorMergeProgress(common::ObIAllocator &allocator)
   : ObPartitionMergeProgress(allocator)
  {
  }
  ~ObPartitionMajorMergeProgress() {}
  virtual int update_tenant_merge_progress(
      const int64_t total_data_size_delta,
      const int64_t scan_data_size_delta) override;
  virtual int finish_merge_progress() override;
  int finish_progress(
    const int64_t merge_version,
    ObCompactionTimeGuard *time_guard,
    const bool is_co_merge);
};

class ObCOMajorMergeProgress : public ObPartitionMajorMergeProgress
{
public:
  ObCOMajorMergeProgress(common::ObIAllocator &allocator)
    : ObPartitionMajorMergeProgress(allocator)
  {}
  ~ObCOMajorMergeProgress() {}
  virtual int finish_merge_progress() override;
};

} //compaction
} //oceanbase

#endif /* OB_COMPACTION_PARTITION_MERGE_PROGRESS_H_ */
