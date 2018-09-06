
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
  TestUpdater(int * update_ptr, int id, bool success = true)
  {
    m_UpdaterPtr = update_ptr;
    m_Id = id;
    m_Success = success;
  }

  bool Update(const TestData & test, const TestContext & context)
  {
    *m_UpdaterPtr = m_Id;
    return m_Success;
  }

  int * m_UpdaterPtr;
  int m_Id;
  bool m_Success;
};

struct TestService
{
  void Update(const TestData & test, const TestContext & context)
  {
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

template <typename StateUpdater, typename ... Args>
inline BT State(Args && ... args)
{
  return BT(StormBehaviorTreeTemplateStateMarker<StateUpdater>{}, std::forward<Args>(args)...);
}

TEST(BTTest, SelectNode)
{
  int update_id = 0;
  auto TestTreeTemplate = StormBehaviorTreeTemplate(
    BT(StormBehaviorNodeType::kSelect)
      .AddChild(
        State<TestUpdater>(&update_id, 1)
      )
      .AddChild(
        State<TestUpdater>(&update_id, 2)
      )
      .AddChild(
        State<TestUpdater>(&update_id, 3)
      ));  

  auto test_tree = StormBehaviorTree(TestTreeTemplate);
}

int main(int argc, char ** argv)
{
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}


