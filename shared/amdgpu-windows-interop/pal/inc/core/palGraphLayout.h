/* Copyright (c) Advanced Micro Devices, Inc., or its affiliates. All rights reserved. */
/**
 ***********************************************************************************************************************
 * @file  palGraphLayout.h
 * @brief Defines the PAL IGraphLayout interface.
 ***********************************************************************************************************************
 */

#pragma once

#include "pal.h"
#include "palDestroyable.h"
#include "palGraphLayoutAbi.h"
#include "palStringView.h"

#if PAL_WORK_GRAPHS_SUPPORT
namespace Pal
{

class IGraphNodeArray;
class IGraphNode;
class IShaderLibrary;
class IColorBlendState;
class IDepthStencilState;
class IMsaaState;
class IPipeline;

enum class IndexType : uint32;

struct BlendConstParams;
struct DepthBiasParams;
struct DepthBoundsParams;
struct InputAssemblyStateParams;
struct LineStippleStateParams;
struct MsaaQuadSamplePattern;
struct PointLineRasterStateParams;
struct TriangleRasterStateParams;
struct VrsRateParams;

/// Specifies the longest chain of nodes in a graph.
constexpr uint32 GraphLongestNodeChain = 32u;

/// Specifies that the information will be derived from the shader.
constexpr int32 GraphUseValueFromShader = -1;

/// Specifies the input sharing max depth api limit
constexpr uint32 InputSharingMaxDepth = 256u;

/// Specifies the name of a node.
struct GraphNodeName
{
    const char*  pNodeName;     ///< Name of the node or node group this node belongs to.
    uint32       arrayIndex;    ///< Array index within an array of nodes.
};

/// Default name for a node which indicates that the node name should come from the metadata in the ELF code object
/// the node's shader occupies.
constexpr GraphNodeName UseNodeNameFromShader { nullptr, uint32(GraphUseValueFromShader), };

#if PAL_CLIENT_DX
/// Unique index assigned to each entry node by the DX runtime in a work graph.
using DxEntrypointIndex = uint32;

/// Special DxEntrypointIndex which represents an invalid entrypoint.
constexpr DxEntrypointIndex InvalidEntrypointIndex = uint32(-1);
#endif

/// Specifies graphics state information for a draw, draw-indexed, or dispatch-mesh node.  All fields are _optional_.
/// If they are null, then the graphics node doesn't define them and they will be either global to the entire graph,
/// or inherited from the current command buffer at the time @ref ICmdBuffer::CmdDispatchGraph is invoked.
///@note  Do we need _all_ of these??  I threw in pretty much everything but the kitchen sink here because the API is
/// still unknown at this point.  Some of these may end up getting removed as development on the updated API evolves.
struct GraphicsNodeStateBlock
{
    /// A shallow copy of this state will be maintained by the graph layout. The callee is responsible for ensuring
    /// the validity of the interface object while the graph layout is in use.
    struct InterfaceState
    {
        const IColorBlendState*   pColorBlend;    ///< Color blending state.
        const IDepthStencilState* pDepthStencil;  ///< Depth/stencil state.
        const IMsaaState*         pMsaa;          ///< Multisample anti-aliasing state.
    } interfaceState;

    /// A deep copy of this state is maintained by the graph layout.
    struct DirectState
    {
        const BlendConstParams*  pBlendConst;  ///< Blend constants.

        const DepthBoundsParams*  pDepthBounds;  ///< Depth bounds.
        const DepthBiasParams*    pDepthBias;    ///< Depth bias.

        const InputAssemblyStateParams*  pInputAssembly;  ///< Input assembly state.

        const TriangleRasterStateParams*   pTriangleRaster;   ///< Triangle rasterization parameters.
        const PointLineRasterStateParams*  pPointLineRaster;  ///< Point or line rasterization parameters.
        const LineStippleStateParams*      pLineStipple;      ///< Line stippling parameters.

        const MsaaQuadSamplePattern*  pQuadSamplePattern;  ///< MSAA Quad sample pattern.
        uint32                        numSamplesPerPixel;  ///< Number of samples per pixel.  Must be zero if
                                                           ///  @ref pQuadSamplePattern is null.

