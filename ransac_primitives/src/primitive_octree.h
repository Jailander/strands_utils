#ifndef PRIMITIVE_OCTREE_H
#define PRIMITIVE_OCTREE_H

#include <pcl/point_cloud.h>
#include <pcl/octree/octree_impl.h>
#include "primitive_leaf.h"
#include "base_primitive.h"

//class base_primitive;

template <typename Point>
class primitive_octree : public pcl::octree::OctreePointCloudSearch<Point, primitive_leaf>
{
public:
    typedef Point point_type;
    typedef pcl::octree::OctreePointCloudSearch<point_type, primitive_leaf> super;
    typedef typename super::OctreeT::BranchNode BranchNode;
    typedef typename super::OctreeT::LeafNode primitive_leaf_node;
    typedef typename super::OctreeT::PointCloudConstPtr PointCloudConstPtr;
    typedef typename super::OctreeT::IndicesConstPtr IndicesConstPtr;
    //typedef pcl::PointXYZRGB PointT;
    typedef int DataT;
protected:
    int points_left;
    void find_node_recursive(const pcl::octree::OctreeKey& key_arg,
                             unsigned int depthMask_arg,
                             BranchNode* branch_arg,
                             pcl::octree::OctreeNode*& result_arg, size_t depth) const;
    void serialize_node_recursive(const BranchNode* branch_arg, std::vector<DataT>* dataVector_arg) const;
    void serialize_inliers(const BranchNode* branch_arg, pcl::octree::OctreeKey& key_arg,
                           unsigned int treeDepth_arg, std::vector<DataT>* dataVector_arg,
                           base_primitive* primitive, double margin) const;
    bool remove_point(int ind);
public:
    int size();
    void remove_points(const std::vector<int>& inds);
    void valid_inds(std::vector<int>& inds);
    void find_points_at_depth(std::vector<DataT>& inds, const point_type& point, size_t depth);
    void find_potential_inliers(std::vector<DataT>& inds, base_primitive* primitive, double margin);
    void setInputCloud(const PointCloudConstPtr& cloud_arg, const IndicesConstPtr& indices_arg = IndicesConstPtr());
    primitive_octree(double resolution);
    ~primitive_octree();
};

#include "primitive_octree.hpp"
#endif // PRIMITIVE_OCTREE_H
