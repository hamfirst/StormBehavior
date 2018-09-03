#pragma once

#include <memory>
#include <vector>
#include <optional>
#include <cassert>
#include <cstdio>

  template <typename DataType, typename ContextType>
  class StormBehaviorTreeTemplate;

enum class StormBehaviorNodeType
{
  kSelect,
  kSequence,
  kRandom,
  kLeaf,
};

template <typename T>
struct StormBehaviorHasActivate
{
public:
  template <typename C>
  static char test(decltype(&C::Activate));

  template <typename C> static long test(...);

  static const bool value = sizeof(test<T>(0)) == sizeof(char);
};

template <typename T>
struct StormBehaviorHasDeactivate
{
public:
  template <typename C>
  static char test(decltype(&C::Deactivate));

  template <typename C> static long test(...);

  static const bool value = sizeof(test<T>(0)) == sizeof(char);
};

template <typename T>
struct StormBehaviorHasUpdate
{
public:
  template <typename C>
  static char test(decltype(&C::Update));

  template <typename C> static long test(...);

  static const bool value = sizeof(test<T>(0)) == sizeof(char);
};

template <typename DataType, typename ContextType>
struct StormBehaviorTreeTemplateState
{
  std::size_t m_TypeId;
  int m_Size;
  int m_Offset;
  int m_Align;
  const char * m_DebugName;
  void(*m_Allocate)(void * memory);
  void(*m_Deallocate)(void * ptr);
  void(*m_Activate)(void * ptr, DataType & data_type, ContextType & context_type);
  void(*m_Deactivate)(void * ptr, DataType & data_type, ContextType & context_type);
  bool(*m_Update)(void * ptr, DataType & data_type, ContextType & context_type);
};

template <typename DataType, typename ContextType>
struct StormBehaviorTreeTemplateConditional
{
  std::size_t m_TypeId;
  int m_Size;
  int m_Offset;
  int m_Align;
  const char * m_DebugName;
  void(*m_Allocate)(void * memory);
  void(*m_Deallocate)(void * ptr);
  bool(*m_Check)(void * ptr, const DataType & data_type, const ContextType & context_type);
  bool m_Preempt;
  bool m_Continuous;
};

template <typename DataType, typename ContextType>
struct StormBehaviorTreeTemplateService
{
  std::size_t m_TypeId;
  int m_Size;
  int m_Offset;
  int m_Align;
  const char * m_DebugName;
  void(*m_Allocate)(void * memory);
  void(*m_Deallocate)(void * ptr);
  void(*m_Activate)(void * ptr, DataType & data_type, ContextType & context_type);
  void(*m_Deactivate)(void * ptr, DataType & data_type, ContextType & context_type);
  void(*m_Update)(void * ptr, DataType & data_type, ContextType & context_type);
};

template <typename UpdaterType>
struct StormBehaviorTreeTemplateStateMarker
{

};

template <typename DataType, typename ContextType>
class StormBehaviorTreeTemplateBuilder
{
public:

  using SubtreeType = StormBehaviorTreeTemplateBuilder<DataType, ContextType>;
  using ServiceType = StormBehaviorTreeTemplateService<DataType, ContextType>;
  using StateType = StormBehaviorTreeTemplateState<DataType, ContextType>;
  using ConditionalType = StormBehaviorTreeTemplateConditional<DataType, ContextType>;

  template <typename ... Args>
  StormBehaviorTreeTemplateBuilder(StormBehaviorNodeType type, Args && ... args) :
    m_Type(type)
  {
    // If you want to add a leaf node, use AddNode instead of creating another sub tree
    assert(m_Type != StormBehaviorNodeType::kLeaf);

    AddChildren(std::forward<Args>(args)...);
  }

