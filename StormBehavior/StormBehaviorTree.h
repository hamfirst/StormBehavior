#pragma once

#include <memory>

#include "StormBehaviorTreeTemplate.h"



enum class StormBehaviorUpdateNodeResult
{
  kContinue,
  kFinished,
  kAbort,
};

template <typename DataType, typename ContextType>
class StormBehaviorTree
{
public:
  StormBehaviorTree() = default;
  StormBehaviorTree(StormBehaviorTreeTemplate<DataType, ContextType> & bt)
  {
    SetBehaviorTree(&bt);
  }

  ~StormBehaviorTree()
  {
    Destroy();
  }

  void SetBehaviorTree(StormBehaviorTreeTemplate<DataType, ContextType> * bt)
  {
    Destroy();
    m_BehaviorTree = bt;

    if(m_BehaviorTree)
    {
      m_TreeMemory = std::make_unique<uint8_t[]>(m_BehaviorTree->m_TotalSize);
      for(auto & elem : m_BehaviorTree->m_InitInfo)
      {
        void * mem = m_TreeMemory.get() + elem.m_Offset;
        elem.m_Allocate(mem);
      }
    }
  }

  template <typename RandomSource>
  void Update(DataType & data, ContextType & context, RandomSource & random)
  {
    if(m_BehaviorTree == nullptr || m_BehaviorTree->m_Nodes.size() == 0)
    {
      return;
    }


    if(m_CurrentNode == -1)
    {
      m_CurrentNode = TraverseNode(0, data, context, random);
      ActivateNode(m_CurrentNode, -1, data, context);
    }
    else
    {
      UpdateNode();
    }
  }

private:

  void Destroy()
  {
    if(m_BehaviorTree == nullptr)
    {
      return;
    }

    for(auto & elem : m_BehaviorTree->m_InitInfo)
    {
      void * mem = m_TreeMemory.get() + elem.m_Offset;
      elem.m_Deallocate(mem);
    }
  }

  template <typename RandomSource>
  void RandomShuffle(std::vector<std::pair<int, int>> & vals, RandomSource & random)
  {
    int total_weight = 0;
    for(auto & elem : vals)
    {
      total_weight += elem.second;
    }

    for(int index = 0; index < static_cast<int>(vals.size()) - 1; ++index)
    {
      if(total_weight == 0)
      {
        return;
      }
      
      auto s = random() % total_weight;
      for(int sort = index; sort < static_cast<int>(vals.size()); ++sort)
      {
        if(s < vals[sort].second)
        {
          total_weight -= vals[sort].second;
          std::swap(vals[index], vals[sort]);
          break;
        }
        else
        {
          s -= vals[sort].second;
        }
      }
    }
  }

  void ActivateNode(int node_index, int prev_node_index, DataType & data, ContextType & context)
  {
    if(node_index == prev_node_index)
    {
      return;
    }

    std::vector<int> new_service_indices;
    std::vector<int> old_service_indices;
    if(node_index == -1)
    {
      auto & node_info = m_BehaviorTree->m_Nodes[node_index];
      auto leaf_index = node_info.m_LeafIndex;

      auto & leaf_info = m_BehaviorTree->m_Leaves[leaf_index];
      for(int index = leaf_info.m_ServiceStart; index < leaf_info.m_ServiceEnd; ++index)
      {
        new_service_indices.push_back(m_BehaviorTree->m_ServiceLookup[index]);
      }
    }

    if(prev_node_index != -1)
    {
      auto & node_info = m_BehaviorTree->m_Nodes[prev_node_index];
      auto leaf_index = node_info.m_LeafIndex;

      auto & leaf_info = m_BehaviorTree->m_Leaves[leaf_index];
      for(int index = leaf_info.m_ServiceStart; index < leaf_info.m_ServiceEnd; ++index)
      {
        old_service_indices.push_back(m_BehaviorTree->m_ServiceLookup[index]);
      }
    }

    for(std::size_t a = 0; a < new_service_indices.size(); ++a)
    {
      for(std::size_t b = 0; b < old_service_indices.size(); ++b)
      {
        if(new_service_indices[a] == old_service_indices[b])
        {
          std::swap(new_service_indices[a], new_service_indices[new_service_indices.size() - 1]);
          new_service_indices.erase(new_service_indices.end() - 1);
          std::swap(old_service_indices[b], old_service_indices[old_service_indices.size() - 1]);
          old_service_indices.erase(old_service_indices.end() - 1);
        }
      }
    }

    for(auto & elem : old_service_indices)
    {
      auto & service_info = m_BehaviorTree->m_Services[elem];
      void * service_mem = m_TreeMemory.get() + service_info.m_Offset;
      service_info.m_Deactivate(service_mem, data, context);
    }

    for(auto & elem : new_service_indices)
    {
      auto & service_info = m_BehaviorTree->m_Services[elem];
      void * service_mem = m_TreeMemory.get() + service_info.m_Offset;
      service_info.m_Activate(service_mem, data, context);
    }
  }

