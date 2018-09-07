
#include "StormBehavior/StormBehaviorTree.h"

#include <cstdio>
#include <random>

#include <gtest/gtest.h>

struct TestContext
{

};

struct TestData
{
  int m_UpdaterId = 0;
};

struct TestUpdater
{
  TestUpdater(int id, bool success = true)
  {
    m_Id = id;
    m_Success = success;
  }

  bool Update(TestData & test, TestContext & context)
  {
    test.m_UpdaterId = m_Id;
    return m_Success;
  }

  int m_Id;
  bool m_Success;
};

struct TestService
{
  void Update(TestData & test, TestContext & context)
  {
  }
};

struct TestConditional
{
  TestConditional(bool success = true)
  {
    m_Success = success;
  }

  bool Check(const TestData & data, const TestContext & context)
  {
    return m_Success;
  }

  bool m_Success;
};

using BT = StormBehaviorTreeTemplateBuilder<TestData, TestContext>;
using BTInst = StormBehaviorTree<TestData, TestContext>;

template <typename StateUpdater, typename ... Args>
inline BT State(Args && ... args)
{
  return BT(StormBehaviorTreeTemplateStateMarker<StateUpdater>{}, std::forward<Args>(args)...);
}

struct StormBehaviorTestFixture : testing::Test
{
  TestData data = {};
  TestContext context = {};

  std::mt19937 r = std::mt19937(0);
};

TEST_F(StormBehaviorTestFixture, SelectNode)
{
  auto TestTreeTemplate = StormBehaviorTreeTemplate(
    BT(StormBehaviorNodeType::kSelect)
      .AddChild(
        State<TestUpdater>(1)
        .AddConditional<TestConditional>(false, false, false)
      )
      .AddChild(
        State<TestUpdater>(2)
      )
      .AddChild(
        State<TestUpdater>(3)
      ));  

  auto test_tree = StormBehaviorTree(TestTreeTemplate);
  EXPECT_EQ(data.m_UpdaterId, 0);
  test_tree.Update(data, context, r);
  EXPECT_EQ(data.m_UpdaterId, 2);
  test_tree.Update(data, context, r);
  EXPECT_EQ(data.m_UpdaterId, 2);
  test_tree.Update(data, context, r);
  EXPECT_EQ(data.m_UpdaterId, 2);
}

TEST_F(StormBehaviorTestFixture, SequenceNode)
{
  auto TestTreeTemplate = StormBehaviorTreeTemplate(
    BT(StormBehaviorNodeType::kSequence)
      .AddChild(
        State<TestUpdater>(1)
      )
      .AddChild(
        State<TestUpdater>(2)
      )
      .AddChild(
        State<TestUpdater>(3)
      ));  

  auto test_tree = StormBehaviorTree(TestTreeTemplate);
  EXPECT_EQ(data.m_UpdaterId, 0);
  test_tree.Update(data, context, r);
  EXPECT_EQ(data.m_UpdaterId, 1);
  test_tree.Update(data, context, r);
  EXPECT_EQ(data.m_UpdaterId, 2);
  test_tree.Update(data, context, r);
  EXPECT_EQ(data.m_UpdaterId, 3);
  test_tree.Update(data, context, r);
  EXPECT_EQ(data.m_UpdaterId, 1);
  test_tree.Update(data, context, r);
  EXPECT_EQ(data.m_UpdaterId, 2);
  test_tree.Update(data, context, r);
  EXPECT_EQ(data.m_UpdaterId, 3);
  test_tree.Update(data, context, r);
  EXPECT_EQ(data.m_UpdaterId, 1);
}

int main(int argc, char ** argv)
{
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}


