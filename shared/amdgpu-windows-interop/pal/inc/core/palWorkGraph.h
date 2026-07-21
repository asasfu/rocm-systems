/* Copyright (c) Advanced Micro Devices, Inc., or its affiliates. All rights reserved. */
/**
 ***********************************************************************************************************************
 * @file  palWorkGraph.h
 * @brief Defines the PAL IWorkGraph interface.
 ***********************************************************************************************************************
 */

#pragma once

#include "palDestroyable.h"
#include "palGraphLayout.h"

#include <type_traits>

#if PAL_BUILD_WORK_GRAPH_TRACE
namespace Util
{
class JsonWriter;
} // Util
#endif

#if PAL_WORK_GRAPHS_SUPPORT
namespace Pal
{

struct GpuMemSubAllocInfo;
struct WorkGraphBindParams;
enum class DispatchInterleaveSize : uint32;

/// Common flags controlling creation of both compute and graphics pipeline.
union WorkGraphCreateFlags
{
    struct
    {
        uint32  clientInternal :  1;    ///< Internal work-graph not created by the application.
        uint32  reserved       : 31;    ///< Reserved for future use.
    };
    uint32  u32All;                     ///< Flags packed as 32-bit uint.
};

/// Specifies properties for creation of a work-graph @ref IPipeline object.  Input structure to
/// IDevice::CreateWorkGraph().
struct WorkGraphCreateInfo
{
    WorkGraphCreateFlags  flags;    ///< Flags controlling work-graph creation.

    /// Specifies the graph layout this work graph pipeline must use.  Cannot be null.
    /// @see IGraphLayout.
    const IGraphLayout*  pGraphLayout;

    bool disablePartialDispatchPreemption; ///< Prevents scenarios where a subset of the dispatched thread groups are
                                           ///  preempted and the remaining thread groups run to completion. This
                                           ///  can occur when thread group granularity preemption is available and
                                           ///  instruction level (CWSR) is not. This setting is useful for allowing
                                           ///  dispatches with interdependent thread groups.

    DispatchInterleaveSize  interleaveSize;  ///< Controls how many thread groups are sent to one SE before switching
                                             ///  to the next one.
    DispatchDims            interleaveDims;  ///< Controls how many thread groups are sent to one SE before switching
                                             ///  to the next one.
                                             ///  Takes priority over interleaveSize if both are set.

    /// Defines the graphics state which is global to the entire graph.  Any element in this state which is not null
    /// cannot also be defined by any of the graphics nodes in @ref pGraphLayout.  Any element other than
    /// @ref indexType that is undefined in both this state and the nodes in the graph are inherited from the current
    /// command buffer state when @ref ICmdBuffer::CmdDispatchGraph is invoked. @ref indexType must be defined in this
    /// state or in every graph node.
    /// If @ref pGraphLayout contains no graphics nodes, this must be null.  Otherwise, this cannot be null.
    const GraphicsNodeStateBlock*  pGlobalGraphicsState;
};

/// Specifies the memory requirements for any backing store memory assigned to a work graph.
struct WorkGraphGpuMemoryRequirements
{
    gpusize  alignment;         ///< Required memory address alignment.
    gpusize  minimumSize;       ///< Minimum amount of space needed for the backing store.  This size is sufficiently
                                ///  large to prevent any deadlock scenarios during execution.
    gpusize  maximumSize;       ///< Maximum amount of space needed for the backing store.  This size is sufficiently
                                ///  large to prevent any kind of stalling during execution while waiting for work to
                                ///  drain due to the backing store being full.
    gpusize  sizeGranularity;   ///< For backing stores sized between the minimum and maximum, this granularity
                                ///  describes intervals in the memory amount which are useful.

    /// List of preferred heaps for the GPU memory in order of predicted performance.  It is permissible to use memory
    /// not allocated from the heaps in the list below, but runtime performance may be suboptimal.
    GpuHeap  preferredHeaps[GpuHeapCount];
    uint32   heapCount;         ///< Number of valid entries in preferredHeaps[].
};

/// Unique identifier for a specific shader program within a work graph.  Used when specifying which shader receives
/// initial work in an @ref ICmdBuffer::CmdDispatchGraph() call.
using ShaderProgramId = uint32;

/// Indicates an invalid shader program ID.
constexpr ShaderProgramId InvalidProgramId = uint32(-1);

#if PAL_CLIENT_DX
static_assert(std::is_same<ShaderProgramId, DxEntrypointIndex>::value,
              "ShaderProgramId and DxEntrypointIndex must be compatible types!");
#endif

/// Reports properties of an assembled work graph.
struct WorkGraphInfo
{
    /// 128-bit identifier accumulated across from this work-graphs's ELF binaries, composed of the state the compiler
    /// decided was appropriate to identify the compiled shaders.  Also incorporates the properties of the IGraphLayout
    /// used to create this graph.  The lower 64 bits are "stable"; the upper 64 bits are "unique".
    WorkGraphHash  internalWorkGraphHash;

    union
    {
        struct
        {
            uint32  isComputeOnly  :  1;    ///< Indicates that this work graph contains only compute nodes.
            uint32  isGraphicsOnly :  1;    ///< Indicates that this work graph contains only graphics nodes.
            uint32  reserved       : 30;    ///< Reserved for future use.
        };
        uint32  u32All;
    } flags;
};

/**
 ***********************************************************************************************************************
 * @interface IWorkGraph
 * @brief     Object representing a graph of shaders which is managed on the GPU.
 *
 * @see IDevice::CreateWorkGraph()
 ***********************************************************************************************************************
 */
class IWorkGraph : public IDestroyable
{
public:
    /// Returns PAL-computed properties of this work-graph and its corresponding shaders.
    ///
    /// @returns Property structure describing this work-graph.
    virtual const WorkGraphInfo& GetInfo() const = 0;

