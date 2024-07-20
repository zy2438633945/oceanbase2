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

#ifndef OB_EXPR_POSITION_H_
#define OB_EXPR_POSITION_H_
#include "sql/engine/expr/ob_expr_operator.h"

namespace oceanbase
{
namespace sql
{
class ObExprPosition : public ObLocationExprOperator
{
public:
  explicit  ObExprPosition(common::ObIAllocator &alloc);
  virtual ~ObExprPosition();
private:
  //disallow copy
  DISALLOW_COPY_AND_ASSIGN(ObExprPosition);
};


}//end of namespace sql
}//end of namespace oceanbase

#endif /* SRC_SQL_ENGINE_EXPR_OB_EXPR_POSITION_H_ */
