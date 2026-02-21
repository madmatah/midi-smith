#include "app/version.hpp"

#if __has_include("app/version_build.hpp")
#include "app/version_build.hpp"
#else
#define APP_VERSION_FULL "unknown"
#define APP_VERSION_COMMIT_DATE "unknown"
#define APP_VERSION_BUILD_TYPE "unknown"
#endif

namespace app::version {

const std::string_view kFullVersion = APP_VERSION_FULL;
const std::string_view kBuildType = APP_VERSION_BUILD_TYPE;
const std::string_view kCommitDate = APP_VERSION_COMMIT_DATE;

}  // namespace app::version
