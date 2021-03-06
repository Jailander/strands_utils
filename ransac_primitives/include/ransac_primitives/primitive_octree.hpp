#include "primitive_octree.h"

template <typename Point>
primitive_octree<Point>::primitive_octree(double resolution) :
    pcl::octree::OctreePointCloudSearch<point_type, primitive_leaf>(resolution)
{

}

template <typename Point>
void primitive_octree<Point>::setInputCloud(const PointCloudConstPtr &cloud_arg, const IndicesConstPtr& indices_arg)
{
    //std::sort(indices_arg->begin(), indices_arg->end());
    super::setInputCloud(cloud_arg, indices_arg);
    if (!indices_arg) {
        points_left = super::input_->size();
    }
    else {
        points_left = super::indices_->size();
    }
}

template <typename Point>
primitive_octree<Point>::~primitive_octree()
{

}

template <typename Point>
int primitive_octree<Point>::size()
{
    return points_left;
}

template <typename Point>
void primitive_octree<Point>::remove_points(const std::vector<int>& inds)
{
    /*std::sort(inds.begin(), inds.end());
    std::vector<int> remove_inds;
    remove_inds.reserve(inds.size());
    std::vector<int> new_indices;
    new_indices.reserve(indices_.size());

    // check if the primitives share any inliers
    int counter1 = 0;
    int counter2 = 0;
    while (counter1 < inds.size() && counter2 < indices_.size()) {
        if (inds[counter1] == indices_[counter2]) {
            remove_inds.push_back(inds[counter1]);
        }
        if (supporting_inds[counter1] < other_inds[counter2]) {
            ++counter1;
        }
        else {
            ++counter2;
        }
    }*/

    for (const int& ind : inds) {
        if (remove_point(ind)) {
            --points_left; // point successfully removed
        }
    }
}

template <typename Point>
bool primitive_octree<Point>::remove_point(int ind)
{
    pcl::octree::OctreeKey key;
    point_type point = super::input_->points[ind];

    // generate key for point
    super::genOctreeKeyforPoint(point, key);

    pcl::octree::OctreeNode* result = NULL;
    find_node_recursive(key, super::depth_mask_, super::root_node_, result, 0);
    if (result == NULL) {
        return false;
    }

    primitive_leaf_node* leafNode = dynamic_cast<primitive_leaf_node*> (result);
    if (leafNode != NULL) {
        if (leafNode->getContainer().remove_if_equal(ind)) {
            return true;
        }
        //else {
            //std::cout << "Couldn't remove point!" << std::endl;
            //exit(0); // This should happen now since we don't know if a point is in an octree
        //}
    }
    //else {
        //std::cout << "Leaf is null!" << std::endl;
        //exit(0);
    //}
    return false;
}

template <typename Point>
void primitive_octree<Point>::find_points_at_depth(std::vector<DataT>& inds, const point_type& point, size_t depth)
{
    if (depth == super::getTreeDepth()) {
        serialize_node_recursive(this->root_node_, &inds);
        return;
    }

    pcl::octree::OctreeKey key;

    // generate key for point
    super::genOctreeKeyforPoint(point, key);

    pcl::octree::OctreeNode* result = 0;
    find_node_recursive(key, super::depth_mask_, super::root_node_, result, depth);

    if (result == NULL) {
        std::cout << "Is null!" << std::endl;
    }

    if (result->getNodeType() == pcl::octree::BRANCH_NODE) {
        serialize_node_recursive(static_cast<const BranchNode*>(result), &inds);
    }
    else if (result->getNodeType() == pcl::octree::LEAF_NODE) {
        const primitive_leaf_node* childLeaf = static_cast<const primitive_leaf_node*>(result);
        //childLeaf->getData(inds);
        childLeaf->getContainer().getPointIndices(inds);
    }
    else {
        std::cout << "Couldn't find node at required level." << std::endl;
        exit(0);
    }

}

template <typename Point>
void primitive_octree<Point>::find_potential_inliers(std::vector<DataT>& inds, base_primitive* primitive, double margin)
{
    pcl::octree::OctreeKey newKey;
    unsigned int treeDepth_arg = 0;
    serialize_inliers(super::root_node_, newKey, treeDepth_arg, &inds, primitive, margin);
}

