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

#ifndef OCEANBASE_SQL_RESOLVER_CMD_OB_SYSTEM_CMD_STMT_
#define OCEANBASE_SQL_RESOLVER_CMD_OB_SYSTEM_CMD_STMT_

#include "sql/resolver/ob_stmt.h"
#include "sql/resolver/ob_cmd.h"
namespace oceanbase
{
namespace sql
{
class ObSystemCmdStmt : public ObStmt, public ObICmd
{
public:
  ObSystemCmdStmt(common::ObIAllocator* name_pool, stmt::StmtType type)
  : ObStmt(name_pool, type)
  {}
  explicit ObSystemCmdStmt(stmt::StmtType type) : ObStmt(type)
  {}
  virtual ~ObSystemCmdStmt() {}
  virtual int get_cmd_type() const { return get_stmt_type(); }
private:
  DISALLOW_COPY_AND_ASSIGN(ObSystemCmdStmt);
};
} // namespace sql
} // namespace oceanbase
#endif /* OCEANBASE_SQL_RESOLVER_CMD_OB_SYSTEM_CMD_STMT_ */
