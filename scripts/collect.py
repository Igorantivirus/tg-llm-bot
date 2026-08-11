#!/usr/bin/env python3
"""
Скрипт для сбора содержимого файлов заданных расширений
из указанной директории и всех поддиректорий в один текстовый файл.
"""

import os
import sys
import argparse
from pathlib import Path
from typing import Iterable

__version__ = "1.1.0"


def normalize_extension(ext: str) -> str:
    """
    Приводит расширение к единому виду с точкой в начале.
    Например: "cpp" -> ".cpp", ".h" -> ".h".
    """
    ext = ext.strip()

    if not ext:
        raise ValueError("Пустое расширение файла")

    if not ext.startswith("."):
        ext = "." + ext

    return ext.lower()


def parse_extensions(raw_extensions: Iterable[str]) -> set[str]:
    """
    Принимает список расширений из CLI.

    Поддерживает оба варианта:
      cpp h py
      cpp,h,py
    """
    extensions = set()

    for raw in raw_extensions:
        for part in raw.split(","):
            part = part.strip()
            if part:
                extensions.add(normalize_extension(part))

    if not extensions:
        raise ValueError("Не указано ни одного расширения")

    return extensions


def make_default_output_name(extensions: set[str]) -> str:
    """
    Делает имя выходного файла по умолчанию:
    {'.cpp', '.h'} -> collected_cpp_h_files.txt
    """
    ext_part = "_".join(ext.lstrip(".").replace(".", "_") for ext in sorted(extensions))
    return f"collected_{ext_part}_files.txt"


def collect_files(
    root_dir: Path,
    extensions: set[str],
    *,
    exclude_paths: set[Path] | None = None
) -> list[Path]:
    """
    Рекурсивно обходит root_dir и возвращает список путей ко всем файлам
    с одним из заданных расширений.

    Сравнение делается через endswith(), поэтому работают и составные
    расширения вроде ".tar.gz".
    """
    exclude_paths = exclude_paths or set()
    collected: list[Path] = []

    for dirpath, _, filenames in os.walk(root_dir):
        for fname in filenames:
            file_path = (Path(dirpath) / fname).resolve()

            if file_path in exclude_paths:
                continue

            file_name_lower = file_path.name.lower()
            if any(file_name_lower.endswith(ext) for ext in extensions):
                collected.append(file_path)

    return sorted(collected)


def write_collected_content(
    file_paths: list[Path],
    output_file: Path,
    root_dir: Path,
    *,
    encoding: str = "utf-8",
    errors: str = "replace"
) -> None:
    """
    Записывает в output_file содержимое всех файлов из file_paths.
    Перед содержимым каждого файла вставляется заголовок с относительным путём
    от root_dir.
    """
    output_file.parent.mkdir(parents=True, exist_ok=True)

    with open(output_file, "w", encoding=encoding) as out_f:
        for src_path in file_paths:
            rel_path = src_path.relative_to(root_dir)

            out_f.write(f"{rel_path}\n")
            out_f.write("=" * 60 + "\n")

            try:
                with open(src_path, "r", encoding=encoding, errors=errors) as in_f:
                    content = in_f.read()

                out_f.write(content)

                if content and not content.endswith("\n"):
                    out_f.write("\n")

            except Exception as e:
                out_f.write(f"[Ошибка чтения файла: {e}]\n")

            out_f.write("\n" + "=" * 60 + "\n\n")


def main() -> None:
    parser = argparse.ArgumentParser(
        description=(
            "Собрать содержимое всех файлов с указанными расширениями "
            "из директории в один текстовый файл."
        ),
        epilog=(
            "Примеры:\n"
            "  %(prog)s cpp h -d ./myproject -o output.txt\n"
            "  %(prog)s .py .md .txt -d ./notes\n"
            "  %(prog)s cpp,h,hpp -d ./myproject"
        ),
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )

    parser.add_argument(
        "extensions",
        nargs="+",
        help="Расширения файлов для сбора: cpp h py или cpp,h,py"
    )

    parser.add_argument(
        "-d", "--dir",
        default=".",
        help="Директория для обхода, по умолчанию текущая"
    )

    parser.add_argument(
        "-o", "--output",
        help="Выходной файл, по умолчанию collected_<ext>_files.txt"
    )

    parser.add_argument(
        "--version",
        action="version",
        version=f"%(prog)s {__version__}"
    )

    args = parser.parse_args()

    try:
        extensions = parse_extensions(args.extensions)
    except ValueError as e:
        print(f"Ошибка: {e}", file=sys.stderr)
        sys.exit(1)

    root_dir = Path(args.dir).expanduser().resolve()

    if not root_dir.is_dir():
        print(f"Ошибка: '{root_dir}' не является директорией.", file=sys.stderr)
        sys.exit(1)

    if args.output:
        output_file = Path(args.output).expanduser().resolve()
    else:
        output_file = Path(make_default_output_name(extensions)).resolve()

    print(
        f"Поиск файлов с расширениями: "
        f"{', '.join(sorted(extensions))} в {root_dir}..."
    )

    files = collect_files(
        root_dir,
        extensions,
        exclude_paths={output_file}
    )

    if not files:
        print("Файлы с указанными расширениями не найдены.")
        sys.exit(0)

    print(f"Найдено файлов: {len(files)}")
    print(f"Запись в: {output_file}")

    write_collected_content(files, output_file, root_dir)

    print("Готово.")


if __name__ == "__main__":
    main()