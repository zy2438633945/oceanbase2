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

#ifndef OB_ALL_VIRTUAL_DUP_LS_TABLET_SET_
#define OB_ALL_VIRTUAL_DUP_LS_TABLET_SET_

#include "share/ob_virtual_table_scanner_iterator.h"
#include "share/ob_scanner.h"
#include "common/row/ob_row.h"
#include "lib/container/ob_se_array.h"
#include "common/ob_simple_iterator.h"
#include "storage/tx/ob_trans_ctx_mgr_v4.h"
#include "storage/tx/ob_dup_table_stat.h"

namespace oceanbase
{
namespace observer
{
class ObAllVirtualDupLSTabletSet: public common::ObVirtualTableScannerIterator
{
public:
  explicit ObAllVirtualDupLSTabletSet() { reset(); }
  virtual ~ObAllVirtualDupLSTabletSet() { destroy(); }
public:
  int init(const common::ObAddr &addr);
  virtual int inner_get_next_row(common::ObNewRow *&row);
  virtual void reset();
  virtual void destroy();
private:
  int prepare_start_to_read_();
  int fill_tenant_ids_();
  bool is_valid_timestamp_(const int64_t timestamp) const;
private:
  enum
  {
    TENANT_ID = common::OB_APP_MIN_COLUMN_ID,
    LS_ID,
    SVR_IP,
    SVR_PORT,
    LS_STATE,
    UNIQUE_ID,
    ATTRIBUTE,
    COUNT,
    READABLE_SCN,
    CHANGE_SCN,
    NEED_CONFIRM_SCN,
    STATE,
    TRX_REF,
  };
  char ip_buffer_[common::OB_IP_STR_BUFF];

private:
  bool init_;
  transaction::ObDupLSTabletSetStatIterator dup_ls_tablet_set_stat_iter_;
  common::ObArray<uint64_t> all_tenants_;
  common::ObAddr self_addr_;
private:
  DISALLOW_COPY_AND_ASSIGN(ObAllVirtualDupLSTabletSet);
};

} //transaction
} //oceanbase
#endif /* OB_ALL_VIRTUAL_TRANS_STAT_H */