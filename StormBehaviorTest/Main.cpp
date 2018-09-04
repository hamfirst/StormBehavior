
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

struct TestUpdater1
{
  bool Update(const TestData & test, const TestContext & context)
  {
    printf("Update 1\n");
    return true;
  }
};

struct TestUpdater2
{
  bool Update(const TestData & test, const TestContext & context)
  {
    printf("Update 2\n");
    return true;
  }
};

struct TestUpdater3
{
  bool Update(const TestData & test, const TestContext & context)
  {
    printf("Update 3\n");
    return true;
  }
};

struct TestUpdater4
{
  bool Update(const TestData & test, const TestContext & context)
  {
    printf("Update 4\n");
    return true;
  }
};

struct TestService
{
  void Update(const TestData & test, const TestContext & context)
  {
    printf("Test service\n");
  }
};

struct TestConditional
{
  bool Check(const TestData & data, const TestContext & context)
  {
    printf("Test conditional\n");
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
  .AddChild(
    BT(StormBehaviorNodeType::kSequence)
      .AddService<TestService>()
      .AddConditional<TestConditional>(true, true)
      .AddChild(
        BT(StormBehaviorNodeType::kRandom)
          .AddChild(
            State<TestUpdater1>()
          )
          .AddChild(
            State<TestUpdater2>()
          )
      )
      .AddChild(
        State<TestUpdater3>()
      )
  )
  .AddChild(
    State<TestUpdater4>()
  ));

int main()
{
  TestTree.DebugPrint();

  std::mt19937 r;
  StormBehaviorTree bt_inst(TestTree);

  TestData data;
  TestContext context;
  bt_inst.Update(data, context, r);
  bt_inst.Update(data, context, r);
  bt_inst.Update(data, context, r);
  
  bt_inst.Update(data, context, r);
  bt_inst.Update(data, context, r);
  bt_inst.Update(data, context, r);

  printf("Done\n");
  return 0;
}