        const VrsRateParams*          pVrsRate;            ///< Vrs rate parameters
        IndexType                     indexType;           ///< Bit size of each element in this node's index buffer.
                                                           ///< Only valid for DrawIndexed nodes. A value of
                                                           ///< IndexType::Count indicates that no value is specified.
    } directState;
};

/// Specifies properties for the creation of an @ref IGraphNode object. Input structure to
/// @ref IGraphLayout::AddNode.
struct GraphNodeCreateInfo
{
    /// Identifies the name for this node.  If nodeName.pNodeName is null, the value will be taken from the metadata
    /// in the ELF code object the shader occupies.  If nodeName.arrayIndex is @ref GraphUseValueFromShader, then the
    /// array index will be taken from the metadata.
    GraphNodeName  nodeName;

    /// Specifies the name of the node this shader shares its input record with.  If inputSharedWith.pNodeName is null,
    /// the node to share input with will be determined from the metadata in the ELF code object the shader occupies
    /// (which may well indicate "no input sharing").
    /// It is invalid for another node to specify _this_ node for input sharing if this node is already sharing another
    /// node's input.
    GraphNodeName  inputSharedWith;

#if PAL_CLIENT_DX
    /// The DX runtime assigns a unique entrypoint index to each entry node in the graph.  These indices are used by
    /// the application to select which node(s) receive initial work payloads when dispatching a graph.  This may
    /// differ from the index assigned to this node when PAL constructs the final graph.  This will be ignored for
    /// non-entrypoint nodes.
    DxEntrypointIndex  runtimeEntrypointIndex;

    /// Each node can be associated with a unique local resource mapping index, which can vary between nodes even if
    /// they use the same shader.  This value is opaque to PAL and is accessible from the node's shader using the
    /// Shader Runtime Library (SRL) provided for WorkGraphs shaders.
    uint32  localResourceMappingId;
#endif

    /// Graphics Pipeline associated with this node.  Must be a graphics pipeline, or be null.  If non-null, this
    /// defines a graphics node, and both @ref pLibrary and @ref pSymbol must be null.  If null, this defines a
    /// compute node, and both @ref pLibrary and @ref pSymbol cannot be null.
    /// pPipeline must have been created with a non-nullptr name.
    const IPipeline*  pPipeline;

    /// Shader library object which contains the ELF symbol @ref pSymbol.  If non-null, this defines a compute node,
    /// and @ref pPipeline must be null.  If null, this defines a graphics node, and @ref pPipeline cannot be null.
    IShaderLibrary*   pLibrary;
    const char*       pSymbol;   ///< ELF symbol name for the node's shader.  Must be null when @ref pLibrary is null,
                                 ///  and cannot be null when @ref pLibrary is non-null.

    /// Overrides the maximum amount of self-recursion which this node can perform.  A value of @c 0 indicates that
    /// this node cannot self-recurse.  A value of @ref GraphUseValueFromShader indicates that this node will each use
    /// the value stored in the metadata of the shader's ELF code object.  It is legal for each node in a node-array to
    /// have a different value for the maximum recursion depth.
    /// If self-recursion is enabled for this node, the output port which sends data back to itself will be determined
    /// by shader metadata.  This must be @c 0 for graphics nodes.
    int32  maxRecursionDepth;

    union
    {
        /// Information for shaders belonging to fixed-expansion nodes.  Any of the fields in the struct below can have
        /// the value @ref GraphUseValueFromShader to indicate that the value should come from shader metadata rather
        /// than this structure.
        struct
        {
            /// Fixed dispatch grid size for the shader.  Each shader in a node can have a different grid size, but
            /// the sizes are fixed for each shader.
            Extent3d  dispatchGridSize;
        } fixedExpansionInfo;
        /// Information for shaders belonging to dynamic-expansion nodes.  Any of the fields in the struct below can
        /// have the value @ref GraphUseValueFromShader to indicate that the value should come from shader metadata
        /// rather than this structure.
        struct
        {
            /// Maximum dispatch grid size for the shader.  Each shader in a node can have a different maximum grid
            /// size.
            Extent3d  maxDispatchGridSize;
        } dynamicExpansionInfo;
        /// Information for shaders belonging to aggregation nodes.  Any of the fields in the struct below can have
        /// the value @ref GraphUseValueFromShader to indicate that the value should come from shader metadata rather
        /// than this structure.
        struct
        {
            /// Maximum number of input payloads each workgroup can process.  Must be at least @c 1.
            uint32  maxNumPayloads;
        } aggregationInfo;
        struct
        {
        } threadLaunchInfo;
        /// Information for graphics nodes.
        struct
        {
            /// Fixed dispatch grid size for dispatch mesh nodes. A zero value in any dimension indicates that the grid
            /// size is not fixed. Can have the value @ref GraphUseValueFromShader to indicate that the value should
            /// come from shader metadata.
            Extent3d dispatchGridSize;
            /// Contains the entire state block defined by this node.  Every graphics node in a graph _must_ define the
            /// same set of the optional elements of this state block (though of course the values of each element can
            /// vary between nodes).
            GraphicsNodeStateBlock  stateBlock;
        } graphicsInfo;
    };
};

/// Specifies properties for the creation of a dependency from one @ref IGraphNode object to one or more @ref IGraphNode
/// objects.  Input to @ref IGraphLayout::AddDependency().
struct GraphDependencyInfo
{
    GraphNodeName  nodeFrom;    ///< Identifies the name of the node the dependency flows out of.
    GraphNodeName  nodeTo;      ///< Identifies the name of the node the dependency flows into.

