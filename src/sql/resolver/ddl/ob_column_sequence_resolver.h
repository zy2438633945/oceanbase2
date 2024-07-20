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

#ifndef OCEANBASE_COLUMN_SEQUENCE_RESOLVER_
#define OCEANBASE_COLUMN_SEQUENCE_RESOLVER_ 1

#include "sql/resolver/ddl/ob_ddl_resolver.h"
#include "sql/resolver/ddl/ob_sequence_stmt.h"

namespace oceanbase
{
namespace sql
{
class ObColumnSequenceResolver: public ObStmtResolver
{
public:
  explicit ObColumnSequenceResolver(ObResolverParams &params);
  virtual ~ObColumnSequenceResolver();

  virtual int resolve(const ParseNode &parse_tree);

  int resolve_sequence_without_name(ObColumnSequenceStmt *&mystmt, ParseNode *&node);
private:
  // types and constants
private:
  // disallow copy
  //DISALLOW_COPY_AND_ASSIGN(ObColumnSequenceResolver);
  // function members

private:
  // data members
};

} // end namespace sql
} // end namespace oceanbase

#endif /* OCEANBASE_COLUMN_SEQUENCE_RESOLVER_ */
