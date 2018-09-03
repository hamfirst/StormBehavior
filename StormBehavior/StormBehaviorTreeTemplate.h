#pragma once

#include "StormBehaviorTreeTemplateBuilder.h"

struct StormBehaviorTreeTemplateNode
{
  StormBehaviorNodeType m_Type;
  int m_ConditionalStart;
  int m_ConditionalEnd;
  int m_ChildStart;
  int m_ChildEnd;

  union
  {
    int m_RandomStart;
    int m_LeafIndex;
  };
};

struct StormBehaviorTreeTemplateLeaf
{
  int m_ConditionalStart;
  int m_ConditionalEnd;
  int m_ServiceStart;
  int m_ServiceEnd;
  int m_NextInSequence;
};

template <typename DataType, typename ContextType>
class StormBehaviorTree;

template <typename DataType, typename ContextType>
class StormBehaviorTreeTemplate
{
public:
  StormBehaviorTreeTemplate(const StormBehaviorTreeTemplateBuilder<DataType, ContextType> & bt)
  {
    std::vector<int> next_in_sequence_nodes;
    std::vector<int> conditionals;
    std::vector<int> services;
    ProcessNode(bt, next_in_sequence_nodes, conditionals, services);
  }

  void DebugPrint()
  {
    if(m_Nodes.size() > 0)
    {
      DebugPrint(0, 0);
    }
  }

private:

  void AlignSize(int & size, int align)
  {
    // Implement this if alignment becomes a problem
  }

  template <typename Type>
  void PushMemInit(const Type & val)
  {    
    m_InitInfo.emplace_back(MemInitInfo{ val.m_Allocate, val.m_Deallocate, val.m_Offset });
  }

  int ProcessNode(const StormBehaviorTreeTemplateBuilder<DataType, ContextType> & bt,
    std::vector<int> & next_in_sequence_nodes, std::vector<int> & conditionals, std::vector<int> & services)
  {
    auto node_index = static_cast<int>(m_Nodes.size());
    m_Nodes.emplace_back();

    auto & node = m_Nodes.back();
    node.m_Type = bt.m_Type;

    auto current_conditional_count = conditionals.size();
    node.m_ConditionalStart = static_cast<int>(m_Conditionals.size());
    for(auto & elem : bt.m_Conditionals)
    {
      auto conditional_index = static_cast<int>(m_Conditionals.size());
      m_Conditionals.emplace_back(elem);
      AlignSize(m_TotalSize, m_Conditionals.back().m_Align);
      m_Conditionals.back().m_Offset = m_TotalSize;
      m_TotalSize += elem.m_Size;
      
      PushMemInit(m_Conditionals.back());

      conditionals.emplace_back(conditional_index);
    }
    node.m_ConditionalEnd = static_cast<int>(m_Conditionals.size());

    auto current_service_count = services.size();
    for(auto & elem : bt.m_Services)
    {
      auto service_index = static_cast<int>(m_Services.size());
      m_Services.emplace_back(elem);
      AlignSize(m_TotalSize, m_Services.back().m_Align);
      m_Services.back().m_Offset = m_TotalSize;
      m_TotalSize += elem.m_Size;
      
      PushMemInit(m_Services.back());

      services.emplace_back(service_index);
    }

    if(bt.m_Type == StormBehaviorNodeType::kLeaf)
    {
      auto leaf_index = static_cast<int>(m_Leaves.size());

      node.m_ChildStart = -1;
      node.m_ChildEnd = -1;
      node.m_LeafIndex = leaf_index;
      
      m_States.emplace_back(bt.m_State.value());
      AlignSize(m_TotalSize, m_States.back().m_Align);
      m_States.back().m_Offset = m_TotalSize;
      m_TotalSize += m_States.back().m_Size;

      PushMemInit(m_States.back());

      m_Leaves.emplace_back();
      auto & leaf = m_Leaves.back();
      leaf.m_NextInSequence = -1;
      next_in_sequence_nodes.push_back(leaf_index);

      leaf.m_ConditionalStart = static_cast<int>(m_ConditionalLookup.size());
      for(auto & conditional_index : conditionals)
      {
        m_ConditionalLookup.emplace_back(conditional_index);
      }
      leaf.m_ConditionalEnd = static_cast<int>(m_ConditionalLookup.size());

      leaf.m_ServiceStart = static_cast<int>(m_ServiceLookup.size());
      for(auto & service_index : services)
      {
        m_ServiceLookup.emplace_back(service_index);
      }
      leaf.m_ServiceEnd = static_cast<int>(m_ServiceLookup.size());
    }
    else if(bt.m_Type == StormBehaviorNodeType::kSequence)
    {
      node.m_LeafIndex = -1;
      node.m_ChildStart = static_cast<int>(m_ChildNodeLookup.size());
      m_ChildNodeLookup.resize(m_ChildNodeLookup.size() + bt.m_Subtrees.size());
      node.m_ChildEnd = static_cast<int>(m_ChildNodeLookup.size());

      auto child_index = node.m_ChildStart;
      std::vector<int> pending_next_in_sequence_nodes;

      for(auto & elem : bt.m_Subtrees)
      {
        std::vector<int> new_next_in_sequence_nodes;
        auto child_node_index = ProcessNode(*elem.m_SubTree, new_next_in_sequence_nodes, conditionals, services);
        m_ChildNodeLookup[child_index] = child_node_index;

        for (auto & leaf_index : pending_next_in_sequence_nodes)
        {
          m_Leaves[leaf_index].m_NextInSequence = child_node_index;
        }

        pending_next_in_sequence_nodes = new_next_in_sequence_nodes;
        child_index++;
      }
    }
    else
    {
      node.m_LeafIndex = -1;
      node.m_ChildStart = static_cast<int>(m_ChildNodeLookup.size());
      m_ChildNodeLookup.resize(m_ChildNodeLookup.size() + bt.m_Subtrees.size());
      node.m_ChildEnd = static_cast<int>(m_ChildNodeLookup.size());

      auto child_index = node.m_ChildStart;
      for(auto & elem : bt.m_Subtrees)
      {
        std::vector<int> new_next_in_sequence_nodes;
        m_ChildNodeLookup[child_index] = ProcessNode(*elem.m_SubTree, next_in_sequence_nodes, conditionals, services);

        child_index++;
      }

      if(bt.m_Type == StormBehaviorNodeType::kRandom)
      {
        node.m_RandomStart = static_cast<int>(m_RandomValues.size());

        for(auto & elem : bt.m_Subtrees)
        {
          m_RandomValues.push_back(elem.m_RandomWeight);
        }
      }
    }

    conditionals.resize(current_conditional_count);
    services.resize(current_service_count);

    return node_index;
  }