template <typename Point>
void primitive_octree<Point>::serialize_inliers(const BranchNode* branch_arg, pcl::octree::OctreeKey& key_arg,
                                                unsigned int treeDepth_arg, std::vector<DataT>* dataVector_arg,
                                                base_primitive* primitive, double margin) const
{
    // child iterator
    unsigned char childIdx;
    Eigen::Vector3f min_pt;
    Eigen::Vector3f max_pt;
    Eigen::Vector3d mid;
    double l;
    double dist;
    ++treeDepth_arg;

    // iterate over all children
    for (childIdx = 0; childIdx < 8; childIdx++)
    {


        // if child exist
        if (branch_arg->hasChild(childIdx))
        {
            // add current branch voxel to key
            key_arg.pushBranch(childIdx);

            super::genVoxelBoundsFromOctreeKey(key_arg, treeDepth_arg, min_pt, max_pt);
            l = double(max_pt(0) - min_pt(0));
            mid = 0.5*(min_pt + max_pt).cast<double>();
            //std::cout << "Diff: " << (max_pt - min_pt).transpose() << std::endl;
            dist = primitive->distance_to_pt(mid);
            if (dist > sqrt(3.0)/2.0*l + margin) {
                key_arg.popBranch();
                continue;
            }

            const pcl::octree::OctreeNode *childNode = branch_arg->getChildPtr(childIdx);

            switch (childNode->getNodeType ())
            {
            case pcl::octree::BRANCH_NODE:
            {
                // recursively proceed with indexed child branch
                serialize_inliers(static_cast<const BranchNode*>(childNode), key_arg,
                                  treeDepth_arg, dataVector_arg, primitive, margin);
                break;
            }
            case pcl::octree::LEAF_NODE:
            {
                const primitive_leaf_node* childLeaf = static_cast<const primitive_leaf_node*>(childNode);

                if (dataVector_arg) {
                    //childLeaf->getData (*dataVector_arg);
                    childLeaf->getContainer().getPointIndices(*dataVector_arg);
                }

                break;
            }
            default:
                break;
            }

            // pop current branch voxel from key
            key_arg.popBranch();
        }
    }
}

template <typename Point>
void primitive_octree<Point>::find_node_recursive(const pcl::octree::OctreeKey& key_arg,
                                                  unsigned int depthMask_arg,
                                                  BranchNode* branch_arg,
                                                  pcl::octree::OctreeNode*& result_arg, size_t depth) const
{
    // index to branch child
    unsigned char childIdx;

    // find branch child from key
    childIdx = key_arg.getChildIdxWithDepthMask(depthMask_arg);

    pcl::octree::OctreeNode* childNode = (*branch_arg)[childIdx];
    if (childNode) {
        switch (childNode->getNodeType()) {
        case pcl::octree::BRANCH_NODE:
            // we have not reached maximum tree depth
            BranchNode* childBranch;
            childBranch = static_cast<BranchNode*> (childNode);

            if (int(depthMask_arg) == 1 << depth) { // 2^depth
                result_arg = childNode;
            }
            else {
                find_node_recursive(key_arg, depthMask_arg / 2, childBranch, result_arg, depth);
            }
            break;

        case pcl::octree::LEAF_NODE:
            // return existing leaf node
            result_arg = childNode;
            break;
        }
    }

}

template <typename Point>
void primitive_octree<Point>::serialize_node_recursive(const BranchNode* branch_arg, std::vector<DataT>* dataVector_arg) const
{

    // child iterator
    unsigned char childIdx;

    // iterate over all children
    for (childIdx = 0; childIdx < 8; childIdx++)
    {

        // if child exist
        if (branch_arg->hasChild(childIdx))
        {
            // add current branch voxel to key
            //key_arg.pushBranch(childIdx);

            const pcl::octree::OctreeNode *childNode = branch_arg->getChildPtr(childIdx);

            switch (childNode->getNodeType ())
            {
            case pcl::octree::BRANCH_NODE:
            {
                // recursively proceed with indexed child branch
                serialize_node_recursive(static_cast<const BranchNode*>(childNode), dataVector_arg);
                break;
            }
            case pcl::octree::LEAF_NODE:
            {
                const primitive_leaf_node* childLeaf = static_cast<const primitive_leaf_node*>(childNode);

                if (dataVector_arg) {
                    //childLeaf->getData (*dataVector_arg);
                    childLeaf->getContainer().getPointIndices(*dataVector_arg);
                }

                break;
            }
            default:
                break;
            }

        }
    }
}
