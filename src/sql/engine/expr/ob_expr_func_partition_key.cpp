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

#define USING_LOG_PREFIX SQL_ENG
#include "sql/engine/expr/ob_expr_func_partition_key.h"

namespace oceanbase
{
using namespace common;
namespace sql
{

ObExprFuncPartKey::ObExprFuncPartKey(ObIAllocator &alloc)
    : ObFuncExprOperator(alloc,T_FUN_SYS_PART_KEY, N_PART_KEY, MORE_THAN_ZERO, NOT_VALID_FOR_GENERATED_COL, NOT_ROW_DIMENSION)
{
}

ObExprFuncPartKey::~ObExprFuncPartKey()
{
}

int ObExprFuncPartKey::calc_result_typeN(ObExprResType &type,
                                            ObExprResType *types_stack,
                                            int64_t param_num,
                                            ObExprTypeCtx &type_ctx) const
{
  UNUSED(type_ctx);
  UNUSED(types_stack);
  UNUSED(param_num);
  type.set_int();
  return OB_SUCCESS;
}

int ObExprFuncPartKey::cg_expr(ObExprCGCtx &, const ObRawExpr &, ObExpr &rt_expr) const
{
  int ret = OB_SUCCESS;
  rt_expr.eval_func_ = calc_partition_key;
  rt_expr.eval_batch_func_ = calc_partition_key_batch;
  // TODO @zongmei.zzm fix decimal int vector hash calc
  rt_expr.eval_vector_func_ = NULL;
  return ret;
}

int ObExprFuncPartKey::calc_partition_key(const ObExpr &expr,
                                          ObEvalCtx &ctx,
                                          ObDatum &expr_datum)
{
  int ret = OB_SUCCESS;
  if (OB_FAIL(expr.eval_param_value(ctx))) {
    LOG_WARN("eval param value failed", K(ret));
  } else {
    uint64_t hash_value = 0;
    for (int i = 0; i < expr.arg_cnt_ && OB_SUCC(ret); i++) {
      ObDatum &param_datum = expr.locate_param_datum(ctx, i);
      ObExprHashFuncType hash_func =
        (ob_is_decimal_int(expr.args_[i]->datum_meta_.type_) ?
           wide::PartitionHash<ObMurmurHash, ObDatum>::calculate :
           expr.args_[i]->basic_funcs_->murmur_hash_);
      if (OB_FAIL(hash_func(param_datum, hash_value, hash_value))) {
        LOG_WARN("hash value failed", K(ret));
      }
    }
    if (OB_SUCC(ret)) {
      int64_t result_num = static_cast<int64_t>(hash_value);
      result_num = result_num < 0 ? -result_num : result_num;
      expr_datum.set_int(result_num);
    }
  }
  return ret;
}

int ObExprFuncPartKey::calc_partition_key_batch(BATCH_EVAL_FUNC_ARG_DECL)
{
  int ret = OB_SUCCESS;
  uint64_t seed = 0;
  uint64_t *hash_values = reinterpret_cast<uint64_t *>(
      ctx.frames_[expr.frame_idx_] + expr.res_buf_off_);
  ObBitVector &flags = expr.get_evaluated_flags(ctx);
  for (int64_t i = 0; i < expr.arg_cnt_ && OB_SUCC(ret) ; i++) {
    ObExpr *e = expr.args_[i];
    ObBatchDatumHashFunc batch_hash_func =
      ob_is_decimal_int(e->datum_meta_.type_) ?
        wide::PartitionHash<ObMurmurHash, ObDatum>::hash_batch :
        e->basic_funcs_->murmur_hash_batch_;
    if (OB_FAIL(e->eval_batch(ctx, skip, size))) {
      LOG_WARN("evaluate batch failed", K(ret), K(*e));
    } else {
      const bool is_batch_seed = (i > 0);
      batch_hash_func(hash_values,
                      e->locate_batch_datums(ctx), e->is_batch_result(),
                      skip, size,
                      is_batch_seed ? hash_values : &seed,
                      is_batch_seed);

    }
  }
  if (OB_SUCC(ret)) {
    ObDatum *datums = expr.locate_batch_datums(ctx);
    for (int64_t i = 0; i < size; i++) {
      if (!skip.at(i)) {
        const int64_t v = static_cast<int64_t>(hash_values[i]);
        datums[i].set_int(v < 0 ? -v : v);
        flags.set(i);
      }
    }
  }
  return ret;
}

int ObExprFuncPartKey::calc_partition_key_vector(const ObExpr &expr,
                                                 ObEvalCtx &ctx,
                                                 const ObBitVector &skip,
                                                 const EvalBound &bound)
{
  int ret = OB_SUCCESS;
  uint64_t hash_seed = 0;
  uint64_t *batch_hash_vals = NULL;
  int64_t *hash_vals = NULL;
  VectorHeader &vec_header = expr.get_vector_header(ctx);
  if (VEC_FIXED == vec_header.format_) {
    ObFixedLengthBase *fixed_vec = static_cast<ObFixedLengthBase *>(vec_header.get_vector());
    batch_hash_vals = reinterpret_cast<uint64_t *>(fixed_vec->get_data());
  } else if (is_uniform_format(vec_header.format_)) {
    batch_hash_vals = reinterpret_cast<uint64_t *>(
                                ctx.frames_[expr.frame_idx_] + expr.res_buf_off_);
  }
  for (int64_t i = 0; OB_SUCC(ret) && i < expr.arg_cnt_; ++i) {
    if (OB_FAIL(expr.args_[i]->eval_vector(ctx, skip, bound))) {
      LOG_WARN("failed to eval batch datum", K(ret));
    } else {
      ObIVector *arg_vec = expr.args_[i]->get_vector(ctx);
      ret = arg_vec->murmur_hash(*expr.args_[i], batch_hash_vals, skip, bound,
                                 i > 0 ? batch_hash_vals: &hash_seed, i > 0);
    }
  }
  if (OB_FAIL(ret)) {
    // do nothing
  } else {
    //TODO shengle opt
    ObBitVector &flags = expr.get_evaluated_flags(ctx);
    for (int64_t i = bound.start(); i < bound.end(); ++i) {
      if (skip.at(i)) {
        continue;
      } else {
        const int64_t v = static_cast<int64_t>(batch_hash_vals[i]);
        batch_hash_vals[i] = std::abs(v);
        LOG_DEBUG("calc partition key vector", K(batch_hash_vals[i]));
        flags.set(i);
      }
    }
    if (is_uniform_format(vec_header.format_)) {
      ObDatum *datum = expr.locate_batch_datums(ctx);
      for (int64_t i = bound.start(); i < bound.end(); ++i) {
        if (!skip.at(i)) {
          datum[i].set_none();
        }
      }
    }
  }
  return ret;
}

}  // namespace sql
}  // namespace oceanbase
