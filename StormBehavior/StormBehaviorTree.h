#pragma once

#include <memory>
#include <cassert>

#include "StormBehaviorTreeTemplate.h"

#define ONE_UPDATE_PER_CALL

enum class StormBehaviorTreeElementType
{
  kConditional,
  kService,
  kState,
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
        void * mem = m_TreeMemory.get() + elem.m_TargetOffset;
        void * init = bt->m_InitDataMemory.get() + elem.m_InitOffset;
        elem.m_Allocate(mem, init);
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

    if (m_CurrentNode == -1)
    {
      AdvanceToNextNode(data, context, random, true);
    }
    else if(m_AdvanceNode)
    {
      AdvanceToNextNode(data, context, random, false);
    }
    else if(CheckNodeConditionals(m_CurrentNode, data, context) == false)
    {
      AdvanceToNextNode(data, context, random, true);
    }
    
    if(m_CurrentNode != -1)
    {
      m_AdvanceNode = UpdateNode(data, context);
    }
  }

  template <typename Visitor>
  void VisitNodes(Visitor && visitor)
  {
    if(m_CurrentNode == -1)
    {
      for(auto & elem : m_BehaviorTree->m_States)
      {
        auto * mem = m_TreeMemory.get() + elem.m_Offset;
        visitor(StormBehaviorTreeElementType::kState, elem.m_TypeId, mem, false);
      }

      for(auto & elem : m_BehaviorTree->m_Conditionals)
      {
        auto * mem = m_TreeMemory.get() + elem.m_Offcset;
        visitor(StormBehaviorTreeElementType::kConditional, elem.m_TypeId, mem, false);
      }

      for(auto & elem : m_BehaviorTree->m_Services)
      {
        auto * mem = m_TreeMemory.get() + elem.m_Offset;
        visitor(StormBehaviorTreeElementType::kService, elem.m_TypeId, mem, false);
      }
    }
    else
    {
      auto & node_info = m_BehaviorTree->m_Nodes[m_CurrentNode];
      auto & leaf_info = m_BehaviorTree->m_Leaves[node_info.m_LeafIndex];

      auto current_node = &m_BehaviorTree->m_States[node_info.m_LeafIndex];

      for(auto & elem : m_BehaviorTree->m_States)
      {
        auto * mem = m_TreeMemory.get() + elem.m_Offset;
        visitor(StormBehaviorTreeElementType::kState, elem.m_TypeId, mem, current_node == &elem);
      }

      for(int index = 0; index < static_cast<int>(m_BehaviorTree->m_Conditionals.size()); ++index)
      {
        bool active = false;
        for(auto lookup_index = leaf_info.m_ConditionalStart; lookup_index < leaf_info.m_ConditionalEnd; ++lookup_index)
        {
          auto conditional_index = m_BehaviorTree->m_ConditionalLookup[lookup_index];
          if(conditional_index == index)
          {
            active = true;
          }
        }

        auto & elem = m_BehaviorTree->m_Conditionals[index];

        auto * mem = m_TreeMemory.get() + elem.m_Offcset;
        visitor(StormBehaviorTreeElementType::kConditional, elem.m_TypeId, mem, active);
      }

      for(int index = 0; index < static_cast<int>(m_BehaviorTree->m_Services.size()); ++index)
      {
        bool active = false;
        for(auto lookup_index = leaf_info.m_ServiceStart; lookup_index < leaf_info.m_ServiceEnd; ++lookup_index)
        {
          auto service_index = m_BehaviorTree->m_ServiceLookup[lookup_index];
          if(service_index == index)
          {
            active = true;
          }
        }
        
        auto & elem = m_BehaviorTree->m_Services[index];

        auto * mem = m_TreeMemory.get() + elem.m_Offset;
        visitor(StormBehaviorTreeElementType::kService, elem.m_TypeId, mem, active);
      }      
    }
  }

  int GetCurrentNode() const
  {
    return m_CurrentNode;
  }

  int GetNodeCount() const
  {
    return static_cast<int>(m_BehaviorTree->m_Nodes.size());
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
      void * mem = m_TreeMemory.get() + elem.m_TargetOffset;
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
      
      auto s = (int)random() % total_weight;
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
    if(node_index != -1)
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
      if(service_info.m_Deactivate)
      {
        void * service_mem = m_TreeMemory.get() + service_info.m_Offset;
        service_info.m_Deactivate(service_mem, data, context);
      }
    }

    for(auto & elem : new_service_indices)
    {
      auto & service_info = m_BehaviorTree->m_Services[elem];
      if(service_info.m_Activate)
      {
        void * service_mem = m_TreeMemory.get() + service_info.m_Offset;
        service_info.m_Activate(service_mem, data, context);
      }
    }

    m_CurrentNode = node_index;
  }

  bool CheckNodeConditionals(int node_index, DataType & data, ContextType & context)
  {
    auto & node_info = m_BehaviorTree->m_Nodes[node_index];
    auto & leaf_info = m_BehaviorTree->m_Leaves[node_info.m_LeafIndex];

    for(int index = leaf_info.m_ContinuousConditionalStart; index < leaf_info.m_ContinuousConditionalEnd; ++index)
    {
      auto conditional_index = m_BehaviorTree->m_ConditionalLookup[index];
      auto & conditional_info = m_BehaviorTree->m_Conditionals[conditional_index];
      auto conditional_mem = m_TreeMemory.get() + conditional_info.m_Offset;

      if(conditional_info.m_Check(conditional_mem, data, context) == false)
      {
        return false;
      }
    }

    for(int index = leaf_info.m_PreemptConditionalStart; index < leaf_info.m_PreemptConditionalEnd; ++index)
    {
      auto conditional_index = m_BehaviorTree->m_ConditionalLookup[index];
      auto & conditional_info = m_BehaviorTree->m_Conditionals[conditional_index];
      auto conditional_mem = m_TreeMemory.get() + conditional_info.m_Offset;

      if(conditional_info.m_Check(conditional_mem, data, context) == true)
      {
        return false;
      }
    }    

    return true;
  }

  bool UpdateNode(DataType & data, ContextType & context)
  {
    auto & node_info = m_BehaviorTree->m_Nodes[m_CurrentNode];
    auto & leaf_info = m_BehaviorTree->m_Leaves[node_info.m_LeafIndex];

    for (int index = leaf_info.m_ServiceStart; index < leaf_info.m_ServiceEnd; ++index)
    {
      auto service_index = m_BehaviorTree->m_ServiceLookup[index];
      auto & service_info = m_BehaviorTree->m_Services[service_index];
      if(service_info.m_Update)
      {
        auto service_mem = m_TreeMemory.get() + service_info.m_Offset;
        service_info.m_Update(service_mem, data, context);
      }
    }

    auto & state_info = m_BehaviorTree->m_States[node_info.m_LeafIndex];
    auto state_mem = m_TreeMemory.get() + state_info.m_Offset;

    bool result = state_info.m_Update(state_mem, data, context);
    if (result)
    {
      return true;
    }

    return false;
  }

  template <typename RandomSource>
  int TraverseNode(int node_index, DataType & data, ContextType & context, RandomSource & random)
  {
    assert(node_index != -1);

    auto & node = m_BehaviorTree->m_Nodes[node_index];
    for(int conditional_index = node.m_ConditionalStart; conditional_index < node.m_ConditionalEnd; ++conditional_index)
    {
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

  template <typename RandomSource>
  void AdvanceToNextNode(DataType & data, ContextType & context, RandomSource & random, bool restart)
  {
    bool restarted = restart;
    int new_node = restart ? -1 : m_CurrentNode;

    while(true)
    {
      if(new_node == -1)
      {
        new_node = TraverseNode(0, data, context, random);

        ActivateNode(new_node, m_CurrentNode, data, context);
        return;
      }
      else
      {
        auto & node_info = m_BehaviorTree->m_Nodes[new_node];
        auto & leaf_info = m_BehaviorTree->m_Leaves[node_info.m_LeafIndex];
        
        new_node = -1;
        if(leaf_info.m_NextInSequence != -1)
        {
          new_node = TraverseNode(leaf_info.m_NextInSequence, data, context, random);
        }

        if (new_node == -1)
        {
          if(restarted)
          {
            ActivateNode(new_node, m_CurrentNode, data, context);
            return;
          }  

          restarted = true;
          continue;
        }

        ActivateNode(new_node, m_CurrentNode, data, context);
        return;
      }
    }
  }

private:

  StormBehaviorTreeTemplate<DataType, ContextType> * m_BehaviorTree = nullptr;
  std::unique_ptr<uint8_t[]> m_TreeMemory;

  int m_CurrentNode = -1;
  bool m_AdvanceNode = false;
};