    /// Returns a list of GPU memory allocations used by this work graph.
    ///
    /// @param [in,out] pNumEntries    Input value specifies the available size in pAllocInfoList; output value
    ///                                reports the number of GPU memory allocations.
    /// @param [out]    pAllocInfoList If pAllocInfoList=nullptr, then pNumEntries is ignored on input.  On output it
    ///                                will reflect the number of allocations that make up this work graph.  If
    ///                                pAllocInfoList!=nullptr, then on input pNumEntries is assumed to be the number
    ///                                of entries in the pAllocInfoList array.  On output, pNumEntries reflects the
    ///                                number of entries in pAllocInfoList that are valid.
    /// @returns Success if the allocation info was successfully written to the buffer.
    ///          + ErrorInvalidValue if the caller provides a buffer size that is different from the size needed.
    ///          + ErrorInvalidPointer if pNumEntries is nullptr.
    virtual Result QueryAllocationInfo(
        size_t*                    pNumEntries,
        GpuMemSubAllocInfo* const  pAllocInfoList) const = 0;

    /// Gives the client access to the resource ID used for internal Pal events.
    /// EX: Resource Create, Resource Bind, Resource Destroy.
    ///
    /// @returns The Resource ID.
    virtual const void* GetResourceId() const = 0;

    /// Returns PAL-generated name of this work-graph.
    virtual const char* Name() const = 0;

    /// Retrieves the unique shader program ID for the given node name and array-index combination.  The program
    /// ID is unique within this work graph.
    ///
    /// @param [in] name  Specifies the node to lookup in terms of node name and array-index.
    ///
    /// @returns
    ///     Shader program identifier.
    virtual ShaderProgramId GetNodeShaderId(const GraphNodeName& name) const = 0;

    /// Retrieves the GPU memory requirements for any backing store memory used in conjunction with a work graph.
    ///
    /// @param [out] pRequirements  GPU memory requirements.
    virtual void GetGpuMemoryRequirements(
        WorkGraphGpuMemoryRequirements* pRequirements) const = 0;

    /// Queries the work graph for the region in the backing store where the scheduler log buffer is stored.
    ///
    /// @param [out] pOffset  Offset where the log buffer begins.  Relative to the base of the graph backing store.
    /// @param [out] pSize    Size of the log buffer, in bytes.
    virtual void GetSchedulerLogRegion(
        gpusize* pOffset,
        gpusize* pSize) const = 0;

#if PAL_CLIENT_EXAMPLE
    /// Temporary interface to initialize a backing store using CPU writes.  Intended only to accelerate initialization
    /// during pre-silicon testing in the emulator.
    /// SHOULD NOT BE USED IN PRODUCTION!!
    ///
    /// @param  [in,out]    pGpuMem         Pointer to the backing store memory where the CPU writes go into
    /// @param  [in]        offset          Offset of the backing store memory
    /// @param  [in]        sizeInBytes     Size of the backing store memory, in bytes.
    virtual Result InitGraphBackingStoreCpu(
        IGpuMemory* pGpuMem,
        gpusize     offset,
        gpusize     sizeInBytes) const { PAL_NOT_IMPLEMENTED(); return Result::ErrorUnavailable; }
#endif //PAL_CLIENT_EXAMPLE

#if PAL_BUILD_WORK_GRAPH_TRACE
    /// Writes the work graph information (settings, legend, etc.) in JSON format.
    /// @param [in] pJsonWriter  Pointer to a JSON writer that will store the output.
    /// @param [in] params       Params used to bind the work graph. Required to provide information on backing store.
    /// @param [in] pGraphLayout Pointer to the graph layout used to create this work graph.
    /// @returns Success if the information was successfully written to the JSON writer.
    virtual Result WriteInfoToJson(
        Util::JsonWriter*          pJsonWriter,
        const WorkGraphBindParams& params,
        const IGraphLayout*        pGraphLayout) const = 0;
#endif

    /// Returns the value of the associated arbitrary client data pointer.
    /// Can be used to associate arbitrary data with a particular PAL object.
    ///
    /// @returns Pointer to client data.
    void* GetClientData() const { return m_pClientData; }

    /// Sets the value of the associated arbitrary client data pointer.
    /// Can be used to associate arbitrary data with a particular PAL object.
    ///
    /// @param  [in]    pClientData     A pointer to arbitrary client data.
    void SetClientData(void* pClientData) { m_pClientData = pClientData; }

protected:
    /// @internal Constructor. Prevent use of new operator on this interface. Client must create objects by explicitly
    /// called the proper create method.
    IWorkGraph() : m_pClientData(nullptr) {}

    /// @internal Destructor.  Prevent use of delete operator on this interface.  Client must destroy objects by
    /// explicitly calling IDestroyable::Destroy() and is responsible for freeing the system memory allocated for the
    /// object on their own.
    virtual ~IWorkGraph() { }

private:
    /// @internal  Client data pointer. This can have an arbitrary value and can be returned by calling GetClientData()
    /// and set via SetClientData().  For non-top-layer objects, this will point to the layer above the current object.
    void*  m_pClientData;

    IWorkGraph(const IWorkGraph&) = delete;
    IWorkGraph& operator=(const IWorkGraph&) = delete;
};

} // Pal
#endif // PAL_WORK_GRAPHS_SUPPORT
