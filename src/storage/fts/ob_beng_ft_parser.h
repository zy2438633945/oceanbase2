/**
 * Copyright (c) 2023 OceanBase
 * OceanBase is licensed under Mulan PubL v2.
 * You can use this software according to the terms and conditions of the Mulan PubL v2.
 * You may obtain a copy of Mulan PubL v2 at:
 *          http://license.coscl.org.cn/MulanPubL-2.0
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PubL v2 for more details.
 */

#ifndef OB_BENG_FT_PARSER_H_
#define OB_BENG_FT_PARSER_H_

#include "lib/ob_plugin.h"
#include "lib/utility/ob_macro_utils.h"
#include "lib/utility/ob_print_utils.h"
#include "share/text_analysis/ob_text_analyzer.h"

namespace oceanbase
{
namespace storage
{

class ObBEngFTParser final
{
public:
  static const int64_t FT_MIN_WORD_LEN = 3;
  static const int64_t FT_MAX_WORD_LEN = 84;
public:
  static int segment(
      lib::ObFTParserParam *param,
      const char *fulltext,
      const int64_t ft_len);

private:
  ObBEngFTParser()
    : analysis_ctx_(),
      english_analyzer_(),
      is_inited_(false)
  {}
  ~ObBEngFTParser() = default;

  static int add_word(
      lib::ObFTParserParam *param,
      common::ObIAllocator *allocator,
      const char *word,
      int64_t word_len);
  int init(lib::ObFTParserParam *param);
  void reset();
  int segment(
      const common::ObDatum &doc,
      share::ObITokenStream *&token_stream);
  TO_STRING_KV(K_(analysis_ctx), K_(english_analyzer), K_(is_inited));

private:
  share::ObTextAnalysisCtx analysis_ctx_;
  share::ObEnglishTextAnalyzer english_analyzer_;
  bool is_inited_;

  DISALLOW_COPY_AND_ASSIGN(ObBEngFTParser);
};

class ObBasicEnglishFTParserDesc final : public lib::ObIFTParserDesc
{
public:
  ObBasicEnglishFTParserDesc();
  virtual ~ObBasicEnglishFTParserDesc() = default;
  virtual int init(lib::ObPluginParam *param) override;
  virtual int deinit(lib::ObPluginParam *param) override;
  virtual int segment(lib::ObFTParserParam *param) const override;
  OB_INLINE void reset() { is_inited_ = false; }
private:
  bool is_inited_;
};

static ObBasicEnglishFTParserDesc beng_parser;

} // end namespace storage
} // end namespace oceanbase

#endif // OB_BENG_FT_PARSER_H_