  void DebugPrintIndent(int indent) const
  {
    for(int index = 0; index < indent; ++index)
    {
      printf(" ");
    }
  }

  void DebugPrint(int indent, int node_index) const
  {
    DebugPrintIndent(indent);
    auto & node = m_Nodes[node_index];

    printf("- (%d) ", node_index);
    switch(node.m_Type)
    {
      case StormBehaviorNodeType::kSelect:
        printf("Select\n");
        break;
      case StormBehaviorNodeType::kSequence:
        printf("Sequence\n");
        break;
      case StormBehaviorNodeType::kRandom:
        printf("Random\n");
        break;
      case StormBehaviorNodeType::kLeaf:
        {
          auto & state_info = m_States[node.m_LeafIndex];
          auto & leaf_info = m_Leaves[node.m_LeafIndex];
          printf("Leaf (%d) (%s) -> (%d)\n", node.m_LeafIndex, state_info.m_DebugName, leaf_info.m_NextInSequence);
        }
        break;
    }

    for(int index = node.m_ConditionalStart; index < node.m_ConditionalEnd; ++index)
    {
      auto conditional_lookup = m_ConditionalLookup[index];
      auto & conditional = m_Conditionals[conditional_lookup];

      DebugPrintIndent(indent);
      printf("|   (%d) Conditional (%s)\n", conditional_lookup, conditional.m_DebugName);
    }

    if(node.m_Type == StormBehaviorNodeType::kLeaf)
    {
      auto & leaf_info = m_Leaves[node.m_LeafIndex];
      for(int index = leaf_info.m_ServiceStart; index < leaf_info.m_ServiceEnd; ++index)
      {
        auto service_lookup = m_ServiceLookup[index];
        auto & service = m_Services[service_lookup];

        DebugPrintIndent(indent);
        printf("|   (%d) Service (%s)\n", service_lookup, service.m_DebugName);
      }
    }

    for(int index = node.m_ChildStart; index < node.m_ChildEnd; ++index)
    {
      auto child_lookup = m_ChildNodeLookup[index];
      DebugPrint(indent + 2, child_lookup);
    }
  }

private:

  friend class StormBehaviorTree<DataType, ContextType>;

  std::vector<StormBehaviorTreeTemplateNode> m_Nodes;
  std::vector<StormBehaviorTreeTemplateLeaf> m_Leaves;
  std::vector<StormBehaviorTreeTemplateState<DataType, ContextType>> m_States;
  std::vector<StormBehaviorTreeTemplateService<DataType, ContextType>> m_Services;
  std::vector<StormBehaviorTreeTemplateConditional<DataType, ContextType>> m_Conditionals;
  std::vector<int> m_ChildNodeLookup;
  std::vector<int> m_ServiceLookup;
  std::vector<int> m_ConditionalLookup;
  std::vector<int> m_RandomValues;

  struct MemInitInfo
  {
    void (*m_Allocate)(void *);
    void (*m_Deallocate)(void *);
    int m_Offset;
  };

  std::vector<MemInitInfo> m_InitInfo;
  int m_TotalSize = 0;
};

