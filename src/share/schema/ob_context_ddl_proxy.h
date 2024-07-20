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

#ifndef __OB_SHARE_SCHEMA_CONTEXT_DDL_PROXY_H__
#define __OB_SHARE_SCHEMA_CONTEXT_DDL_PROXY_H__

#include "lib/utility/ob_macro_utils.h"
#include "lib/container/ob_bit_set.h"

namespace oceanbase
{
namespace common
{
class ObMySQLProxy;
class ObMySQLTransaction;
}
namespace share
{
namespace schema
{
class ObSchemaGetterGuard;
class ObContextSchema;
class ObMultiVersionSchemaService;
}

class ObContextDDLProxy
{
public:
  ObContextDDLProxy(share::schema::ObMultiVersionSchemaService &schema_service);
  virtual ~ObContextDDLProxy();
  int create_context(share::schema::ObContextSchema &ctx_schema,
                      common::ObMySQLTransaction &trans,
                      share::schema::ObSchemaGetterGuard &schema_guard,
                      const bool or_replace,
                      const bool obj_exist,
                      const share::schema::ObContextSchema *old_schema,
                      bool &need_clean,
                      const common::ObString *ddl_stmt_str);
  int inner_create_context(share::schema::ObContextSchema &ctx_schema,
                            common::ObMySQLTransaction &trans,
                            share::schema::ObSchemaGetterGuard &schema_guard,
                            const common::ObString *ddl_stmt_str);
  int drop_context(share::schema::ObContextSchema &ctx_schema,
                    common::ObMySQLTransaction &trans,
                    share::schema::ObSchemaGetterGuard &schema_guard,
                    const share::schema::ObContextSchema *old_schema,
                    bool &need_clean,
                    const common::ObString *ddl_stmt_str);
  int create_or_replace_context(schema::ObContextSchema &ctx_schema,
                                common::ObMySQLTransaction &trans,
                                share::schema::ObSchemaGetterGuard &schema_guard,
                                const bool obj_exist,
                                const schema::ObContextSchema *old_schema,
                                bool &need_clean,
                                const ObString *ddl_stmt_str);
  int inner_alter_context(share::schema::ObContextSchema &ctx_schema,
                            common::ObMySQLTransaction &trans,
                            share::schema::ObSchemaGetterGuard &schema_guard,
                            const ObString *ddl_stmt_str);
private:
  /* functions */
  /* variables */
  DISALLOW_COPY_AND_ASSIGN(ObContextDDLProxy);
  share::schema::ObMultiVersionSchemaService &schema_service_;
};
}
}
#endif /* __OB_SHARE_SCHEMA_CONTEXT_DDL_PROXY_H__ */
//// end of header file