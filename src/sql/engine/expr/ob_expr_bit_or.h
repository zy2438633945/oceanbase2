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

#ifndef _OB_EXPR_BITWISE_OR_H_
#define _OB_EXPR_BITWISE_OR_H_

#include "sql/engine/expr/ob_expr_operator.h"

namespace oceanbase
{
namespace sql
{
class ObExprBitOr : public ObBitwiseExprOperator
{
public:
  explicit  ObExprBitOr(common::ObIAllocator &alloc);
  virtual ~ObExprBitOr() {};
  virtual int cg_expr(ObExprCGCtx &expr_cg_ctx, const ObRawExpr &raw_expr,
                       ObExpr &rt_expr) const override;
private:
  DISALLOW_COPY_AND_ASSIGN(ObExprBitOr);
};
}
}
#endif  /* _OB_EXPR_BITWISE_OR_H_ */