  template <typename State>
  StormBehaviorTreeTemplateBuilder(const StormBehaviorTreeTemplateStateMarker<State> &) :
    m_Type(StormBehaviorNodeType::kLeaf)
  {
    StateType updater;
    updater.m_TypeId = typeid(State).hash_code();
    updater.m_DebugName = typeid(State).name();
    updater.m_Size = sizeof(State);
    updater.m_Align = alignof(State);
    updater.m_Allocate = [](void * mem) { new(mem) State(); };
    updater.m_Deallocate = [](void * mem) { delete static_cast<State *>(mem); };

    if constexpr(StormBehaviorHasActivate<State>::value)
    {
      updater.m_Activate = [](void * ptr, DataType & data_type, ContextType & context_type)
      {
        State * updater = reinterpret_cast<State *>(ptr);
        updater->Activate(data_type, context_type);
      };
    }

    if constexpr(StormBehaviorHasDeactivate<State>::value)
    {
      updater.m_Deactivate = [](void * ptr, DataType & data_type, ContextType & context_type)
      {
        State * updater = reinterpret_cast<State *>(ptr);
        updater->Deactivate(data_type, context_type);
      };
    }

    updater.m_Update = [](void * ptr, DataType & data_type, ContextType & context_type)
    {
      State * updater = reinterpret_cast<State *>(ptr);
      return updater->Update(data_type, context_type);
    };

    m_State.emplace(updater);
  }

  StormBehaviorTreeTemplateBuilder(const StormBehaviorTreeTemplateBuilder<DataType, ContextType> & rhs) = delete;
  StormBehaviorTreeTemplateBuilder & operator = (const StormBehaviorTreeTemplateBuilder<DataType, ContextType> & rhs) = delete;

  StormBehaviorTreeTemplateBuilder(StormBehaviorTreeTemplateBuilder<DataType, ContextType> && rhs) = default;
  StormBehaviorTreeTemplateBuilder & operator = (StormBehaviorTreeTemplateBuilder<DataType, ContextType> && rhs) = default;

  SubtreeType && AddChild(SubtreeType && sub_tree) &&
  {
    AddChildInternal(std::move(sub_tree));
    return std::forward<SubtreeType>(*this);
  }

  template <typename State>
  SubtreeType && AddState() &&
  {
    m_OwnedSubtrees.emplace_back(std::make_unique<SubtreeType>(StormBehaviorTreeTemplateStateMarker<State>{}));
    m_Subtrees.emplace_back(SubtreeInfo{ m_OwnedSubtrees.back().get(), 100 });
    return std::forward<SubtreeType>(*this);
  }

  template <typename Service>
  SubtreeType && AddService() &&
  {
    AddServiceInternal<Service>();
    return std::forward<SubtreeType>(*this);
  }

  template <typename Conditional>
  SubtreeType && AddConditional() &&
  {
    AddConditionalInternal<Conditional>();
    return std::forward<SubtreeType>(*this);
  }

  void DebugPrint() const
  {
    DebugPrint(0);
  }

private:

  void AddChildInternal(SubtreeType && sub_tree)
  {
    m_OwnedSubtrees.emplace_back(std::make_unique<SubtreeType>(std::move(sub_tree)));
    m_Subtrees.emplace_back(SubtreeInfo{ m_OwnedSubtrees.back().get(), 100 });
  }

  void AddChildInternal(int random_weight, StormBehaviorTreeTemplateBuilder && sub_tree)
  {
    m_OwnedSubtrees.emplace_back(std::make_unique<SubtreeType>(std::move(sub_tree)));
    m_Subtrees.emplace_back(SubtreeInfo{ m_OwnedSubtrees.back().get(), random_weight });
  }

  void AddChildInternal(const StormBehaviorTreeTemplateBuilder & sub_tree)
  {
    m_Subtrees.emplace_back(SubtreeInfo{ &sub_tree, 100 });
  }

  void AddChildInternal(int random_weight, const StormBehaviorTreeTemplateBuilder & sub_tree)
  {
    m_Subtrees.emplace_back(SubtreeInfo{ &sub_tree, random_weight });
  }