    /// Number of consecutive array indices of @ref nodeTo which are included in the dependency.  Must be at least @c 1.
    /// It is an error if @ref arrayCount + @ref nodeTo.arrayIndex is beyond the number of valid indices for the
    /// destination.  A value of @c UINT_MAX indicates "all valid indices greater than or equal to nodeTo.arrayIndex".
    uint32  arrayCount;

    uint32  outputPortIndex;    ///< Specifies which output port of @ref nodeFrom this dependency corresponds to.  The
                                ///  number of output ports a node has is defined by the node's shader metadata.

    int32  maxOutputRecords;    ///< Maximum number of output payloads which each work-group of @ref nodeFrom can send
                                ///  along this dependency.  For thread-launch nodes, this is per-thread rather than
                                ///  per-work-group.  If @ref GraphUseValueFromShader, then the value defined in the
                                ///  shader metadata for @ref nodeFrom is used.  Otherwise, must be at least @c 1.
};

/// Specifies properties for the addition of a successor to an existing node.  Input to @ref IGraphNode::AddSuccessor().
/// A node's successor is a contiguous set of nodes contained in a node array.
struct GraphNodeSuccessorInfo
{
    IGraphNodeArray*  pSuccessor; ///< Node array which the successor node(s) belong to.

    uint32  arrayIndex;           ///< Starting array index within the @ref pSuccessor array which the graph dependency
                                  ///  can send data to.
    uint32  arrayCount;           ///< Number of consecutive array indices of @ref pSuccessor which are included in the
                                  ///  dependency.  Must be at least @c 1.  It is an error if @ref arrayCount +
                                  ///  @ref arrayIndex is beyond the number of valid indices for the destination.
                                  ///  A value of @c UINT_MAX indicates "all valid indices greater than or equal to
                                  ///  arrayIndex".

    uint32  outputPortIndex;      ///< Specifies which output port of @ref pSuccessor this dependency corresponds to.
                                  ///  The number of output ports a node has is defined by the node's shader metadata.

    int32  maxOutputRecords;      ///< Maximum number of output payloads which each work-group of the node can send
                                  ///  along this dependency.  For thread-launch nodes, this is per-thread rather than
                                  ///  per-work-group.  If @ref GraphUseValueFromShader, then the value defined in the
                                  ///  shader metadata for the origin node is used.  Otherwise, must be at least @c 1.
};

/// Specifies properties for the addition of a predecessor node to an existing node.  Input to
/// @ref IGraphNode::AddPredecessor().
struct GraphNodePredecessorInfo
{
    IGraphNode*  pPredecessor;   ///< Predecessor node.

    uint32  outputPortIndex;    ///< Specifies which output port of @ref pPredecessor this dependency corresponds to.
                                ///  The number of output ports a node has is defined by the node's shader metadata.

    int32  maxOutputRecords;    ///< Maximum number of output payloads which each work-group of @ref pPredecessor can
                                ///  send along this dependency. For thread-launch nodes, this is per-thread rather
                                ///  than per-work-group.  If @ref GraphUseValueFromShader, then the value defined in
                                ///  the shader metadata for the predecessor is used. Otherwise, must be at least @c 1.
};

/// Specifies all the information associated with an output port of a @ref IGraphNode.
struct GraphOutputPortInfo
{
    IGraphNodeArray*  pSuccessor;   ///< Node array which the output port flows into.

