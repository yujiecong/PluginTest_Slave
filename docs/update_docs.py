import os
import json
import re
from datetime import datetime
from pathlib import Path

PROJECT_ROOT = Path(r"c:\Users\42458\Documents\Unreal Projects\PluginTest")

def count_lines(file_path):
    try:
        with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
            return len(f.readlines())
    except:
        return 0

def scan_code_stats():
    stats = {
        'cpp_files': {'count': 0, 'lines': 0},
        'h_files': {'count': 0, 'lines': 0},
        'py_files': {'count': 0, 'lines': 0},
        'md_files': {'count': 0, 'lines': 0},
    }

    patterns = {
        '*.cpp': 'cpp_files',
        '*.h': 'h_files',
        '*.py': 'py_files',
        '*.md': 'md_files',
    }

    for pattern, key in patterns.items():
        for file_path in PROJECT_ROOT.rglob(pattern):
            if '.git' not in str(file_path) and '__pycache__' not in str(file_path):
                stats[key]['count'] += 1
                stats[key]['lines'] += count_lines(file_path)

    return stats

def scan_todos():
    todo_pattern = re.compile(r'(TODO|FIXME|BUG|HACK|XXX|NOTE):(.+)', re.IGNORECASE)
    todos = []

    for pattern in ['*.cpp', '*.h', '*.py']:
        for file_path in PROJECT_ROOT.rglob(pattern):
            if '.git' not in str(file_path) and '__pycache__' not in str(file_path):
                try:
                    with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
                        for i, line in enumerate(f, 1):
                            matches = todo_pattern.findall(line)
                            for match in matches:
                                todos.append({
                                    'type': match[0].upper(),
                                    'content': match[1].strip(),
                                    'file': str(file_path.relative_to(PROJECT_ROOT)),
                                    'line': i
                                })
                except:
                    pass

    return todos

def scan_classes():
    classes = {
        'cpp': [],
        'python': []
    }

    cpp_pattern = re.compile(r'class\s+(\w+)\s*[:{]', re.MULTILINE)
    py_pattern = re.compile(r'^class\s+(\w+)', re.MULTILINE)

    for file_path in PROJECT_ROOT.rglob('*.cpp'):
        if '.git' not in str(file_path):
            try:
                with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
                    content = f.read()
                    matches = cpp_pattern.findall(content)
                    classes['cpp'].extend(matches)
            except:
                pass

    for file_path in PROJECT_ROOT.rglob('*.py'):
        if '.git' not in str(file_path) and '__pycache__' not in str(file_path):
            try:
                with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
                    content = f.read()
                    matches = py_pattern.findall(content)
                    classes['python'].extend(matches)
            except:
                pass

    return classes

def get_test_results():
    report_path = PROJECT_ROOT / 'Scripts' / 'tests' / 'test_report' / 'report.json'
    if report_path.exists():
        try:
            with open(report_path, 'r', encoding='utf-8') as f:
                return json.load(f)
        except:
            pass
    return None

