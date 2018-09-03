
#include "StormBehavior/StormBehaviorTree.h"

#include <cstdio>
#include <random>

struct TestContext
{
  int a;
};

struct TestData
{
  int b;
};

struct TestUpdater
{
  bool Update(const TestData & test, const TestContext & context)
  {
    return true;
  }
};

struct TestService
{
  bool Update(const TestData & test, const TestContext & context)
  {
    return true;
  }
};

struct TestConditional
{
  bool Check(const TestData & data, const TestContext & context)
  {
    return true;
  }
};

using BT = StormBehaviorTreeTemplateBuilder<TestData, TestContext>;
using BTInst = StormBehaviorTree<TestData, TestContext>;

template <typename StateUpdater>
inline BT State()
{
  return BT(StormBehaviorTreeTemplateStateMarker<StateUpdater>{});
}

auto TestTree = StormBehaviorTreeTemplate(BT(StormBehaviorNodeType::kSelect)
  .AddService<TestUpdater>()
  .AddChild(
    BT(StormBehaviorNodeType::kSelect)
      .AddChild(
        State<TestUpdater>()
        .AddConditional<TestConditional>()
      )
      .AddChild(
        State<TestUpdater>()
        .AddService<TestService>()
      )
  )
  .AddChild(
    State<TestUpdater>()
    .AddService<TestService>()
    .AddConditional<TestConditional>()
  ));

int main()
{
  TestTree.DebugPrint();

  std::mt19937 r;
  StormBehaviorTree bt_inst(TestTree);

  TestData data;
  TestContext context;
  bt_inst.Update(data, context, r);

  printf("Done\n");
  return 0;
}


