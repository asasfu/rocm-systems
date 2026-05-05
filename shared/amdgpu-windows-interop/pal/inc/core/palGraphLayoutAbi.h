/* Copyright (c) Advanced Micro Devices, Inc., or its affiliates. All rights reserved. */
/**
 ***********************************************************************************************************************
 * @file  palGraphLayoutAbi.h
 * @brief Defines used in the PAL IGraphLayout interface that are also part of the ABI so need to be known
 * by the compiler.
 ***********************************************************************************************************************
 */

#pragma once

#include "palUtil.h"

#if PAL_WORK_GRAPHS_SUPPORT
namespace Pal
{

/// Enumerates the different types of nodes supported.
enum class GraphNodeType : Util::uint32
{
    /// Fixed-expansion nodes are nodes where a single queued work item corresponds to a single compute dispatch.  The
    /// node specifies a fixed-size dispatch grid.
    FixedExpansion = 0x0,
    /// Dynamic-expansion nodes are similar to fixed-expansion ones except that the dispatch grid is specified in the
    /// work item payload itself.  These nodes must instead specify a maximum dispatch grid size.
    DynamicExpansion,
    /// Aggregation nodes are launched one thread-group at a time.  Each dispatched thread group can consume multiple
    /// work item payloads, up to a maximum specified by the node itself.  Thread groups _may_ be launched with fewer
    /// payloads than the maximum in cases where not enough work is available when the system chooses to launch the
    /// thread group.
    Aggregation,
    /// Thread-launch nodes are launched one thread at a time (from the application's perspective).  The implementation
    /// will launch full thread-groups as much as it can, though this is never guaranteed.  Depending on the amount of
    /// work available, partial thread-groups _may_ be launched sometimes.  Each thread is considered a separate
    /// invocation of the node and consumes a single queued work item payload.  Application shaders for these nodes
    /// do not use LDS because they are only running a single thread (again, from the application's perspective).
    ThreadLaunch,
    /// Draw nodes are leaf nodes in the graph which consume non-indexed draws.  Each node is described by a graphics
    /// pipeline and some related graphics state which determines the behavior of the draw.  Each work item payload
    /// contains (at minimum) a @ref DrawIndirectArgs structure, plus any other input data the Vertex shader in the
    /// graphics pipeline can consume.
    Draw,
    /// DrawIndexed nodes are leaf nodes in the graph which consume indexed draws.  Each node is described by a graphics
    /// pipeline and some related graphics state which determines the behavior of the draw.  Each work item payload
    /// contains (at minimum) a @ref DrawIndexedIndirectArgs structure, plus any other input data the Vertex shader in
    /// the graphics pipeline can consume.
    DrawIndexed,
    /// DispatchMesh nodes are leaf nodes in the graph which consume mesh dispatches.  Each node is described by a
    /// graphics pipeline and some related graphics state which determines the behavior of the draw.  A DispatchMesh
    /// node must have a Mesh shader and can optionally have a Pixel shader.  Each work item payload contains (at
    /// minimum) a @ref DispatchMeshIndirectArgs structure, plus any other input data the Mesh shader in the graphics
    /// pipeline can consume.
    DispatchMesh,
    Count, ///< Number of node types.
};

/// Specifies creation flags for the creation of an @ref IGraphLayout object.
union GraphLayoutFlags
{
    struct
    {
        /// If true, the client opts-in to all nodes in a graph being treated as potential entry nodes into the graph
        /// graph by default, unless later overridden by calling @ref IGraphNode::SetEntry(false).  Otherwise, all
        /// nodes are treated as non-entry nodes unless later overridden by calling @ref IGraphNode::SetEntry(true).
        /// CrossGroupSharing cannot be entry nodes and they will be not set to true.
        Util::uint32  defaultAllNodesToEntry  :  1;
        /// If true, the client opts-in to manually specify the edges connecting the nodes of the graph.  If false,
        /// the edges will be determined based on metadata contained in the shader code objects.  It is undefined
        /// behavior to set this to true and subsequently call @ref AddPredecessor, @ref AddSuccessor, or related
        /// functions on this graph's nodes.
        Util::uint32  explicitDependencies    :  1;
        Util::uint32  reserved                : 30;   ///< Reserved for future use.
    };
    Util::uint32  u32All; ///< Flags packed as a Util::uint32.
};

/// Specifies properties for the creation of an @ref IGraphLayout object. Input structure to
/// @ref IDevice::CreateGraphLayout.
struct GraphLayoutCreateInfo
{
    GraphLayoutFlags  flags; ///< Flags describing the graph layout object to create.
};

/// Layout of graph layout data in the lead ELF in a new path workgraphs archive-of-ELFs.
/// First there is this GraphLayoutHeader struct, then multiple graph layout action structs,
/// distinguished by the GraphLayoutAction value at the start of each one.
struct GraphLayoutHeader
{
    GraphLayoutCreateInfo createInfo; ///< Info for IDevice::CreateGraphLayout
    Util::uint32          padding;    ///< Padding to ensure that the size of this struct is 8 aligned, to avoid
                                      ///  the data being 32- or 64-bit host specific.
};
static_assert(sizeof(GraphLayoutHeader) % sizeof(Util::uint64) == 0, "misaligned");

/// Enumerates the different graph layout action struct types.
enum class GraphLayoutAction : Util::uint16
{
    End = 0,              ///< End of graph layout data
    CreateNode,           ///< Create a node
    AddSuccessor,         ///< Add a successor to an existing node
    SetMaxRecursiveDepth, ///< Mark an edge as recursive
#if PAL_CLIENT_INTERFACE_MAJOR_VERSION >= 918
    CreateDrawNode,       ///< Create a draw node
#endif
};

/// The base of all graph layout action structs.
struct GraphLayoutActionBase
{
    Util::uint16      size;   ///< Size in bytes of the struct, so the reader can skip to the next one
    GraphLayoutAction action; ///< Which graph layout action struct this is
};
static_assert(sizeof(GraphLayoutActionBase) == sizeof(Util::uint32),
              "Unexpected size; may be different layout between 32 and 64 bit hosts. Manually check layout.");

/// Specifies the name of a node in graph layout data.
struct GraphLayoutNodeName
{
    Util::uint32 nodeName;   ///< Name of the node or node group this node belongs to, as a string offset
    Util::uint32 arrayIndex; ///< Array index within an array of nodes.
};

/// The action struct in graph layout data for creating a node.
/// Created nodes are given a _node reference_ sequentially starting at 0; other actions refer to a node using that
/// node reference.
struct GraphLayoutCreateNode : public GraphLayoutActionBase
{
    bool                 isEntry;         ///< Whether the node is a graph entry.

