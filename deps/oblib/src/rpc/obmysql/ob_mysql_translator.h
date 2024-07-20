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

#ifndef _OB_MYSQL_TRANSLATOR_H_
#define _OB_MYSQL_TRANSLATOR_H_

#include "rpc/frame/ob_req_translator.h"

// used when initializing processor table
namespace oceanbase
{
namespace obmysql
{

using rpc::frame::ObReqProcessor;

class ObMySQLTranslator
    : public rpc::frame::ObReqTranslator
{
public:
  ObMySQLTranslator() {}
  virtual ~ObMySQLTranslator() {}
}; // end of class ObMySQLTranslator

} // end of namespace obmysql
} // end of namespace oceanbase

#endif /* _OB_MYSQL_TRANSLATOR_H_ */
