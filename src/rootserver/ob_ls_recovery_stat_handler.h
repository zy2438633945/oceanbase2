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

#ifndef OCEANBASE_STORAGE_OB_LS_RECOVERY_STAT_HANDLER
#define OCEANBASE_STORAGE_OB_LS_RECOVERY_STAT_HANDLER

#include "lib/ob_define.h"
#include "rootserver/ob_rs_async_rpc_proxy.h" //ObGetLSReplayedScnProxy
#include "share/ls/ob_ls_recovery_stat_operator.h" // ObLSRecoveryStatOperator
#include "logservice/palf/palf_handle_impl.h"                  // PalfStat
#include "logservice/palf/log_meta_info.h"//LogConfigVersion

namespace oceanbase
{

namespace storage
{
class ObLS;
}

namespace rootserver
{
struct ObLSReplicaReadableSCN
{
public:
  ObLSReplicaReadableSCN() : server_(), readable_scn_() {}
  ~ObLSReplicaReadableSCN() {}
  int init(const common::ObAddr &server, const share::SCN &readable_scn);

  share::SCN get_readable_scn()const
  {
    return readable_scn_;
  }
  common::ObAddr &get_server()
  {
    return server_;
  }
   TO_STRING_KV(K_(server), K_(readable_scn));
private:
  common::ObAddr server_;
  share::SCN readable_scn_;
};

/**
  * @description:
  *    ObLSRecoveryStatHandler exists on the LS of each observer and is responsible for
  *    the each LS recovery stat
  */
class ObLSRecoveryStatHandler
{
public:
  ObLSRecoveryStatHandler() { reset(); }
  ~ObLSRecoveryStatHandler() { reset(); }
  int init(const uint64_t tenant_id, ObLS *ls);
  void reset();
  /**
   * @description:
   *    get ls readable_scn considering readable scn, sync scn and replayable scn.
   * @param[out] readable_scn ls readable_scn
   * @return return code
   */
  int get_ls_replica_readable_scn(share::SCN &readable_scn);

  /*
   * @description:
   * Get the readable_scn of other replicas
   * @param[out] readable_scn ls readable_scn
   * @return return code
   * */
  int get_all_replica_min_readable_scn(share::SCN &readable_scn);

  /**
   * @description:
   *    get ls level recovery_stat by LS leader.
   *    If follower LS replica call this function, it will return OB_NOT_MASTER.
   * @param[out] ls_recovery_stat
   * @return return code
   */
  int get_ls_level_recovery_stat(share::ObLSRecoveryStat &ls_recovery_stat);

  int set_add_replica_server(const common::ObAddr &server);
  /*
  * @description:
  * get all ls replica readable and set to replicas_scn_;
  */
  int gather_replica_readable_scn();

  TO_STRING_KV(K_(tenant_id), K_(ls));

private:
  int check_inner_stat_();

  /**
   * @description:
   *    increase LS readable_scn when replayable_scn is pushed forward in switchover
   * @param[in/out] readable_scn
   *                  in: actual readable_scn
   *                  out: increased readable_scn
   * @return return code
   */
  int increase_ls_replica_readable_scn_(share::SCN &sync_scn);

  int do_get_ls_level_readable_scn_(share::SCN &read_scn);

  /**
   * @description:
   *    do not use this function.
   *    Since PalfHandleGuard holds lock, it may cause deadlock with other palf operations,
   *    so use a separate function to obtain palf_stat, please do not add new operations in this function
   * @param[out] palf_stat
   * @return return code
   */
  int get_palf_stat_(
      palf::PalfStat &palf_stat);

  /**
   * @description:
   *    palf_stat get from  palf_handle_guard.stat can guarantee that <config_version, paxos_member_list, paxos_replica_num, degraded_list>
   *    is a snapshot, but because it is a cache, it may be not latest, in order to ensure that
   *    the latest palf_stat can be obtained, it is necessary to obtain the latest member list
   *    and compare with palf_stat. If they are same, the obtained palf_stat is considered to be latest.
   * @param[out] palf_stat
   * @return return code
   */
  int get_latest_palf_stat_(
      palf::PalfStat &palf_stat);
  int do_get_each_replica_readable_scn_(
      const ObIArray<common::ObAddr> &ob_member_list,
      ObArray<ObLSReplicaReadableSCN> &replicas_scn);
  int get_majority_readable_scn_(
      const share::SCN &leader_readable_scn,
      share::SCN &majority_min_readable_scn);
  int do_get_majority_readable_scn_(
      const ObIArray<common::ObAddr> &ob_member_list,
      const share::SCN &leader_readable_scn,
      const int64_t need_query_member_cnt,
      share::SCN &majority_min_readable_scn);
  int do_get_majority_readable_scn_V2_(
      const ObIArray<common::ObAddr> &ob_member_list,
      const int64_t need_query_member_cnt,
      const palf::LogConfigVersion &config_version,
      share::SCN &majority_min_readable_scn);
  int calc_majority_min_readable_scn_(
      const share::SCN &leader_readable_scn,
      const int64_t majority_cnt,
      const ObIArray<int> &return_code_array,
      const ObGetLSReplayedScnProxy &proxy,
      share::SCN &majority_min_readable_scn);
  int do_calc_majority_min_readable_scn_(
    const int64_t majority_cnt,
    ObArray<SCN> &readable_scn_list,
    share::SCN &majority_min_readable_scn);

  int construct_new_member_list_(
      const common::ObMemberList &member_list_ori,
      const common::GlobalLearnerList &degraded_list,
      const int64_t paxos_replica_number_ori,
      ObIArray<common::ObAddr> &member_list_new,
      int64_t &paxos_replica_number_new);

  int try_reload_and_fix_config_version_(const palf::LogConfigVersion &current_version);

  DISALLOW_COPY_AND_ASSIGN(ObLSRecoveryStatHandler);

private:
  bool is_inited_;
  uint64_t tenant_id_;
  ObLS *ls_;
  common::SpinRWLock lock_;
  share::SCN readable_scn_in_inner_;//readable_scn of inner_table
  palf::LogConfigVersion config_version_in_inner_;//config_version in inner_table
  common::ObAddr extra_server_;//for add replica, need to gather add_replica's readable_scn
  palf::LogConfigVersion config_version_;
  ObArray<ObLSReplicaReadableSCN> replicas_scn_;
};

}
}

#endif // OCEANBASE_STORAGE_OB_LS_RECOVERY_STAT_HANDLER