    /// Identifies the name for this node.
    GraphLayoutNodeName  nodeName;

    /// Specifies the name of the node this shader shares its input record with.  If inputSharedWith.nodeName is 0,
    /// the node to share input with will be determined from the metadata in the ELF code object the shader occupies
    /// (which may well indicate "no input sharing").
    /// It is invalid for another node to specify _this_ node for input sharing if this node is already sharing another
    /// node's input.
    GraphLayoutNodeName  inputSharedWith;

    Util::uint64         library;         ///< First 64 bits of ELF name of shader library object which contains
                                          ///  the ELF symbol.  Or (for a draw node) the mesh pipeline.
    union
    {
        Util::uint32     symbol;          ///< ELF symbol name for the node's shader, as offset to string.
#if PAL_CLIENT_INTERFACE_MAJOR_VERSION >= 918
        Util::uint32     pipelineIndex;   ///< For a draw node, index into the draw pipelines provided to SCPC, which
                                          ///  is the same as the index into the graphics state array passed by the
                                          ///  client into IPipeline::GetGraphLayout().
#endif
    };

    /// Index used by the client to select which node(s) receive initial work payloads when dispatching a graph.  This may
    /// differ from the index assigned to this node when PAL constructs the final graph.  This will be ignored for
    /// non-entrypoint nodes.
    Util::int32          entrypointIndex;

    /// Overrides the maximum amount of self-recursion which this node can perform.  A value of @c 0 indicates that
    /// this node cannot self-recurse.  It is legal for each node in a node-array to
    /// have a different value for the maximum recursion depth.
    Util::int32          maxRecursionDepth;

