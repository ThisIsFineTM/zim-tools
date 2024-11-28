#ifndef ZIM_TOOL_ZIMFILECHECKS_H_
#define ZIM_TOOL_ZIMFILECHECKS_H_

#include <zim-tools/zimcheck/json_tools.h>  // for OutputStream, property
//
#include <mustache.hpp>  // for basic_data, object
//
#include <bitset>   // for bitset
#include <cstddef>  // for size_t
#include <string>   // for basic_string, string, hash
#include <vector>   // for vector

class ProgressBar;

namespace zim
{
class Archive;
}  // namespace zim

enum StatusCode : int { PASS = 0, FAIL = 1, EXCEPTION = 2 };

enum class LogTag { ERROR, WARNING };

enum class TestType {
  CHECKSUM,
  INTEGRITY,
  EMPTY,
  METADATA,
  FAVICON,
  MAIN_PAGE,
  REDUNDANT,
  URL_INTERNAL,
  URL_EXTERNAL,
  URL_EMPTY,
  REDIRECT,

  COUNT
};

class EnabledTests
{
  std::bitset<static_cast<size_t>(TestType::COUNT)> tests;

 public:
  EnabledTests() = default;

  void enableAll() { tests.set(); }
  void enable(TestType tt) { tests.set(static_cast<size_t>(tt)); }

  [[nodiscard]] bool isEnabled(TestType tt) const
  {
    return tests[static_cast<size_t>(tt)];
  }
};

enum class MsgId {
  CHECKSUM,
  MAIN_PAGE,
  METADATA,
  EMPTY_ENTRY,
  OUTOFBOUNDS_LINK,
  EMPTY_LINKS,
  DANGLING_LINKS,
  EXTERNAL_LINK,
  REDUNDANT_ITEMS,
  REDIRECT_LOOP,
  MISSING_FAVICON
};

using MsgParams = kainjow::mustache::object;

JSON::OutputStream& operator<<(JSON::OutputStream& out, TestType check);
JSON::OutputStream& operator<<(JSON::OutputStream& out, EnabledTests checks);

// TODO rule of 5
class ErrorLogger
{
 private:
  struct MsgIdWithParams {
    MsgId msgId;
    MsgParams msgParams;
  };

  // reportMsgs[i] holds messages for the i'th test/check
  std::vector<std::vector<MsgIdWithParams>> reportMsgs;

  // testStatus[i] corresponds to the status of i'th test
  std::bitset<static_cast<size_t>(TestType::COUNT)> testStatus;

  mutable JSON::OutputStream jsonOutputStream;

  static std::string expand(const MsgIdWithParams& msg);
  void jsonOutput(const MsgIdWithParams& msg) const;

 public:
  explicit ErrorLogger(bool _jsonOutputMode = false);
  ~ErrorLogger();

  void infoMsg(const std::string& msg) const;

  template <class T>
  void addInfo(const std::string& key, const T& value)
  {
    if (jsonOutputStream.enabled()) {
      jsonOutputStream << JSON::property(key, value);
    }
  }

  void setTestResult(TestType type, bool status);
  void addMsg(MsgId msgid, const MsgParams& msgParams);
  void report(bool error_details) const;
  bool overallStatus() const;
};

void test_checksum(zim::Archive& archive, ErrorLogger& reporter);
bool test_integrity(const std::string& filename, ErrorLogger& reporter);
void test_metadata(const zim::Archive& archive, ErrorLogger& reporter);
void test_favicon(const zim::Archive& archive, ErrorLogger& reporter);
void test_mainpage(const zim::Archive& archive, ErrorLogger& reporter);
void test_articles(const zim::Archive& archive,
                   ErrorLogger& reporter,
                   ProgressBar& progress,
                   const EnabledTests enabled_tests,
                   int thread_count = 1);
void test_redirect_loop(const zim::Archive& archive, ErrorLogger& reporter);

#endif
