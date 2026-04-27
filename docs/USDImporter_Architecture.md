# MyUSDImporter Architecture Analysis

This document outlines the core architecture, entry points, data flow, and specialized translators for the MyUSDImporter plugin (a customized version of Unreal Engine's official USDImporter).

## 1. Entry Points (UE Import System Registration)

The plugin registers with Unreal Engine's import subsystem primarily through classes that inherit from `UFactory`.

*   **[UMyUsdStageAssetImportFactory](file:///workspace/Plugins/MyUSDImporter/Source/MyUSDStageImporter/Private/MyUSDStageAssetImportFactory.h)**: 
    *   Inherits from `UFactory` and `FReimportHandler`.
    *   **Trigger**: When a user drags and drops a `.usd`, `.usda`, or `.usdc` file into the Content Browser, or clicks "Import" in the Content Browser.
    *   **Role**: It creates a `UMyUsdStageAsset` (or similar cache asset) representing the USD file as a trackable Unreal asset.

*   **[UMyUsdStageImportFactory](file:///workspace/Plugins/MyUSDImporter/Source/MyUSDStageImporter/Private/MyUSDStageImportFactory.h)**: 
    *   Inherits from `USceneImportFactory` (which inherits from `UFactory`).
    *   **Trigger**: When a user uses the "File -> Import Into Level..." menu option.
    *   **Role**: It is responsible for instantiating the USD scene hierarchy directly into the current World/Level as Actors and Components, rather than just creating Content Browser assets.

## 2. Core Data Flow (Importing a .usd file)

When an import is triggered, the execution flows from the Factory into the core Importer class.

### 2.1 The Orchestrator: `UMyUsdStageImporter`
The central hub for the import process is `UMyUsdStageImporter::ImportFromFile()` located in **[MyUSDStageImporter.cpp](file:///workspace/Plugins/MyUSDImporter/Source/MyUSDStageImporter/Private/MyUSDStageImporter.cpp)**.

The typical execution sequence is as follows:

1.  **Open Stage**: 
    *   The `.usd` file is loaded into memory using the Pixar USD API (wrapped by Unreal).
    *   A `UE::FUsdStage` handle is instantiated to represent the parsed USD stage.
2.  **Setup Context**: 
    *   A `FUsdSchemaTranslationContext` is initialized. This is a crucial state object passed around during the entire import process.
    *   It contains the `UsdInfoCache` (tracks what has been parsed), `PrimLinkCache` (links USD Prims to UE Assets), and user-defined `UMyUsdStageImportOptions`.
3.  **Setup Scene Actor (Level Import Only)**: 
    *   If importing into a level, a root `AMyUsdStageActor` (or a standard `AActor`) is spawned to serve as the parent for all imported components.
4.  **Cache Collapsing State**: 
    *   The system traverses the USD Prim hierarchy to determine which subtrees should be "collapsed". 
    *   *Collapsing* is an optimization where multiple small USD meshes under a common parent are merged into a single `UStaticMesh` in Unreal to reduce draw calls.
5.  **Pre-Import Assets (Assets Pass)**: 
    *   Before building the Actor hierarchy, the importer does a pass to generate standalone Unreal assets (UObjects).
    *   It calls `ImportMaterials()` and `ImportMeshes()`.
    *   These functions iterate over the relevant Prims and invoke the `CreateAssets()` method on their respective Translators.
6.  **Import Actors/Components (Hierarchy Pass)**: 
    *   The importer recursively traverses the USD Stage starting from the root Prim.
    *   For each Prim, it queries the `FUsdSchemaTranslatorRegistry` to find the appropriate `FUsdSchemaTranslator` subclass (e.g., Mesh, Light, Camera).
    *   It invokes `CreateComponents()` on the Translator, which spawns the corresponding `USceneComponent` and attaches it to the parent, assigning the assets generated in step 5.
7.  **Publish and Remap**: 
    *   Temporary assets are moved to their final destination packages (`PublishAssets`).
    *   `RemapReferences` is called to ensure all newly created components point to the correct final UObject paths.

## 3. Specialized Translators (Readers/Builders)

The USDImporter uses a modular "Translator" architecture. Instead of one massive parsing function, specific classes handle specific USD Schemas (Prim types). These are primarily located in the `MyUSDSchemas` module.

### 3.1 Models & Geometry
*   **[FMyUsdGeomMeshTranslator](file:///workspace/Plugins/MyUSDImporter/Source/MyUSDSchemas/Private/MyUSDGeomMeshTranslator.cpp)**: 
    *   **Target**: `UsdGeomMesh` Prims.
    *   **Role**: Converts USD mesh data into Unreal `UStaticMesh` assets and `UStaticMeshComponent`s. It handles LODs, Nanite settings, and collision setups.
*   **[MeshTranslationImpl](file:///workspace/Plugins/MyUSDImporter/Source/MyUSDSchemas/Private/MeshTranslationImpl.h)**: 
    *   **Role**: A shared utility namespace/class that performs the heavy lifting of reading raw USD arrays (vertices, face indices, normals, UVs) and converting them into Unreal's `FMeshDescription` format, which is used to build the final mesh assets.
*   **[FMyUsdGeomXformableTranslator](file:///workspace/Plugins/MyUSDImporter/Source/MyUSDSchemas/Private/MyUSDGeomXformableTranslator.cpp)**: 
    *   **Target**: `UsdGeomXformable` Prims (the base schema for anything with a 3D transform).
    *   **Role**: Extracts translation, rotation, and scale data (including animated transforms) and applies them to the resulting Unreal `USceneComponent`s.
*   **[FMyUsdNaniteAssemblyTranslator](file:///workspace/Plugins/MyUSDImporter/Source/MyUSDSchemas/Private/MyUSDNaniteAssemblyTranslator.cpp)**: 
    *   **Role**: Specialized translator designed to efficiently parse and assemble complex USD hierarchies directly into Unreal Nanite assemblies.

### 3.2 Materials & Textures
*   **[FMyUsdShadeMaterialTranslator](file:///workspace/Plugins/MyUSDImporter/Source/MyUSDSchemas/Private/MyUSDShadeMaterialTranslator.cpp)**: 
    *   **Target**: `UsdShadeMaterial` Prims.
    *   **Role**: Translates standard USD Preview Surface material networks into Unreal `UMaterialInterface` or `UMaterialInstanceDynamic` objects. It traverses the USD shader graph, imports referenced textures, and maps USD inputs to Unreal material parameters.
*   **[FMaterialXUSDShadeMaterialTranslator](file:///workspace/Plugins/MyUSDImporter/Source/MyUSDSchemas/Private/Custom/MaterialXUSDShadeMaterialTranslator.cpp)**: 
    *   **Role**: A specialized extension that parses MaterialX shader networks embedded within USD files, translating them into compatible Unreal materials.

### 3.3 Skeleton & Animation
*   **[FMyUsdSkelRootTranslator](file:///workspace/Plugins/MyUSDImporter/Source/MyUSDSchemas/Private/MyUSDSkelRootTranslator.cpp)**: 
    *   **Target**: `UsdSkelRoot` Prims.
    *   **Role**: Acts as the container/entry point for skinned meshes and skeletal animation. It sets up the root `USkinnedMeshComponent`.
*   **[FMyUsdSkelSkeletonTranslator](file:///workspace/Plugins/MyUSDImporter/Source/MyUSDSchemas/Private/MyUSDSkelSkeletonTranslator.cpp)**: 
    *   **Target**: `UsdSkelSkeleton` and `UsdSkelAnimation` Prims.
    *   **Role**: Extremely complex translator responsible for:
        *   Extracting bone hierarchies and rest poses to create a `USkeleton`.
        *   Binding skinned `FMeshDescription` data to create a `USkeletalMesh`.
        *   Parsing time-sampled joint transforms to generate a `UAnimSequence`.
        *   Often generates a dynamic `UAnimBlueprint` to drive the skeletal mesh at runtime based on the USD animation data.