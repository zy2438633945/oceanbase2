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

#ifndef OCEANBASE_COMMON_COMPRESS_ZSTD_COMPRESSOR_
#define OCEANBASE_COMMON_COMPRESS_ZSTD_COMPRESSOR_
#include "lib/compress/ob_compressor.h"

namespace oceanbase
{
namespace common
{

namespace zstd
{

class ObZstdCompressor : public ObCompressor
{
public:
  explicit ObZstdCompressor(ObIAllocator &allocator)
    : allocator_(allocator) {}
  virtual ~ObZstdCompressor() {}
  int compress(const char *src_buffer,
               const int64_t src_data_size,
               char *dst_buffer,
               const int64_t dst_buffer_size,
               int64_t &dst_data_size) override;
  int decompress(const char *src_buffer,
                 const int64_t src_data_size,
                 char *dst_buffer,
                 const int64_t dst_buffer_size,
                 int64_t &dst_data_size) override;
  const char *get_compressor_name() const;
  ObCompressorType get_compressor_type() const;
  int get_max_overflow_size(const int64_t src_data_size,
                                   int64_t &max_overflow_size) const;
private:
  ObIAllocator &allocator_;

};
} // namespace zstd
} //namespace common
} //namespace oceanbase
#endif //OCEANBASE_COMMON_COMPRESS_ZSTD_COMPRESSOR_
