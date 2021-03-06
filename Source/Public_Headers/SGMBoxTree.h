#ifndef SGM_BOXTREE_H
#define SGM_BOXTREE_H

#include "sgm_export.h"
#include "SGMInterval.h"
#include "SGMBounded.h"
#include "SGMMemoryPool.h"

#include <list>
#include <stack>
#include <vector>
#include <map>

// #define BOX_TREE_USE_MEMORY_POOL // disable this if multithreaded

#define SGM_BOX_MAX_RAY_HITS 32 // reserved space for size of vector returned by FindIntersectsRay()

namespace SGM {

    /**
     * An index of items and their associated bounding box
     * (pairs of <void*, Interval3D>) based on the R* Tree algorithm.
     *
     * The void* to the object on leaves of the tree may be changed, but the
     * bounding box of the object must remain identical to avoid violating the tree.
     */
    class SGM_EXPORT BoxTree
    {
    public:

        typedef std::pair<void const*,Interval3D> BoundedItemType;

        /**
         * Construct an empty tree.
         */
        BoxTree();

        /**
         * Construct an empty tree, all bounded inserted will be extended by the given tolerance.
         */
        explicit BoxTree(double tolerance);

        /**
         * Construct a copy of another tree.
         */
        BoxTree(BoxTree const & other);

        /**
         * Assignment operator.
         */
        BoxTree& operator=( const BoxTree &rhs);

        /**
         * Destroy the tree and free all resources.
         */
        ~BoxTree();

        /**
         * Checks whether the tree contains any items
         */
        bool IsEmpty() const;

        /**
         * The number of items in the tree.
         *
         * @return count of items/boxes in the container
         */
        size_t Size() const;

        /**
         *  Clears the contents of the container.
         *
         *  Removes all elements from the container. Invalidates any references, pointers, or iterators referring to
         *  Bounded elements. Any past-the-end iterator remains valid.
         */
         void Clear();

        /**
         * Insert a single item/box into the container.
         *
         * @param object the item
         * @param bound the bounding box containing the item
         */
        void Insert(void const* object, Interval3D const & bound);

        /**
         * Remove any entries (item/box) contained in the given bounding box.
         *
         * @param bound a bounding box
         */
        void EraseEnclosed(Interval3D const &bound);

        /**
         * Remove a specific entry.
         *
         * @param item the item to remove
         * @param removeDuplicates if false, only the first item found will be removed.
         */
        void Erase(const void *&item, bool removeDuplicates = true);

        /**
         * Replace any item pointer in the tree that match Key with Value from the map while not
         * changing the item's associated bounding box.
         */
        void Replace(std::map<const void *, const void *> const &itemMap);

        /**
         * Exchanges the contents of the index with those of other.
         *
         * Does not invoke any move, copy, or swap operations on individual elements.
         */
        void Swap( BoxTree& other );

        ///////////////////////////////////////////////////////////////////////
        //
        // Query Member Functions
        //
        ///////////////////////////////////////////////////////////////////////

        /// Return all the items in the tree
        std::vector<void const*> FindAll() const;

        /// Return items whose bounds are enclosed in the given bound
        std::vector<void const*> FindEnclosed(Interval3D const &bound) const;

        /// Return items whose bounds intersect a given bound
        std::vector<void const*> FindIntersectsBox(Interval3D const &bound) const;

        /// Return items whose bounds intersect the half space
        std::vector<void const*> FindIntersectsHalfSpace(Point3D      const &point,
                                                         UnitVector3D const &unitVector,
                                                         double              tolerance) const;

        /// Return items whose bounds intersect the line
        std::vector<void const*> FindIntersectsLine(Ray3D const &ray,
                                                    double       tolerance) const;

        std::vector<void const*> FindIntersectsLine(Ray3D const &ray) const;

        /// Return items whose bounds intersect the plane
        std::vector<void const*> FindIntersectsPlane(Point3D      const &point,
                                                         UnitVector3D const &unitVector,
                                                         double              tolerance) const;

