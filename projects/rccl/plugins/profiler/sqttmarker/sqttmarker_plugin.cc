/*
 * RCCL SQTT marker profiler plugin: adds SQTT (SQ Thread Trace) instrumentation
 * markers to RCCL operations for performance analysis and profiling.
 *
 * This plugin integrates with the NCCL profiler API to instrument RCCL operations
 * with SQTT markers, enabling detailed GPU thread trace analysis.
 */

#include "sqttmarker_plugin_shim.h"
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <string>
#include <unordered_map>
#include <memory>

#include "nccl_profiler.h"

// Include SQTT markers
#if SQTT_ENABLED
#include <rocprof_trace_decoder/cxx/markers.hpp>
#else
#define sqtt_marker_enter(name) do {} while(0)
#define sqtt_marker_exit(name) do {} while(0)
#endif

// Plugin context per communicator
struct CommCtx {
  uint64_t commHash;
  int rank;
  int nRanks;
  std::string commName;
};

// Event tracking structure
struct ProfilerEvent {
  uint64_t eventType;
  ncclProfilerEventDescr_v6_t descr;
  CommCtx* comm;
  std::string markerName;
  bool markerActive;
};

static ncclDebugLogger_t gLog;

// Helper to generate marker names for different event types
static std::string generateMarkerName(ncclProfilerEventDescr_v6_t* eDescr) {
  std::string name;

  if (eDescr->type == ncclProfileColl) {
    name = "RCCL_Coll_";
    name += std::to_string(eDescr->coll.collType);
  } else if (eDescr->type == ncclProfileP2p) {
    name = "RCCL_P2P_";
    name += eDescr->p2p.isSendNotRecv ? "Send" : "Recv";
  } else if (eDescr->type == ncclProfileProxyOp) {
    name = "RCCL_Proxy_";
    name += eDescr->proxyOp.isSend ? "Send" : "Recv";
    name += "_Chan" + std::to_string(eDescr->proxyOp.channelId);
  } else if (eDescr->type == ncclProfileProxyStep) {
    name = "RCCL_ProxyStep";
  } else if (eDescr->type == ncclProfileGroup) {
    name = "RCCL_Group";
  } else if (eDescr->type == ncclProfileKernelLaunch) {
    name = "RCCL_Kernel";
  } else if (eDescr->type == ncclProfileCeColl) {
    name = "RCCL_CE_Coll";
  } else if (eDescr->type == ncclProfileCeSync) {
    name = "RCCL_CE_Sync";
  } else if (eDescr->type == ncclProfileCeBatch) {
    name = "RCCL_CE_Batch";
  } else {
    name = "RCCL_Event_" + std::to_string(eDescr->type);
  }

  return name;
}

static ncclResult_t pluginInit(void** context, uint64_t commId, int* eActivationMask, const char* commName,
                               int /*nNodes*/, int nRanks, int rank, ncclDebugLogger_t logfn) {
  gLog = logfn;

  // Activate all event types for comprehensive coverage
  *eActivationMask = ncclProfileGroup | ncclProfileColl | ncclProfileP2p |
                     ncclProfileProxyOp | ncclProfileProxyStep |
                     ncclProfileKernelLaunch | ncclProfileCeColl |
                     ncclProfileCeSync | ncclProfileCeBatch;

  CommCtx* c = new CommCtx();
  c->commHash = commId;
  c->rank = rank;
  c->nRanks = nRanks;
  c->commName = commName ? commName : "unknown";
  *context = c;

  if (gLog) {
    gLog(NCCL_LOG_INFO, NCCL_INIT, __func__, __LINE__,
         "SQTT marker profiler plugin initialized for comm %s (rank %d/%d)",
         c->commName.c_str(), rank, nRanks);
  }

  return ncclSuccess;
}

static ncclResult_t pluginStartEvent(void* context, void** eHandle, ncclProfilerEventDescr_v6_t* eDescr) {
  auto* ev = new ProfilerEvent();
  ev->eventType = eDescr->type;
  memcpy(&ev->descr, eDescr, sizeof(ev->descr));
  ev->comm = (CommCtx*)context;
  ev->markerName = generateMarkerName(eDescr);
  ev->markerActive = false;

  // Enter SQTT marker for this event
  sqtt_marker_enter(ev->markerName.c_str());
  ev->markerActive = true;

  *eHandle = ev;
  return ncclSuccess;
}

static ncclResult_t pluginStopEvent(void* eHandle) {
  if (!eHandle) return ncclSuccess;

  auto* ev = (ProfilerEvent*)eHandle;

  // Exit SQTT marker if it was entered
  if (ev->markerActive) {
    sqtt_marker_exit(ev->markerName.c_str());
  }

  delete ev;
  return ncclSuccess;
}

static ncclResult_t pluginRecordEventState(void* eHandle, ncclProfilerEventState_v6_t eState,
                                           ncclProfilerEventStateArgs_v6_t* eStateArgs) {
  if (!eHandle) return ncclSuccess;

  auto* ev = (ProfilerEvent*)eHandle;

  // Add state-specific markers for detailed tracing
  std::string stateName;
  bool addMarker = false;

  switch (eState) {
    case ncclProfilerProxyOpInProgress_v4:
      stateName = ev->markerName + "_InProgress";
      addMarker = true;
      break;
    case ncclProfilerKernelChStop:
      stateName = ev->markerName + "_KernelStop";
      addMarker = true;
      break;
    case ncclProfilerCeCollStart:
      stateName = ev->markerName + "_Start";
      addMarker = true;
      break;
    case ncclProfilerCeCollComplete:
      stateName = ev->markerName + "_Complete";
      addMarker = true;
      break;
    default:
      break;
  }

  if (addMarker) {
    sqtt_marker_enter(stateName.c_str());
    sqtt_marker_exit(stateName.c_str());
  }

  return ncclSuccess;
}

static ncclResult_t pluginFinalize(void* context) {
  CommCtx* c = (CommCtx*)context;
  if (!c) return ncclSuccess;

  if (gLog) {
    gLog(NCCL_LOG_INFO, NCCL_INIT, __func__, __LINE__,
         "SQTT marker profiler plugin finalized for comm %s", c->commName.c_str());
  }

  delete c;
  return ncclSuccess;
}

extern "C" {

// Export the v6 profiler interface
ncclProfiler_v6_t ncclProfiler_v6 = {
  "SQTTMarker",
  pluginInit,
  pluginStartEvent,
  pluginStopEvent,
  pluginRecordEventState,
  pluginFinalize
};

}
