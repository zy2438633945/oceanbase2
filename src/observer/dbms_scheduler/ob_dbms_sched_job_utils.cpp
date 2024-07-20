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

#define USING_LOG_PREFIX RS

#include "ob_dbms_sched_job_utils.h"

#include "lib/ob_errno.h"
#include "lib/oblog/ob_log_module.h"
#include "lib/time/ob_time_utility.h"
#include "lib/string/ob_string.h"
#include "lib/string/ob_sql_string.h"
#include "lib/mysqlclient/ob_mysql_proxy.h"
#include "lib/mysqlclient/ob_mysql_result.h"
#include "lib/mysqlclient/ob_isql_client.h"
#include "lib/mysqlclient/ob_mysql_transaction.h"
#include "rpc/obrpc/ob_rpc_packet.h"
#include "lib/worker.h"
#include "share/ob_dml_sql_splicer.h"
#include "share/inner_table/ob_inner_table_schema_constants.h"
#include "share/schema/ob_schema_utils.h"
#include "observer/omt/ob_tenant_config_mgr.h"
#include "observer/ob_server_struct.h"
#include "sql/session/ob_sql_session_mgr.h"
#include "share/schema/ob_schema_getter_guard.h"
#include "share/stat/ob_dbms_stats_maintenance_window.h"

