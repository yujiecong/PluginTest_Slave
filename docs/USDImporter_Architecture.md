# MyUSDImporter 架构分析

本文档概述了 MyUSDImporter 插件（基于 Unreal Engine 官方 USDImporter 定制魔改版）的核心架构、系统入口点、数据流向以及专门用于解析各类 USD 数据的转换器（Translators）。

## 1. 引擎导入系统入口点 (Entry Points)

插件主要通过继承自 `UFactory` 的类，将其导入能力注册到虚幻引擎的导入子系统中。

*   **[UMyUsdStageAssetImportFactory](file:///workspace/Plugins/MyUSDImporter/Source/MyUSDStageImporter/Private/MyUSDStageAssetImportFactory.h)**: 
    *   继承自 `UFactory` 和 `FReimportHandler`。
    *   **触发时机**：当用户将 `.usd`, `.usda`, 或 `.usdc` 文件拖拽到内容浏览器（Content Browser）中，或在内容浏览器点击“导入”时触发。
    *   **核心职责**：它主要负责创建一个 `UMyUsdStageAsset` （或其他缓存资产）作为该 USD 文件在引擎内容浏览器中的资产代理。

*   **[UMyUsdStageImportFactory](file:///workspace/Plugins/MyUSDImporter/Source/MyUSDStageImporter/Private/MyUSDStageImportFactory.h)**: 
    *   继承自 `USceneImportFactory` （其基类为 `UFactory`）。
    *   **触发时机**：当用户使用菜单栏的 “File -> Import Into Level...” （文件 -> 导入到关卡...）选项时触发。
    *   **核心职责**：负责将 USD 场景层级直接实例化到当前的关卡（World/Level）中，生成实际的 Actor 和 Component 树，而不仅仅是生成内容浏览器里的静态资产。

## 2. 核心数据流 (导入 .usd 文件的生命周期)

当导入流程被触发后，执行流会从 `Factory` 移交到核心的 `Importer` 类。

### 2.1 总指挥调度器：`UMyUsdStageImporter`
导入过程的中央枢纽是位于 **[MyUSDStageImporter.cpp](file:///workspace/Plugins/MyUSDImporter/Source/MyUSDStageImporter/Private/MyUSDStageImporter.cpp)** 中的 `UMyUsdStageImporter::ImportFromFile()` 函数。

典型的数据流转与执行顺序如下：

1.  **打开 Stage (Open Stage)**: 
    *   使用 Pixar 官方的 USD API（由 UE 封装）将 `.usd` 文件加载到内存中。
    *   实例化一个 `UE::FUsdStage` 句柄，作为解析好的 USD Stage 的代表。
2.  **设置上下文 (Setup Context)**: 
    *   初始化一个核心状态对象 `FUsdSchemaTranslationContext`，该上下文会在整个导入过程中在各个函数间传递。
    *   上下文内部包含了 `UsdInfoCache`（记录哪些节点已被解析）、`PrimLinkCache`（记录 USD Prim 到 UE 资产的映射链接），以及用户在界面配置的 `UMyUsdStageImportOptions` 导入选项。
3.  **创建场景根 Actor (Setup Scene Actor - 仅限关卡导入)**: 
    *   如果是导入到关卡中，会生成一个根 `AMyUsdStageActor`（或者普通的 `AActor`），作为所有后续生成的组件的父容器。
4.  **计算折叠状态 (Cache Collapsing State)**: 
    *   系统会遍历整个 USD Prim 层级树，以决定哪些子树应当被“折叠”（Collapsed）。
    *   *折叠（Collapsing）* 是一种优化手段，旨在将拥有同一个父节点的多个细碎的小网格体（Mesh）合并为 Unreal 中的单一 `UStaticMesh` 资产，以大幅降低渲染时的 Draw Call。
5.  **预导入资产 (Assets Pass)**: 
    *   在构建 Actor 层级树之前，导入器会先进行一轮扫描，生成独立的虚幻引擎资产（UObjects）。
    *   依次调用 `ImportMaterials()`（导入材质）和 `ImportMeshes()`（导入网格体）。
    *   这些函数会遍历对应的 Prim，并调用它们专属 Translator 上的 `CreateAssets()` 方法。
6.  **导入 Actor 与组件 (Hierarchy Pass)**: 
    *   导入器从 Root Prim 开始递归遍历整个 USD Stage。
    *   对于每一个 Prim，系统会查询 `FUsdSchemaTranslatorRegistry`，寻找注册的对应 `FUsdSchemaTranslator` 子类（例如网格体翻译器、灯光翻译器、相机翻译器等）。
    *   调用该 Translator 上的 `CreateComponents()` 方法。该方法负责在场景中生成对应的 `USceneComponent`，将其挂载到父节点上，并为其分配在第 5 步中生成的静态资产。
7.  **资产发布与引用重定向 (Publish and Remap)**: 
    *   将之前生成的临时资产正式移动到目标 Package 路径下（`PublishAssets`）。
    *   调用 `RemapReferences` 来修复引用关系，确保新生成的组件都指向了正确、最终的 UObject 路径。

## 3. 专属翻译器 (Translators / Readers / Builders)

USDImporter 使用了高度模块化的“翻译器”（Translator）架构设计。它并没有将所有的解析逻辑塞在一个巨大的函数里，而是通过具体的类来处理特定的 USD Schemas（Prim 类型）。这些翻译器主要集中在 `MyUSDSchemas` 模块中。

### 3.1 模型与几何体解析 (Models & Geometry)
*   **[FMyUsdGeomMeshTranslator](file:///workspace/Plugins/MyUSDImporter/Source/MyUSDSchemas/Private/MyUSDGeomMeshTranslator.cpp)**: 
    *   **目标 Prim**：`UsdGeomMesh`。
    *   **核心职责**：将 USD 网格体数据转换为 Unreal 的 `UStaticMesh` 资产和 `UStaticMeshComponent` 组件。负责处理多级细节（LOD）、Nanite 设置开启以及碰撞（Collision）几何体的构建。
*   **[MeshTranslationImpl](file:///workspace/Plugins/MyUSDImporter/Source/MyUSDSchemas/Private/MeshTranslationImpl.h)**: 
    *   **核心职责**：这是一个共享的底层数据提取工具命名空间/类。它负责干最脏最累的活——读取原始的 USD 数组（顶点、面索引、法线、UV），并将它们转换填充为虚幻引擎标准的 `FMeshDescription` 结构。最终生成的 Mesh 资产都依赖此结构。
*   **[FMyUsdGeomXformableTranslator](file:///workspace/Plugins/MyUSDImporter/Source/MyUSDSchemas/Private/MyUSDGeomXformableTranslator.cpp)**: 
    *   **目标 Prim**：`UsdGeomXformable`（任何包含 3D 空间变换的对象的基类）。
    *   **核心职责**：提取平移（Translation）、旋转（Rotation）和缩放（Scale）数据（包括带动画时间采样的变换数据），并将它们应用为 Unreal `USceneComponent` 的相对 Transform。
*   **[FMyUsdNaniteAssemblyTranslator](file:///workspace/Plugins/MyUSDImporter/Source/MyUSDSchemas/Private/MyUSDNaniteAssemblyTranslator.cpp)**: 
    *   **核心职责**：专用的构建器，旨在高效解析兼容的复杂 USD 层级数据，并直接组装为虚幻引擎中性能极高的 Nanite 装配体（Nanite Assemblies）。

### 3.2 材质与纹理着色器 (Materials & Textures)
*   **[FMyUsdShadeMaterialTranslator](file:///workspace/Plugins/MyUSDImporter/Source/MyUSDSchemas/Private/MyUSDShadeMaterialTranslator.cpp)**: 
    *   **目标 Prim**：`UsdShadeMaterial`。
    *   **核心职责**：负责将标准的 USD Preview Surface 材质网络翻译为 Unreal 的 `UMaterialInterface` 或 `UMaterialInstanceDynamic` 对象。它会遍历 USD 材质节点图、将引用的贴图导入为 `UTexture`，并将 USD 的输入端口映射为虚幻材质的参数。
*   **[FMaterialXUSDShadeMaterialTranslator](file:///workspace/Plugins/MyUSDImporter/Source/MyUSDSchemas/Private/Custom/MaterialXUSDShadeMaterialTranslator.cpp)**: 
    *   **核心职责**：这是一个专用的扩展翻译器，专门负责解析嵌入在 USD 文件内部的 MaterialX 高级着色器节点网络，将其转译为兼容的虚幻材质。

### 3.3 骨骼与动画 (Skeleton & Animation)
*   **[FMyUsdSkelRootTranslator](file:///workspace/Plugins/MyUSDImporter/Source/MyUSDSchemas/Private/MyUSDSkelRootTranslator.cpp)**: 
    *   **目标 Prim**：`UsdSkelRoot`。
    *   **核心职责**：通常作为包含蒙皮网格体（Skinned Meshes）和骨架动画系统的根容器节点。它负责在 UE 中搭建根部的 `USkinnedMeshComponent`。
*   **[FMyUsdSkelSkeletonTranslator](file:///workspace/Plugins/MyUSDImporter/Source/MyUSDSchemas/Private/MyUSDSkelSkeletonTranslator.cpp)**: 
    *   **目标 Prim**：`UsdSkelSkeleton` 和 `UsdSkelAnimation`。
    *   **核心职责**：极其复杂的核心动画翻译器，主要负责：
        *   提取骨骼层级树和绑定姿势（Rest Poses）以生成 `USkeleton` 资产。
        *   绑定蒙皮的 `FMeshDescription` 数据以生成 `USkeletalMesh` 资产。
        *   深度解析带时间采样的关节变换（Joint Transforms）数据，烘焙生成 `UAnimSequence` 动画序列。
        *   通常还会自动生成并关联一个动态的 `UAnimBlueprint`（动画蓝图），以便在运行时驱动 USD 的骨骼动画数据播放。