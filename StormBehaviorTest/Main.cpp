
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
  bool m_ServiceActive = false;
  int m_SerivceUpdated = false;
  bool m_ToggleActive = true;
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
  void Activate(TestData & test, TestContext & context)
  {
    test.m_ServiceActive = true;
  }

  void Deactivate(TestData & test, TestContext & context)
  {
    test.m_ServiceActive = false;
  }

  void Update(TestData & test, TestContext & context)
  {
    test.m_SerivceUpdated++;
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

struct TestConditionalToggle
{
  bool Check(const TestData & data, const TestContext & context)
  {
    return data.m_ToggleActive;
  }
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
/*
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

TEST_F(StormBehaviorTestFixture, Service)
{
  auto TestTreeTemplate = StormBehaviorTreeTemplate(
    BT(StormBehaviorNodeType::kSequence)
      .AddChild(
        State<TestUpdater>(1)
      )
      .AddChild(
        State<TestUpdater>(2)
        .AddService<TestService>()
      )
      .AddChild(
        State<TestUpdater>(3)
      ));  

  auto test_tree = StormBehaviorTree(TestTreeTemplate);
  EXPECT_EQ(data.m_ServiceActive, false);
  EXPECT_EQ(data.m_SerivceUpdated, 0);
  test_tree.Update(data, context, r);
  EXPECT_EQ(data.m_ServiceActive, false);
  EXPECT_EQ(data.m_SerivceUpdated, 0);
  test_tree.Update(data, context, r);
  EXPECT_EQ(data.m_ServiceActive, true);
  EXPECT_EQ(data.m_SerivceUpdated, 1);
  test_tree.Update(data, context, r);
  EXPECT_EQ(data.m_ServiceActive, false);
  EXPECT_EQ(data.m_SerivceUpdated, 1);
  test_tree.Update(data, context, r);
  EXPECT_EQ(data.m_ServiceActive, false);
  EXPECT_EQ(data.m_SerivceUpdated, 1);
  test_tree.Update(data, context, r);
  EXPECT_EQ(data.m_ServiceActive, true);
  EXPECT_EQ(data.m_SerivceUpdated, 2);
  test_tree.Update(data, context, r);
  EXPECT_EQ(data.m_ServiceActive, false);
  EXPECT_EQ(data.m_SerivceUpdated, 2);
  test_tree.Update(data, context, r);
  EXPECT_EQ(data.m_ServiceActive, false);
  EXPECT_EQ(data.m_SerivceUpdated, 2);
}
*/
TEST_F(StormBehaviorTestFixture, Conditional)
{
  auto TestTreeTemplate = StormBehaviorTreeTemplate(
    BT(StormBehaviorNodeType::kSelect)
      .AddChild(
        State<TestUpdater>(1, false)
        .AddConditional<TestConditionalToggle>(true, true)
      )
      .AddChild(
        State<TestUpdater>(2, false)
      )
      .AddChild(
        State<TestUpdater>(3)
      ));  

  auto test_tree = StormBehaviorTree(TestTreeTemplate);
  EXPECT_EQ(data.m_UpdaterId, 0);
  test_tree.Update(data, context, r);
  EXPECT_EQ(data.m_UpdaterId, 1);
  test_tree.Update(data, context, r);
  EXPECT_EQ(data.m_UpdaterId, 1);
  data.m_ToggleActive = false;
  test_tree.Update(data, context, r);
  EXPECT_EQ(data.m_UpdaterId, 2);
  test_tree.Update(data, context, r);
  EXPECT_EQ(data.m_UpdaterId, 2);
  data.m_ToggleActive = true;
  EXPECT_EQ(data.m_UpdaterId, 1);
  test_tree.Update(data, context, r);
  EXPECT_EQ(data.m_UpdaterId, 1);
}

int main(int argc, char ** argv)
{
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}