  template <typename Service>
  void AddServiceInternal()
  {
    ServiceType service;
    service.m_TypeId = typeid(Service).hash_code();
    service.m_DebugName = typeid(Service).name();
    service.m_Size = sizeof(Service);    
    service.m_Align = alignof(Service);
    service.m_Allocate = [](void * mem) { new(mem) Service(); };
    service.m_Deallocate = [](void * mem) { delete static_cast<Service *>(mem); };

    service.m_Activate = nullptr;
    service.m_Deactivate = nullptr;
    service.m_Update = nullptr;

    if constexpr(StormBehaviorHasActivate<Service>::value)
    {
      service.m_Activate = [](void * ptr, DataType & data_type, ContextType & context_type)
      {
        Service * service = reinterpret_cast<Service *>(ptr);
        service->Activate(data_type, context_type);
      };
    }

    if constexpr(StormBehaviorHasDeactivate<Service>::value)
    {
      service.m_Deactivate = [](void * ptr, DataType & data_type, ContextType & context_type)
      {
        Service * service = reinterpret_cast<Service *>(ptr);
        service->Deactivate(data_type, context_type);
      };
    }

    if constexpr(StormBehaviorHasUpdate<Service>::value)
    {
      service.m_Update = [](void * ptr, DataType & data_type, ContextType & context_type)
      {
        Service * service = reinterpret_cast<Service *>(ptr);
        service->Update(data_type, context_type);
      };
    }

    m_Services.emplace_back(std::move(service));
  }

  template <typename Conditional>
  void AddConditionalInternal(bool preempt = false, bool continuous = false)
  {
    ConditionalType conditional;
    conditional.m_TypeId = typeid(Conditional).hash_code();
    conditional.m_DebugName = typeid(Conditional).name();
    conditional.m_Size = sizeof(Conditional);
    conditional.m_Align = alignof(Conditional);
    conditional.m_Allocate = [](void * mem) { new(mem) Conditional(); };
    conditional.m_Deallocate = [](void * mem) { delete static_cast<Conditional*>(mem); };

    conditional.m_Check = [](void * ptr, const DataType & data_type, const ContextType & context_type)
    {
      Conditional * conditional = reinterpret_cast<Conditional *>(ptr);
      return conditional->Check(data_type, context_type);
    };

    conditional.m_Preempt = preempt;
    conditional.m_Continuous = continuous;
    m_Conditionals.emplace_back(std::move(conditional));
  }

  void AddChildren()
  {

  }

  template <typename Arg, typename ... Args>
  void AddChildren(Arg && arg, Args && ... args)
  {
    AddChildInternal(std::forward<Arg>(arg));
    AddChildren(std::forward<Args>(args)...);
  }

  struct SubtreeInfo
  {
    const SubtreeType * m_SubTree;
    int m_RandomWeight;
  };
  
  void DebugPrintIndent(int indent) const
  {
    for(int index = 0; index < indent; ++index)
    {
      printf(" ");
    }
  }

  void DebugPrint(int indent) const
  {
    DebugPrintIndent(indent);

    printf("-");
    switch(m_Type)
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
        printf("Leaf (%s)\n", m_State->m_DebugName);
        break;
    }

    for(auto & elem : m_Conditionals)
    {
      DebugPrintIndent(indent);
      printf("| Conditional (%s)\n", elem.m_DebugName);
    }

    for(auto & elem : m_Services)
    {
      DebugPrintIndent(indent);
      printf("| Service (%s)\n", elem.m_DebugName);
    }

    for(auto & elem : m_Subtrees)
    {
      elem.m_SubTree->DebugPrint(indent + 2);
    }
  }

private:

  friend class StormBehaviorTreeTemplate<DataType, ContextType>;

  StormBehaviorNodeType m_Type;

  std::vector<ServiceType> m_Services;
  std::vector<ConditionalType> m_Conditionals;
  std::optional<StateType> m_State;

  const char * m_DebugName;

  std::vector<SubtreeInfo> m_Subtrees;
  std::vector<std::unique_ptr<SubtreeType>> m_OwnedSubtrees;
};