  StormBehaviorUpdateNodeResult UpdateNode(DataType & data, ContextType & context)
  {
    auto & node_info = m_BehaviorTree->m_Nodes[m_CurrentNode];
    auto & leaf_info = m_BehaviorTree->m_Leaves[m_CurrentNode];

    for(int index = node_info.m_ConditionalStart; index < node_info.m_ConditionalEnd; ++index)
    {
      auto conditional_index = m_BehaviorTree->m_ConditionalLookup[index];
      auto & conditional_info = m_BehaviorTree->m_Conditionals[conditional_index];
      void * conditional_mem = m_TreeMemory.get() + conditional_info.m_Offset;

      if(conditional_info.m_Check(conditional_mem, data, context) == false)
      {
        return StormBehaviorUpdateNodeResult::kAbort;
      }
    }
  }

  template <typename RandomSource>
  int TraverseNode(int node_index, DataType & data, ContextType & context, RandomSource & random)
  {
    auto & node = m_BehaviorTree->m_Nodes[node_index];
    for(int index = node.m_ConditionalStart; index < node.m_ConditionalEnd; ++index)
    {
      auto conditional_index = m_BehaviorTree->m_ConditionalLookup[index];
      auto & conditional_info = m_BehaviorTree->m_Conditionals[conditional_index];

      void * conditional_mem = m_TreeMemory.get() + conditional_info.m_Offset;
      if(conditional_info.m_Check(conditional_mem, data, context) == false)
      {
        return -1;
      }
    }

    switch(node.m_Type)
    {
      case StormBehaviorNodeType::kSelect:
      {
        for(int index = node.m_ChildStart; index < node.m_ChildEnd; ++index)
        {
          auto child_index = m_BehaviorTree->m_ChildNodeLookup[index];
          auto result_node = TraverseNode(child_index, data, context, random);

          if(result_node != -1)
          {
            return result_node;
          }
        }
      }
      break;
      case StormBehaviorNodeType::kRandom:
      {
        std::vector<std::pair<int, int>> potential_nodes;
        for(int index = node.m_ChildStart, random_index = 0; index < node.m_ChildEnd; ++index, ++random_index)
        {
          auto child_index = m_BehaviorTree->m_ChildNodeLookup[index];
          auto r = m_BehaviorTree->m_RandomValues[node.m_RandomStart + random_index];
          potential_nodes.push_back(std::make_pair(child_index, r));
        }

        RandomShuffle(potential_nodes, random);

        for(auto & elem : potential_nodes)
        {
          auto result_node = TraverseNode(elem.first, data, context, random);
          if(result_node != -1)
          {
            return result_node;
          }
        }
      }
      break;
      case StormBehaviorNodeType::kSequence:
      {
        if(node.m_ChildStart == node.m_ChildEnd)
        {
          return -1;
        }

        auto child_index = m_BehaviorTree->m_ChildNodeLookup[node.m_ChildStart];
        auto result_node = TraverseNode(child_index, data, context, random);

        if(result_node != -1)
        {
          return result_node;
        }
      }
      break;
      case StormBehaviorNodeType::kLeaf:
      {
        return node_index;
      }
      break;
    }

    return -1;
  }

private:

  StormBehaviorTreeTemplate<DataType, ContextType> * m_BehaviorTree = nullptr;
  std::unique_ptr<uint8_t[]> m_TreeMemory;

  int m_CurrentNode = -1;
};
