# ue-ai CLI — AI的UE外挂

## 核心理念

**一次扫描，永久索引，即时定位。**

AI使用这个CLI时，就像装了UE的"外挂"：
1. **首次**：指向UE源码根目录，全量扫描，生成静态JSON索引
2. **之后**：AI通过关键字搜索，毫秒级定位到文件路径、行号、依赖关系
3. **渐进式披露**：从概览 → 类定义 → 成员详情 → 实现代码，按需深入

不需要UE编辑器在线，不需要MCP协议，不需要网络。纯静态数据，AI用bash搜就行。

## 一、工作流

```
首次使用:
  ue-ai index /path/to/UnrealEngine/Source --version 5.4
  → 扫描所有.h/.cpp文件
  → 解析UCLASS/UFUNCTION/UPROPERTY等宏
  → 构建类层次树、符号表、依赖图
  → 输出静态JSON索引到 ~/.ue-ai/indexes/5.4/

日常使用:
  ue-ai search "ACharacter"              → 定位类定义+文件路径+行号
  ue-ai search "TakeDamage"              → 找到所有函数定义和调用
  ue-ai class ACharacter --depth 2       → 类继承+子类+关键成员
  ue-ai deps UCharacterMovementComponent → 这个类依赖哪些模块
  ue-ai locate "UPROPERTY.*Health"       → 正则搜索，返回文件:行号
```

## 二、索引数据结构

### 2.1 顶层索引文件 `index.json`

```json
{
  "ue_version": "5.4",
  "scan_time": "2026-05-07T10:30:00",
  "source_path": "C:/UnrealEngine/Source",
  "stats": {
    "total_files": 45230,
    "total_classes": 8950,
    "total_functions": 128000,
    "total_modules": 47
  },
  "index_files": {
    "classes": "classes.json",
    "symbols": "symbols.json",
    "modules": "modules.json",
    "hierarchy": "hierarchy.json",
    "file_map": "file_map.json"
  }
}
```

### 2.2 类索引 `classes.json`（核心）

每个类一条记录，AI搜索时匹配类名即可定位：

```json
{
  "ACharacter": {
    "file": "Engine/Source/Runtime/Engine/Classes/GameFramework/Character.h",
    "line": 47,
    "module": "Engine",
    "parent": "APawn",
    "prefix": "A",
    "macro": "UCLASS(Abstract, Blueprintable, BlueprintType)",
    "interfaces": ["IAbilityInterface"],
    "brief": "Base class for characters in Unreal Engine",
    "key_properties": [
      {"name": "CharacterMovementComponent", "type": "UCharacterMovementComponent*", "line": 89, "macro": "UPROPERTY(Category=Character, VisibleAnywhere, BlueprintReadOnly)"},
      {"name": "Mesh", "type": "USkeletalMeshComponent*", "line": 102, "macro": "UPROPERTY(Category=Character, VisibleAnywhere, BlueprintReadOnly)"}
    ],
    "key_functions": [
      {"name": "TakeDamage", "line": 156, "macro": "UFUNCTION(BlueprintCallable, Category=Damage)", "return": "float", "params": "float, FDamageEvent, AController, AActor*"},
      {"name": "Jump", "line": 178, "macro": "UFUNCTION(BlueprintCallable, Category=Character)", "return": "void"},
      {"name": "OnStartCrouch", "line": 201, "macro": "UFUNCTION(BlueprintNativeEvent, Category=Character)", "return": "void"}
    ]
  }
}
```

### 2.3 符号索引 `symbols.json`

所有公开符号的扁平索引，支持快速搜索：

```json
[
  {"name": "TakeDamage", "type": "function", "class": "AActor", "file": "Engine/Source/Runtime/Engine/Classes/GameFramework/Actor.h", "line": 2345, "module": "Engine"},
  {"name": "TakeDamage", "type": "function", "class": "ACharacter", "file": "Engine/Source/Runtime/Engine/Classes/GameFramework/Character.h", "line": 156, "module": "Engine"},
  {"name": "UPROPERTY", "type": "macro", "file": "Engine/Source/Runtime/CoreUObject/Public/UObject/ObjectMacros.h", "line": 890},
  {"name": "FGameplayAbilitySpec", "type": "struct", "file": "Engine/Source/Runtime/GameplayAbilities/Classes/Abilities/Tasks/AbilityTask_NetworkSyncPoint.h", "line": 23, "module": "GameplayAbilities"}
]
```

