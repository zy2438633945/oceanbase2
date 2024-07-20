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

#ifndef OB_TRANSFORM_SIMPLIFY_EXPR_H
#define OB_TRANSFORM_SIMPLIFY_EXPR_H

#include "sql/rewrite/ob_transform_rule.h"

namespace oceanbase {
namespace sql {

struct ObNotNullContext;

class ObTransformSimplifyExpr : public ObTransformRule
{
public:
  ObTransformSimplifyExpr(ObTransformerCtx *ctx)
    : ObTransformRule(ctx, TransMethod::POST_ORDER, T_SIMPLIFY_EXPR)
  {}

  virtual ~ObTransformSimplifyExpr() {}

  virtual int transform_one_stmt(common::ObIArray<ObParentDMLStmt> &parent_stmts,
                                 ObDMLStmt *&stmt,
                                 bool &trans_happened) override;
private:
  int replace_is_null_condition(ObDMLStmt *stmt, bool &trans_happened);
  int inner_replace_is_null_condition(ObDMLStmt *stmt,
                                      ObRawExpr *&expr,
                                      ObNotNullContext &not_null_ctx,
                                      bool &trans_happened);
  int do_replace_is_null_condition(ObDMLStmt *stmt,
                                   ObRawExpr *&expr,
                                   ObNotNullContext &not_null_ctx,
                                   bool &trans_happened);
  int replace_op_null_condition(ObDMLStmt *stmt, bool &trans_happened);
  int replace_cmp_null_condition(ObRawExpr *&expr,
                                 const ObDMLStmt &stmt,
                                 const ParamStore &param_store,
                                 bool &trans_happened);

  int extract_null_expr(ObRawExpr *expr,
                        const ObDMLStmt &stmt,
                        const ParamStore &param_store,
                        ObIArray<const ObRawExpr *> &null_expr_lists);
  int convert_preds_vector_to_scalar(ObDMLStmt *stmt, bool &trans_happened);
  int recursively_convert_join_preds_vector_to_scalar(TableItem *table_item, bool &trans_happened);
  int remove_dummy_exprs(ObDMLStmt *stmt, bool &trans_happened);
  int remove_dummy_filter_exprs(common::ObIArray<ObRawExpr*> &exprs,
                                ObIArray<ObExprConstraint> &constraints);
  int remove_dummy_join_condition_exprs(TableItem *table,
                                        ObIArray<ObExprConstraint> &constraints);
  int inner_remove_dummy_expr(ObRawExpr *&expr,
                              ObIArray<ObExprConstraint> &constraints);
  int inner_remove_dummy_expr(common::ObIArray<ObRawExpr*> &exprs,
                              ObIArray<ObExprConstraint> &constraints);
                              
  int is_valid_transform_type(ObRawExpr *expr,
                              bool &is_valid);

  int adjust_dummy_expr(const ObIArray<int64_t> &true_exprs,
                        const ObIArray<int64_t> &false_exprs,
                        const bool is_and_op,
                        ObIArray<ObRawExpr *> &adjust_exprs,
                        ObRawExpr *&transed_expr,
                        ObIArray<ObExprConstraint> &constraints);

  int remove_dummy_case_when(ObDMLStmt *stmt, bool &trans_happened);
  int remove_dummy_case_when(ObQueryCtx* query_ctx,
                             ObRawExpr *expr,
                             bool &trans_happened);
  int inner_remove_dummy_case_when(ObQueryCtx* query_ctx,
                                   ObCaseOpRawExpr *case_expr,
                                   bool &trans_happened);

  int remove_dummy_nvl(ObDMLStmt *stmt,
                       bool &trans_happened);
  int inner_remove_dummy_nvl(ObDMLStmt *stmt,
                             ObRawExpr *&expr,
                             ObNotNullContext &not_null_ctx,
                             ObIArray<ObRawExpr *> &ignore_exprs,
                             bool &trans_happened);
  int do_remove_dummy_nvl(ObDMLStmt *stmt,
                          ObRawExpr *&expr,
                          ObNotNullContext &not_null_ctx,
                          bool &trans_happened);