    uint32  arrayIndex;         ///< Starting array index within the pSuccessor array which the graph edge flows into.
    uint32  arrayCount;         ///< Number of consecutive array indices which are included in the edge.

    uint32  maxOutputRecords;   ///< Maximum number of output records per work-group.  It is always at least @c 1.
    uint32  outputRecordSize;   ///< Size of each output record, in bytes.

    /// Index of another output port with which this port shares its output budget.  A value of
    /// @ref NoOutputBudgetSharing indicates that the output budget is _not_ shared.
    int16  outputBudgetSharedWith;
};

/// Special value of @ref GraphOutputPortInfo::outputBudgetSharedWith which indicates that no output budget sharing
/// is ocurring.  This should be the default value for most node shader outputs.
constexpr int16 NoOutputBudgetSharing = -1;

/**
 ***********************************************************************************************************************
 * @interface IGraphNode
 * @brief     Object representing a node in a graph of shaders.
 *
 * @see IGraphLayout
 ***********************************************************************************************************************
 */
class IGraphNode
{
public:
    /// Returns the node name.
    virtual GraphNodeName Name() const = 0;

    /// Returns the type of the node.
    virtual GraphNodeType Type() const = 0;

    /// Returns the node array which contains this node.  All nodes belong to one node array, even if they are the
    /// only node in that array.
    virtual IGraphNodeArray* GetArray() = 0;

    /// Returns the number of output ports.
    virtual uint32 NumOutputPorts() const = 0;

    /// Returns the maximum recursive depth for the node.
    virtual uint32 MaxRecursiveDepth() const = 0;

    virtual uint16 DrawParamsOffset() const = 0;
    virtual uint16 IndexBufferInfoOffset() const = 0;
    virtual uint16 VbTableOffset() const = 0;

    /// Returns the size of the longest chain of nodes leading to this node which includes this node's recursive depth.
    ///
    /// One can think of this as the <tt>longest-path + 1</tt> to account for the starting node.
    ///
    /// The longest chain of nodes of a single node graph is @c 1 if it is not recursive or its recursive depth
    /// <tt>+1</tt> if it is recursive.
    ///
    /// @warning This function will return a valid value only after @ref IGraphLayout::Finalize() has been called.
    virtual uint32 NodeChainLength() const = 0;

    /// Returns the minimum depth of the node when all entry nodes are considered roots.
    ///
    /// The depth of entry nodes is always @c 0.
    ///
    /// @warning This function will return a valid value only after @ref IGraphLayout::Finalize() has been called.
    virtual uint32 Depth() const = 0;

    /// Returns @c true if this is an entry node, otherwise @c false.
    virtual bool IsEntry() const = 0;

    /// Returns information about the @p n -th output port of this node.
    virtual GraphOutputPortInfo GetOutputPortInfo(
        uint32 n) const = 0;

    /// Connects an output port for this node to their successor.  Each port can be connected to one or more
    /// nodes belonging to a node array.
    ///
    /// @param [in] successorInfo  Successor node array to connect to an output port.
    ///
    /// None of the successors can include this node.  For recursive calls, see @ref SetMaxRecursiveDepth.
    virtual Result AddSuccessor(
        const GraphNodeSuccessorInfo& successorInfo) = 0;

    /// Connects a predeccessor node for this node.
    ///
    /// @param [in] predecessorInfo  Predecessor node to connect to this node.
    ///
    /// The predecessor cannot include this node.  For recursive calls, see @ref SetMaxRecursiveDepth.
    virtual Result AddPredecessor(
        const GraphNodePredecessorInfo& predecessorInfo) = 0;

    /// Sets the maximum recursive calls to the node to @p count.
    ///
    /// @param [in] depth            Maximum recusive depth for this node.
    /// @param [in] outputPortIndex  Index of the output port to use for recursion.  A value of
    ///                              @ref GraphUseValueFromShader indicates to use shader metadata to determine which
    ///                              output port's destination has the same name as this node.
    ///
    /// This call will connect output port @c 0 of this node to the input port with shader-defined maximum output
    /// records.
    virtual Result SetMaxRecursiveDepth(
        uint32 depth,
        int32 outputPortIndex) = 0;
    Result SetMaxRecursiveDepth(uint32 depth) { return SetMaxRecursiveDepth(depth, GraphUseValueFromShader); }

