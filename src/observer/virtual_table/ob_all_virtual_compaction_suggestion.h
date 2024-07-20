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

#ifndef OB_ALL_VIRTUAL_COMPACTION_SUGGESTION_H_
#define OB_ALL_VIRTUAL_COMPACTION_SUGGESTION_H_
#include "share/ob_virtual_table_scanner_iterator.h"
#include "lib/container/ob_array.h"
#include "storage/compaction/ob_compaction_suggestion.h"

namespace oceanbase
{
namespace observer
{

class ObAllVirtualCompactionSuggestion : public common::ObVirtualTableScannerIterator
{
public:
  enum COLUMN_ID_LIST
  {
    SVR_IP  = common::OB_APP_MIN_COLUMN_ID,
    SVR_PORT,
    TENANT_ID,
    MERGE_TYPE,
    LS_ID,
    TABLET_ID,
    MERGE_START_TIME,
    MERGE_FINISH_TIME,
    SUGGESTION,
  };
  ObAllVirtualCompactionSuggestion();
  virtual ~ObAllVirtualCompactionSuggestion();
  int init();
  virtual int inner_get_next_row(common::ObNewRow *&row);
  virtual void reset();
protected:
  int fill_cells();
private:
  char ip_buf_[common::OB_IP_STR_BUFF];
  compaction::ObCompactionSuggestion suggestion_;
  compaction::ObCompactionSuggestionIterator suggestion_iter_;
  bool is_inited_;
  DISALLOW_COPY_AND_ASSIGN(ObAllVirtualCompactionSuggestion);
};

} /* namespace observer */
} /* namespace oceanbase */
#endif