  int convert_nvl_predicate(ObDMLStmt *stmt, bool &trans_happened);
  int inner_convert_nvl_predicate(ObDMLStmt *stmt,
                                  ObRawExpr *&expr,
                                  ObIArray<ObRawExpr *> &ignore_exprs,
                                  bool &trans_happened);
  int do_convert_nvl_predicate(ObDMLStmt *stmt,
                               ObRawExpr *&parent_expr,
                               ObRawExpr *&nvl_expr,
                               ObRawExpr *&sibling_expr,
                               bool nvl_at_left,
                               bool &trans_happened);
  int replace_like_condition(ObDMLStmt *stmt,
                             bool &trans_happened);
  int check_like_condition(ObRawExpr *&expr,
                           ObIArray<ObRawExpr *> &old_exprs,
                           ObIArray<ObRawExpr *> &new_exprs,
                           ObIArray<ObExprConstraint> &constraints);
  int do_check_like_condition(ObRawExpr *&expr,
                              ObIArray<ObRawExpr *> &old_exprs,
                              ObIArray<ObRawExpr *> &new_exprs,
                              ObIArray<ObExprConstraint> &constraints);

  int remove_subquery_when_filter_is_false(ObDMLStmt* stmt,
                                           bool& trans_happened);
  int try_remove_subquery_in_expr(ObDMLStmt* stmt,
                                  ObRawExpr*& expr,
                                  bool& trans_happened);
  int adjust_subquery_comparison_expr(ObRawExpr *&expr,
                                      bool is_empty_left,
                                      bool is_empty_right,
                                      ObRawExpr* param_expr_left,
                                      ObRawExpr* param_expr_right);
  int do_remove_subquery(ObDMLStmt* stmt,
                         ObRawExpr*& expr,
                         bool& trans_happened,
                         bool& is_empty);
  int is_valid_for_remove_subquery(const ObSelectStmt* stmt,
                                   bool& is_valid);
  int is_filter_false(ObSelectStmt* stmt,
                      bool& is_where_false,
                      bool& is_having_false,
                      bool& is_having_true);
  int is_filter_exprs_false(common::ObIArray<ObRawExpr*>& condition_exprs,
                            bool& is_false,
                            bool& is_true);
  int check_limit_value(ObSelectStmt* stmt,
                        bool& is_limit_filter_false,
                        ObRawExpr*& limit_cons);
  int replace_expr_when_filter_is_false(ObRawExpr*& expr);
  int build_expr_for_not_empty_set(ObSelectStmt* sub_stmt,
                                   ObRawExpr*& expr);
  int build_null_for_empty_set(const ObSelectStmt* sub_stmt,
                               ObRawExpr*& expr);
  int build_null_expr_and_cast(const ObRawExpr* expr,
                               ObRawExpr*& cast_expr);

  int flatten_stmt_exprs(ObDMLStmt *stmt, bool &trans_happened);
  int flatten_join_condition_exprs(TableItem *table, bool &trans_happened);
  int flatten_exprs(common::ObIArray<ObRawExpr*> &exprs, bool &trans_happened);

  int transform_is_false_true_expr(ObDMLStmt *stmt, bool &trans_happened);
  int remove_false_true(common::ObIArray<ObRawExpr*> &exprs, bool &trans_happened);
  int is_valid_remove_false_true(ObRawExpr *expr, bool &is_valid);
  int remove_false_true(ObRawExpr *expr, ObRawExpr *&ret_expr, bool &trans_happened);
  int remove_ora_decode(ObDMLStmt *stmt, bool &trans_happened);
  int inner_remove_ora_decode(ObRawExpr *&expr,
                              ObIArray<ObRawExpr *> &old_exprs,
                              ObIArray<ObRawExpr *> &new_exprs);
  int check_remove_ora_decode_valid(ObRawExpr *&expr, int64_t &result_idx, bool &is_valid);
  int try_remove_ora_decode(ObRawExpr *&expr, ObRawExpr *&new_expr);