### 2.4 模块索引 `modules.json`

```json
{
  "Engine": {
    "path": "Engine/Source/Runtime/Engine",
    "build_cs": "Engine/Source/Runtime/Engine/Engine.Build.cs",
    "dependencies": ["Core", "CoreUObject", "InputCore", "NavigationSystem", "Renderer"],
    "key_classes": ["AActor", "ACharacter", "APawn", "UWorld", "UGameInstance"]
  },
  "GameplayAbilities": {
    "path": "Engine/Source/Runtime/GameplayAbilities",
    "build_cs": "Engine/Source/Runtime/GameplayAbilities/GameplayAbilities.Build.cs",
    "dependencies": ["Engine", "Core", "CoreUObject"],
    "key_classes": ["UAbilitySystemComponent", "UGameplayAbility", "FGameplayAbilitySpec"]
  }
}
```

### 2.5 类层次 `hierarchy.json`

紧凑的继承树，AI一眼看懂：

```json
{
  "UObject": {
    "AActor": {
      "APawn": {
        "ACharacter": {},
        "ADefaultPawn": {},
        "ASpectatorPawn": {}
      },
      "AInfo": {
        "AGameModeBase": {
          "AGameMode": {}
        },
        "AGameSession": {}
      }
    },
    "UActorComponent": {
      "UMovementComponent": {
        "UCharacterMovementComponent": {}
      }
    }
  }
}
```

### 2.6 文件映射 `file_map.json`

路径 → 模块 的快速映射：

```json
{
  "Engine/Source/Runtime/Engine/Classes/GameFramework/Character.h": {
    "module": "Engine",
    "classes": ["ACharacter"],
    "includes": ["GameFramework/Pawn.h", "Components/SkeletalMeshComponent.h"]
  }
}
```

## 三、CLI命令设计

### 3.1 索引管理

```bash
# 首次索引（扫描UE源码，耗时5-15分钟）
ue-ai index /path/to/UnrealEngine/Source --version 5.4

# 索引项目代码（增量，秒级）
ue-ai index-project /path/to/MyProject

# 查看已索引的版本
ue-ai list

# 删除索引
ue-ai remove --version 5.4
```

### 3.2 搜索（核心高频命令）

```bash
# 搜索类/函数/结构体 — 返回文件路径+行号+摘要
ue-ai search ACharacter
# → Engine/Source/Runtime/Engine/Classes/GameFramework/Character.h:47
#   class ACharacter : public APawn
#   UCLASS(Abstract, Blueprintable)
#   Module: Engine | 2 key props | 3 key funcs

# 搜索函数
ue-ai search TakeDamage --type function
# → AActor::TakeDamage    Actor.h:2345   float(float, FDamageEvent, AController, AActor*)
# → ACharacter::TakeDamage Character.h:156  float(float, FDamageEvent, AController, AActor*)

# 搜索模块内的所有类
ue-ai search --module GameplayAbilities --type class
# → UAbilitySystemComponent, UGameplayAbility, UAttributeSet, ...

# 正则搜索
ue-ai search "UPROPERTY.*BlueprintReadWrite.*Health"
# → UMyCharacter::Health   MyCharacter.h:34

# 搜索依赖
ue-ai search --depends-on UCharacterMovementComponent
# → ACharacter (uses it as property)
# → UCharacterMovementComponent::GetMaxSpeed() (method)
```

### 3.3 渐进式披露

```bash
# Level 1: 概览（最轻量，AI先看这个）
ue-ai overview ACharacter
# → ACharacter : APawn > AActor > UObject
#   Module: Engine
#   File: Engine/Source/Runtime/Engine/Classes/GameFramework/Character.h:47
#   2 key props, 3 key funcs, 1 interface

# Level 2: 类详情（展开关键成员）
ue-ai class ACharacter
# → 继承链 + 所有UPROPERTY/UFUNCTION列表 + 文件路径

# Level 3: 完整定义（输出整个头文件内容或指定行范围）
ue-ai class ACharacter --full
# → 输出Character.h的类定义部分

# Level 4: 实现代码（定位到.cpp文件）
ue-ai impl ACharacter::TakeDamage
# → Engine/Source/Runtime/Engine/Private/GameFramework/Character.cpp:890
#   float ACharacter::TakeDamage(float DamageAmount, ...) { ... }
```

