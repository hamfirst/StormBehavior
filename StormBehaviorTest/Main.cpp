
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

int main(int argc, char ** argv)
{
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}


