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

#define USING_LOG_PREFIX LIB_UTIL
#include "rpc/obmysql/obp20_extra_info.h"
namespace oceanbase {
namespace obmysql {

int Obp20FeedbackProxyInfoEncoder::serialize(char *buf, int64_t len, int64_t &pos)
{
  int ret = OB_SUCCESS;
  int64_t org_pos = pos;
  // resrver for type and len
  if (pos + TYPE_KEY_PLACEHOLDER_LENGTH + TYPE_VALUE_LEGNTH_PLACEHOLDER_LENGTH > len) {
    ret = OB_SIZE_OVERFLOW;
    OB_LOG(WARN, "buffer size overflow", K(ret), K(pos), K(len));
  } else {
    MEMSET(buf + pos, 0x00, len - pos);
  }
  if (OB_FAIL(ret)) {
    // do nothing
  } else if (OB_FAIL(ObProtoTransUtil::store_str(buf,
                                                 len,
                                                 pos,
                                                 feedback_proxy_info_.ptr(),
                                                 feedback_proxy_info_.length(),
                                                 type_))) {
    // ATTENTION: the response packet should be little-endian order.
    // Please read example in file: ob_feedback_proxy_utils.h
    OB_LOG(WARN, "failed to store extra info id", K(type_), K(feedback_proxy_info_), K(buf));
  } else {
    is_serial_ = true;
  }
  OB_LOG(DEBUG,
         "Obp20FeedbackProxyInfoEncoder::serialize",
         K(ret),
         K(len),
         K(org_pos),
         K(pos),
         KPHEX(buf + org_pos, pos - org_pos));

  return ret;
}

int Obp20FeedbackProxyInfoEncoder::get_serialize_size()
{
  return TYPE_KEY_PLACEHOLDER_LENGTH + TYPE_VALUE_LEGNTH_PLACEHOLDER_LENGTH
         + feedback_proxy_info_.length();
}

// proxy -> server verify sess info required: addr, sess_id, proxy_sess_id.
int Obp20SessInfoVeriDecoder::deserialize(const char *buf, int64_t len, int64_t &pos,
                                                            Ob20ExtraInfo &extra_info)
{
  int ret = OB_SUCCESS;
  char* ptr = NULL;
  int32_t v_len = 0;
  int16_t extra_id = 0;
  if (OB_FAIL(ObProtoTransUtil::resolve_type_and_len(buf, len, pos, extra_id, v_len))) {
      OB_LOG(WARN,"failed to get extra_info", K(ret), KP(buf));
  } else if (static_cast<ExtraInfoKeyType>(extra_id) != type_) {
    ret = OB_ERR_UNEXPECTED;
    OB_LOG(WARN, "invalid encoder", K(ret), K(extra_id), K(type_));
  } else if (OB_FAIL(ObProtoTransUtil::get_str(buf, len, pos, v_len, ptr))) {
    OB_LOG(WARN,"failed to resolve sess info verification required", K(ret));
  } else {
    extra_info.sess_info_veri_.assign_ptr(ptr, v_len);
    OB_LOG(TRACE,"success to deserialize sess info verification required", K(ret));
  }
  return ret;
}

int Obp20TraceInfoEncoder::serialize(char *buf, int64_t len, int64_t &pos) {
  int ret = OB_SUCCESS;
  // resrver for type and len
  int64_t org_pos = pos;
  if (pos + TYPE_KEY_PLACEHOLDER_LENGTH + TYPE_VALUE_LEGNTH_PLACEHOLDER_LENGTH > len) {
    ret = OB_SIZE_OVERFLOW;
    OB_LOG(WARN,"buffer size overflow", K(ret), K(pos), K(len));
  } else {
    MEMSET(buf+pos, 0x00, len-pos);
  }
  if (OB_FAIL(ret)) {
    // do nothing
  } else if (OB_FAIL(ObProtoTransUtil::store_str(buf, len, pos,
                        trace_info_.ptr(), trace_info_.length(), type_))) {
    OB_LOG(WARN, "failed to store extra info id", K(type_), K(trace_info_), K(buf));
  } else {
    is_serial_ = true;
  }
  return ret;
}

int Obp20TraceInfoEncoder::get_serialize_size() {
  // type, len, val
  return TYPE_KEY_PLACEHOLDER_LENGTH + TYPE_VALUE_LEGNTH_PLACEHOLDER_LENGTH + trace_info_.length();
}

int Obp20TaceInfoDecoder::deserialize(const char *buf, int64_t len, int64_t &pos, Ob20ExtraInfo &extra_info) {
  int ret = OB_SUCCESS;
  char* ptr = NULL;
  int32_t v_len = 0;
  int16_t extra_id = 0;
  if (OB_FAIL(ObProtoTransUtil::resolve_type_and_len(buf, len, pos, extra_id, v_len))) {
      OB_LOG(WARN,"failed to get extra_info", K(ret), KP(buf));
  } else if (static_cast<ExtraInfoKeyType>(extra_id) != type_) {
    ret = OB_ERR_UNEXPECTED;
    OB_LOG(WARN, "invalid encoder", K(ret), K(extra_id), K(type_));
  } else if (OB_FAIL(ObProtoTransUtil::get_str(buf, len, pos, v_len, ptr))) {
    OB_LOG(WARN,"failed to resolve flt level", K(ret));
  } else {
    extra_info.exist_trace_info_ = true;
    extra_info.trace_info_.assign(ptr, v_len);
  }
  return ret;
}



int Obp20SessInfoEncoder::serialize(char *buf, int64_t len, int64_t &pos) {
  int ret = OB_SUCCESS;
  // resrver for type and len
  int64_t org_pos = pos;
  if (pos + TYPE_KEY_PLACEHOLDER_LENGTH + TYPE_VALUE_LEGNTH_PLACEHOLDER_LENGTH > len) {
    ret = OB_SIZE_OVERFLOW;
    OB_LOG(WARN,"buffer size overflow", K(ret), K(pos), K(len));
  }
  if (OB_FAIL(ret)) {
    // do nothing
  } else if (OB_FAIL(ObProtoTransUtil::store_str(buf, len, pos,
                        sess_info_.ptr(), sess_info_.length(), type_))) {
    OB_LOG(WARN, "failed to store extra info id", K(type_), K(sess_info_), K(buf));
  } else {
    is_serial_ = true;
  }
  OB_LOG(DEBUG, "serialize session info", K(ret));
  return ret;
}

int Obp20SessInfoEncoder::get_serialize_size() {
  // type, len, val
  return TYPE_KEY_PLACEHOLDER_LENGTH + TYPE_VALUE_LEGNTH_PLACEHOLDER_LENGTH + sess_info_.length();
}

int Obp20SessInfoDecoder::deserialize(const char *buf, int64_t len, int64_t &pos, Ob20ExtraInfo &extra_info) {
  int ret = OB_SUCCESS;
  char* ptr = NULL;
  int32_t v_len = 0;
  int16_t extra_id = 0;
  if (OB_FAIL(ObProtoTransUtil::resolve_type_and_len(buf, len, pos, extra_id, v_len))) {
      OB_LOG(WARN,"failed to get extra_info", K(ret), KP(buf));
  } else if (static_cast<ExtraInfoKeyType>(extra_id) != type_) {
    ret = OB_ERR_UNEXPECTED;
    OB_LOG(WARN, "invalid encoder", K(ret), K(extra_id), K(type_));
  } else if (OB_FAIL(ObProtoTransUtil::get_str(buf, len, pos, v_len, ptr))) {
    OB_LOG(WARN,"failed to resolve flt level", K(ret));
  } else {
    extra_info.sync_sess_info_.assign(ptr, v_len);
  }
  return ret;
}

int Obp20FullTrcEncoder::serialize(char *buf, int64_t len, int64_t &pos) {
  int ret = OB_SUCCESS;
  // resrver for type and len
  int64_t org_pos = pos;
  if (pos + TYPE_KEY_PLACEHOLDER_LENGTH + TYPE_VALUE_LEGNTH_PLACEHOLDER_LENGTH > len) {
    ret = OB_SIZE_OVERFLOW;
    OB_LOG(WARN,"buffer size overflow", K(ret), K(pos), K(len));
  } else {
    MEMSET(buf + pos, 0x00, len - pos);
    if (OB_FAIL(ObProtoTransUtil::store_str(buf, len, pos,
                          full_trc_.ptr(), full_trc_.length(), type_))) {
      OB_LOG(WARN, "failed to store extra info id", K(type_), K(full_trc_), K(buf));
    } else {
      is_serial_ = true;
    }
  }
  return ret;
}

int Obp20FullTrcEncoder::get_serialize_size() {
  // type, len, val
  return TYPE_KEY_PLACEHOLDER_LENGTH + TYPE_VALUE_LEGNTH_PLACEHOLDER_LENGTH + full_trc_.length();
}

int Obp20FullTrcDecoder::deserialize(const char *buf, int64_t len, int64_t &pos, Ob20ExtraInfo &extra_info) {
  int ret = OB_SUCCESS;
  char* ptr = NULL;
  int32_t v_len = 0;
  int16_t extra_id = 0;
  if (OB_FAIL(ObProtoTransUtil::resolve_type_and_len(buf, len, pos, extra_id, v_len))) {
      OB_LOG(WARN,"failed to get extra_info", K(ret), KP(buf));
  } else if (static_cast<ExtraInfoKeyType>(extra_id) != type_) {
    ret = OB_ERR_UNEXPECTED;
    OB_LOG(WARN, "invalid encoder", K(ret), K(extra_id), K(type_));
  } else if (OB_FAIL(ObProtoTransUtil::get_str(buf, len, pos, v_len, ptr))) {
    OB_LOG(WARN,"failed to resolve flt level", K(ret));
  } else {
    extra_info.full_link_trace_.assign(ptr, v_len);
  }
  return ret;
}

};
};