  int canonicalize_conditions(ObDMLStmt *stmt, bool &trans_happened);
  int recursive_canonicalize_join_conditions(ObDMLStmt *stmt, TableItem *table, bool &trans_happened);
  int do_canonicalize(ObDMLStmt *stmt, ObIArray<ObRawExpr*> &conditions, bool &trans_happened);
  int push_not(ObIArray<ObRawExpr*> &conditions,
               bool &trans_happened);
  int recursive_push_not(ObRawExpr *&expr,
                         hash::ObHashMap<uint64_t, ObRawExpr *> &push_expr_map,
                         bool &trans_happened);
  int do_push_not(ObRawExpr *&expr,
                  hash::ObHashMap<uint64_t, ObRawExpr *> &push_expr_map,
                  bool &trans_happened);
  int reverse_cmp_type_of_align_date4cmp(ObRawExpr* expr,
                                         ObRawExprFactory *expr_factory,
                                         const ObItemType cmp_type,
                                         bool is_left);

  int get_opposite_op(ObItemType type, ObItemType& opposit_type);
  int remove_duplicate_exprs(ObQueryCtx* query_ctx, ObIArray<ObRawExpr*> &conditions, bool &trans_happened);
  int recursive_remove_duplicate_exprs(ObQueryCtx &query_ctx, ObRawExpr* &expr, bool &trans_happened);
  int do_remove_duplicate_exprs(ObQueryCtx &query_ctx, ObIArray<ObRawExpr*> &exprs);
  int pull_similar_expr(ObDMLStmt *stmt, ObIArray<ObRawExpr*> &conditions, bool &trans_happened);
  int recursive_pull_similar(ObDMLStmt *stmt, ObRawExpr* &expr, bool &trans_happened);
  int do_pull_similar(ObDMLStmt *stmt,
                      ObRawExpr* &expr,
                      ObIArray<ObRawExpr*> &exprs,
                      ObItemType expr_type,
                      bool &trans_happened);
  int gen_not_intersect_param(ObRawExpr* &expr,
                              ObIArray<ObSEArray<ObRawExpr *, 4>> &params_sets,
                              ObItemType expr_type);
  int get_intersection(ObDMLStmt *stmt,
                       ObIArray<ObSEArray<ObRawExpr *, 4>> &params_sets,
                       ObIArray<ObRawExpr *> &intersection);
  int remove_intersect_item(ObDMLStmt *stmt,
                            ObIArray<ObSEArray<ObRawExpr *, 4>> &params_sets,
                            ObIArray<ObRawExpr *> &intersection);
  int get_params_array(ObIArray<ObRawExpr*> &exprs,
                   ObRawExpr *&expr,
                   ObItemType parent_type);
  int convert_case_when_predicate(ObDMLStmt *stmt, bool &trans_happened);
  int try_convert_case_when_by_then_exprs(ObRawExpr *&expr,
                                          ObIArray<ObRawExpr *> &extracted_preds,
                                          bool &trans_happened);
  int check_convert_then_exprs_validity(ObRawExpr *parent_expr,
                                        ObCaseOpRawExpr *&case_expr,
                                        bool &is_all_false,
                                        bool &is_all_true,
                                        int64_t &reserved_branch_cnt,
                                        ObIArray<ObRawExpr*> &true_exprs,
                                        ObIArray<ObRawExpr*> &false_null_exprs,
                                        ObIArray<ObRawExpr*> &extracted_preds,
                                        bool &is_valid);
  int get_preds_for_convert_case_when_by_then(ObCaseOpRawExpr *case_expr,
                                              bool is_all_true,
                                              bool is_all_false,
                                              int64_t reserved_branch_cnt,
                                              ObIArray<ObRawExpr*> &extracted_preds);
  int add_constraint_for_convert_case_when_by_then(ObIArray<ObRawExpr *> &false_null_exprs,
                                                   ObIArray<ObRawExpr *> &true_exprs);
};
}
}

#endif // OB_TRANSFORM_SIMPLIFY_EXPR_H
