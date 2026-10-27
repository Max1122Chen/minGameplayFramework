namespace minGameplayFramework::TechnologyTree
{
    class Technology
    {
    private:
        std::string m_Name;
        minEngine::GameplayTag m_TechnologyTag;
    };

    class TechnologyCatalog : public minEngine::DataAsset
    {
    private:
        std::vector<Technology> m_Technologies;
    };
    
    class TechnologyTreeDef : public minEngine::DataAsset
    {
    public:
    }

    class TechnologyTreeBuilder
    {
    public:
        GameplayDataGraph::GameplayDataGraph<minEngine::GameplayTag> Build(const TechnologyTreeDef& dataSource, GameplayDataGraph::GraphSettings settings) override;
    };

    class TechnologyTreeInstance
    {
    public:
    };
}