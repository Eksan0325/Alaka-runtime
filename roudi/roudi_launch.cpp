/**
 * @brief iceoryx RouDi launcher interface implementation， start Roudi Progress
 * @author Eksan Ablikim
 */

#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <string>

#include <grp.h>
#include <unistd.h>

#include "iceoryx_posh/iceoryx_posh_config.hpp"
#include "iceoryx_posh/iceoryx_posh_types.hpp"
#include "iceoryx_posh/internal/log/posh_logging.hpp"
#include "iceoryx_posh/mepoo/mepoo_config.hpp"
#include "iceoryx_posh/mepoo/segment_config.hpp"
#include "iceoryx_posh/roudi/cmd_line_args.hpp"
#include "iceoryx_posh/roudi/iceoryx_roudi_app.hpp"
#include "iceoryx_posh/roudi/roudi_cmd_line_parser.hpp"

static const std::string roudiGroupName() {
   const char * segment_from_env = std::getenv("ALAKA_ROUDI_SEGMENT");
   if (segment_from_env != nullptr) {
      return std::string(segment_from_env);
   }
   const gid_t gid = getegid();
   const struct group * gp = getgrgid(gid);
   if (gp != nullptr) {
      return std::string(gp->gr_name);
   }
   return std::string("root");
}

static iox::RouDiConfig_t configureRoudi() {
   iox::mepoo::MePooConfig mempool;
   mempool.addMemPool({128, 512});   // data size = 128 B * 512 =64KB
   mempool.addMemPool({1024, 1024}); // data size = 1024 B * 1024 = 1MB
   mempool.addMemPool({16484, 256}); // data size = 16484 B * 256 = 4MB
   mempool.optimize();
   auto grp_str = roudiGroupName();
   iox::mepoo::SegmentConfig scfg;
   //Init system memory segment with writer and reader group. only support one same Unix group.
   scfg.m_sharedMemorySegments.push_back(
      iox::mepoo::SegmentConfig::SegmentEntry(
         iox::posix::PosixGroup::string_t(iox::cxx::TruncateToCapacity, grp_str.c_str()), 
         iox::posix::PosixGroup::string_t(iox::cxx::TruncateToCapacity, grp_str.c_str()), 
         mempool
      ));
   iox::RouDiConfig_t rcfg;
   rcfg.m_sharedMemorySegments = scfg.m_sharedMemorySegments;
   return rcfg;
}  

int main(int argc, char* argv[]) {
  iox::config::CmdLineParser parser;
  auto args_parse_result = parser.parse(argc, argv);
  if (args_parse_result.has_error()) {
     if (args_parse_result.get_error() != iox::config::CmdLineParserResult::INFO_OUTPUT_ONLY) {
        iox::LogFatal() << "Roudi command line parsing failed with error: " << static_cast<int>(args_parse_result.get_error());
        return -1;
     }
     // CmdLineParser will be output about --help | --version info stream.
     return 0;
  }
  auto cfg = configureRoudi();
  iox::roudi::IceOryxRouDiApp app(args_parse_result.value(), cfg);
  return app.run();
}