        /// Return items whose bounds intersect the point
        std::vector<void const*> FindIntersectsPoint(Point3D const &point,
                                                     double         tolerance) const;

        /// Return items whose bounds intersect the point
        std::vector<void const*> FindIntersectsPoint(Point3D const &point) const;

        /// Return items whose bounds intersect the ray
        std::vector<void const*> FindIntersectsRay(Ray3D const &ray,
                                                   double       tolerance) const;

        std::vector<void const*> FindIntersectsRay(Ray3D const &ray) const;

        /// Return items whose bounds intersect the segment
        std::vector<void const*> FindIntersectsSegment(Point3D const &p1,
                                                       Point3D const &p2,
                                                       double         tolerance) const;

        /// Return items whose bounds intersect the segment
        std::vector<void const*> FindIntersectsSegment(Point3D const &p1,
                                                       Point3D const &p2) const;

        /// Return items whose bounds intersect the sphere
        std::vector<void const*> FindIntersectsSphere(Point3D const &center,
                                                      double         radius,
                                                      double         tolerance) const;

        /// Count items whose bounds intersect the ray
        size_t CountIntersectsRay(Ray3D const &ray, double tolerance) const;

        /// Count items whose bounds intersect the ray with zero tolerance
        size_t CountIntersectsRay(Ray3D const &ray) const;

        /// True if any bounded item intersects the ray
        bool AnyIntersectsRay(Ray3D const &ray, double tolerance) const;

        /// True if any bounded item intersects the ray
        bool AnyIntersectsRay(Ray3D const &ray) const;

        /// True if any bounded item intersects the box
        bool AnyIntersectsBox(Interval3D const& bound) const;

        /// Compute the center of mass of the leaf boxes
        /// (the point at which the volume weighted relative position of the centers of leaf boxes sum to zero).
        Point3D FindCenterOfMass() const;

        ///////////////////////////////////////////////////////////////////////
        //
        // Utility functors, for advanced users and the implementation
        //
        // Visitor interfaces (following the Visitor pattern) that enable
        // walking the R* tree in a conditional way using a Filter.
        //
        // A Filter is a struct functor that provides two operators, one for Node
        // and one for Leaf, returning true or false.
        //
        // A Visitor is a struct functor that provides either or both operators,
        // one for Node and Leaf, that performs some operation or query or
        // operation on the nodes.
        //
        ///////////////////////////////////////////////////////////////////////

        enum { DIMENSION = 3};

        struct Leaf;
        struct Node;

        /// A convenience Filter that matches any node and leaf of the tree.
        struct IsAny {
            bool operator()(Node const* node) const { return node!=nullptr; }

            bool operator()(Leaf const* leaf) const { return leaf!=nullptr; }
        };

        /// A Filter that matches node or leaf when when the given bounding box intersects.
        struct IsOverlapping {

            Interval3D const *m_pBound;

            IsOverlapping() = delete;

            explicit IsOverlapping(Interval3D const & bound)
                    : m_pBound(&bound) { }

            bool operator()(Node const * node) const;

            bool operator()(Leaf const * leaf) const;
        };

        /// A Filter that matches node or leaf if their bounding box completely covers the given bounding box.
        struct IsEnclosing {

            Interval3D const *m_pBound;

            IsEnclosing() = delete;

            explicit IsEnclosing(Interval3D const & bound)
                    :m_pBound(&bound) { }

            bool operator()(Node const * node) const;

            bool operator()(Leaf const * leaf) const;
        };

        /// A Filter that matches node or leaf when the given half-space intersects.
        struct IsIntersectingHalfSpace {
            Point3D m_point;
            UnitVector3D m_unitVector;
            double m_tolerance;
            IsIntersectingHalfSpace() = delete;

            IsIntersectingHalfSpace(Point3D const &point, UnitVector3D const &unitVector, double tolerance)
                    : m_point(point), m_unitVector(unitVector), m_tolerance(tolerance) { }

