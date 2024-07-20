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

#include "lib/string/ob_string.h"      // ObString
#include "share/backup/ob_backup_struct.h"

#ifndef OCEANBASE_ARCHIVE_OB_ARCHIVE_IO_H_
#define OCEANBASE_ARCHIVE_OB_ARCHIVE_IO_H_

namespace oceanbase
{
namespace archive
{
using oceanbase::common::ObString;
class ObArchiveIO
{
public:
  ObArchiveIO() {}
  ~ObArchiveIO() {}

public:
  int push_log(const ObString &uri,
      const share::ObBackupStorageInfo *storage_info,
      char *data,
      const int64_t data_len,
      const int64_t offset,
      const bool is_full_file,
      const bool is_can_seal);

  int mkdir(const ObString &uri,
      const share::ObBackupStorageInfo *storage_info);

private:
  int check_context_match_in_normal_file_(const ObString &uri,
      const share::ObBackupStorageInfo *storage_info,
      char *data,
      const int64_t data_len,
      const int64_t offset);
};
} // namespace archive
} // namespace oceanbase

#endif /* OCEANBASE_ARCHIVE_OB_ARCHIVE_IO_H_ */
