
#include "StormBehavior/StormBehaviorTree.h"

#include <cstdio>
#include <random>

#include <gtest/gtest.h>

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

<<<<<<< HEAD
TEST(BTTest, SelectNode)
{
  auto TestTreeTemplate = StormBehaviorTreeTemplate(
    BT(StormBehaviorNodeType::kSelect)
      .AddService<TestUpdater>()
      .AddChild(
        BT(StormBehaviorNodeType::kSelect)
          .AddChild(
            State<TestUpdater>()
            .AddConditional<TestConditional>(false, false)
          )
          .AddChild(
            State<TestUpdater>()
            .AddService<TestService>()
          )
      )
      .AddChild(
        State<TestUpdater>()
        .AddService<TestService>()
        .AddConditional<TestConditional>(false, false)
      ));  

  auto test_tree = StormBehaviorTree(TestTreeTemplate);
  
}
=======
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
>>>>>>> 978d18792a5397992900d8f8b9a02b2ec303b5a0

int main(int argc, char ** argv)
{
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}