    union
    {
        /// Information for shaders belonging to fixed-expansion nodes.
        struct
        {
            /// Fixed dispatch grid size for the shader.  Each shader in a node can have a different grid size, but
            /// the sizes are fixed for each shader.
            Util::uint32  dispatchGridSize[3];
        } fixedExpansionInfo;
        /// Information for shaders belonging to dynamic-expansion nodes.
        struct
        {
            /// Maximum dispatch grid size for the shader.  Each shader in a node can have a different maximum grid
            /// size.
            Util::uint32  maxDispatchGridSize[3];
        } dynamicExpansionInfo;
        /// Information for shaders belonging to aggregation nodes.
        struct
        {
            /// Maximum number of input payloads each workgroup can process.  Must be at least @c 1.
            Util::uint32  maxNumPayloads;
        } aggregationInfo;
        struct
        {
        } threadLaunchInfo;
#if PAL_CLIENT_INTERFACE_MAJOR_VERSION >= 918
        /// Information for graphics nodes.
        struct
        {
            /// Fixed dispatch grid size for dispatch mesh nodes. A zero value in any dimension indicates that the grid
            /// size is not fixed. Can have the value @ref GraphUseValueFromShader to indicate that the value should
            /// come from shader metadata.
            uint32 dispatchGridSize[3];
        } graphicsInfo;
#endif
    };

    /// Each node can be associated with a unique local resource mapping index, which can vary between nodes even if
    /// they use the same shader.  This value is opaque to PAL and is accessible from the node's shader using the
    /// Shader Runtime Library (SRL) provided for WorkGraphs shaders.
    Util::uint32         localResourceMappingId;

    Util::uint32         padding60;
};
static_assert(sizeof(GraphLayoutCreateNode) == 8 * sizeof(Util::uint64),
              "Unexpected size; may be different layout between 32 and 64 bit hosts. Manually check layout.");

/// The action struct in graph layout data for setting max recursion depth for a recursive successor.
struct GraphLayoutSetMaxRecursiveDepth : public GraphLayoutActionBase
{
    Util::uint32  nodeRef;         ///< 0-based node reference (see GraphLayoutCreateNode) to mark as recursive and
                                   ///  set max recursion.
    Util::uint32  depth;           ///< Max recursive depth for this node.
    Util::uint32  outputPortIndex; ///< Specifies which output port of @ref pSuccessor this dependency corresponds to.
                                   ///  The number of output ports a node has is defined by the node's shader metadata.
};
static_assert(sizeof(GraphLayoutSetMaxRecursiveDepth) == 2 * sizeof(Util::uint64),
              "Unexpected size; may be different layout between 32 and 64 bit hosts. Manually check layout.");

/// The action struct in graph layout data for adding a successor.
struct GraphLayoutAddSuccessor : public GraphLayoutActionBase
{
    Util::uint32  nodeRef;          ///< 0-based node reference (see GraphLayoutCreateNode) to add the successor to.
    Util::uint32  successor;        ///< 0-based node reference (see GraphLayoutCreateNode) whose parent array the
                                    ///  successor node(s) belong to. The indicated node does not have to actually be
                                    ///  one of the successors; it is just used to indicate the node array containing
                                    ///  the successors.
    Util::uint32  arrayIndex;       ///< Starting array index within the @ref pSuccessor array which the graph
                                    ///  dependency can send data to.
    Util::uint32  arrayCount;       ///< Number of consecutive array indices of @ref pSuccessor which are included in
                                    ///  the dependency. Must be at least @c 1.  It is an error if @ref arrayCount +
                                    ///  @ref arrayIndex is beyond the number of valid indices for the destination.
                                    ///  A value of @c UINT_MAX indicates "all valid indices greater than or equal to
                                    ///  arrayIndex".

    Util::uint32  outputPortIndex;  ///< Specifies which output port of @ref pSuccessor this dependency corresponds to.
                                    ///  The number of output ports a node has is defined by the node's shader metadata.

    Util::int32   maxOutputRecords; ///< Maximum number of output payloads which each work-group of the node can send
                                    ///  along this dependency.  For thread-launch nodes, this is per-thread rather than
                                    ///  per-work-group. The value must be at least @c 1.
    Util::uint32  padding;
};
static_assert(sizeof(GraphLayoutAddSuccessor) == 4 * sizeof(Util::uint64),
              "Unexpected size; may be different layout between 32 and 64 bit hosts. Manually check layout.");

} // Pal
#endif // PAL_WORK_GRAPHS_SUPPORT