namespace oceanbase
{

using namespace common;
using namespace share;
using namespace share::schema;
using namespace sqlclient;
using namespace sql;

namespace dbms_scheduler
{

int ObDBMSSchedJobInfo::deep_copy(ObIAllocator &allocator, const ObDBMSSchedJobInfo &other)
{
  int ret = OB_SUCCESS;
  tenant_id_ = other.tenant_id_;
  job_ = other.job_;
  last_modify_ = other.last_modify_;
  last_date_ = other.last_date_;
  this_date_ = other.this_date_;
  next_date_ = other.next_date_;
  total_ = other.total_;
  failures_ = other.failures_;
  flag_ = other.flag_;
  scheduler_flags_ = other.scheduler_flags_;
  start_date_ = other.start_date_;
  end_date_ = other.end_date_;
  enabled_ = other.enabled_;
  auto_drop_ = other.auto_drop_;
  interval_ts_ = other.interval_ts_;
  is_oracle_tenant_ = other.is_oracle_tenant_;
  max_run_duration_ = other.max_run_duration_;

  OZ (ob_write_string(allocator, other.lowner_, lowner_));
  OZ (ob_write_string(allocator, other.powner_, powner_));
  OZ (ob_write_string(allocator, other.cowner_, cowner_));

  OZ (ob_write_string(allocator, other.interval_, interval_));

  OZ (ob_write_string(allocator, other.what_, what_));
  OZ (ob_write_string(allocator, other.nlsenv_, nlsenv_));
  OZ (ob_write_string(allocator, other.charenv_, charenv_));
  OZ (ob_write_string(allocator, other.field1_, field1_));
  OZ (ob_write_string(allocator, other.exec_env_, exec_env_));
  OZ (ob_write_string(allocator, other.job_name_, job_name_));
  OZ (ob_write_string(allocator, other.job_class_, job_class_));
  OZ (ob_write_string(allocator, other.program_name_, program_name_));
  OZ (ob_write_string(allocator, other.state_, state_));
  return ret;
}

int ObDBMSSchedJobClassInfo::deep_copy(common::ObIAllocator &allocator, const ObDBMSSchedJobClassInfo &other)
{
  int ret = OB_SUCCESS;
  tenant_id_ = other.tenant_id_;
  is_oracle_tenant_ = other.is_oracle_tenant_;
  log_history_ = other.log_history_;
  OZ (ob_write_string(allocator, other.job_class_name_, job_class_name_));
  OZ (ob_write_string(allocator, other.service_, service_));
  OZ (ob_write_string(allocator, other.resource_consumer_group_, resource_consumer_group_));
  OZ (ob_write_string(allocator, other.logging_level_, logging_level_));
  OZ (ob_write_string(allocator, other.comments_, comments_));
  return ret;
}

int ObDBMSSchedJobUtils::disable_dbms_sched_job(
    ObISQLClient &sql_client,
    const uint64_t tenant_id,
    const ObString &job_name,
    const bool if_exists)
{
  int ret = OB_SUCCESS;
  if (OB_UNLIKELY(OB_INVALID_TENANT_ID == tenant_id || job_name.empty())) {
    ret = OB_INVALID_ARGUMENT;
    LOG_WARN("invalid args", KR(ret), K(tenant_id), K(job_name));
  } else {
    const uint64_t exec_tenant_id = ObSchemaUtils::get_exec_tenant_id(tenant_id);
    ObDMLSqlSplicer dml;
    if (OB_FAIL(dml.add_pk_column(
        "tenant_id", ObSchemaUtils::get_extract_tenant_id(exec_tenant_id, tenant_id)))
        || OB_FAIL(dml.add_pk_column("job_name", job_name))
        || OB_FAIL(dml.add_column("enabled", false))) {
      LOG_WARN("add column failed", KR(ret));
    } else {
      ObDMLExecHelper exec(sql_client, exec_tenant_id);
      int64_t affected_rows = 0;
      if (OB_FAIL(exec.exec_update(OB_ALL_TENANT_SCHEDULER_JOB_TNAME, dml, affected_rows))) {
        LOG_WARN("execute update failed", KR(ret));
      } else if (!if_exists && !is_double_row(affected_rows)) {
        ret = OB_ERR_UNEXPECTED;
        LOG_WARN("affected_rows unexpected to be two", KR(ret), K(affected_rows));
      }
    }
  }
  return ret;
}

int ObDBMSSchedJobUtils::remove_dbms_sched_job(
    ObISQLClient &sql_client,
    const uint64_t tenant_id,
    const ObString &job_name,
    const bool if_exists)
{
  int ret = OB_SUCCESS;
  if (OB_UNLIKELY(OB_INVALID_TENANT_ID == tenant_id || job_name.empty())) {
    ret = OB_INVALID_ARGUMENT;
    LOG_WARN("invalid args", KR(ret), K(tenant_id), K(job_name));
  } else {
    const uint64_t exec_tenant_id = ObSchemaUtils::get_exec_tenant_id(tenant_id);
    ObDMLSqlSplicer dml;
    if (OB_FAIL(dml.add_pk_column(
        "tenant_id", ObSchemaUtils::get_extract_tenant_id(exec_tenant_id, tenant_id)))
        || OB_FAIL(dml.add_pk_column("job_name", job_name))) {
      LOG_WARN("add column failed", KR(ret));
    } else {
      ObDMLExecHelper exec(sql_client, exec_tenant_id);
      int64_t affected_rows = 0;
      if (OB_FAIL(exec.exec_delete(OB_ALL_TENANT_SCHEDULER_JOB_TNAME, dml, affected_rows))) {
        LOG_WARN("execute delete failed", KR(ret));
      } else if (!if_exists && !is_double_row(affected_rows)) {
        ret = OB_ERR_UNEXPECTED;
        LOG_WARN("affected_rows unexpected to be two", KR(ret), K(affected_rows));
      }
    }
  }
  return ret;
}

int ObDBMSSchedJobUtils::create_dbms_sched_job(
    common::ObISQLClient &sql_client,
    const uint64_t tenant_id,
    const int64_t job_id,
    const dbms_scheduler::ObDBMSSchedJobInfo &job_info)
{
  int ret = OB_SUCCESS;
  if (OB_INVALID_TENANT_ID == tenant_id) {
    ret = OB_INVALID_ARGUMENT;
    LOG_WARN("invalid tenant id", KR(ret), K(tenant_id));
  } else {
    if (OB_FAIL(add_dbms_sched_job(sql_client, tenant_id, job_id, job_info))) {
      LOG_WARN("failed to add dbms scheduler job", KR(ret));
    } else if (OB_FAIL(add_dbms_sched_job(sql_client, tenant_id, 0, job_info))) {
      LOG_WARN("failed to add dbms scheduler job", KR(ret));
    }
  }
  return ret;
}

int ObDBMSSchedJobUtils::add_dbms_sched_job(
    common::ObISQLClient &sql_client,
    const uint64_t tenant_id,
    const int64_t job_id,
    const dbms_scheduler::ObDBMSSchedJobInfo &job_info)
{
  int ret = OB_SUCCESS;
  if (OB_INVALID_TENANT_ID == tenant_id) {
    ret = OB_INVALID_ARGUMENT;
    LOG_WARN("invalid tenant id", KR(ret), K(tenant_id));
  } else {
    ObDMLSqlSplicer dml;
    const uint64_t exec_tenant_id = ObSchemaUtils::get_exec_tenant_id(tenant_id);
    ObDMLExecHelper exec(sql_client, exec_tenant_id);
    int64_t affected_rows = 0;
    const int64_t now = ObTimeUtility::current_time();

    OZ (dml.add_gmt_create(now));
    OZ (dml.add_gmt_modified(now));
    OZ (dml.add_pk_column("tenant_id",
        ObSchemaUtils::get_extract_tenant_id(job_info.tenant_id_, job_info.tenant_id_)));
    OZ (dml.add_pk_column("job", job_id));
    OZ (dml.add_column("lowner", ObHexEscapeSqlStr(job_info.lowner_)));
    OZ (dml.add_column("powner", ObHexEscapeSqlStr(job_info.powner_)));
    OZ (dml.add_column("cowner", ObHexEscapeSqlStr(job_info.cowner_)));
    OZ (dml.add_raw_time_column("next_date", job_info.start_date_));
    OZ (dml.add_column("total", 0));
    OZ (dml.add_column("`interval#`", ObHexEscapeSqlStr(
        job_info.repeat_interval_.empty() ? ObString("null") : job_info.repeat_interval_)));
    OZ (dml.add_column("flag", job_info.flag_));
    OZ (dml.add_column("job_name", ObHexEscapeSqlStr(job_info.job_name_)));
    OZ (dml.add_column("job_style", ObHexEscapeSqlStr(job_info.job_style_)));
    OZ (dml.add_column("job_type", ObHexEscapeSqlStr(job_info.job_type_)));
    OZ (dml.add_column("job_class", ObHexEscapeSqlStr(job_info.job_class_)));
    OZ (dml.add_column("job_action", ObHexEscapeSqlStr(job_info.job_action_)));
    OZ (dml.add_column("what", ObHexEscapeSqlStr(job_info.job_action_)));
    OZ (dml.add_raw_time_column("start_date", job_info.start_date_));
    OZ (dml.add_raw_time_column("end_date", job_info.end_date_));
    OZ (dml.add_column("repeat_interval", ObHexEscapeSqlStr(job_info.repeat_interval_)));
    OZ (dml.add_column("enabled", job_info.enabled_));
    OZ (dml.add_column("auto_drop", job_info.auto_drop_));
    OZ (dml.add_column("max_run_duration", job_info.max_run_duration_));
    OZ (dml.add_column("interval_ts", job_info.interval_ts_));
    OZ (dml.add_column("scheduler_flags", job_info.scheduler_flags_));
    OZ (dml.add_column("exec_env", job_info.exec_env_));

    if (OB_SUCC(ret) && OB_FAIL(exec.exec_insert(
        OB_ALL_TENANT_SCHEDULER_JOB_TNAME, dml, affected_rows))) {
      LOG_WARN("failed to execute insert", KR(ret));
    } else if (OB_UNLIKELY(!is_single_row(affected_rows))) {
      ret = OB_ERR_UNEXPECTED;
      LOG_WARN("affected_rows unexpected to be one", KR(ret), K(affected_rows));
    }
  }
  return ret;
}

int ObDBMSSchedJobUtils::init_session(
  ObSQLSessionInfo &session,
  ObSchemaGetterGuard &schema_guard,
  const ObString &tenant_name,
  uint64_t tenant_id,
  const ObString &database_name,
  uint64_t database_id,
  const ObUserInfo* user_info,
  const ObDBMSSchedJobInfo &job_info)
{
  int ret = OB_SUCCESS;
  ObArenaAllocator *allocator = NULL;
  const bool print_info_log = true;
  const bool is_sys_tenant = true;
  ObPCMemPctConf pc_mem_conf;
  ObObj compatibility_mode;
  ObObj sql_mode;
  if (job_info.is_oracle_tenant_) {
    compatibility_mode.set_int(1);
    sql_mode.set_uint(ObUInt64Type, DEFAULT_ORACLE_MODE);
  } else {
    compatibility_mode.set_int(0);
    sql_mode.set_uint(ObUInt64Type, DEFAULT_MYSQL_MODE);
  }
  OX (session.set_inner_session());
  OZ (session.load_default_sys_variable(print_info_log, is_sys_tenant));
  OZ (session.update_max_packet_size());
  OZ (session.init_tenant(tenant_name.ptr(), tenant_id));
  OZ (session.load_all_sys_vars(schema_guard));
  OZ (session.update_sys_variable(share::SYS_VAR_SQL_MODE, sql_mode));
  OZ (session.update_sys_variable(share::SYS_VAR_OB_COMPATIBILITY_MODE, compatibility_mode));
  OZ (session.update_sys_variable(share::SYS_VAR_NLS_DATE_FORMAT,
                                  ObTimeConverter::COMPAT_OLD_NLS_DATE_FORMAT));
  OZ (session.update_sys_variable(share::SYS_VAR_NLS_TIMESTAMP_FORMAT,
                                  ObTimeConverter::COMPAT_OLD_NLS_TIMESTAMP_FORMAT));
  OZ (session.update_sys_variable(share::SYS_VAR_NLS_TIMESTAMP_TZ_FORMAT,
                                  ObTimeConverter::COMPAT_OLD_NLS_TIMESTAMP_TZ_FORMAT));
  OZ (session.set_default_database(database_name));
  OZ (session.get_pc_mem_conf(pc_mem_conf));
  CK (OB_NOT_NULL(GCTX.sql_engine_));
  OX (session.set_database_id(database_id));
  OZ (session.set_user(
    user_info->get_user_name(), user_info->get_host_name_str(), user_info->get_user_id()));
  OX (session.set_user_priv_set(OB_PRIV_ALL | OB_PRIV_GRANT));
  if (OB_SUCC(ret) && job_info.is_date_expression_job_class()) {
    // set larger timeout for mview scheduler jobs
    const int64_t QUERY_TIMEOUT_US = (24 * 60 * 60 * 1000000L); // 24hours
    const int64_t TRX_TIMEOUT_US = (24 * 60 * 60 * 1000000L); // 24hours
    ObObj query_timeout_obj;
    ObObj trx_timeout_obj;
    query_timeout_obj.set_int(QUERY_TIMEOUT_US);
    trx_timeout_obj.set_int(TRX_TIMEOUT_US);
    OZ (session.update_sys_variable(SYS_VAR_OB_QUERY_TIMEOUT, query_timeout_obj));
    OZ (session.update_sys_variable(SYS_VAR_OB_TRX_TIMEOUT, trx_timeout_obj));
  }
  return ret;
}

int ObDBMSSchedJobUtils::reserve_user_with_minimun_id(ObIArray<const ObUserInfo *> &user_infos)
{
  int ret = OB_SUCCESS;
  if (user_infos.count() > 1) {
    //bug:
    //resver the minimum user id to execute
    const ObUserInfo *minimum_user_info = user_infos.at(0);
    for (int64_t i = 1; OB_SUCC(ret) && i < user_infos.count(); ++i) {
      if (OB_ISNULL(minimum_user_info) || OB_ISNULL(user_infos.at(i))) {
        ret = OB_ERR_UNEXPECTED;
        LOG_WARN("get unexpected error", K(ret), K(minimum_user_info), K(user_infos.at(i)));
      } else if (minimum_user_info->get_user_id() > user_infos.at(i)->get_user_id()) {
        minimum_user_info = user_infos.at(i);
      } else {/*do nothing*/}
    }
    if (OB_SUCC(ret)) {
      user_infos.reset();
      if (OB_FAIL(user_infos.push_back(minimum_user_info))) {
        LOG_WARN("failed to push back", K(ret));
      }
    }
  }
  return ret;
}

int ObDBMSSchedJobUtils::init_env(
    ObDBMSSchedJobInfo &job_info,
    ObSQLSessionInfo &session)
{
  int ret = OB_SUCCESS;
  ObSchemaGetterGuard schema_guard;
  const ObTenantSchema *tenant_info = NULL;
  const ObSysVariableSchema *sys_variable_schema = NULL;
  ObSEArray<const ObUserInfo *, 1> user_infos;
  const ObUserInfo* user_info = NULL;
  const ObDatabaseSchema *database_schema = NULL;
  share::schema::ObUserLoginInfo login_info;
  ObExecEnv exec_env;
  CK (OB_NOT_NULL(GCTX.schema_service_));
  CK (job_info.valid());
  OZ (GCTX.schema_service_->get_tenant_schema_guard(job_info.get_tenant_id(), schema_guard));
  OZ (schema_guard.get_tenant_info(job_info.get_tenant_id(), tenant_info));
  OZ (schema_guard.get_database_schema(
    job_info.get_tenant_id(), job_info.get_cowner(), database_schema));
  if (OB_SUCC(ret)) {
    if (job_info.is_oracle_tenant()) {
      OZ (schema_guard.get_user_info(
        job_info.get_tenant_id(), job_info.get_powner(), user_infos));
      OV (1 == user_infos.count(), OB_ERR_UNEXPECTED, K(job_info), K(user_infos));
      CK (OB_NOT_NULL(user_info = user_infos.at(0)));
    } else {
      ObString user = job_info.get_powner();
      if (OB_SUCC(ret)) {
        const char *c = user.reverse_find('@');
        if (OB_ISNULL(c)) {
          OZ (schema_guard.get_user_info(
            job_info.get_tenant_id(), user, user_infos));
          if (OB_SUCC(ret) && user_infos.count() > 1) {
            OZ(reserve_user_with_minimun_id(user_infos));
          }
          OV (1 == user_infos.count(), 0 == user_infos.count() ? OB_USER_NOT_EXIST : OB_ERR_UNEXPECTED, K(job_info), K(user_infos));
          CK (OB_NOT_NULL(user_info = user_infos.at(0)));
        } else {
          ObString user_name;
          ObString host_name;
          user_name = user.split_on(c);
          host_name = user;
          OZ (schema_guard.get_user_info(
            job_info.get_tenant_id(), user_name, host_name, user_info));
        }
      }
    }
    CK (OB_NOT_NULL(user_info));
    CK (OB_NOT_NULL(tenant_info));
    CK (OB_NOT_NULL(database_schema));
    OZ (exec_env.init(job_info.get_exec_env()));
    OZ (init_session(session,
                    schema_guard,
                    tenant_info->get_tenant_name(),
                    job_info.get_tenant_id(),
                    database_schema->get_database_name(),
                    database_schema->get_database_id(),
                    user_info,
                    job_info));
    OZ (exec_env.store(session));
  }
  return ret;
}

int ObDBMSSchedJobUtils::create_session(
    const uint64_t tenant_id,
    ObFreeSessionCtx &free_session_ctx,
    ObSQLSessionInfo *&session_info)
{
  int ret = OB_SUCCESS;
  uint32_t sid = sql::ObSQLSessionInfo::INVALID_SESSID;
  uint64_t proxy_sid = 0;
  if (OB_ISNULL(GCTX.session_mgr_)) {
    ret = OB_ERR_UNEXPECTED;
    LOG_WARN("session_mgr_ is null", KR(ret));
  } else if (OB_FAIL(GCTX.session_mgr_->create_sessid(sid))) {
    LOG_WARN("alloc session id failed", KR(ret));
  } else if (OB_FAIL(GCTX.session_mgr_->create_session(
                tenant_id, sid, proxy_sid, ObTimeUtility::current_time(), session_info))) {
    LOG_WARN("create session failed", K(ret), K(sid));
    GCTX.session_mgr_->mark_sessid_unused(sid);
    session_info = NULL;
  } else if (OB_ISNULL(session_info)) {
    ret = OB_ERR_UNEXPECTED;
    LOG_WARN("unexpected session info is null", K(ret));
  } else {
    free_session_ctx.sessid_ = sid;
    free_session_ctx.proxy_sessid_ = proxy_sid;
  }
  return ret;
}

int ObDBMSSchedJobUtils::destroy_session(
    ObFreeSessionCtx &free_session_ctx,
    ObSQLSessionInfo *session_info)
{
  int ret = OB_SUCCESS;
  if (OB_ISNULL(GCTX.session_mgr_)) {
    ret = OB_ERR_UNEXPECTED;
    LOG_WARN("session_mgr_ is null", KR(ret));
  } else if (OB_ISNULL(session_info)) {
    ret = OB_INVALID_ARGUMENT;
    LOG_WARN("session_info is null", KR(ret));
  } else {
    session_info->set_session_sleep();
    GCTX.session_mgr_->revert_session(session_info);
    GCTX.session_mgr_->free_session(free_session_ctx);
    GCTX.session_mgr_->mark_sessid_unused(free_session_ctx.sessid_);
  }
  return ret;
}

} // end for namespace dbms_scheduler
} // end for namespace oceanbase