### 3.4 依赖分析

```bash
# 类的依赖
ue-ai deps ACharacter
# → Depends on: UCharacterMovementComponent, USkeletalMeshComponent, UCapsuleComponent
# → Module: Engine (depends on Core, CoreUObject, Renderer)

# 模块依赖图
ue-ai deps --module Engine --format dot
# → 输出Graphviz dot格式

# 依赖链追踪
ue-ai deps --trace ACharacter::Jump
# → ACharacter::Jump → UCharacterMovementComponent::DoJump → bPressedJump
```

### 3.5 版本差异

```bash
# API变更查询
ue-ai diff --from 5.3 --to 5.4 --class ACharacter
# → [CHANGED] TakeDamage signature: added EDamageType parameter
# → [ADDED] bUseControllerRotationPitch property
# → [DEPRECATED] OldJumpMethod()

# 版本兼容检查（检查项目代码）
ue-ai check --target 5.4 --project /path/to/MyProject
# → MyCharacter.cpp:45: Uses deprecated OldJumpMethod() → migrate to Jump()
```

## 四、输出格式

所有命令支持三种输出格式：

```bash
# JSON（AI工具默认，结构化，可管道到jq）
ue-ai search ACharacter --format json

# Compact（节省AI上下文窗口，一行式）
ue-ai search ACharacter --format compact
# → ACharacter : APawn>AActor>UObject | Engine | Character.h:47 | 2P 3F

# Human（终端使用，带颜色和表格）
ue-ai search ACharacter --format human
```

## 五、索引构建流程

```
ue-ai index /path/to/UE/Source --version 5.4

Step 1: 文件发现（5秒）
  ├── 扫描所有 .h/.cpp 文件
  ├── 按模块分组（Runtime/Editor/Developer/Programs）
  └── 输出: file_list.json

Step 2: AST解析（5-10分钟）
  ├── tree-sitter并行解析所有文件
  ├── 提取class/struct/enum/function声明
  ├── 解析UCLASS/UFUNCTION/UPROPERTY等宏参数
  ├── 提取继承关系
  └── 输出: raw_symbols.json

Step 3: 索引构建（1-2分钟）
  ├── 构建类层次树
  ├── 解析模块依赖（读取Build.cs）
  ├── 构建符号倒排索引
  ├── 生成文件映射
  └── 输出: classes.json, symbols.json, modules.json, hierarchy.json, file_map.json

Step 4: 压缩存储
  ├── 去除冗余信息
  ├── 生成紧凑JSON
  └── 存储到 ~/.ue-ai/indexes/5.4/
```

**索引大小估算**：
- UE源码 ~45000文件
- 索引后JSON ~50-100MB（只存元数据，不存源码内容）
- 搜索时按需读取源文件内容

## 六、项目代码索引（增量）

项目代码量远小于引擎，索引秒级完成：

```bash
ue-ai index-project /path/to/MyProject
```

- 扫描项目的Source/和Plugins/目录
- 解析.uproject获取UE版本
- 自动关联对应版本的引擎索引
- 项目类可以引用引擎类（如 `AMyCharacter : public ACharacter`）

## 七、技术实现

### 7.1 核心技术栈

| 组件 | 选择 | 理由 |
|------|------|------|
| 语言 | Python 3.10+ | 生态丰富，AI开发者熟悉 |
| CLI | Typer | 类型安全，自动帮助文档 |
| C++解析 | tree-sitter + tree-sitter-cpp | 离线AST解析，速度快 |
| 宏解析 | tree-sitter查询 + 正则补充 | UE宏语法特殊需要额外处理 |
| Build.cs解析 | 正则 + 简单语法解析 | C# Build.cs语法相对简单 |
| 搜索 | 内存倒排索引 + 二分查找 | JSON加载到内存，毫秒级搜索 |
| 存储 | JSON文件 | 零依赖，人可读，可git管理 |

### 7.2 搜索实现

不使用向量数据库，纯关键字匹配，追求速度：