            bool operator()(Bounded const * node) const;
        };

        /// A Filter that matches node or leaf when the given line intersects.
        struct IsIntersectingLine {
            Ray3D const *m_pRay;
            double m_tolerance;

            IsIntersectingLine() = delete;

            explicit IsIntersectingLine(Ray3D const & ray, double tolerance)
            : m_pRay(&ray), m_tolerance(tolerance) { }

            bool operator()(Bounded const * node) const;
        };

        struct IsIntersectingLineTight {
            Ray3D const *m_pRay;

            IsIntersectingLineTight() = delete;

            explicit IsIntersectingLineTight(Ray3D const & ray)
                : m_pRay(&ray) { }

            bool operator()(Bounded const * node) const;
        };

        /// A Filter that matches node or leaf when the given plane intersects.
        struct IsIntersectingPlane {
            Point3D const *m_pPoint;
            UnitVector3D const *m_pUnitVector;
            double m_tolerance;
            
            IsIntersectingPlane() = delete;
            
            IsIntersectingPlane(Point3D const & point, UnitVector3D const & unitVector, double tolerance)
                    : m_pPoint(&point), m_pUnitVector(&unitVector), m_tolerance(tolerance) { }
            
            bool operator()(Bounded const * node) const;
        };

        /// A Filter that matches node or leaf when the given point intersects.
        struct IsIntersectingPoint {

            double m_tolerance;
            Point3D const *m_pPoint;

            IsIntersectingPoint() = delete;

            explicit IsIntersectingPoint(Point3D const & point, double tolerance)
                    : m_tolerance(tolerance), m_pPoint(&point) { }

            bool operator()(Bounded const * node) const;
        };

        struct IsIntersectingPointTight {

            Point3D const *m_pPoint;

            IsIntersectingPointTight() = delete;

            explicit IsIntersectingPointTight(Point3D const & point)
                : m_pPoint(&point) { }

            bool operator()(Bounded const * node) const;
            };

        /// A Filter that matches node or leaf when the given ray intersects.
        struct IsIntersectingRay {

            Ray3D const *m_pRay;
            double m_tolerance;
            
            IsIntersectingRay() = delete;

            explicit IsIntersectingRay(Ray3D const & ray, double tolerance)
                    : m_pRay(&ray), m_tolerance(tolerance) { }

            bool operator()(Bounded const * node) const;
        };

        /// A Filter that matches node or leaf when the given ray intersects.
        struct IsIntersectingRayTight {

            Ray3D const *m_pRay;

            IsIntersectingRayTight() = delete;

            explicit IsIntersectingRayTight(Ray3D const & ray)
                : m_pRay(&ray) { }

            bool operator()(Bounded const * node) const;
            };

        /// A Filter that matches node or leaf when the given segment intersects
        struct IsIntersectingSegment
        {
            Ray3D const *m_pRay; // implicitly constant, it will not be changed
            double m_length;
            double m_tolerance;
            
            IsIntersectingSegment() = delete;
            
            explicit IsIntersectingSegment(Ray3D const &ray, double length, double tolerance)
                : m_pRay(&ray), m_length(length), m_tolerance(tolerance)
                { }
            
            bool operator()(Bounded const * node) const;
        };

        /// A Filter that matches node or leaf when the given segment intersects
        struct IsIntersectingSegmentTight
            {
            Ray3D const *m_pRay; // implicitly constant, it will not be changed
            double m_length;

            IsIntersectingSegmentTight() = delete;

            explicit IsIntersectingSegmentTight(Ray3D const &ray, double length)
                : m_pRay(&ray), m_length(length)
                { }

            bool operator()(Bounded const * node) const;
            };

        /// A Filter that matches node or leaf when the given sphere intersects
        struct IsIntersectingSphere
        {
            Point3D const *m_pCenter;
            double m_radius;
            double m_tolerance;

            IsIntersectingSphere() = delete;

