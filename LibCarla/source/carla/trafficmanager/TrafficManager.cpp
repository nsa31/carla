// Copyright (c) 2019 Computer Vision Center (CVC) at the Universitat Autonoma
// de Barcelona (UAB).
//
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT>.

#include "carla/Exception.h"
#include "carla/client/Client.h"
#include "carla/client/World.h"
#include "carla/trafficmanager/TrafficManager.h"
#include "carla/trafficmanager/TrafficManagerBase.h"
#include "carla/trafficmanager/TrafficManagerUtil.h"

#define DEBUG_PRINT_TM  0

namespace carla {
namespace traffic_manager {

/// Unique pointer to hold the TM instance
std::unique_ptr<TrafficManagerBase> TrafficManager::singleton_pointer = nullptr;

/// Private constructor for singleton life cycle management.
TrafficManager::TrafficManager(
  carla::client::detail::EpisodeProxy episodeProxy,
  uint16_t port) {

  /// Check singleton instance already created or not
  if (!singleton_pointer) {

    /// Check TM instance already registered with server or not
    if(episodeProxy.Lock()->IsTrafficManagerRunning(port)) {

      /// Get TM server info (Remote IP & PORT)
      std::pair<std::string, uint16_t> serverTM =
        episodeProxy.Lock()->GetTrafficManagerRunning(port);

      /// Set remote TM server IP and port
      TrafficManagerRemote* tm_ptr = new(std::nothrow)
        TrafficManagerRemote(serverTM, episodeProxy);

      /// Try to connect to remote TM server
      try {

        /// Check memory allocated or not
        if(tm_ptr != nullptr) {

          #if DEBUG_PRINT_TM
          /// Test print
          std::cout 	<< "OLD@: Registered TM at "
                << serverTM.first  << ":"
                << serverTM.second << " ..... TRY "
                << std::endl;
          #endif
          /// Try to check remote TM exist or not
          tm_ptr->HealthCheckRemoteTM();

          /// Test print
          std::cout 	<< "OLD@: Registered TM at "
                << serverTM.first  << ":"
                << serverTM.second << " ..... SUCCESS "
                << std::endl;

          /// Set the pointer of the instance
          singleton_pointer = std::unique_ptr<TrafficManagerBase>(tm_ptr);
        }
      } catch (...) {

        /// Clear previously allocated memory
        delete tm_ptr;

        #if DEBUG_PRINT_TM
        /// Test print
        std::cout 	<< "OLD@: Registered TM at "
              << serverTM.first  << ":"
              << serverTM.second << " ..... FAILED "
              << std::endl;
        #endif
      }
    }
  }


  /// As TM server not running
  if(!singleton_pointer) {

    /// Set default port
    uint16_t RPCportTM = port;

    /// Define local constants
    const std::vector<float> longitudinal_param = {2.0f, 0.05f, 0.07f};
    const std::vector<float> longitudinal_highway_param = {4.0f, 0.02f, 0.03f};
    const std::vector<float> lateral_param = {10.0f, 0.02f, 1.0f};
    const std::vector<float> lateral_highway_param = {9.0f, 0.02f, 1.0f};
    const float perc_difference_from_limit = 30.0f;

    std::pair<std::string, uint16_t> serverTM;

    /// Create local instance of TM
    TrafficManagerLocal* tm_ptr = new TrafficManagerLocal(
      longitudinal_param,
      longitudinal_highway_param,
      lateral_param,
      lateral_highway_param,
      perc_difference_from_limit,
      episodeProxy,
      RPCportTM);

    /// Get TM server info (Local IP & PORT)
    serverTM = TrafficManagerUtil::GetLocalIP(RPCportTM);

    /// Set this client as the TM to server
    episodeProxy.Lock()->AddTrafficManagerRunning(serverTM);

    /// Print status
    std::cout 	<< "NEW@: Registered TM at "
          << serverTM.first  << ":"
          << serverTM.second << " ..... SUCCESS."
          << std::endl;

    /// Set the pointer of the instance
    singleton_pointer = std::unique_ptr<TrafficManagerBase>(tm_ptr);
  }
}

void TrafficManager::Release() {
  if(singleton_pointer) {
    TrafficManagerBase *base_ptr = singleton_pointer.release();
    delete base_ptr;
  }
}

} // namespace traffic_manager
} // namespace carla
