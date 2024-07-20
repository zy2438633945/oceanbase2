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
#ifndef SRC_STORAGE_MEMTABLE_OB_ROW_CONFLICT_INFO_H
#define SRC_STORAGE_MEMTABLE_OB_ROW_CONFLICT_INFO_H
#include "lib/net/ob_addr.h"
#include "meta_programming/ob_mover.h"
#include "ob_tablet_id.h"
#include "share/ob_ls_id.h"
#include "storage/tx/ob_trans_define.h"
#include "lib/string/ob_string_holder.h"
#include "lib/utility/ob_unify_serialize.h"
#include "deps/oblib/src/common/meta_programming/ob_mover.h"
#include <utility>
namespace oceanbase {
namespace storage {
struct ObRowConflictInfo
{
  OB_UNIS_VERSION(1);
public:
  ObRowConflictInfo()
  : conflict_happened_addr_(),
  conflict_ls_(),
  conflict_tablet_(),
  conflict_row_key_str_(),
  conflict_sess_id_(0),
  conflict_tx_scheduler_(),
  conflict_tx_id_(),
  conflict_tx_hold_seq_() {}
  ~ObRowConflictInfo() = default;
  ObRowConflictInfo(const ObRowConflictInfo &) = delete;// disallow default copy
  ObRowConflictInfo &operator=(const ObRowConflictInfo &) = delete;// disallow default assign operator
  ObRowConflictInfo &operator=(meta::ObMover<ObRowConflictInfo>) { return *this; }
  int init(const common::ObAddr &, const share::ObLSID &,
           const common::ObTabletID &, meta::ObMover<ObStringHolder>,
           const uint32_t, const common::ObAddr &,
           const transaction::ObTransID &, const transaction::ObTxSEQ &) { return OB_SUCCESS; }
  int assign(const ObRowConflictInfo &) { return OB_SUCCESS; }
  int assign(meta::ObMover<ObRowConflictInfo>) { return OB_SUCCESS; }
  bool is_valid() const { return true; }
  TO_STRING_KV(K_(conflict_happened_addr), K_(conflict_ls), K_(conflict_tablet),
               K_(conflict_row_key_str), K_(conflict_sess_id), K_(conflict_tx_scheduler),
               K_(conflict_tx_id), K_(conflict_tx_hold_seq));
  common::ObAddr conflict_happened_addr_;// distributed info
  share::ObLSID conflict_ls_;// resource related
  common::ObTabletID conflict_tablet_;// resource related
  common::ObStringHolder conflict_row_key_str_;// resource related
  uint32_t conflict_sess_id_;// holder related
  common::ObAddr conflict_tx_scheduler_;// holder related
  transaction::ObTransID conflict_tx_id_;// holder related
  transaction::ObTxSEQ conflict_tx_hold_seq_;// holder use sql identified by this seq to add row lock
};
OB_SERIALIZE_MEMBER_TEMP(inline, ObRowConflictInfo, conflict_happened_addr_, conflict_ls_,
                         conflict_tablet_, conflict_row_key_str_, conflict_sess_id_,
                         conflict_tx_scheduler_, conflict_tx_id_, conflict_tx_hold_seq_);
}
}
#endif