            explicit IsIntersectingSphere(Point3D const &center, double radius, double tolerance)
                    : m_pCenter(&center), m_radius(radius), m_tolerance(tolerance) { }

            bool operator()(Bounded const * node) const;
        };

        /// Visitor operation for removing objects (leaf nodes) from the tree.
        struct RemoveLeaf {

            bool m_bContinue; // if false, visitor stops as soon as possible

            RemoveLeaf()
                    :m_bContinue(true) { }

            bool operator()(Leaf const * /*leaf*/) const { return true; }
        };

        /**
         * Visitor operation for removing a specific object (leaf node) in the tree.
         *
         * When remove duplicates is true, it searches for all possible instances of the given object.
         */
        struct RemoveSpecificLeaf {

            mutable bool m_bContinue;
            bool m_bRemoveDuplicates;
            void const* m_pLeafObject;

            RemoveSpecificLeaf() = delete;

            explicit RemoveSpecificLeaf(void const* object, bool remove_duplicates = false)
                    :m_bContinue(true), m_bRemoveDuplicates(remove_duplicates), m_pLeafObject(object) { }

            bool operator()(Leaf const* leaf) const;
        };

        typedef std::map<const void *, const void *> ItemMapType;

        /// Visitor operation for replacing the item on leaves using a map from old item to new item.
        struct ReplaceLeafItem {

            ItemMapType const *m_pItemMap; // implicitly const, it will not be changed
            bool bContinueVisiting;

            ReplaceLeafItem() = delete;

            explicit ReplaceLeafItem(ItemMapType const &itemMap)
                : m_pItemMap(&itemMap), bContinueVisiting(true) {}

            ReplaceLeafItem(const ReplaceLeafItem&) = default;
            ReplaceLeafItem& operator=(const ReplaceLeafItem &) = delete;

            void operator()(Leaf * leaf); // this will modify the leaf if the leaf item is found in the map
        };

        /**
         * Visitor operation that pushes a Leaf into its own container when it passes a given Filter.
         */
        struct PushLeaf {
            std::vector<void const*> m_aContainer;
            bool bContinueVisiting;

            PushLeaf() : m_aContainer(), bContinueVisiting(true) {};

            explicit PushLeaf(size_t nReserve);

            void operator()(Leaf const *leaf);
        };

        /**
         * Visitor operation that pushes a Leaf object into a given container when it passes a given Filter.
         * It casts the void* into the pointer type of the given container.
         */
        template <class ObjectType>
        struct PushLeafObject {
            std::vector<ObjectType*> *m_p_aObjects;
            bool bContinueVisiting;
            
            PushLeafObject() = delete;
            
            explicit PushLeafObject(std::vector<ObjectType*> &aObjects) 
                    : m_p_aObjects(&aObjects), bContinueVisiting(true)
                { m_p_aObjects->clear(); };
            
            void operator()(Leaf const *leaf);
        };

        /**
         * Visitor operation that counts a Leaf when it passes a given Filter.
         */
        struct LeafCounter {
            size_t m_nCount;
            bool bContinueVisiting;

            LeafCounter() : m_nCount(0), bContinueVisiting(true) {};

            void operator()(Leaf const *)
                {
                ++m_nCount;
                }
            };

        /**
         * Visitor operation that holds the first leaf item that passes a given Filter, or none.
         */
        struct FirstLeaf {
            void const* m_pObject;
            bool bContinueVisiting;

            FirstLeaf() : m_pObject(nullptr), bContinueVisiting(true) {};

            void operator()(Leaf const *leaf)
                {
                m_pObject = leaf->m_pObject;
                bContinueVisiting = false;
                }
            };

        /**
         * Visitor operation that sums volumes Leaf into a container when it passes a given Filter.
         */
        struct LeafSumMassCentroid {
            Point3D m_SumVolumeWeightedPosition;
            double m_SumVolume;
            bool bContinueVisiting;