    /// Declares this node as an entry node if @p isEntry is @c true, otherwise it is an internal node.
    virtual void SetEntry(
        bool isEntry) = 0;

protected:
    /// @internal Constructor.  Prevent use of new operator on this interface.  Client must create objects by explicitly
    /// called the proper create method.
    IGraphNode() { }

    /// @internal Destructor.  Prevent use of delete operator on this interface.  These objects are destroyed when the
    /// parent @ref IGraphLayout object is destroyed.
    virtual ~IGraphNode() { }

    IGraphNode(const IGraphNode&) = delete;
    IGraphNode& operator=(const IGraphNode&) = delete;
};

/**
 ***********************************************************************************************************************
 * @interface IGraphNodeArray
 * @brief     Object representing the an array of nodes in a graph of shaders.  An "array" of nodes is a logical
 *            grouping of nodes which have the same input payload type and launch type.  A node's output port can
 *            connect to an entire group or a subset in the group instead of a single node.
 *
 * @note      _Every_ node in a graph belongs to a parent array of nodes, even if the node is the only one in that
 *            array.
 *
 * @see IGraphLayout
 ***********************************************************************************************************************
 */
class IGraphNodeArray
{
public:
    using StringViewType = Util::StringView<char>;

    /// Returns the node-array's name.
    virtual StringViewType Name() const = 0;

    /// Returns the number of valid index slots for this array.  This value can change over the lifetime of the object
    /// if @ref IGraphLayout::AddNode is used to add more nodes to an existing array.
    virtual size_t NumValidSlots() const = 0;

    /// Returns the number of index slots which are currently populated with nodes.  This value can change over the
    /// lifetime of the object if @ref IGraphLayout::AddNode is used to add more nodes to an existing array.
    virtual size_t NumPopulatedSlots() const = 0;

    /// Queries the node array for the node associated with a particular array index.  Note that the array indices are
    /// allowed to be sparsely populated.
    ///
    /// @param [in] index  Array index of the node to query.  Must be less than the return value of @ref NumValidSlots.
    virtual IGraphNode* GetNode(uint32 index) const = 0;

protected:
    /// @internal Constructor.  Prevent use of new operator on this interface.  Client must create objects by explicitly
    /// called the proper create method.
    IGraphNodeArray() { }

    /// @internal Destructor.  Prevent use of delete operator on this interface.  These objects are destroyed when the
    /// parent @ref IGraphLayout object is destroyed.
    virtual ~IGraphNodeArray() { }

    IGraphNodeArray(const IGraphNodeArray&) = delete;
    IGraphNodeArray& operator=(const IGraphNodeArray&) = delete;
};

/**
 ***********************************************************************************************************************
 * @interface IGraphLayout
 * @brief     Object representing the layout of a graph of shaders.  The graph must be a directed acyclic graph of nodes
 *            where the maximum length of any path through the graph cannot exceed @ref MaximumPathLength.
 *
 * @see IDevice::CreateGraphLayout()
 * @see IDevice::CreateComputePipeline()
 ***********************************************************************************************************************
 */
class IGraphLayout : public IDestroyable
{
public:
    using StringViewType = Util::StringView<char>;

    /// Returns the value of the associated arbitrary client data pointer.
    /// Can be used to associate arbitrary data with a particular PAL object.
    ///
    /// @returns Pointer to client data.
    void* GetClientData() const { return m_pClientData; }

    /// Sets the value of the associated arbitrary client data pointer.
    /// Can be used to associate arbitrary data with a particular PAL object.
    ///
    /// @param [in] pClientData  A pointer to arbitrary client data.
    void SetClientData(void* pClientData) { m_pClientData = pClientData; }

    /// Returns the amount of system memory required for copying this graph.
    virtual size_t GetSize() const = 0;

    /// Copies this graph.
    ///
    /// @param [in]  pPlacementAddr  Pointer to the location where PAL should construct this object.  There must be as
    ///                              much size available here as reported by calling @ref GetSize.
    /// @param [out] ppGraphLayout   Constructed graph layout object.  When successful, the returned address will be the
    ///                              same as specified in @p pPlacementAddr.
    virtual Result CreateCopy(
        void*          pPlacementAddr,
        IGraphLayout** ppGraphLayout) const = 0;

    /// Creates a new node and adds it to the graph.  If the new node's name.pNodeName string matches that of any
    /// existing node, the new node is added to the existing node's parent node array.
    virtual Result AddNode(
        const GraphNodeCreateInfo& createInfo,
        IGraphNode**               ppNode) = 0;

