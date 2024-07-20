/**
 * Copyright (c) 2023 OceanBase
 * OceanBase CE is licensed under Mulan PubL v2.
 * You can use this software according to the terms and conditions of the Mulan PubL v2.
 * You may obtain a copy of Mulan PubL v2 at:
 *          http://license.coscl.org.cn/MulanPubL-2.0
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PubL v2 for more details.
 */

#define USING_LOG_PREFIX SERVER

#include "observer/table_load/ob_table_load_client_task.h"
#include "observer/ob_server.h"
#include "observer/omt/ob_tenant.h"
#include "observer/table_load/ob_table_load_exec_ctx.h"
#include "observer/table_load/ob_table_load_schema.h"
#include "observer/table_load/ob_table_load_service.h"
#include "observer/table_load/ob_table_load_table_ctx.h"
#include "observer/table_load/ob_table_load_task.h"
#include "observer/table_load/ob_table_load_task_scheduler.h"
#include "observer/table_load/ob_table_load_utils.h"

namespace oceanbase
{
namespace observer
{
using namespace common;
using namespace sql;
using namespace storage;
using namespace share::schema;
using namespace table;

/**
 * ObTableLoadClientTaskParam
 */

ObTableLoadClientTaskParam::ObTableLoadClientTaskParam()
  : tenant_id_(OB_INVALID_TENANT_ID),
    user_id_(OB_INVALID_ID),
    database_id_(OB_INVALID_ID),
    table_id_(OB_INVALID_ID),
    parallel_(0),
    max_error_row_count_(0),
    dup_action_(ObLoadDupActionType::LOAD_INVALID_MODE),
    timeout_us_(0),
    heartbeat_timeout_us_(0)
{
}

ObTableLoadClientTaskParam::~ObTableLoadClientTaskParam() {}

void ObTableLoadClientTaskParam::reset()
{
  client_addr_.reset();
  tenant_id_ = OB_INVALID_TENANT_ID;
  user_id_ = OB_INVALID_ID;
  database_id_ = OB_INVALID_ID;
  table_id_ = OB_INVALID_ID;
  parallel_ = 0;
  max_error_row_count_ = 0;
  dup_action_ = ObLoadDupActionType::LOAD_INVALID_MODE;
  timeout_us_ = 0;
  heartbeat_timeout_us_ = 0;
}

int ObTableLoadClientTaskParam::assign(const ObTableLoadClientTaskParam &other)
{
  int ret = OB_SUCCESS;
  if (this != &other) {
    reset();
    client_addr_ = other.client_addr_;
    tenant_id_ = other.tenant_id_;
    user_id_ = other.user_id_;
    database_id_ = other.database_id_;
    table_id_ = other.table_id_;
    parallel_ = other.parallel_;
    max_error_row_count_ = other.max_error_row_count_;
    dup_action_ = other.dup_action_;
    timeout_us_ = other.timeout_us_;
    heartbeat_timeout_us_ = other.heartbeat_timeout_us_;
  }
  return ret;
}

bool ObTableLoadClientTaskParam::is_valid() const
{
  return client_addr_.is_valid() && OB_INVALID_TENANT_ID != tenant_id_ &&
         OB_INVALID_ID != user_id_ && OB_INVALID_ID != database_id_ && OB_INVALID_ID != table_id_ &&
         parallel_ > 0 && ObLoadDupActionType::LOAD_INVALID_MODE != dup_action_ &&
         timeout_us_ > 0 && heartbeat_timeout_us_ > 0;
}

/**
 * ClientTaskExecuteProcessor
 */

class ObTableLoadClientTask::ClientTaskExectueProcessor : public ObITableLoadTaskProcessor
{
public:
  ClientTaskExectueProcessor(ObTableLoadTask &task, ObTableLoadClientTask *client_task)
    : ObITableLoadTaskProcessor(task), client_task_(client_task)
  {
    client_task_->inc_ref_count();
  }
  virtual ~ClientTaskExectueProcessor() { ObTableLoadClientService::revert_task(client_task_); }
  int process() override
  {
    int ret = OB_SUCCESS;
    ObSQLSessionInfo *origin_session_info = THIS_WORKER.get_session();
    ObSQLSessionInfo *session_info = client_task_->get_session_info();
    THIS_WORKER.set_session(session_info);
    if (OB_ISNULL(session_info)) {
      ret = OB_ERR_UNEXPECTED;
      LOG_WARN("unexpected null session info", KR(ret));
    } else {
      session_info->set_thread_id(GETTID());
      session_info->update_last_active_time();
      if (OB_FAIL(session_info->set_session_state(QUERY_ACTIVE))) {
        LOG_WARN("fail to set session state", K(ret));
      }
    }
    // begin
    if (OB_SUCC(ret)) {
      if (OB_FAIL(client_task_->init_instance())) {
        LOG_WARN("fail to init instance", KR(ret));
      } else if (OB_FAIL(client_task_->set_status_waitting())) {
        LOG_WARN("fail to set status waitting", KR(ret));
      } else if (OB_FAIL(client_task_->set_status_running())) {
        LOG_WARN("fail to set status running", KR(ret));
      }
    }
    // wait client commit
    while (OB_SUCC(ret)) {
      ObTableLoadClientStatus status = client_task_->get_status();
      if (ObTableLoadClientStatus::RUNNING == status) {
        ob_usleep(100LL * 1000); // sleep 100ms
      } else if (ObTableLoadClientStatus::COMMITTING == status) {
        break;
      } else {
        ret = OB_ERR_UNEXPECTED;
        LOG_WARN("unexpected client status", KR(ret), K(status));
      }
    }
    // commit
    if (OB_SUCC(ret)) {
      if (OB_FAIL(client_task_->commit_instance())) {
        LOG_WARN("fail to commit instance", KR(ret));
      } else if (OB_FAIL(client_task_->set_status_commit())) {
        LOG_WARN("fail to set status running", KR(ret));
      }
    }
    client_task_->destroy_instance();
    THIS_WORKER.set_session(origin_session_info);
    return ret;
  }

private:
  ObTableLoadClientTask *client_task_;
};

/**
 * ClientTaskExectueCallback
 */

class ObTableLoadClientTask::ClientTaskExectueCallback : public ObITableLoadTaskCallback
{
public:
  ClientTaskExectueCallback(ObTableLoadClientTask *client_task) : client_task_(client_task)
  {
    client_task_->inc_ref_count();
  }
  virtual ~ClientTaskExectueCallback() { ObTableLoadClientService::revert_task(client_task_); }
  void callback(int ret_code, ObTableLoadTask *task) override
  {
    if (OB_UNLIKELY(OB_SUCCESS != ret_code)) {
      client_task_->set_status_abort(ret_code);
    }
    task->~ObTableLoadTask();
  }

private:
  ObTableLoadClientTask *client_task_;
};

/**
 * ObTableLoadClientTask
 */

ObTableLoadClientTask::ObTableLoadClientTask()
  : task_id_(OB_INVALID_ID),
    allocator_("TLD_ClientTask"),
    task_scheduler_(nullptr),
    session_info_(nullptr),
    exec_ctx_(nullptr),
    session_count_(0),
    next_batch_id_(0),
    client_status_(ObTableLoadClientStatus::MAX_STATUS),
    error_code_(OB_SUCCESS),
    ref_count_(0),
    is_inited_(false)
{
  allocator_.set_tenant_id(MTL_ID());
  free_session_ctx_.sessid_ = sql::ObSQLSessionInfo::INVALID_SESSID;
}

ObTableLoadClientTask::~ObTableLoadClientTask()
{
  if (nullptr != task_scheduler_) {
    task_scheduler_->stop();
    task_scheduler_->wait();
    task_scheduler_->~ObITableLoadTaskScheduler();
    allocator_.free(task_scheduler_);
    task_scheduler_ = nullptr;
  }
  if (nullptr != exec_ctx_) {
    exec_ctx_->~ObTableLoadClientExecCtx();
    allocator_.free(exec_ctx_);
    exec_ctx_ = nullptr;
  }
  if (nullptr != session_info_) {
    ObTableLoadUtils::free_session_info(session_info_, free_session_ctx_);
    session_info_ = nullptr;
  }
}

int ObTableLoadClientTask::init(const ObTableLoadClientTaskParam &param)
{
  int ret = OB_SUCCESS;
  if (IS_INIT) {
    ret = OB_INIT_TWICE;
    LOG_WARN("ObTableLoadClientTask init twice", KR(ret), KP(this));
  } else if (OB_UNLIKELY(!param.is_valid())) {
    ret = OB_INVALID_ARGUMENT;
    LOG_WARN("invalid args", KR(ret), K(param));
  } else if (OB_FAIL(param_.assign(param))) {
    LOG_WARN("fail to assign param", KR(ret));
  } else {
    const int64_t origin_timeout_ts = THIS_WORKER.get_timeout_ts();
    THIS_WORKER.set_timeout_ts(ObTimeUtil::current_time() + param_.get_timeout_us());
    if (OB_FAIL(create_session_info(param_.get_tenant_id(), param_.get_user_id(),
                                    param_.get_database_id(), param_.get_table_id(), session_info_,
                                    free_session_ctx_))) {
      LOG_WARN("fail to create session info", KR(ret));
    } else if (OB_FAIL(init_exec_ctx(param_.get_timeout_us(), param_.get_heartbeat_timeout_us()))) {
      LOG_WARN("fail to init client exec ctx", KR(ret));
    } else if (OB_ISNULL(task_scheduler_ =
                           OB_NEWx(ObTableLoadTaskThreadPoolScheduler, (&allocator_), 1,
                                   param_.get_table_id(), "Client"))) {
      ret = OB_ALLOCATE_MEMORY_FAILED;
      LOG_WARN("fail to new ObTableLoadTaskThreadPoolScheduler", KR(ret));
    } else if (OB_FAIL(task_scheduler_->init())) {
      LOG_WARN("fail to init task scheduler", KR(ret));
    } else if (OB_FAIL(task_scheduler_->start())) {
      LOG_WARN("fail to start task scheduler", KR(ret));
    } else {
      is_inited_ = true;
    }
    THIS_WORKER.set_timeout_ts(origin_timeout_ts);
  }
  return ret;
}

int ObTableLoadClientTask::create_session_info(uint64_t tenant_id, uint64_t user_id,
                                               uint64_t database_id, uint64_t table_id,
                                               ObSQLSessionInfo *&session_info,
                                               ObFreeSessionCtx &free_session_ctx)
{
  int ret = OB_SUCCESS;
  schema::ObSchemaGetterGuard schema_guard;
  const schema::ObTenantSchema *tenant_info = nullptr;
  const ObUserInfo *user_info = nullptr;
  const ObDatabaseSchema *database_schema = nullptr;
  const ObTableSchema *table_schema = nullptr;
  if (OB_ISNULL(GCTX.schema_service_)) {
    ret = OB_ERR_UNEXPECTED;
    LOG_WARN("invalid argument", K(ret), K(GCTX.schema_service_));
  } else if (OB_FAIL(GCTX.schema_service_->get_tenant_schema_guard(tenant_id, schema_guard))) {
    LOG_WARN("get_schema_guard failed", K(ret));
  } else if (OB_FAIL(schema_guard.get_tenant_info(tenant_id, tenant_info))) {
    LOG_WARN("get tenant info failed", K(ret));
  } else if (OB_ISNULL(tenant_info)) {
    ret = OB_ERR_UNEXPECTED;
    LOG_WARN("invalid tenant schema", K(ret), K(tenant_id));
  } else if (OB_FAIL(schema_guard.get_user_info(tenant_id, user_id, user_info))) {
    LOG_WARN("get user info failed", K(ret));
  } else if (OB_ISNULL(user_info)) {
    ret = OB_ERR_UNEXPECTED;
    LOG_WARN("invalid user_info", K(ret), K(tenant_id), K(user_id));
  } else if (OB_FAIL(schema_guard.get_database_schema(tenant_id, database_id, database_schema))) {
    LOG_WARN("get database schema failed", K(ret));
  } else if (OB_ISNULL(database_schema)) {
    ret = OB_ERR_UNEXPECTED;
    LOG_WARN("invalid database schema", K(ret), K(tenant_id), K(database_id));
  } else if (OB_FAIL(schema_guard.get_table_schema(tenant_id, table_id, table_schema))) {
    LOG_WARN("fail to get table schema", KR(ret));
  } else if (OB_ISNULL(table_schema)) {
    ret = OB_ERR_UNEXPECTED;
    LOG_WARN("invalid table schema", K(ret), K(tenant_id), K(table_id));
  } else if (OB_FAIL(ObTableLoadUtils::create_session_info(session_info, free_session_ctx))) {
    LOG_WARN("create session id failed", KR(ret));
  } else {
    ObArenaAllocator allocator("TLD_Tmp");
    allocator.set_tenant_id(MTL_ID());
    ObStringBuffer buffer(&allocator);
    buffer.append("DIRECT LOAD: ");
    buffer.append(table_schema->get_table_name());
    ObObj timeout_val;
    timeout_val.set_int(param_.get_timeout_us());
    OZ(session_info->load_default_sys_variable(false, false)); //加载默认的session参数
    OZ(session_info->load_default_configs_in_pc());
    OX(session_info->init_tenant(tenant_info->get_tenant_name(), tenant_id));
    OX(session_info->set_priv_user_id(user_id));
    OX(session_info->store_query_string(ObString(buffer.length(), buffer.ptr())));
    OX(session_info->set_user(user_info->get_user_name(), user_info->get_host_name_str(),
                              user_info->get_user_id()));
    OX(session_info->set_user_priv_set(OB_PRIV_ALL | OB_PRIV_GRANT));
    OX(session_info->set_default_database(database_schema->get_database_name(),
                                          CS_TYPE_UTF8MB4_GENERAL_CI));
    OX(session_info->set_mysql_cmd(COM_QUERY));
    OX(session_info->set_current_trace_id(ObCurTraceId::get_trace_id()));
    OX(session_info->set_client_addr(param_.get_client_addr()));
    OX(session_info->set_peer_addr(ObServer::get_instance().get_self()));
    OX(session_info->set_query_start_time(ObTimeUtil::current_time()));
    OZ(session_info->update_sys_variable(SYS_VAR_OB_QUERY_TIMEOUT, timeout_val));
  }
  if (OB_FAIL(ret)) {
    if (session_info != nullptr) {
      observer::ObTableLoadUtils::free_session_info(session_info, free_session_ctx);
      session_info = nullptr;
    }
  }
  return ret;
}

int ObTableLoadClientTask::get_column_idxs(ObIArray<int64_t> &column_idxs) const
{
  int ret = OB_SUCCESS;
  const uint64_t tenant_id = param_.get_tenant_id();
  const uint64_t table_id = param_.get_table_id();
  ObSchemaGetterGuard schema_guard;
  const ObTableSchema *table_schema = nullptr;
  ObArray<int64_t> column_ids; // in user define order
  ObArray<ObColDesc> column_descs; // in storage order
  bool found_column = true;
  column_idxs.reset();
  if (OB_FAIL(
        ObTableLoadSchema::get_table_schema(tenant_id, table_id, schema_guard, table_schema))) {
    LOG_WARN("fail to get table schema", KR(ret), K(tenant_id), K(table_id));
  } else if (OB_FAIL(ObTableLoadSchema::get_user_column_ids(table_schema, column_ids))) {
    LOG_WARN("failed to get all column idx", K(ret));
  } else if (OB_UNLIKELY(column_ids.empty())) {
    ret = OB_ERR_UNEXPECTED;
    LOG_WARN("unexpected empty column idxs", KR(ret));
  } else if (OB_FAIL(table_schema->get_column_ids(column_descs))) {
    LOG_WARN("fail to get column descs", KR(ret));
  }
  for (int64_t i = 0; OB_SUCC(ret) && OB_LIKELY(found_column) && i < column_descs.count(); ++i) {
    const ObColDesc &col_desc = column_descs.at(i);
    const ObColumnSchemaV2 *col_schema = table_schema->get_column_schema(col_desc.col_id_);
    if (OB_ISNULL(col_schema)) {
      ret = OB_ERR_UNEXPECTED;
      LOG_WARN("unexpected null column schema", KR(ret), K(col_desc));
    } else {
      found_column = col_schema->is_hidden();
    }
    // 在用户定义的列数组中找到对应的列
    for (int64_t j = 0; OB_SUCC(ret) && OB_LIKELY(!found_column) && j < column_ids.count(); ++j) {
      const int64_t column_id = column_ids.at(j);
      if (col_desc.col_id_ == column_id) {
        found_column = true;
        if (OB_FAIL(column_idxs.push_back(j))) {
          LOG_WARN("fail to push back column desc", KR(ret), K(column_idxs), K(i), K(col_desc),
                   K(j), K(column_ids));
        }
      }
    }
  }
  if (OB_SUCC(ret) && OB_UNLIKELY(!found_column)) {
    ret = OB_ERR_UNEXPECTED;
    LOG_WARN("unexpected column not found", KR(ret), K(column_ids), K(column_descs),
             K(column_idxs));
  }
  return ret;
}

int ObTableLoadClientTask::init_exec_ctx(int64_t timeout_us, int64_t heartbeat_timeout_us)
{
  int ret = OB_SUCCESS;
  if (OB_ISNULL(exec_ctx_ = OB_NEWx(ObTableLoadClientExecCtx, &allocator_))) {
    ret = OB_ALLOCATE_MEMORY_FAILED;
    LOG_WARN("fail to new client exec ctx", KR(ret));
  } else {
    exec_ctx_->allocator_ = &allocator_;
    exec_ctx_->session_info_ = session_info_;
    exec_ctx_->timeout_ts_ = ObTimeUtil::current_time() + timeout_us;
    exec_ctx_->last_heartbeat_time_ = ObTimeUtil::current_time();
    exec_ctx_->heartbeat_timeout_us_ = heartbeat_timeout_us;
  }
  return ret;
}

int ObTableLoadClientTask::start()
{
  int ret = OB_SUCCESS;
  if (IS_NOT_INIT) {
    ret = OB_NOT_INIT;
    LOG_WARN("ObTableLoadClientTask not init", KR(ret));
  } else {
    obsys::ObWLockGuard guard(rw_lock_);
    if (OB_UNLIKELY(ObTableLoadClientStatus::MAX_STATUS != client_status_)) {
      ret = OB_STATE_NOT_MATCH;
      LOG_WARN("unexpected status", KR(ret), K(client_status_));
    } else {
      client_status_ = ObTableLoadClientStatus::INITIALIZING;
      ObTableLoadTask *task = nullptr;
      if (OB_ISNULL(task = OB_NEWx(ObTableLoadTask, &allocator_, param_.get_tenant_id()))) {
        ret = OB_ALLOCATE_MEMORY_FAILED;
        LOG_WARN("fail to new ObTableLoadTask", KR(ret));
      } else if (OB_FAIL(task->set_processor<ClientTaskExectueProcessor>(this))) {
        LOG_WARN("fail to set client task processor", KR(ret));
      } else if (OB_FAIL(task->set_callback<ClientTaskExectueCallback>(this))) {
        LOG_WARN("fail to set common task callback", KR(ret));
      } else if (OB_FAIL(task_scheduler_->add_task(0, task))) {
        LOG_WARN("fail to add task", KR(ret));
      }
      if (OB_FAIL(ret)) {
        client_status_ = ObTableLoadClientStatus::ERROR;
        error_code_ = ret;
        if (nullptr != task) {
          task->~ObTableLoadTask();
          allocator_.free(task);
          task = nullptr;
        }
      }
    }
  }
  return ret;
}

int ObTableLoadClientTask::write(ObTableLoadObjRowArray &obj_rows)
{
  int ret = OB_SUCCESS;
  if (IS_NOT_INIT) {
    ret = OB_NOT_INIT;
    LOG_WARN("ObTableLoadClientTask not init", KR(ret));
  } else if (OB_UNLIKELY(obj_rows.empty())) {
    ret = OB_INVALID_ARGUMENT;
    LOG_WARN("invalid args", KR(ret), K(obj_rows));
  } else if (OB_UNLIKELY(session_count_ <= 0)) {
    ret = OB_ERR_UNEXPECTED;
    LOG_WARN("unexpected session count", KR(ret), K(session_count_));
  } else {
    const int64_t batch_id = ATOMIC_FAA(&next_batch_id_, 1);
    ;
    const int32_t session_id = batch_id % session_count_ + 1;
    ObTableLoadSequenceNo start_seq_no(batch_id << ObTableLoadSequenceNo::BATCH_ID_SHIFT);
    for (int64_t i = 0; OB_SUCC(ret) && i < obj_rows.count(); ++i) {
      ObTableLoadObjRow &row = obj_rows.at(i);
      row.seq_no_ = start_seq_no++;
    }
    if (OB_SUCC(ret)) {
      if (OB_FAIL(instance_.write(session_id, obj_rows))) {
        LOG_WARN("fail to write", KR(ret));
      }
    }
  }
  return ret;
}

int ObTableLoadClientTask::commit()
{
  int ret = OB_SUCCESS;
  if (IS_NOT_INIT) {
    ret = OB_NOT_INIT;
    LOG_WARN("ObTableLoadClientTask not init", KR(ret));
  } else if (OB_FAIL(check_status(ObTableLoadClientStatus::RUNNING))) {
    LOG_WARN("fail to check status", KR(ret));
  } else if (OB_FAIL(set_status_committing())) {
    LOG_WARN("fail to set status committing", KR(ret));
  }
  return ret;
}

void ObTableLoadClientTask::abort()
{
  int ret = OB_SUCCESS;
  if (IS_NOT_INIT) {
    ret = OB_NOT_INIT;
    LOG_WARN("ObTableLoadClientTask not init", KR(ret));
  } else {
    set_status_abort();
    if (nullptr != session_info_ && OB_FAIL(session_info_->kill_query())) {
      LOG_WARN("fail to kill query", KR(ret));
    }
  }
}

int ObTableLoadClientTask::set_status_waitting()
{
  int ret = OB_SUCCESS;
  obsys::ObWLockGuard guard(rw_lock_);
  if (OB_UNLIKELY(ObTableLoadClientStatus::INITIALIZING != client_status_)) {
    ret = OB_STATE_NOT_MATCH;
    LOG_WARN("unexpected status", KR(ret), K(client_status_));
  } else {
    client_status_ = ObTableLoadClientStatus::WAITTING;
  }
  return ret;
}

int ObTableLoadClientTask::set_status_running()
{
  int ret = OB_SUCCESS;
  obsys::ObWLockGuard guard(rw_lock_);
  if (OB_UNLIKELY(ObTableLoadClientStatus::WAITTING != client_status_)) {
    ret = OB_STATE_NOT_MATCH;
    LOG_WARN("unexpected status", KR(ret), K(client_status_));
  } else {
    client_status_ = ObTableLoadClientStatus::RUNNING;
  }
  return ret;
}

int ObTableLoadClientTask::set_status_committing()
{
  int ret = OB_SUCCESS;
  obsys::ObWLockGuard guard(rw_lock_);
  if (OB_UNLIKELY(ObTableLoadClientStatus::RUNNING != client_status_)) {
    ret = OB_STATE_NOT_MATCH;
    LOG_WARN("unexpected status", KR(ret), K(client_status_));
  } else {
    client_status_ = ObTableLoadClientStatus::COMMITTING;
  }
  return ret;
}

int ObTableLoadClientTask::set_status_commit()
{
  int ret = OB_SUCCESS;
  obsys::ObWLockGuard guard(rw_lock_);
  if (OB_UNLIKELY(ObTableLoadClientStatus::COMMITTING != client_status_)) {
    ret = OB_STATE_NOT_MATCH;
    LOG_WARN("unexpected status", KR(ret), K(client_status_));
  } else {
    client_status_ = ObTableLoadClientStatus::COMMIT;
  }
  return ret;
}

int ObTableLoadClientTask::set_status_error(int error_code)
{
  int ret = OB_SUCCESS;
  if (OB_UNLIKELY(OB_SUCCESS == error_code)) {
    ret = OB_INVALID_ARGUMENT;
    LOG_WARN("invalid args", KR(ret), K(error_code));
  } else {
    obsys::ObWLockGuard guard(rw_lock_);
    if (ObTableLoadClientStatus::ERROR == client_status_ ||
        ObTableLoadClientStatus::ABORT == client_status_) {
      // ignore
    } else {
      client_status_ = ObTableLoadClientStatus::ERROR;
      error_code_ = error_code;
    }
  }
  return ret;
}

void ObTableLoadClientTask::set_status_abort(int error_code)
{
  obsys::ObWLockGuard guard(rw_lock_);
  if (ObTableLoadClientStatus::ABORT == client_status_) {
    // ignore
  } else {
    client_status_ = ObTableLoadClientStatus::ABORT;
    if (OB_SUCCESS == error_code_) {
      error_code_ = error_code;
    }
  }
}

int ObTableLoadClientTask::check_status(ObTableLoadClientStatus client_status)
{
  int ret = OB_SUCCESS;
  obsys::ObRLockGuard guard(rw_lock_);
  if (OB_UNLIKELY(client_status != client_status_)) {
    if (ObTableLoadClientStatus::ERROR == client_status_ ||
        ObTableLoadClientStatus::ABORT == client_status_) {
      ret = error_code_;
    } else {
      ret = OB_STATE_NOT_MATCH;
    }
  }
  return ret;
}

ObTableLoadClientStatus ObTableLoadClientTask::get_status() const
{
  obsys::ObRLockGuard guard(rw_lock_);
  return client_status_;
}

void ObTableLoadClientTask::get_status(ObTableLoadClientStatus &client_status,
                                       int &error_code) const
{
  obsys::ObRLockGuard guard(rw_lock_);
  client_status = client_status_;
  error_code = error_code_;
}

int ObTableLoadClientTask::init_instance()
{
  int ret = OB_SUCCESS;
  if (IS_NOT_INIT) {
    ret = OB_NOT_INIT;
    LOG_WARN("ObTableLoadClientTask not init", KR(ret));
  } else {
    omt::ObTenant *tenant = nullptr;
    ObArray<int64_t> column_idxs;
    if (OB_FAIL(GCTX.omt_->get_tenant(param_.get_tenant_id(), tenant))) {
      LOG_WARN("fail to get tenant handle", KR(ret), K(param_.get_tenant_id()));
    } else if (OB_FAIL(get_column_idxs(column_idxs))) {
      LOG_WARN("failed to get column idxs", K(ret));
    } else if (OB_UNLIKELY(column_idxs.empty())) {
      ret = OB_ERR_UNEXPECTED;
      LOG_WARN("unexpected empty column idxs", KR(ret));
    } else {
      ObTableLoadParam load_param;
      load_param.tenant_id_ = param_.get_tenant_id();
      load_param.table_id_ = param_.get_table_id();
      load_param.parallel_ = param_.get_parallel();
      load_param.session_count_ = load_param.parallel_;
      load_param.batch_size_ = 100;
      load_param.max_error_row_count_ = param_.get_max_error_row_count();
      load_param.column_count_ = column_idxs.count();
      load_param.need_sort_ = true;
      load_param.px_mode_ = false;
      load_param.online_opt_stat_gather_ = false; // 支持统计信息收集需要构造ObExecContext
      load_param.dup_action_ = param_.get_dup_action();
      load_param.method_ = ObDirectLoadMethod::FULL;
      load_param.insert_mode_ = ObDirectLoadInsertMode::NORMAL;
      const ObTableLoadTableCtx *tmp_ctx = nullptr;
      if (OB_FAIL(instance_.init(load_param, column_idxs, exec_ctx_))) {
        LOG_WARN("fail to init instance", KR(ret));
      } else if (OB_ISNULL(tmp_ctx = instance_.get_table_ctx())) {
        ret = OB_ERR_UNEXPECTED;
        LOG_WARN("fail to get table ctx", KR(ret));
      } else {
        session_count_ = tmp_ctx->param_.write_session_count_;
        tmp_ctx = nullptr;
      }
    }
  }
  return ret;
}

int ObTableLoadClientTask::commit_instance()
{
  int ret = OB_SUCCESS;
  if (IS_NOT_INIT) {
    ret = OB_NOT_INIT;
    LOG_WARN("ObTableLoadClientTask not init", KR(ret));
  } else {
    if (OB_FAIL(instance_.commit())) {
      LOG_WARN("fail to commit instance", KR(ret));
    } else {
      result_info_ = instance_.get_result_info();
    }
  }
  return ret;
}

void ObTableLoadClientTask::destroy_instance()
{
  int ret = OB_SUCCESS;
  if (IS_NOT_INIT) {
    ret = OB_NOT_INIT;
    LOG_WARN("ObTableLoadClientTask not init", KR(ret));
  } else {
    instance_.destroy();
  }
}

} // namespace observer
} // namespace oceanbase