            LeafSumMassCentroid() : m_SumVolumeWeightedPosition(0.0,0.0,0.0), m_SumVolume(0.0), bContinueVisiting(true) {};

            void operator()(Leaf const *leaf);
            };


        /**
         * Traverse the tree; for each Node that passes the Filter and has Leaf children, perform the given
         * Operation on each Leaf; also break if Visitor.ContinueVisiting is false.
         *
         * @tparam Filter An functor that returns true if this branch Node of the tree should be visited.
         * @tparam Operation An functor that performs an operation on a leaf
         * @param filter the filter instance object
         * @param operation the operator instance object
         * @return the operation instance object at the end of traversal, allowing retrieval of data inside it
         *         (for example, count of items visited)
         */
        template<typename Filter, typename Operation>
        Operation Query(Filter const& filter, Operation operation) const;

        /**
         * Traverse the tree; for each Node that passes the Filter and has Leaf children, perform the given
         * Operation on each Leaf the Leaf may be modified; also break if Visitor.ContinueVisiting is false.
         *
         * @tparam Filter An functor that returns true if this branch Node of the tree should be visited.
         * @tparam Operation An functor that performs an operation that may modify a leaf
         * @param filter the filter instance object
         * @param operation the operator instance object
         * @return the operation instance object at the end of traversal, allowing retrieval of data inside it
         *         (for example, count of items visited)
         */
        template<typename Filter, typename Operation>
        Operation Modify(Filter const& filter, Operation operation);
        
        /**
         * Traverse the tree and remove leaf nodes on branches that pass a filter.
         *
         * @tparam Filter    A functor returning true if a branch or leaf of the tree should be considered.
         * @tparam LeafRemover A functor returning true if a branch or leaf of the tree should actually be removed.
         * @param accept       the Filter functor instance
         * @param leafRemover  the Visitor functor instance
         *
         * @see RemoveBoundedArea, RemoveItem
         */
        template<typename Filter, typename LeafRemover>
        void Remove(Filter const & accept, LeafRemover leafRemover);

        //
        // The Leaf and Node types
        //

        typedef std::vector<Bounded*> NodeChildrenContainerType;

        /// Leaf node class with no children holding void* to objects
        struct Leaf : Bounded {

            void const* m_pObject{};

#if defined(BOX_TREE_USE_MEMORY_POOL)
            static std::unique_ptr<MemoryPool<Leaf>> m_MemoryPool;  //TODO: remove #define
            void* operator new(size_t /*size*/) { return m_MemoryPool->Alloc(); }
            void operator delete(void* p) { m_MemoryPool->Free(p); }
#endif
        };

        /// Node class with child nodes and a minimal bounding box enclosing the children.
        struct Node : Bounded {

            NodeChildrenContainerType m_aItems;

            bool m_bHasLeaves{};

#if defined(BOX_TREE_USE_MEMORY_POOL)
            static std::unique_ptr<MemoryPool<Node>> m_MemoryPool;
            void* operator new(size_t /*size*/) { return m_MemoryPool->Alloc(); }
            void operator delete(void* p) { m_MemoryPool->Free(p); }
#endif
        };

    private:

        Node* CreateDeepCopy(Node const &other);

        Node* ChooseSubtree(Node* node, Interval3D const* bound);

        Node* InsertInternal(Leaf* leaf, Node* node, bool firstInsert = true);

        Node* OverflowTreatment(Node* level, bool firstInsert);

        Node* Split(Node* node);

        void Reinsert(Node* node);

        template<typename Filter, typename Visitor>
        struct QueryLeafFunctor
        {
            Filter m_filter;
            Visitor &m_visitor;

            explicit QueryLeafFunctor(Filter const &filter, Visitor &visitor)
                    : m_filter(filter), m_visitor(visitor)
            {}

            QueryLeafFunctor(const QueryLeafFunctor&) = default;
            QueryLeafFunctor& operator=(const QueryLeafFunctor &) = delete;