    /// Creates one or more new dependencies between nodes and adds them to the graph.  This is provided as a
    /// convenience function for clients who don't want to look up the source and destination nodes prior to
    /// defining each dependency.
    ///
    /// It is an error if any of the nodes specified by the dependency list haven't been added to the graph yet.
    ///
    /// @param [in] pInfos  Array of structures defining the dependencies to create.
    /// @param [in] count   Length of the @ref pInfos array.  Must be at least @c 1.
    virtual Result AddDependencies(
        const GraphDependencyInfo* pInfos,
        uint32                     count) = 0;

    /// Returns whether or not this graph contains only compute nodes.
    ///
    /// @warning This function will return a valid value only after @ref IGraphLayout::Finalize() has been called.
    virtual bool IsComputeOnly() const = 0;

    /// Returns the number of valid nodes currently contained within the graph.
    virtual size_t NumNodes() const = 0;

    /// Performs a lookup of a previously-added graph node based on the name of the node.
    ///
    /// @param [in] name  Name to search for.
    ///
    /// Returns the node associated with the given name.  Nullptr is returned if no such node exists.
    virtual IGraphNode* LookupNode(const GraphNodeName& name) const = 0;

    /// Performs a lookup of an array of previously-added graph nodes based on the name of the array.
    ///
    /// @param [in] pName  Name to search for.
    ///
    /// Returns the node array associated with the given name.  Nullptr is returned if no such node exists.
    virtual IGraphNodeArray* LookupNodeArray(const char* pName) const = 0;

    /// Performs a lookup of an array of previously-added graph nodes based on the name of the array.
    ///
    /// @param [in] pName  Name to search for.
    ///
    /// Returns the node array associated with the given name.  Nullptr is returned if no such node exists.
    virtual IGraphNodeArray* LookupNodeArray(StringViewType pName) const = 0;

    /// Finalizes this graph.
    ///
    /// Modifying the graph or its nodes after @ref Finalize() is called constitutes undefined behavior.
    ///
    /// It will calculate all internal metadata that are too costly to do during graph creation and it will verify that
    /// the graph is well-formed.
    ///
    /// During validation it will validate the graph and return:
    /// - @ref Result::ErrorGraphNoEntryNodes if there are no entry nodes,
    /// - @ref Result::ErrorGraphNoPredecessors if an internal node is unreachable,
    /// - @ref Result::ErrorGraphNotAcyclic if there is a cycle in the graph,
    /// - @ref Result::ErrorGraphExceededMaxPath if the longest graph path is more than @ref GraphMaximumPathLength.
    virtual Result Finalize() = 0;

    /// @internal
    /// Returns if the graph is finalized.
    ///
    /// @warning This is an internal function and its existence, its signature and its semantics are not guaranteed
    ///          across different PAL versions.
    ///
    /// @see Finalize()
    virtual bool IsFinalized() const = 0;

    /// @internal
    /// Compares this graph with @p other.
    ///
    /// This function does not require the graphs to be finalized. It will return @c false if the graphs are just
    /// isomorphic.
    ///
    /// @warning This is an internal function and its existence, its signature and its semantics are not guaranteed
    ///          across different PAL versions.
    virtual bool operator==(
        const IGraphLayout& other) const = 0;

    bool operator!=(const IGraphLayout& other) const { return (*this == other) == false; }

protected:
    /// @internal Constructor.  Prevent use of new operator on this interface. Client must create objects by explicitly
    /// called the proper create method.
    IGraphLayout() : m_pClientData(nullptr) {}

    /// @internal Destructor.  Prevent use of delete operator on this interface.  Client must destroy objects by
    /// explicitly calling IDestroyable::Destroy() and is responsible for freeing the system memory allocated for the
    /// object on their own.
    virtual ~IGraphLayout() { }

private:
    /// @internal Client data pointer.  This can have an arbitrary value and can be returned by calling GetClientData()
    /// and set via SetClientData().
    /// For non-top-layer objects, this will point to the layer above the current object.
    void*  m_pClientData;

    IGraphLayout(const IGraphLayout&) = delete;
    IGraphLayout& operator=(const IGraphLayout&) = delete;
};

} // Pal
#endif // PAL_WORK_GRAPHS_SUPPORT