```python
class SymbolIndex:
    def __init__(self, index_path: Path):
        self.symbols = json.loads((index_path / "symbols.json").read_text())
        self.name_index = defaultdict(list)  # name → [symbol_entries]
        for sym in self.symbols:
            self.name_index[sym["name"]].append(sym)

    def search(self, query: str, type_filter=None, module_filter=None):
        results = self.name_index.get(query, [])
        if type_filter:
            results = [r for r in results if r["type"] == type_filter]
        if module_filter:
            results = [r for r in results if r["module"] == module_filter]
        return results
```

### 7.3 项目结构

```
ue-ai/
├── pyproject.toml
├── src/ue_ai/
│   ├── __init__.py
│   ├── cli/
│   │   ├── main.py          # 主入口
│   │   ├── index_cmd.py     # index 子命令
│   │   ├── search_cmd.py    # search 子命令
│   │   ├── class_cmd.py     # class/overview/impl 子命令
│   │   ├── deps_cmd.py      # deps 子命令
│   │   └── diff_cmd.py      # diff/check 子命令
│   ├── indexer/
│   │   ├── scanner.py       # 文件发现
│   │   ├── parser.py        # tree-sitter C++解析
│   │   ├── macro_parser.py  # UE宏解析
│   │   ├── build_cs_parser.py # Build.cs解析
│   │   ├── hierarchy.py     # 类层次构建
│   │   └── writer.py        # JSON索引写入
│   ├── search/
│   │   ├── engine.py        # 搜索引擎
│   │   ├── symbol_index.py  # 符号索引
│   │   └── class_index.py   # 类索引
│   ├── version/
│   │   ├── detector.py      # 版本检测
│   │   ├── diff_engine.py   # 版本差异引擎
│   │   └── manifests/       # 版本变更YAML
│   └── output/
│       ├── json_fmt.py
│       ├── compact_fmt.py
│       └── human_fmt.py
├── tests/
└── README.md
```

## 八、开发路线图

### Phase 1: 索引引擎（1周）

- [ ] 项目脚手架（Python + Typer）
- [ ] 文件扫描器（发现.h/.cpp/Build.cs）
- [ ] tree-sitter C++解析器
- [ ] UE宏解析器（UCLASS/UFUNCTION/UPROPERTY/USTRUCT/UENUM）
- [ ] JSON索引写入
- [ ] `ue-ai index` 命令

### Phase 2: 搜索引擎（1周）

- [ ] 符号倒排索引
- [ ] `ue-ai search` 命令（关键字/类型/模块过滤）
- [ ] `ue-ai class` 命令（渐进式披露）
- [ ] `ue-ai overview` 命令
- [ ] `ue-ai impl` 命令（定位实现）
- [ ] 三种输出格式（JSON/Compact/Human）

### Phase 3: 依赖与版本（1周）

- [ ] Build.cs解析器
- [ ] 模块依赖图
- [ ] `ue-ai deps` 命令
- [ ] 版本检测
- [ ] API变更清单
- [ ] `ue-ai diff` / `ue-ai check` 命令

### Phase 4: 项目索引（3天）

- [ ] `ue-ai index-project` 命令
- [ ] 项目-引擎索引关联
- [ ] 项目类引用引擎类

### Phase 5: 打磨发布（3天）

- [ ] pip包发布
- [ ] 性能优化（大索引加载速度）
- [ ] 错误处理
- [ ] CI/CD

## 九、与Trae集成

### .trae/rules/project_rules.md 配置

```markdown
# UE AI CLI

在开发UE C++代码时，使用以下CLI命令辅助：

- `ue-ai search <关键字>` — 搜索类/函数/结构体，返回文件路径+行号
- `ue-ai class <类名>` — 查看类定义详情
- `ue-ai overview <类名>` — 快速概览类继承和关键成员
- `ue-ai impl <类名>::<方法名>` — 定位方法实现
- `ue-ai deps <类名>` — 查看类依赖
- `ue-ai diff --from <V1> --to <V2>` — 查看API版本差异

编写UE代码前，先用ue-ai搜索确认API签名和版本兼容性。
```

### AI使用示例

```
用户: "帮我重写角色的跳跃逻辑"

AI:
1. ue-ai search ACharacter --type class
   → 定位 Character.h:47

2. ue-ai class ACharacter
   → 看到 Jump(), OnStartJump(), bPressedJump 等成员

3. ue-ai impl ACharacter::Jump
   → 定位 Character.cpp:890，查看实现

4. ue-ai deps UCharacterMovementComponent
   → 理解移动组件依赖

5. 基于以上信息，编写代码
```