            void operator()(Bounded const *item);
        };

        template<typename Filter, typename Visitor>
        struct QueryNodeFunctor {
            Filter m_filter;
            Visitor& m_visitor;

            explicit QueryNodeFunctor(Filter const & filter, Visitor& visitor)
                    :m_filter(filter), m_visitor(visitor) { }

            // non-copyable
            QueryNodeFunctor(const QueryNodeFunctor&) = default;
            QueryNodeFunctor& operator=(const QueryNodeFunctor &) = delete;

            void operator()(Bounded const* item);
        };

        template<typename Filter, typename Visitor>
        struct ModifyLeafFunctor
        {
            Filter m_filter;
            Visitor &m_visitor;

            explicit ModifyLeafFunctor(Filter const &filter, Visitor &visitor)
                    : m_filter(filter), m_visitor(visitor)
            {}

            ModifyLeafFunctor(const ModifyLeafFunctor&) = default;
            ModifyLeafFunctor& operator=(const ModifyLeafFunctor &) = delete;

            void operator()(Bounded *item);
        };

        template<typename Filter, typename Visitor>
        struct ModifyNodeFunctor {
            Filter m_filter;
            Visitor& m_visitor;

            explicit ModifyNodeFunctor(Filter const & filter, Visitor& visitor)
                    :m_filter(filter), m_visitor(visitor) { }

            // non-copyable
            ModifyNodeFunctor(const ModifyNodeFunctor&) = default;
            ModifyNodeFunctor& operator=(const ModifyNodeFunctor &) = delete;

            void operator()(Bounded * item);
        };
        
        template<typename Filter, typename LeafRemover>
        struct RemoveLeafFunctor {
            Filter m_filter;
            LeafRemover& m_leafRemover;
            size_t* size;

            explicit RemoveLeafFunctor(Filter const & a, LeafRemover& r, size_t* s)
                    :m_filter(a), m_leafRemover(r), size(s) { }

            RemoveLeafFunctor(const RemoveLeafFunctor&) = default;
            RemoveLeafFunctor& operator=(const RemoveLeafFunctor &) = delete;

            bool operator()(Bounded* item) const;
        };

        typedef std::list<Leaf*> ReinsertLeafContainerType;

        template<typename Filter, typename LeafRemover>
        struct RemoveFunctor {
            Filter accept;
            LeafRemover& remove;

            // parameters that are passed in
            ReinsertLeafContainerType* itemsToReinsert;
            size_t* m_size;

            // the third parameter is a list holding the items that need to be reinserted
            explicit RemoveFunctor(Filter const & na, LeafRemover& lr, ReinsertLeafContainerType* ir, size_t* size)
                    :accept(na), remove(lr), itemsToReinsert(ir), m_size(size) { }

            RemoveFunctor(const RemoveFunctor& rf) = default;
            RemoveFunctor& operator=(const RemoveFunctor &) = delete;

            bool operator()(Bounded* item, bool isRoot = false);

            // traverse and finds any leaves, and adds them to a
            // list of items that will later be reinserted
            void QueueItemsToReinsert(Node* node);
        };

    private:
        Node* m_treeRoot;

        size_t m_treeSize;

        double m_dTolerance;

        static const size_t REINSERT_CHILDREN;      // in [1 <= m <= MIN_CHILDREN]
        static const size_t MIN_CHILDREN;           // in [1 <= m < M)
        static const size_t MAX_CHILDREN;           // in [2*MIN_CHILDREN <= m < M)
        static const size_t RESERVE_CHILDREN;       // in [MIN_CHILDREN, MAX_CHILDREN]
        static const size_t CHOOSE_SUBTREE;
        static const size_t MEMORY_POOL_BYTES;      // size of chunks in memory pool allocator
        static const bool MAX_GT_CHOOSE_SUBTREE;    // condition for entering subtree
    };

}

#include "Inline/SGMBoxTree.inl"

#endif //SGM_BOXTREE_H
