// 这段关于GameplayDataGraph设计的描述必须保留
// 在游戏中，常常存在例如像技能树、科技树、对话树、事件树等等数据结构，虽然它们常被俗称成“树”，但从数据结构的角度来看它们中的一些是图模型
// 例如：
// 1. 技能树，一个高级技能的解锁可能依赖多个低级技能的解锁；
// 2. 科技树中，一个高级科技的解锁可能依赖多个低级科技的解锁；
// 3. 对话树中，一个对话的触发可能依赖多个前置对话的触发；同时往往存在一些对话会返回到之前已经触发过的对话，这是典型的环
// 4. 事件树中，一个事件的触发可能依赖多个前置事件的触发。
// 
// 然而，游戏业务中的图模型的出现方式往往是隐晦的，同时伴随着大量复杂的各异的业务逻辑
// 作者尝试抽象出其中共通的可复用的结构和算法，以简化游戏业务中图模型的实现，并帮助游戏开发者理清业务逻辑。
// 作者为此提出了几项重要的设计思想
// 1. 分离游戏系统主要的业务逻辑和与隐含的图关系。
//    - 例如在技能系统中，技能的Effect是技能本身主要的业务逻辑，技能树中的依赖是一种图关系
//    - 一个技能本身只关心自己造成什么效果，而不关心和其他技能的关系，技能之间的关系由技能树资产定义，遵守单一职责原则
// 2. 分离数据定义和运行时
//    - 数据定义应当仅由设计师在编辑期定义，运行时不应该被修改。
//    - 另一方面，资产定义需要被序列化，而运行时实例不需要，两者的数据结构可能存在区别
//    - 例如Unity中不支持map的直接序列化，因此需要用List来存储邻接表，而运行时实例则可使用map来存储邻接表并优化查询效率
// 3. 分离通用的图模型数据结构和图构建器
//    - 通用的图模型数据结构只负责存储图的结构和算法，不关心图的构建方式
//    - 图构建器负责根据游戏业务逻辑构建图，例如技能树构建器负责根据技能树资产定义构建技能树图
//
// 此外，必须说明的是，这套设计主要的价值在于设计思想，您并不需要完全遵循这套设计，而是需要根据具体业务需求进行调整。
// 这套设计对于不同游戏的价值可能并不相同，
// - 例如对于一些游戏而言，例如技能树的UI可能是设计师手动布置而非程序化排布的，此时本套设计的统一构图功能可能体现不出价值
// - 但对于一些游戏，例如《环世界》《异星工厂》《饥荒》等，它们的科技树的节点数量可能很大，同时还可能考虑支持mods，其科技树UI往往需要程序化排布
//   此时本套设计的图构建器功能可能体现其价值

// 还没想好，仅供参考
namespace minGameplayFramework::GameplayDataGraph
{
    struct GraphSettings
    {
        bool IsWeighted;
        bool IsDirected;
        bool IsCyclic;
        bool IsTree;
        bool NeedTopologicalSort;
    };

    template<class TGraphDataSource, class TNode>
    class GameplayDataGraphBuilder
    {
    public:
        virtual GameplayDataGraph<TNode> Build(const TGraphDataSource& dataSource, GraphSettings settings) = 0;
    };
    
    // 运行时游戏数据图，只承载游戏数据图的运行时状态，不承载游戏数据图的定义。
    // TNode 表示节点类型，需要是可哈希的。
    // GameplayDataGraph只表示依赖关系，不关心节点的业务语义，TNode可以是简单类型，仅用于标识节点身份。
    template<class TNode>
    class GameplayDataGraph
    {
    public:
        static GameplayDataGraph<TNode> Invalid(const GraphSettings& settings);
        void AddEdge(TNode from, TNode to, float weight = 0.0f);
        void RemoveEdge(TNode from, TNode to);
        bool HasEdge(TNode from, TNode to) const;
        std::vector<TNode> GetNeighbors(TNode node) const;
        std::vector<std::pair<TNode, float>> GetWeightedNeighbors(TNode node) const;
        bool IsReachable(TNode from, TNode to) const;

        bool IsValid() const;
        bool IsCyclic() const;
        bool IsTree() const;
        const GraphSettings& GetSettings() const;
    private:
        bool m_IsValid = false;
        // 实际构建出来的图的性质，可能和Settings不一致，不一致时m_IsValid为false，只是为了保存构建出来的图的性质，方便后续判断。
        bool m_IsCyclic = false;
        bool m_IsTree = false;

        GraphSettings m_Settings;
        // 邻接表
        std::unordered_map<TNode, std::vector<TNode>> m_AdjacencyList;

        // 带权邻接表
        std::unordered_map<TNode, std::vector<std::pair<TNode, float>>> m_WeightedAdjacencyList;

        // 拓扑排序结果
        std::vector<TNode> m_TopologicalSort;
    };
}