def generate_stats_report():
    stats = scan_code_stats()
    todos = scan_todos()
    classes = scan_classes()
    test_results = get_test_results()

    total_lines = sum(stats[k]['lines'] for k in stats)
    total_files = sum(stats[k]['count'] for k in stats)

    report = []
    report.append(f"# 项目统计报告")
    report.append(f"\n> **生成时间**: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}\n")

    report.append(f"## 代码统计\n")
    report.append(f"| 文件类型 | 文件数 | 代码行数 |")
    report.append(f"|---------|--------|---------|")
    report.append(f"| C++ 头文件 (.h) | {stats['h_files']['count']} | {stats['h_files']['lines']} |")
    report.append(f"| C++ 源文件 (.cpp) | {stats['cpp_files']['count']} | {stats['cpp_files']['lines']} |")
    report.append(f"| Python 文件 (.py) | {stats['py_files']['count']} | {stats['py_files']['lines']} |")
    report.append(f"| 文档文件 (.md) | {stats['md_files']['count']} | {stats['md_files']['lines']} |")
    report.append(f"| **总计** | **{total_files}** | **{total_lines}** |")
    report.append("")

    report.append(f"## 类和接口统计\n")
    report.append(f"- C++ 类数量: **{len(classes['cpp'])}**")
    report.append(f"- Python 类数量: **{len(classes['python'])}**")

    if classes['cpp']:
        report.append(f"\n### C++ 类列表\n")
        for cls in sorted(set(classes['cpp'])):
            report.append(f"- {cls}")

    if classes['python']:
        report.append(f"\n### Python 类列表\n")
        for cls in sorted(set(classes['python'])):
            report.append(f"- {cls}")

    if todos:
        report.append(f"\n## 待办事项 (TODO/FIXME)\n")
        report.append(f"共发现 **{len(todos)}** 个标记\n")

        todo_types = {}
        for todo in todos:
            t = todo['type']
            todo_types[t] = todo_types.get(t, 0) + 1

        report.append(f"| 类型 | 数量 |")
        report.append(f"|------|------|")
        for t, count in sorted(todo_types.items()):
            report.append(f"| {t} | {count} |")
        report.append("")

        report.append(f"\n### 详细列表\n")
        for todo in sorted(todos, key=lambda x: (x['file'], x['line'])):
            report.append(f"- [{todo['file']}:{todo['line']}] **{todo['type']}**: {todo['content']}")
    else:
        report.append(f"\n## 待办事项\n")
        report.append("未发现任何 TODO/FIXME 标记。")

    if test_results:
        report.append(f"\n## 测试结果\n")
        report.append(f"- 测试总数: **{test_results.get('total', 0)}**")
        report.append(f"- 通过: **{test_results.get('passed', 0)}** ✅")
        report.append(f"- 失败: **{test_results.get('failed', 0)}** ❌")
        report.append(f"- 跳过: **{test_results.get('skipped', 0)}** ⏭️")
        report.append(f"- 耗时: **{test_results.get('duration_s', 0):.2f}秒**")
        report.append(f"- 执行时间: {test_results.get('timestamp', 'N/A')}")

        failed_tests = [t for t in test_results.get('tests', []) if t.get('status') == 'fail']
        if failed_tests:
            report.append(f"\n### 失败的测试\n")
            for test in failed_tests:
                report.append(f"- {test.get('name', 'Unknown')}: {test.get('error', 'No error message')}")
    else:
        report.append(f"\n## 测试结果\n")
        report.append("未找到测试报告。请在 UE 编辑器中运行测试以生成报告。")

    return '\n'.join(report)

def update_main_doc():
    stats_report = generate_stats_report()
    doc_path = PROJECT_ROOT / 'docs' / '01-架构总览与模块设计.md'

    if not doc_path.exists():
        print(f"文档文件不存在: {doc_path}")
        return

    with open(doc_path, 'r', encoding='utf-8') as f:
        content = f.read()

    version_pattern = r'> \*\*文档版本\*\*: v[\d.]+ \(\d{4}-\d{2}-\d{2} 更新\)'
    new_version = f'> **文档版本**: v2.1 ({datetime.now().strftime("%Y-%m-%d")} 更新)'
    content = re.sub(version_pattern, new_version, content)

    verification_pattern = r'> \*\*最后验证\*\*: .+'
    new_verification = f'> **最后验证**: 自动生成于 {datetime.now().strftime("%Y-%m-%d %H:%M:%S")}'
    content = re.sub(verification_pattern, new_verification, content)

    stats_section_pattern = r'## 13\. 项目统计信息\n.*?(?=\n##|\Z)'
    if re.search(stats_section_pattern, content, re.DOTALL):
        content = re.sub(stats_section_pattern, f'## 13. 项目统计信息\n\n{stats_report}\n', content, flags=re.DOTALL)
    else:
        content += f'\n\n## 13. 项目统计信息\n\n{stats_report}\n'

    with open(doc_path, 'w', encoding='utf-8') as f:
        f.write(content)

    print("文档更新完成！")

def save_stats_json():
    stats = scan_code_stats()
    todos = scan_todos()
    classes = scan_classes()
    test_results = get_test_results()

    report = {
        'timestamp': datetime.now().isoformat(),
        'code_stats': stats,
        'total_lines': sum(stats[k]['lines'] for k in stats),
        'total_files': sum(stats[k]['count'] for k in stats),
        'classes': {
            'cpp_count': len(classes['cpp']),
            'python_count': len(classes['python']),
            'cpp_list': sorted(set(classes['cpp'])),
            'python_list': sorted(set(classes['python']))
        },
        'todos': todos,
        'test_results': test_results
    }

    output_path = PROJECT_ROOT / 'docs' / 'stats_report.json'
    with open(output_path, 'w', encoding='utf-8') as f:
        json.dump(report, f, indent=2, ensure_ascii=False)

    print(f"统计报告已保存到: {output_path}")

if __name__ == '__main__':
    print(f"开始生成项目统计报告... ({datetime.now().strftime('%Y-%m-%d %H:%M:%S')})")
    print("=" * 60)

    update_main_doc()
    save_stats_json()

    print("=" * 60)
    print("所有任务完成！")
